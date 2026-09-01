#include <iostream>
#include "Vtop.h"
#include "verilated.h"
#include <verilated_fst_c.h> // 1. 引入 FST 头文件
#define PMEM_SIZE 128 * 1024 * 1024
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include "minirvemu.h"
#include <svdpi.h>

bool sim_done;
int exit_code;

extern "C" void npc_trap(int code, int pc) {
    if (code == 0) {
        printf("\033[1;32mNPC TRAP: HIT GOOD TRAP at PC = 0x%08x\033[0m\n", pc);
    } else {
        printf("\033[1;31mNPC TRAP: HIT BAD TRAP (code = %d) at PC = 0x%08x\033[0m\n", code, pc);
    }
    sim_done = true;
    exit_code = code;
}

uint32_t npc_gpr[32] = {0};

extern "C" void set_gpr_val(int idx, int val) {
    if (idx >= 0 && idx < 32) {
        npc_gpr[idx] = (uint32_t)val;
    }
}

uint32_t npc_pc = 0;

// 简单的内存模拟（128MB）
uint8_t pmem[PMEM_SIZE];

static void load_image(const char *img_path) {
    if (img_path == NULL) {
        printf("No image is given. Simulation stopped.\n");
        exit(1);
    }

    FILE *fp = fopen(img_path, "rb");
    if (fp == NULL) {
        printf("Error: Cannot open image file '%s'\n", img_path);
        exit(1);
    }

    // 1. 获取 bin 文件大小
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 2. 检查内存是否放得下
    assert(size <= PMEM_SIZE && "Image size exceeds PMEM size!");

    // 3. 读取到 pmem 数组中
    int ret = fread(pmem, 1, size, fp);
    assert(ret == size);

    fclose(fp);
    printf("The image is %s, size = %ld bytes\n", img_path, size);
}

static inline uint32_t guest_to_host(uint32_t paddr) {
    return paddr - 0x80000000;  // RISC-V 从 0x80000000 开始
}

extern "C" int pmem_read(int raddr) {
    // 4 字节对齐读取
    uint32_t addr = raddr & ~0x3u;
    uint32_t offset = guest_to_host(addr);
    
    // 小端序读取 4 字节
    uint32_t data = *(uint32_t *)(pmem + offset);
    
    // printf("pmem_read: addr=0x%x, data=0x%x\n", addr, data);
    return data;
}

extern "C" void pmem_write(int waddr, int wdata, char wmask) {
    if (waddr == 0x10000000) {  // 写入UART
        fputc(wdata & 0xff, stderr);   // 在stdio.h中定义
        return;
    }

    // 4 字节对齐写入
    uint32_t addr = waddr & ~0x3u;
    uint32_t offset = guest_to_host(addr);
    
    // printf("pmem_write: addr=0x%x, data=0x%x, mask=0x%x\n", 
    //       addr, wdata, (uint8_t)wmask);
    
    // 按照 wmask 写入对应字节
    uint8_t *ptr = pmem + offset;
    for (int i = 0; i < 4; i++) {
        if (wmask & (1 << i)) {
            ptr[i] = (wdata >> (i * 8)) & 0xff;
        }
    }
}

bool check_difftest() {
    if (npc_pc != minirvemu_get_pc()) {
        printf("[DiffTest ERROR] PC 不一致!\n");
        printf("NPC PC = 0x%08x, REF PC = 0x%08x\n", npc_pc, minirvemu_get_pc());
        return false;
    }

    for (int i = 0; i < 32; i++) {
        uint32_t npc_reg = (uint32_t)npc_gpr[i];
        uint32_t minirvemu_reg = (uint32_t)minirvemu_get_reg(i);
        if (npc_reg != minirvemu_reg) {
            printf("[DiffTest ERROR] 寄存器 x%d 不一致!\n", i);
            printf("当前 PC: 0x%08x\n", npc_pc);
            printf("NPC 结果: 0x%08x\n", npc_reg);
            printf("REF 结果: 0x%08x\n", minirvemu_reg);
            return false;
        }
    }
    return true;
}

void npc_step_instruction(Vtop* top, VerilatedFstC* tfp, int &sim_time) {
    // 1. 低电平阶段
    top->clk = 0;
    top->eval();
    if (tfp) tfp->dump(sim_time++);

    // 2. 高电平阶段
    top->clk = 1;
    top->eval();
    if (tfp) tfp->dump(sim_time++);
}

int main(int argc, char** argv) {
    if(argc > 1) {
        load_image(argv[1]);
        minirvemu_init(argv[1]);
    }
    else {
        load_image("../am-kernels/tests/cpu-tests/build/dummy-minirv-npc.bin");
        minirvemu_init("../am-kernels/tests/cpu-tests/build/dummy-minirv-npc.bin");
    }
    
    Verilated::commandArgs(argc, argv);
    Vtop* top = new Vtop;

    // 2. 开启 FST 波形追踪
    Verilated::traceEverOn(true);
    VerilatedFstC* tfp = new VerilatedFstC; // 使用 VerilatedFstC
    top->trace(tfp, 99);
    tfp->open("wave.fst");                 // 3. 后缀改为 .fst

    int sim_time = 0;
    while (!sim_done) {
        // NPC 硬件前进一步（时钟翻转，直到一条指令执行结束）
        npc_step_instruction(top, tfp, sim_time);
        // printf("complete one step in npc\n");
        npc_pc = top->pc_out;
        // printf("update pc for 0x%08x\n", npc_pc);

        // 软件模拟器 REF 前进一条指令
        minirvemu_step();
        // printf("complete one step in minirvemu\n");

        // 对比两者状态
        if (!check_difftest()) {
            // 发现了分歧！返回非 0 值，让 make 捕捉到 FAIL
            printf("DiffTest 发现错误，仿真终止！\n");
            return 1;
        }
        // if(sim_time % 1000 == 0) printf("sim_time: %d\n", sim_time);
    }

    tfp->close();
    delete top;
    return exit_code;
}