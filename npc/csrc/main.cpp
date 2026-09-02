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
#include <sys/time.h>


static uint64_t boot_time = 0;
uint64_t g_current_time = 0;

// 获取从仿真开始经过的微秒数 (us)
uint64_t get_time() {
    struct timeval now;
    gettimeofday(&now, NULL);
    uint64_t us = (uint64_t)now.tv_sec * 1000000 + now.tv_usec;
    if (boot_time == 0) {
        boot_time = us; // 记录启动时刻
    }
    g_current_time = us - boot_time;
    return g_current_time;
}

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

static inline int get_inst_rd(uint32_t inst) {
    return (inst >> 7) & 0x1f;
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

bool is_load_uart_stat = false;
uint32_t uart_stat_val = 0;

extern "C" int pmem_read(int raddr) {
    if (raddr == 0x10000004) {  // 读出UART状态
        is_load_uart_stat = true;
        uart_stat_val = (rand() & 0x7) == 0 ? 1 : 0; // 就绪概率为12.5%
        return uart_stat_val;
    }
    if (raddr == 0x20000000 || raddr == 0x20000004) {
        // 读出时钟的低32位
        if (raddr == 0x20000000) { 
            g_current_time = get_time(); 
            // printf("[DEBUG] 成功访问了 RTC 时钟地址!, current: %lx\n", g_current_time); 
            return g_current_time & 0xffffffff; 
        }
        // 读出时钟的高32位
        else { return g_current_time >> 32; }
    }
    else if (raddr >= 0x20000010 && raddr <= 0x20000024) {
        time_t now = time(NULL);
        struct tm *t = gmtime(&now); // 或 localtime(&now)
        if (raddr == 0x20000010) return t->tm_year + 1900;
        if (raddr == 0x20000014) return t->tm_mon + 1;
        if (raddr == 0x20000018) return t->tm_mday;
        if (raddr == 0x2000001c) return t->tm_hour;
        if (raddr == 0x20000020) return t->tm_min;
        if (raddr == 0x20000024) return t->tm_sec;
    }

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
        // NPC 硬件前进一步
        npc_step_instruction(top, tfp, sim_time);
        npc_pc = top->pc_out;

        // 软件模拟器 REF 前进一条指令
        minirvemu_step();

        // 对比两者状态
        if (!check_difftest()) {
            printf("DiffTest 发现错误，仿真终止！\n");
            return 1;
        }
    }

    tfp->close();
    delete top;
    return exit_code;
}