/* 主程序补丁用的小型 x64 机器码拼装器与字节模式查找。 */

#include "exe/x64asm.h"

void asm_hex(Asm *a, const char *s) {
    int hi = -1;
    for (; *s; s++) {
        int v;
        if (*s >= '0' && *s <= '9') v = *s - '0';
        else if (*s >= 'a' && *s <= 'f') v = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') v = *s - 'A' + 10;
        else continue;
        if (hi < 0) hi = v;
        else {
            if (a->n >= a->cap) { a->bad = 1; return; }
            a->b[a->n++] = (uint8_t)(hi * 16 + v);
            hi = -1;
        }
    }
}

void asm_u32(Asm *a, uint32_t v) {
    if (a->n + 4 > a->cap) { a->bad = 1; return; }
    memcpy(a->b + a->n, &v, 4);
    a->n += 4;
}

static void asm_rel(Asm *a, uint8_t op, int target) {
    int32_t rel;
    if (a->n + 5 > a->cap) { a->bad = 1; return; }
    rel = (int32_t)(target - (a->base + a->n + 5));
    a->b[a->n++] = op;
    memcpy(a->b + a->n, &rel, 4);
    a->n += 4;
}

void asm_call(Asm *a, int target) { asm_rel(a, 0xE8, target); }

void asm_jmp(Asm *a, int target) { asm_rel(a, 0xE9, target); }

/* call qword [rip+slot] */
void asm_calli(Asm *a, int slot) {
    int32_t rel;
    asm_hex(a, "FF 15");
    if (a->n + 4 > a->cap) { a->bad = 1; return; }
    rel = (int32_t)(slot - (a->base + a->n + 4));
    memcpy(a->b + a->n, &rel, 4);
    a->n += 4;
}

/* 0F cc rel32，目标由 asm_fix 回填。 */
int asm_jcc(Asm *a, uint8_t cc) {
    int at = a->n;
    asm_hex(a, "0F");
    if (a->n >= a->cap) { a->bad = 1; return at; }
    a->b[a->n++] = cc;
    asm_hex(a, "00 00 00 00");
    return at;
}

void asm_fix(Asm *a, int at, int label) {
    int32_t rel = (int32_t)(label - (at + 6));
    if (at + 6 > a->cap) { a->bad = 1; return; }
    memcpy(a->b + at + 2, &rel, 4);
}

/* E9 rel32 跳到代码洞内标签，由 asm_fixj 回填。 */
int asm_jmpf(Asm *a) {
    int at = a->n;
    asm_hex(a, "E9 00 00 00 00");
    return at;
}

void asm_fixj(Asm *a, int at, int label) {
    int32_t rel = (int32_t)(label - (at + 5));
    if (at + 5 > a->cap) { a->bad = 1; return; }
    memcpy(a->b + at + 1, &rel, 4);
}

void asm_lea_rdx(Asm *a, int target) {
    int32_t rel;
    asm_hex(a, "48 8D 15");
    if (a->n + 4 > a->cap) { a->bad = 1; return; }
    rel = (int32_t)(target - (a->base + a->n + 4));
    memcpy(a->b + a->n, &rel, 4);
    a->n += 4;
}

int pat_find(const uint8_t *data, size_t n, const char *hex, int *at) {
    uint8_t pat[96];
    Asm a;
    size_t i;
    int hits = 0;
    a.b = pat; a.n = 0; a.base = 0; a.cap = (int)sizeof pat; a.bad = 0;
    asm_hex(&a, hex);
    *at = -1;
    if (a.bad || a.n == 0 || (size_t)a.n > n) return 0;
    for (i = 0; i + (size_t)a.n <= n; i++) {
        if (memcmp(data + i, pat, (size_t)a.n) != 0) continue;
        hits++;
        *at = (int)i;
    }
    return hits == 1;
}

/* 同 pat_find，但 "?" 字节匹配任意值；最多填 max 个命中，返回命中总数。 */
int pat_findall(const uint8_t *data, size_t n, const char *hex, int *at, int max) {
    uint8_t pat[96], any[96];
    int m = 0, hits = 0, hi = -1;
    size_t i;
    const char *s;
    for (s = hex; *s && m < (int)sizeof pat; s++) {
        int v;
        if (*s == '?') { if (hi < 0) { pat[m] = 0; any[m++] = 1; } continue; }
        if (*s >= '0' && *s <= '9') v = *s - '0';
        else if (*s >= 'A' && *s <= 'F') v = *s - 'A' + 10;
        else if (*s >= 'a' && *s <= 'f') v = *s - 'a' + 10;
        else continue;
        if (hi < 0) hi = v;
        else { pat[m] = (uint8_t)(hi * 16 + v); any[m++] = 0; hi = -1; }
    }
    if (m == 0 || (size_t)m > n) return 0;
    for (i = 0; i + (size_t)m <= n; i++) {
        int k;
        if (data[i] != pat[0] && !any[0]) continue;
        for (k = 0; k < m; k++)
            if (!any[k] && data[i + k] != pat[k]) break;
        if (k < m) continue;
        if (hits < max) at[hits] = (int)i;
        hits++;
    }
    return hits;
}

int pat_is(const uint8_t *data, size_t n, int at, const char *hex) {
    uint8_t pat[96];
    Asm a;
    a.b = pat; a.n = 0; a.base = 0; a.cap = (int)sizeof pat; a.bad = 0;
    asm_hex(&a, hex);
    if (a.bad || at < 0 || (size_t)at + (size_t)a.n > n) return 0;
    return memcmp(data + at, pat, (size_t)a.n) == 0;
}

int ru16(const uint8_t *p) { return p[0] | (p[1] << 8); }
