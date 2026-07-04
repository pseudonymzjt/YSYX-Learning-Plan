#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "Vtest.h"
#include "verilated.h"
#include "verilated_fst_c.h"
#include <../nvboard.h>

static TOP_NAME dut;

void nvboard_bind_all_pins(TOP_NAME* top);

int main(int argc, char** argv) {
    VerilatedContext* contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    contextp->traceEverOn(true);
    VerilatedFstC* tfp = new VerilatedFstC;
    Vtest* top = new Vtest{contextp};
    top->trace(tfp, 99);
    tfp->open("wave.fst");
    int i = 0;
    while (i < 20) {
        int a = rand() & 1;
        int b = rand() & 1;
        top->a = a;
        top->b = b;
        top->eval();
        printf("a = %d, b = %d, f = %d\n", a, b, top->f);
        assert(top->f == (a ^ b));
        i++;
        // 将当前时刻的信号状态写入波形
        tfp->dump(contextp->time());
        // 时间往前推进 1 个单位
        contextp->timeInc(1);
    }
    tfp->close();
    delete top;
    delete contextp;
    return 0;
}