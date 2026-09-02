#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "minirvemu.h"

#define MEM_SIZE (128 * 1024 * 1024) // 128MB 内存
#define RESET_VECTOR 0x80000000

static uint8_t  ref_M[MEM_SIZE];
static uint32_t ref_R[32];
static uint32_t ref_pc = RESET_VECTOR;

extern uint32_t uart_stat_val;
extern uint64_t g_current_time;

/* 内存读写辅助函数 */
static inline uint32_t mem_read(uint32_t addr, int len) {
    if (addr == 0x10000004) {
        return uart_stat_val;
    }
    if (addr == 0x20000000) {
        // printf("[minirvEMU] [DEBUG] 成功访问了 RTC 时钟地址!, current: %lx\n", g_current_time);
        return (g_current_time & 0xffffffff);
    }
    if (addr == 0x20000004) return (g_current_time >> 32);

    if (addr >= 0x20000010 && addr <= 0x20000024) {
        time_t now = time(NULL);
        struct tm *t = gmtime(&now); // 或 localtime(&now)
        if (addr == 0x20000010) return t->tm_year + 1900;
        if (addr == 0x20000014) return t->tm_mon + 1;
        if (addr == 0x20000018) return t->tm_mday;
        if (addr == 0x2000001c) return t->tm_hour;
        if (addr == 0x20000020) return t->tm_min;
        if (addr == 0x20000024) return t->tm_sec;
    }

    uint32_t offset = addr - RESET_VECTOR;
    if (offset >= MEM_SIZE) {
        printf("[minirvEMU] 读内存越界! addr=0x%08x\n", addr);
        assert(0);
    }
    if (len == 1) return *(uint8_t  *)(ref_M + offset);
    if (len == 2) return *(uint16_t *)(ref_M + offset);
    if (len == 4) return *(uint32_t *)(ref_M + offset);
    return 0;
}

static inline void mem_write(uint32_t addr, uint32_t data, int len) {
    uint32_t offset = addr - RESET_VECTOR;
    if (offset >= MEM_SIZE) {
        printf("[minirvEMU] 写内存越界! addr=0x%08x\n", addr);
        assert(0);
    }
    if (len == 1) *(uint8_t  *)(ref_M + offset) = (uint8_t)data;
    if (len == 2) *(uint16_t *)(ref_M + offset) = (uint16_t)data;
    if (len == 4) *(uint32_t *)(ref_M + offset) = data;
}

/* 初始化 */
void minirvemu_init(const char *img_path) {
    for (int i = 0; i < 32; i++) ref_R[i] = 0;
    ref_pc = RESET_VECTOR;

    if (img_path == NULL) return;
    FILE *fp = fopen(img_path, "rb");
    if (!fp) {
        printf("[minirvEMU] 无法打开镜像文件: %s\n", img_path);
        assert(0);
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    assert(size <= MEM_SIZE);
    int ret = fread(ref_M, 1, size, fp);
    assert(ret == size);
    fclose(fp);
}

/* 核心单步执行：包含 8 条 minirv 指令 */
void minirvemu_step() {
    uint32_t inst = mem_read(ref_pc, 4);

    // 字段拆解
    uint32_t opcode = inst & 0x7f;
    uint32_t rd     = (inst >> 7)  & 0x1f;
    uint32_t funct3 = (inst >> 12) & 0x7;
    uint32_t rs1    = (inst >> 15) & 0x1f;
    uint32_t rs2    = (inst >> 20) & 0x1f;
    uint32_t funct7 = (inst >> 25) & 0x7f;

    // 立即数解析（符号扩展）
    int32_t  imm_i = (int32_t)inst >> 20;                                      // I 型
    int32_t  imm_s = (((int32_t)inst >> 25) << 5) | ((inst >> 7) & 0x1f);       // S 型
    uint32_t imm_u = inst & 0xfffff000;                                        // U 型

    uint32_t next_pc = ref_pc + 4; // 默认下一条指令

    // 执行
    switch (opcode) {
        // LUI 指令
        case 0x37: {
            ref_R[rd] = imm_u;
            break;
        }

        // ADDI 指令 (I 型计算)
        case 0x13: {
            if (funct3 == 0) { // addi
                ref_R[rd] = ref_R[rs1] + imm_i;
            } else {
                goto unhandled;
            }
            break;
        }

        // ADD 指令 (R 型计算)
        case 0x33: {
            if (funct3 == 0 && funct7 == 0) { // add
                ref_R[rd] = ref_R[rs1] + ref_R[rs2];
            } else {
                goto unhandled;
            }
            break;
        }

        // JALR 指令 (I 型跳转)
        case 0x67: {
            if (funct3 == 0) {
                uint32_t target = (ref_R[rs1] + imm_i) & ~1u;
                ref_R[rd] = ref_pc + 4; // 保存返回地址
                next_pc = target;       // 跳转
            } else {
                goto unhandled;
            }
            break;
        }

        // LW / LBU 指令 (I 型 Load)
        case 0x03: {
            uint32_t addr = ref_R[rs1] + imm_i;
            if (funct3 == 2) {        // lw (4 字节)
                ref_R[rd] = mem_read(addr, 4);
            } else if (funct3 == 4) { // lbu (1 字节无符号零扩展)
                ref_R[rd] = (uint32_t)(uint8_t)mem_read(addr, 1);
            } else {
                goto unhandled;
            }
            break;
        }

        // SW / SB 指令 (S 型 Store)
        case 0x23: {
            uint32_t addr = ref_R[rs1] + imm_s;
            if (addr == 0x10000000) {}
            else {
                if (funct3 == 2) {        // sw (4 字节)
                    mem_write(addr, ref_R[rs2], 4);
                } else if (funct3 == 0) { // sb (1 字节)
                    mem_write(addr, ref_R[rs2] & 0xff, 1);
                } else {
                    goto unhandled;
                }
            }
            break;
        }

        // EBREAK (用于测试程序结束退出)
        case 0x73: {
            if (inst == 0x00100073) {
                // 遇到 ebreak，保持原地
                next_pc = ref_pc;
                break;
            }
            goto unhandled;
        }

        default:
        unhandled:
            printf("[minirvEMU] 未实现的指令: 0x%08x at PC=0x%08x\n", inst, ref_pc);
            assert(0);
    }

    // 强制维持 x0 = 0
    ref_R[0] = 0;

    // 更新 PC
    ref_pc = next_pc;
}

/* 状态获取接口 */
uint32_t minirvemu_get_pc() {
    return ref_pc;
}

uint32_t minirvemu_get_reg(int idx) {
    if (idx < 0 || idx >= 32) return 0;
    return ref_R[idx];
}

void minirvemu_set_gpr(int rd, uint32_t val) {
    if (rd != 0) { // x0 恒为 0
        ref_R[rd] = val;
    }
}