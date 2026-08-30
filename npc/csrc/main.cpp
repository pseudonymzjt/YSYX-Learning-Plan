#include <iostream>
#include "Vtop.h"
#include "verilated.h"
#include <verilated_fst_c.h> // 1. 引入 FST 头文件
#define PMEM_SIZE 128 * 1024 * 1024
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>

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
    
    printf("pmem_read: addr=0x%x, data=0x%x\n", addr, data);
    return data;
}

extern "C" void pmem_write(int waddr, int wdata, char wmask) {
    // 4 字节对齐写入
    uint32_t addr = waddr & ~0x3u;
    uint32_t offset = guest_to_host(addr);
    
    printf("pmem_write: addr=0x%x, data=0x%x, mask=0x%x\n", 
           addr, wdata, (uint8_t)wmask);
    
    // 按照 wmask 写入对应字节
    uint8_t *ptr = pmem + offset;
    for (int i = 0; i < 4; i++) {
        if (wmask & (1 << i)) {
            ptr[i] = (wdata >> (i * 8)) & 0xff;
        }
    }
}

int main(int argc, char** argv) {
    if(argc > 1) {
        load_image(argv[1]);
    }
    else load_image("../am-kernels/tests/cpu-tests/build/dummy-minirv-npc.bin");
    Verilated::commandArgs(argc, argv);
    Vtop* top = new Vtop;

    // 2. 开启 FST 波形追踪
    Verilated::traceEverOn(true);
    VerilatedFstC* tfp = new VerilatedFstC; // 使用 VerilatedFstC
    top->trace(tfp, 99);
    tfp->open("wave.fst");                 // 3. 后缀改为 .fst

    int sim_time = 0;
    while (!sim_done) {
        top->clk = !top->clk;
        top->eval();
        
        tfp->dump(sim_time); // 像以前一样写入波形
        sim_time++;
    }

    tfp->close();
    delete top;
    return exit_code;
}