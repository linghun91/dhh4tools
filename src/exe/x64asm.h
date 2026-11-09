/* 主程序补丁用的小型 x64 机器码拼装器与字节模式查找。 */
#ifndef DK4_EXE_X64ASM_H
#define DK4_EXE_X64ASM_H
#include "common/platform.h"

typedef struct Asm {
    uint8_t *b;
    int n;
    int base;
    int cap;
    int bad;
} Asm;

void asm_hex(Asm *a, const char *s);
void asm_u32(Asm *a, uint32_t v);
void asm_call(Asm *a, int target);
void asm_jmp(Asm *a, int target);
void asm_calli(Asm *a, int slot);
/* 发出 0F cc rel32，返回位置，之后用 asm_fix 回填目标。 */
int asm_jcc(Asm *a, uint8_t cc);
void asm_fix(Asm *a, int at, int label);
/* 发出待回填的 E9 rel32，之后用 asm_fixj 回填目标。 */
int asm_jmpf(Asm *a);
void asm_fixj(Asm *a, int at, int label);
void asm_lea_rdx(Asm *a, int target);
/* 十六进制字节模式查找：pat_find 仅唯一命中时返回 1；pat_findall 支持 "?"
 * 通配并返回命中数；pat_is 判断 at 处是否匹配。 */
int pat_find(const uint8_t *data, size_t n, const char *hex, int *at);
int pat_findall(const uint8_t *data, size_t n, const char *hex, int *at, int max);
int pat_is(const uint8_t *data, size_t n, int at, const char *hex);
int ru16(const uint8_t *p);

typedef struct PatchSite {
    int at;
    int len;
    int kind;   /* 0 jmp + nop 填充，1 jcc rel32 在 +2，2 call rel32 在 +1，
                   3 原地改写（只记原字节，不装跳转） */
    int entry;
} PatchSite;

#endif
