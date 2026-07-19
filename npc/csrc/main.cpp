#include <iostream>
#include "Vtop.h"
#include "verilated.h"
#include <verilated_fst_c.h> // 1. 引入 FST 头文件

extern "C" void npc_trap(int code) {
    if (code == 0) {
        std::cout << "NPC TRAP: HIT GOOD TRAP!" << std::endl;
    } else {
        std::cout << "NPC TRAP: HIT BAD TRAP! Code: " << code << std::endl;
    }
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vtop* top = new Vtop;

    // 2. 开启 FST 波形追踪
    Verilated::traceEverOn(true);
    VerilatedFstC* tfp = new VerilatedFstC; // 使用 VerilatedFstC
    top->trace(tfp, 99);
    tfp->open("wave.fst");                 // 3. 后缀改为 .fst

    int sim_time = 0;
    while (sim_time < 100) {
        top->clk = !top->clk;
        top->eval();
        
        tfp->dump(sim_time); // 像以前一样写入波形
        sim_time++;
    }

    tfp->close();
    delete top;
    return 0;
}