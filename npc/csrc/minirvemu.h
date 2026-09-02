#ifndef __MINIRVEMU_H__
#define __MINIRVEMU_H__

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 minirvEMU，加载程序镜像到它的内存中
void minirvemu_init(const char *img_path);

// 单步执行一条指令（Fetch -> Decode -> Execute -> Update PC/Reg）
void minirvemu_step();

// 供 NPC 读取它的 PC 和 32 个寄存器
uint32_t minirvemu_get_pc();
uint32_t minirvemu_get_reg(int idx);

void minirvemu_set_gpr(int rd, uint32_t val);

#ifdef __cplusplus
}
#endif

#endif