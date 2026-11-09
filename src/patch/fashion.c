#include "common/bytes.h"
#include "exe/pe.h"
#include "exe/x64asm.h"
#include "patch/caps.h"
#include "patch/cave.h"
#include "patch/stack/stack.h"
#include "patch/fashion.h"

/* 交易所卖流行品（"DK4FSH1"）。
 *
 * 确认交易（CTrade::CommitDeal）时，本城两种流行品的卖出件数 x 影响占有率。原版：
 *   占有率 < 80：每种需卖 >= ceil(船舱格数 / 2) 件，然后占有率 += x / 6 + 1（游戏封顶 100）；
 *   占有率 >= 80：x 在 10..15 给介绍信，x >= 16 给推荐信（或把介绍信升级）。
 * 这些常数按一格一件设计，本补丁改为可设：格数 * fthr / 2（向上取整）、
 * x / fdiv + 1、fl1..fl2-1、fl2。
 *
 * 原地改：半数格数的计算和除数。
 * 钩子：四处 "cmp [rsp+0x564], imm8 / jcc" 信件判断需要 imm32，跳到 int3 空位里的桩代码。
 * 代码洞放在该空位末尾（堆叠代码洞在开头），头和原字节表格式同 DK4STK3，
 * 四个值在 +16。全是原版值时不写任何东西。 */
#define FASH_MAGIC "DK4FSH1"

enum { kFsHead = 32, kFsMax = 0x200, kFsThrLen = 61 };

typedef struct FashSites {
    int thr;        /* s4D0 = ceil(s4A4 / 2)，61 字节 */
    int div;        /* mov eax,[x] / cdq / mov ecx,6 / idiv */
    int l1;         /* cmp x,10 / jl l2a */
    int l1h;        /* cmp x,15 / jg l2a */
    int l2a;        /* cmp x,16 / jl（还没有信） */
    int l2b;        /* cmp x,16 / jl（已有介绍信） */
} FashSites;

static int fash_jcc_target(const uint8_t *data, int at) {
    int32_t rel;
    memcpy(&rel, data + at + 10, 4);
    return at + 14 + rel;
}

/* 要求原版字节（已装的补丁需先移除）。 */
static int fash_find(const uint8_t *data, size_t n, FashSites *s) {
    int hits[4];
    if (pat_findall(data, n, "8B 84 24 A4 04 00 00 99 83 E0 01 33 C2 2B C2 85 C0 74 17"
                            " 8B 84 24 A4 04 00 00 99 2B C2 D1 F8 FF C0 89 84 24 D0 04 00 00 EB 13"
                            " 8B 84 24 A4 04 00 00 99 2B C2 D1 F8 89 84 24 D0 04 00 00", hits, 4) != 1)
        return 0;
    s->thr = hits[0];
    if (pat_findall(data, n, "8B 84 24 60 05 00 00 99 B9 06 00 00 00 F7 F9 FF C0 89 84 24", hits, 4) != 1)
        return 0;
    s->div = hits[0];
    if (pat_findall(data, n, "83 BC 24 64 05 00 00 0A 0F 8C", hits, 4) != 1) return 0;
    s->l1 = hits[0];
    s->l1h = s->l1 + 14;
    if (!pat_is(data, n, s->l1h, "83 BC 24 64 05 00 00 0F 0F 8F")) return 0;
    if (pat_findall(data, n, "83 BC 24 64 05 00 00 10 0F 8C", hits, 4) != 2) return 0;
    s->l2a = fash_jcc_target(data, s->l1);
    if (fash_jcc_target(data, s->l1h) != s->l2a) return 0;
    if (hits[0] == s->l2a) s->l2b = hits[1];
    else if (hits[1] == s->l2a) s->l2b = hits[0];
    else return 0;
    /* 都在 CommitDeal 内，且按此顺序。 */
    if (!(s->thr < s->div && s->div < s->l1 && s->l1 < s->l2a && s->l2a < s->l2b) ||
        s->l2b - s->thr > 0x2000)
        return 0;
    return 1;
}

static int fash_cave(const uint8_t *data, size_t n) { return cave_find(data, n, FASH_MAGIC, 1, 1); }

static int fash_is_vanilla(const CapValues *v) {
    return v->fthr == kFashThr && v->fdiv == kFashDiv && v->fl1 == kFashL1 && v->fl2 == kFashL2;
}

/* 读出主程序里的值；未装补丁时为原版值。 */
int fash_read(const uint8_t *data, size_t n, CapValues *v) {
    int cave = fash_cave(data, n);
    v->fthr = kFashThr;
    v->fdiv = kFashDiv;
    v->fl1 = kFashL1;
    v->fl2 = kFashL2;
    if (cave < 0) return 1;
    if ((size_t)cave + kFsHead > n) return 0;
    v->fthr = ru32(data + cave + 16);
    v->fdiv = ru32(data + cave + 20);
    v->fl1 = ru32(data + cave + 24);
    v->fl2 = ru32(data + cave + 28);
    return 1;
}

int fash_remove(uint8_t *data, size_t n) {
    int cave = fash_cave(data, n);
    return cave < 0 || cave_restore(data, n, cave, kFsMax);
}

