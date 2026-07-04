# "一生一芯"工程项目

这是"一生一芯"的工程项目. 通过运行
```bash
bash init.sh subproject-name
```
进行初始化, 具体请参考[实验讲义][lecture note].

[lecture note]: https://ysyx.oscc.cc/docs/
这个仓库中存放着我的一生一芯课程计划和学习资料
---

### 第一阶段：硬思维构建与 Logisim 处理器（第 1 - 2 周）

#### 🎯 小目标
1. 掌握科学的提问与 Debug 基础方法（拒绝当“伸手党”，建立独立排障直觉）。
2. 在 Logisim 平台中不写代码，仅通过连线理解计算机内部“时钟+寄存器+组合逻辑”的运转。
3. 手动连出一个可以计算 $1+2+...+10$ 的电路 `sCPU`。
4. 连出一个支持 RISC-V 迷你指令集（8条指令）的 `minirv` 处理器。

#### 📚 推荐参考资料
* **《如何科学地提问》：** [ysyx F1 讲义](https://ysyx.oscc.cc/docs/prestudy/F1.html)
* **数字电路教材：** 阎石《数字电子技术基础》、或 B 站哈工大刘家茂老师的《数字电子技术》视频。
  * *重点复习：* 组合逻辑（加法器、译码器、MUX）、时序逻辑（D 触发器、时钟信号、状态机编写）。
* **RISC-V 指令集标准：** 官方非特权级规范手册 [RISC-V Unprivileged ISA Specification](https://riscv.org/technical/specifications/)（第 2 章 RV32I 基础指令）。

#### 💻 细化任务与代码细节
* **任务 1.1：数字逻辑 Logisim 预热（3 天）**
  * 安装 `Logisim-evolution` 软件。
  * 练习在 Logisim 中拉出逻辑门，并连线完成一个：3-8译码器、带进位输出的 4 位加法器。
  * 用 D触发器 连线搭建一个 3 位二进制计数器，并用时钟脉冲驱动，观察各个输出引脚的值如何呈状态机式跳转。
* **任务 1.2：搭建 sCPU 电路（4 天）**
  * **理解 sISA 指令集：** 只包含 4 位 PC、2个通用寄存器、几条极其有限的硬编码控制指令。
  * **电路连线：**
    * 拖入一个 ROM（存储指令），将 4 位 PC 连入其地址引脚。
    * 拖入一个 8 位的通用寄存器堆（GPR），用于存放加数和结果。
    * 拖入一个加法器作为 ALU。
    * 连出一条数据反馈通路（将加法器结果写回通用寄存器堆，由 PC 控制时钟）。
  * **验证：** 将计算 $1+2+...+10$ 的二进制控制码手动写入 ROM 中，开启时钟仿真，单步运行直到寄存器得出正确的值。
* **任务 1.3：设计 minirv 处理器（1 周）**
  * **理解 minirv 指令集：** 8条核心 RISC-V 32位指令：`ADDI`, `LW`, `SW`, `ADD`, `SUB`, `JAL`, `BEQ`, `LUI`。
  * **电路连线：**
    * **译码部分：** 将从 ROM 读出的 32 位指令用 Splitter 分线器拆开，提取 `opcode` (7位)、`funct3` (3位)、`rs1`、`rs2`、`rd` (5位)，以及不同指令格式（I型、S型、B型等）对应的立即数。
    * **寄存器堆：** 搭建一个 32个×32位 的寄存器堆，支持双端读、单端写（注意寄存器 0 恒为 0）。
    * **ALU 部分：** 支持加法、减法、逻辑移位等基本操作。
    * **PC 部分：** 引入 MUX，使得 PC 的来源既可以是 `PC + 4`，也可以是 `BEQ` 跳转或 `JAL` 的目标地址。
  * **验证：** 手动用 8 组指令跑通基本的跳转与数据加载测试。

---

### 第二阶段：开发环境迁入、C 语言与 PA1（第 3 - 4 周）

#### 🎯 小目标
1. 搭建完整的 Linux/WSL2 开发环境，能够编写基本的 Makefile 进行编译自动化。
2. 刷完 C 语言高级练习，重点彻底搞懂多级指针、结构体对齐和内存分配。
3. 掌握 Verilator 仿真工具的使用，写出第一个可仿真的 C++ 顶层代码。
4. 实现 NEMU 的基本框架：具备单步执行、扫描内存和简易表达式求值的调试器。

#### 📚 推荐参考资料
* **C 语言强化：** Zed Shaw 的经典书 [*Learn C the Hard Way*](https://learncodethehardway.org/)（完成练习 0-22, 24-25, 27-33）。
* **Verilog 实战网站：** [HDLBits Online Exercises](https://hdlbits.01xz.net/) (建议通关 “Getting Started”, “Verilog Language”, 以及 “Circuits” 章节)。
* **模拟器圣经：** 南京大学 [“计算机系统基础” (PA) 实验讲义（PA1 部分）](https://nju-projectn.github.io/ics-pa-gitbook/ics2024/)。

#### 💻 细化任务与代码细节
* **任务 2.1：C 语言与 Linux 环境磨炼（4 天）**
  * 在 Windows 上安装 WSL2 运行 Ubuntu 22.04，配合 VS Code 进行远程连接。
  * 编写第一个包含多个 `.c` 和 `.h` 文件的 C 语言程序，手写一个 `Makefile` 体验它的自动化编译规则（包含 `gcc -I`, `gcc -L` 选项，编译静态和动态链接库）。
  * 练习在 GDB 中对段错误（Segment fault）的代码进行断点追踪，并使用 `valgrind` 分析 C 程序中的内存泄漏（Memory leak）。
* **任务 2.2：搭建第一个 Verilator 仿真（3 天）**
  * **编写 RTL：** 用 Verilog 写一个带复位和写使能的 8位寄存器 `reg.v`。
  * **编写 C++ 仿真顶层：** 创建 `sim_main.cpp`，利用 Verilator 导出的 C++ 类结构进行仿真操作：
    ```cpp
    #include "Vreg.h"
    #include "verilated.h"
    Vreg* top = new Vreg;
    while (!Verilated::gotFinish()) {
        top->clk = !top->clk; // 翻转时钟
        top->eval();          // 评估电路状态
    }
    ```
  * **波形查看：** 在 C++ 顶层中启用 `VerilatedVcdC`，输出 `.vcd` 轨迹文件，在 Linux 下启动 `gtkwave sim.vcd` 查看信号跳转。
* **任务 2.3：完成 PA1 —— 简易调试器（sdb）（1 周）**
  * 克隆 `ysyx-workbench` 仓库，进入 `nemu` 目录。
  * **命令解析：** 在 `nemu/src/monitor/sdb/sdb.c` 中通过 `strtok` 分割指令。实现 `si [N]` 从而控制 `cpu-exec` 循环执行 $N$ 次指令。
  * **表达式求值（重点）：** 
    * 采用正则表达式（`regex.h`）对计算字符串（例如 `*0x80000000 + 4 * (12 - 3)`）进行 Token 切分。
    * 递归求值：实现 `eval(p, q)` 寻找“主运算符”（Dominant Operator），递归解析子表达式并返回结果。
  * **监视点：** 实现 `w [EXPR]`，每次单步执行指令后遍历监视点链表，若值发生改变则调用 `nemu_state.state = NEMU_STOP`。

---
### 第三阶段：软件轨纵深——RISC-V 32 指令集译码与运行时（第 5 - 6 周）

#### 💻 细化任务与代码细节（接上文）

*   **任务 3.1：理解机器级表示与 ELF 解析（接上文）（3 天）**
    *   使用 `riscv64-unknown-elf-gcc`（RISC-V 交叉编译器）将一个简单的 C 程序（如 `int a = 1;`）编译为 `.o` 文件和 `.elf` 可执行文件。
    *   运行 `riscv64-unknown-elf-readelf -a file.elf`，观察并理解 ELF 头部的 Section Header Table、Symbol Table，了解编译器是如何为变量和函数分配逻辑地址的。
    *   运行 `riscv64-unknown-elf-objdump -d file.elf` 进行反汇编，将你写的 C 语言分支语句、循环语句、函数调用和指针操作，对照翻译出来的 RISC-V 汇编，观察编译器是如何生成指令的。
*   **任务 3.2：NEMU RV32IM 指令集扩展（5 天）**
    *   在 `nemu/src/isa/riscv32/inst.c`（或相应架构目录）下编写指令译码与执行的核心逻辑。
    *   **译码设计：** 抽象出统一的指令译码模板（R-type, I-type, S-type, B-type, U-type, J-type），利用宏或位提取操作将指令中的寄存器索引（`rs1`, `rs2`, `rd`）和立即数（`imm`）解析并存入临时上下文 `Decode` 结构体。
    *   **指令实现：**
        *   **I 类与 R 类基本计算指令：** 实现 `addi`, `slli`, `add`, `sub`, `sll`, `srl` 等。
        *   **访存指令：** 实现 `lw` 和 `sw`（以及 `lb`, `lh`, `sb`, `sh` 等无符号/有符号加载写入操作）。
        *   **分支与跳转指令：** 实现 `beq`, `bne`, `blt`, `bge`, `jal`, `jalr`。注意，在分支跳转执行时需要更新 PC。
        *   **M 扩展指令：** 实现 `mul`, `div`, `rem` 等乘除法逻辑。
*   **任务 3.3：Abstract Machine (AM) 运行时基础实现（5 天）**
    *   了解什么是 **TRM (Turing Receiver Machine)**。它是 AM 框架中最精简的运行时环境，只包含处理器、物理存储器和一个用于字符输出的串口。
    *   在 NEMU 侧实现一个简单的串口外设映射（通常是向特定地址如 `0xa00003f8` 写入数据时，NEMU 在 C 语言侧调用 `putchar()` 打印该字符）。
    *   在 `am-kernels/tests/cpu-tests/` 下，使用 `make ARCH=riscv32-nemu` 编译基本的指令测试样例。NEMU 读取生成的 `.bin` 镜像后，能成功运行并打印 `PASS`。

---

### 第四阶段：硬件轨会师——NPC 设计、工具设施与联调（第 7 - 8 周）

#### 🎯 小目标
1. 用 Verilog/Chisel 设计并实现一个 RV32E（16个寄存器）单周期处理器（NPC）。
2. 在 NPC 硬件仿真中通过 **DPI-C**（Direct Programming Interface）实现硬件与 C++ 侧仿真存储器的低延迟交互。
3. 将 NEMU 编译为共享库并集成入 NPC 的仿真平台，实现基本的 **DiffTest（差分测试）**，大幅提升排障效率。
4. 让你的 NPC 处理器成功在仿真环境下跑通 AM 裸机测试套件。

#### 📚 推荐参考资料
* **《Computer Organization and Design: The Hardware/Software Interface (RISC-V Edition)》：** 重点看第 4 章“处理器设计”中关于单周期数据通路（Single-Cycle Datapath）与控制通路的硬连线设计。
* **Verilator DPI-C 官方手册：** 理解如何利用 DPI-C 在 RTL 硬件逻辑中调用 C/C++ 编写的系统级辅助函数。
* **ysyx 基础阶段 C1/C2 讲义：** [工具与基础设施](https://ysyx.oscc.cc/docs/ics-pa/3.1.html)、[单周期 NPC 处理器](https://ysyx.oscc.cc/docs/ics-pa/3.2.html)。

#### 💻 细化任务与代码细节

*   **任务 4.1：设计单周期处理器 NPC（1 周）**
    *   在 `npc/` 目录下建立你的 RTL 工程。
    *   依照在 F 阶段设计的 `minirv` 思路，用 Verilog / Chisel 描述以下模块：
        *   **IF（取指）：** 包含 PC 寄存器、根据跳转信号更新 PC 值的复用器、向外设存储器请求指令的接口。
        *   **ID（译码）：** 根据指令生成独热码（One-hot）格式的控制信号（如 `src1_is_pc`, `src2_is_imm`, `reg_write_enable` 等）。
        *   **Regfile（寄存器堆）：** 设计一个 16个×32位 的 RV32E 寄存器堆（注意：x0 寄存器硬件恒接 0）。
        *   **EX（执行）：** 一个支持加减、逻辑移位、比较操作的 32位 ALU。
        *   **MEM（访存）：** 通过读写地址、掩码和控制信号向外设读写数据。
        *   **WB（写回）：** 将 ALU 结果或从内存读取的数据写回到寄存器堆。
    *   *注：此时暂时不需要设计 AXI 总线，直接使用简单的读写使能及地址数据总线，让 C++ 测试平台来直接响应内存读写。*
*   **任务 4.2：引入 DPI-C 与硬件调试工具（3 天）**
    *   在 RTL 代码中声明 DPI-C 函数，用来在每次时钟上升沿将当前硬件内部的执行状态回传给 C++ 仿真侧：
        ```verilog
        import "DPI-C" function void set_gpr_ptr(input logic [31:0] gpr []);
        import "DPI-C" function void trace_inst(input logic [31:0] pc, input logic [31:0] inst);
        ```
    *   在 C++ 顶层代码中实现这些函数，用以抓取 NPC 内部寄存器的值。
    *   **开发 Itrace（指令追踪）：** 在仿真运行时，把抓取到的当前 PC 和机器码，通过调用外部的反汇编库（如 `llvm-mc` 绑定的动态库），生成对应的汇编文本输出到日志中，便于定位错误。
*   **任务 4.3：接入 DiffTest，联调攻坚（5 天）**
    *   **将 NEMU 编译为 So（共享库）：** 通过在 NEMU Makefile 中配置，使其编译生成 `nemu.so`。
    *   **仿真框架重构：** 
        *   在 NPC 的 C++ `main` 函数启动时，通过 `dlopen()` 动态加载 `nemu.so`。
        *   利用 API 初始化 NEMU（REF）的寄存器和内存，并使其保持与 NPC 的初始状态完全一致。
    *   **差分比对循环实现：**
        ```cpp
        while (!sim_end) {
            single_step_npc(); // NPC 硬件仿真向前走一步（一个周期）
            if (npc_just_executed_an_instruction) {
                difftest_exec(1); // 让 NEMU 也向前走一步
                // 抓取并对比两者寄存器
                if (npc.regs != nemu.regs || npc.pc != nemu.pc) {
                    print_error_log(); // 打印寄存器差异
                    exit(-1);          // 报错退出
                }
            }
        }
        ```
    *   **运行 AM 裸机程序测试：**
        将 AM 中的测试程序（如 `dummy`, `add` 等简单的指令集测试）编译成二进制镜像（`.bin`），加载到 C++ 侧模拟的物理内存（RAM）中。
        运行仿真。NPC 会自动执行 RAM 中的指令，而 DiffTest 会在后台时刻监督你的硬件通路是否与 NEMU 严格保持一致。此时，你将体验到自动化调试的效率。

---

### 五、 阶段性 Checkpoint 汇总与最终产出

如果你能够坚持在 7 月和 8 月保持高强度的开发，你将在 9 月初获得以下实质性的**技术产出**：

| 时间节点 | 阶段产出物 | 核心 Checkpoint（如何自我检验） |
| :--- | :--- | :--- |
| **第 2 周末** | `minirv` Logisim 原型电路 | 能够在 Logisim 仿真环境下，通过手动置时钟脉冲，正确执行 8 条 RISC-V 核心指令，并能计算出累计求和等小算法。 |
| **第 4 周末** | `ysyx-workbench/nemu` 工具框架 | 能够在 Linux 终端运行 `nemu` 并进入交互式命令行，通过手敲命令实现单步调试和对输入的加减乘除算术表达式求值。 |
| **第 6 周末** | 具备 RV32IM 执行能力的 NEMU | 将 AM 的 `cpu-tests`（如快速排序、字符串拷贝）交叉编译为 RISC-V 32 二进制，NEMU 能够将其完全跑通，打印 `PASS`。 |
| **第 8 周末** | **单周期 NPC 处理器 + DiffTest 验证平台** | 将相同的 `cpu-tests` 二进制文件加载进 NPC 的内存中，硬件在 Verilator 环境下跑通程序；一旦硬件设计出错（如某个数据通路连错），DiffTest 能够精准捕捉并在出线不一致的第一周期强制退出仿真。 |

### 🛠️ 写在最后：如何面对庞杂的知识点？
*一生一芯*的内容确实非常繁杂（包含了脚本、C、汇编、Make、Git、Verilog、仿真、调试器等），如果你被支线（如时序约束、物理版图、总线协议等）带偏，很容易在暑期结束前颗粒无收。

请在学习过程中牢记：**紧贴主线**。如果在某个非核心问题上卡住超过一天（比如为了美化终端写了很长的脚本，或者在思考如何实现一个非常复杂的除法器 RTL），请果断**使用 ysyx 提供的基础设施（例如先用 `/` 行为级算子实现除法，后续再重构）**，快速把软硬件通路的“闭环”跑通。先完成，再完美。