/* 各补丁点必须是原版字节（先调 fash_remove）。 */
int fash_apply(uint8_t *data, size_t n, const CapValues *v, wchar_t *err, int errn) {
    uint8_t code[kFsMax];
    Asm a;
    PatchSite site[8];
    FashSites s;
    int ns = 0, i, j, cave, tx_raw, tx_size, tx_delta, at;
#define FASH_ERR(msg) do { StringCchCopyW(err, (size_t)errn, msg); return 0; } while (0)
#define FASH_SITE(where, length, how) do { \
        site[ns].at = (where); site[ns].len = (length); site[ns].kind = (how); \
        site[ns].entry = a.n; ns++; \
    } while (0)
    if (fash_is_vanilla(v)) return 1;
    if (v->fdiv < 1 || v->fl1 < 1 || v->fl2 <= v->fl1 || v->fthr > 0x7FFFFF || v->fl2 > 0x7FFFFFFF)
        FASH_ERR(L"流行品的数值不对：除数至少 1，推荐信件数要大于介绍信件数。");
    if (!fash_find(data, n, &s)) FASH_ERR(L"找不到交易所卖流行品的代码，或和原版对不上。");

    /* 取跳板表后 int3 空位的末尾。 */
    if (!pe_text(data, n, &tx_raw, &tx_size, &tx_delta)) FASH_ERR(L"主程序的 .text 段认不出来。");
    cave = -1;
    for (i = tx_raw; i < tx_raw + tx_size; ) {
        if (data[i] != 0xCC) { i++; continue; }
        for (j = i; j < tx_raw + tx_size && data[j] == 0xCC; j++) {}
        if (j - i >= 0x8000) {
            cave = (j - kFsMax - 16) & ~15;
            /* 避开空位开头的堆叠代码洞。 */
            if (cave - i < kCaveMax + 0x100) cave = -1;
            break;
        }
        i = j;
    }
    if (cave < 0) FASH_ERR(L"主程序里找不到放流行品补丁的空位。");

    a.b = code; a.n = 0; a.base = cave; a.cap = kFsMax; a.bad = 0;
    memset(code, 0xCC, sizeof code);
    memcpy(code, FASH_MAGIC, 8);
    code[8] = 1;
    wu32(code + 16, v->fthr);
    wu32(code + 20, v->fdiv);
    wu32(code + 24, v->fl1);
    wu32(code + 28, v->fl2);
    a.n = kFsHead;

    /* cmp dword [rsp+0x564], imm32 / jcc 原目标 / jmp 回原处。 */
#define FASH_CMP(where, imm, cc) do { \
        FASH_SITE(where, 14, 0); \
        asm_hex(&a, "81 BC 24 64 05 00 00"); \
        asm_u32(&a, (imm)); \
        at = asm_jcc(&a, (cc)); \
        asm_fix(&a, at, fash_jcc_target(data, (where)) - cave); \
        asm_jmp(&a, (where) + 14); \
    } while (0)
    FASH_CMP(s.l1, v->fl1, 0x8C);
    FASH_CMP(s.l1h, v->fl2 - 1, 0x8F);
    FASH_CMP(s.l2a, v->fl2, 0x8C);
    FASH_CMP(s.l2b, v->fl2, 0x8C);
#undef FASH_CMP
    {
        int jumps = ns;
        FASH_SITE(s.thr, kFsThrLen, 3);
        FASH_SITE(s.div + 9, 4, 3);
        code[9] = (uint8_t)ns;
        code[10] = (uint8_t)(a.n & 0xFF);
        code[11] = (uint8_t)(a.n >> 8);
        for (i = 0; i < ns; i++) {
            uint32_t off = (uint32_t)site[i].at;
            if (a.n + 5 + site[i].len > kFsMax) { a.bad = 1; break; }
            memcpy(code + a.n, &off, 4);
            code[a.n + 4] = (uint8_t)site[i].len;
            memcpy(code + a.n + 5, data + site[i].at, (size_t)site[i].len);
            a.n += 5 + site[i].len;
        }
        code[12] = (uint8_t)(a.n & 0xFF);
        code[13] = (uint8_t)(a.n >> 8);
        if (a.bad) FASH_ERR(L"流行品补丁放不下。");
        for (i = 0; i < a.n; i++)
            if (data[cave + i] != 0xCC) FASH_ERR(L"放流行品补丁的空位被占用。");
        memcpy(data + cave, code, (size_t)a.n);
        for (i = 0; i < jumps; i++) {
            int k;
            slot_jmp(data, site[i].at, cave + site[i].entry);
            for (k = 5; k < site[i].len; k++) data[site[i].at + k] = 0x90;
        }
    }
    /* s4D0 = (s4A4 * fthr + 1) >> 1，即 格数 * fthr / 2 向上取整。 */
    {
        uint8_t *p = data + s.thr;
        memset(p, 0x90, kFsThrLen);
        memcpy(p, "\x8B\x84\x24\xA4\x04\x00\x00\x69\xC0", 9);
        wu32(p + 9, v->fthr);
        memcpy(p + 13, "\xFF\xC0\xD1\xE8\x89\x84\x24\xD0\x04\x00\x00", 11);
        p[24] = 0xEB;
        p[25] = (uint8_t)(kFsThrLen - 26);
    }
    wu32(data + s.div + 9, v->fdiv);
#undef FASH_SITE
#undef FASH_ERR
    return 1;
}
