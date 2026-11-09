/* 堆叠：买、卖和货物代码里的补丁点，以及写入时先移除的旧版
 * （1150da3、v3.1）布局。 */

#include "patch/stack/stack.h"

/* 旧版在这里放大空闲舱位容量，但这不是占格的那次点击，
 * 所以写入时放回原版 33 字节。 */
const uint8_t kCargoVanilla[] = {
    0x48, 0x89, 0x4C, 0x24, 0x08, 0x57,
    0x48, 0x8B, 0x44, 0x24, 0x10,
    0x48, 0x8B, 0x4C, 0x24, 0x10,
    0x8B, 0x09,
    0x8B, 0x40, 0x04,
    0x2B, 0xC1,
    0x48, 0x8B, 0x4C, 0x24, 0x10,
    0x2B, 0x41, 0x08,
    0x5F, 0xC3
};

static const uint8_t kCargoHead[] = {
    0x48, 0x89, 0x4C, 0x24, 0x08, 0x57,
    0x48, 0x8B, 0x44, 0x24, 0x10,
    0x8B, 0x40, 0x04,
    0x6B, 0xC0
};

static const uint8_t kCargoTail[] = {
    0x48, 0x8B, 0x4C, 0x24, 0x10,
    0x2B, 0x01,
    0x48, 0x8B, 0x4C, 0x24, 0x10,
    0x2B, 0x41, 0x08,
    0x5F, 0xC3
};

_Static_assert(sizeof kCargoVanilla == 33, "vanilla cargo free-space size");

_Static_assert(sizeof kCargoHead + 1 + sizeof kCargoTail == 34, "stacked cargo free-space size");

int find_cargo_stack(const uint8_t *data, size_t n, int *at, uint32_t *stack) {
    size_t i;
    int hits = 0;
    int found = 0;
    *at = -1;
    *stack = 1;
    if (n < sizeof kCargoVanilla) return 0;
    for (i = 0; i + sizeof kCargoVanilla <= n; i++) {
        if (memcmp(data + i, kCargoVanilla, sizeof kCargoVanilla) == 0) {
            hits++;
            *at = (int)i;
            *stack = 1;
            found = 1;
        }
    }
    if (n >= sizeof kCargoHead + 1 + sizeof kCargoTail) {
        for (i = 0; i + sizeof kCargoHead + 1 + sizeof kCargoTail <= n; i++) {
            if (memcmp(data + i, kCargoHead, sizeof kCargoHead) != 0) continue;
            if (memcmp(data + i + sizeof kCargoHead + 1, kCargoTail, sizeof kCargoTail) != 0)
                continue;
            hits++;
            *at = (int)i;
            *stack = data[i + sizeof kCargoHead];
            found = 2;
        }
    }
    if (hits != 1) return 0;
    return found;
}

/* 买入循环：已占的格跳过；同货且未满时改为增加件数。
 * 各距离从 jne 起算，四个语言版主程序相同。 */
static const uint8_t kSlotSig[] = {
    0x48, 0x89, 0x44, 0x24, 0x68,
    0x48, 0x8B, 0x4C, 0x24, 0x68,
    0xE8
};

static const uint8_t kSlotAfter[] = {
    0x85, 0xC0, 0x0F, 0x85
};

static const uint8_t kSlotNext[] = {
    0x8B, 0x44, 0x24, 0x44, 0x39, 0x44, 0x24, 0x40
};

int find_slot_pat(const uint8_t *data, size_t n, int *at) {
    size_t i;
    int hits = 0;
    *at = -1;
    if (n < 32) return 0;
    for (i = 0; i + 31 < n; i++) {
        if (memcmp(data + i, kSlotSig, sizeof kSlotSig) != 0) continue;
        if (memcmp(data + i + 15, kSlotAfter, sizeof kSlotAfter) != 0) continue;
        if (memcmp(data + i + 23, kSlotNext, sizeof kSlotNext) != 0) continue;
        hits++;
        *at = (int)i;
    }
    return hits == 1;
}

int slot_call_off(const uint8_t *data, size_t n, int at) {
    int32_t rel;
    if (at < 0 || (size_t)at + 5 > n || data[at] != 0xE8) return -1;
    memcpy(&rel, data + at + 1, 4);
    return at + 5 + rel;
}

int slot_cave_off(const uint8_t *data, size_t n, int pat) {
    size_t i, end;
    if (pat < 0) return -1;
    end = (size_t)pat + 0x300;
    if (end > n) end = n;
    /* 打补丁后 pop/ret 之后的字节是代码洞，不再是 CC。 */
    for (i = (size_t)pat; i + 8 < end; i++) {
        if (data[i] != 0x5F || data[i + 1] != 0xC3) continue;
        if (data[i + 2] == 0xCC) return (int)(i + 2);
        if (data[i + 2] == 0x48 && data[i + 3] == 0x8B && data[i + 4] == 0x84)
            return (int)(i + 2);
    }
    return -1;
}

static int slot_rel(uint8_t *code, int at, int insn_off, int target_off) {
    int32_t rel = (int32_t)(target_off - (insn_off + 5));
    memcpy(code + at + 1, &rel, 4);
    return 1;
}

void slot_jmp(uint8_t *data, int from, int to) {
    int32_t rel = (int32_t)(to - (from + 5));
    data[from] = 0xE9;
    memcpy(data + from + 1, &rel, 4);
}

static void slot_lea32(uint8_t *code, int at, int insn_off, int target_off) {
    int32_t rel = (int32_t)(target_off - (insn_off + 7));
    memcpy(code + at + 3, &rel, 4);
}

int read_slot_stack(const uint8_t *data, size_t n, int pat, int cave, uint32_t *stack) {
    int32_t rel;
    int jne, target;
    if (pat < 0 || (size_t)pat + kSlotJne + 6 > n) return 0;
    jne = pat + kSlotJne;
    if (data[jne] != 0x0F || data[jne + 1] != 0x85) return 0;
    memcpy(&rel, data + jne + 2, 4);
    target = jne + 6 + rel;
    if (target == pat + kSlotJne + kSlotSkip) {
        *stack = 1;
        return 1;
    }
    if (cave >= 0 && target == cave && data[cave] == 0x48 &&
        data[cave + 1] == 0x8B && data[cave + 2] == 0x84) {
        uint32_t s = 0;
        if ((size_t)cave + kSlotImmAt < n &&
            data[cave + kSlotImmAt - 3] == 0x83 &&
            data[cave + kSlotImmAt - 2] == 0x78 &&
            data[cave + kSlotImmAt - 1] == 0x08) {
            s = data[cave + kSlotImmAt];
            if (s >= 1 && s <= 10) {
                *stack = s;
                return 2;
            }
        }
        if ((size_t)cave + kSlotOldImmAt + 4 <= n &&
            data[cave + kSlotOldImmAt - 1] == 0xB9) {
            memcpy(&s, data + cave + kSlotOldImmAt, 4);
            if (s >= 1 && s <= 10) {
                *stack = s;
                return 2;
            }
        }
    }
    return 0;
}

static int find_jmp_to(const uint8_t *data, int lo, int hi, int target) {
    int i;
    int32_t rel;
    if (lo < 0) lo = 0;
    for (i = lo; i + 5 <= hi; i++) {
        if (data[i] != 0xE9) continue;
        memcpy(&rel, data + i + 1, 4);
        if (i + 5 + rel == target) return i;
    }
    return -1;
}

int find_sell_site(const uint8_t *data, size_t n, int *at) {
    static const uint8_t head[] = {0x48, 0x8B, 0x4C, 0x24, 0x28, 0xE8};
    static const uint8_t tail[] = {0x41, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x8B, 0x54, 0x24, 0x64};
    static const uint8_t epi[] = {0x48, 0x81, 0xC4, 0x80, 0x00, 0x00, 0x00, 0x5F, 0xC3};
    size_t i;
    int hits = 0;
    *at = -1;
    for (i = 0; i + 20 <= n; i++) {
        if (memcmp(data + i, head, 6) != 0) continue;
        if (memcmp(data + i + 10, tail, 10) != 0) continue;
        hits++;
        *at = (int)i;
    }
    if (hits == 1) return 1;
    for (i = 0; i + 9 + kSellPadLen <= n; i++) {
        int spad, site;
        if (memcmp(data + i, epi, 9) != 0) continue;
        spad = (int)(i + 9);
        if (memcmp(data + spad, head, 6) != 0) continue;
        site = find_jmp_to(data, spad - 0x200, spad, spad + kSellHookAt);
        if (site >= 0) {
            *at = site;
            return 1;
        }
    }
    return 0;
}

static int find_sell_pad(const uint8_t *data, size_t n, int sell_at) {
    static const uint8_t epi[] = {0x48, 0x81, 0xC4, 0x80, 0x00, 0x00, 0x00, 0x5F, 0xC3};
    size_t i, end;
    if (sell_at < 0) return -1;
    end = (size_t)sell_at + 0xC0;
    if (end > n) end = n;
    for (i = (size_t)sell_at; i + 9 <= end; i++) {
        if (memcmp(data + i, epi, 9) != 0) continue;
        if (i + 9 + kSellPadLen > n) return -1;
        return (int)(i + 9);
    }
    return -1;
}

static int find_sell_imul(const uint8_t *data, size_t n, int sell_at, int spad) {
    static const uint8_t pat[] = {
        0x8B, 0xD0, 0x48, 0x8B, 0x84, 0x24, 0x90, 0x00, 0x00, 0x00
    };
    int last = -1, site;
    size_t i, end;
    if (sell_at < 0) return -1;
    if (spad >= 0) {
        site = find_jmp_to(data, sell_at, sell_at + 0x80, spad + kSellImulAt);
        if (site >= 0) return site;
    }
    end = (size_t)sell_at + 0x80;
    if (end > n) end = n;
    for (i = (size_t)sell_at; i + sizeof pat <= end; i++) {
        if (memcmp(data + i, pat, sizeof pat) == 0) last = (int)i;
    }
    return last;
}

int find_disp_site(const uint8_t *data, size_t n, int cave, int *at) {
    static const uint8_t pat[] = {
        0xEB, 0x34, 0x48, 0x8B, 0x8C, 0x24, 0x68, 0x03, 0x00, 0x00, 0xE8
    };
    size_t i;
    int hits = 0, last = -1, site;
    *at = -1;
    for (i = 0; i + sizeof pat <= n; i++) {
        if (memcmp(data + i, pat, sizeof pat) != 0) continue;
        hits++;
        last = (int)i;
    }
    if (hits == 2) {
        *at = last + 2;
        return 1;
    }
    if (cave < 0) return 0;
    site = find_jmp_to(data, 0, (int)n - 5, cave + kSlotDispAt + 28);
    if (site < 0) return 0;
    *at = site;
    return 1;
}

static int find_pack_site(const uint8_t *data, size_t n, int *at) {
    static const uint8_t pat[] = {
        0x4C, 0x8B, 0x44, 0x24, 0x68, 0x8B, 0x54, 0x24, 0x54,
        0x48, 0x8B, 0x4C, 0x24, 0x38, 0xE8
    };
    size_t i;
    int hits = 0, pad, site;
    *at = -1;
    for (i = 0; i + sizeof pat <= n; i++) {
        if (memcmp(data + i, pat, sizeof pat) != 0) continue;
        hits++;
        *at = (int)i + 14;
    }
    if (hits == 1) return 1;
    for (i = 0; i + 16 <= n; i++) {
        if (data[i] != 0x5F || data[i + 1] != 0xC3 || data[i + 2] != 0xE8) continue;
        pad = (int)(i + 2);
        site = find_jmp_to(data, pad - 0x80, pad, pad + kPackHookAt);
        if (site >= 0) {
            *at = site;
            return 1;
        }
    }
    return 0;
}

static int find_pack_pad(const uint8_t *data, size_t n, int pack_at) {
    size_t i, end;
    if (pack_at < 0) return -1;
    end = (size_t)pack_at + 0x50;
    if (end > n) end = n;
    for (i = (size_t)pack_at; i + 3 < end; i++) {
        if (data[i] == 0x5F && data[i + 1] == 0xC3 &&
            (data[i + 2] == 0xCC || data[i + 2] == 0xE8))
            return (int)(i + 2);
    }
    return -1;
}

static int find_unpack_site(const uint8_t *data, size_t n, int *at) {
    static const uint8_t pat[] = {
        0x45, 0x33, 0xC0, 0x48, 0x8B, 0x8C, 0x24, 0xA8, 0x00, 0x00, 0x00,
        0x48, 0x8B, 0x11, 0x48, 0x8B, 0xC8, 0xE8
    };
    static const uint8_t epi[] = {0x48, 0x81, 0xC4, 0xD0, 0x00, 0x00, 0x00, 0x5F, 0xC3};
    size_t i;
    int hits = 0, pad, site;
    *at = -1;
    for (i = 0; i + sizeof pat <= n; i++) {
        if (memcmp(data + i, pat, sizeof pat) != 0) continue;
        hits++;
        *at = (int)i;
    }
    if (hits == 1) return 1;
    for (i = 0; i + 9 + 16 <= n; i++) {
        if (memcmp(data + i, epi, 9) != 0) continue;
        pad = (int)(i + 9);
        if (memcmp(data + pad, "\x45\x33\xC0", 3) != 0) continue;
        site = find_jmp_to(data, pad - 0x180, pad, pad + kUnpackHookAt);
        if (site >= 0) {
            *at = site;
            return 1;
        }
    }
    return 0;
}

static int find_unpack_pad(const uint8_t *data, size_t n, int unpack_at) {
    static const uint8_t epi[] = {0x48, 0x81, 0xC4, 0xD0, 0x00, 0x00, 0x00, 0x5F, 0xC3};
    size_t i, end;
    if (unpack_at < 0) return -1;
    end = (size_t)unpack_at + 0x180;
    if (end > n) end = n;
    for (i = (size_t)unpack_at; i + 9 <= end; i++) {
        if (memcmp(data + i, epi, 9) != 0) continue;
        return (int)(i + 9);
    }
    return -1;
}

static int find_list_inc(const uint8_t *data, size_t n, const uint8_t *pat, size_t pn, int hook, int spad, int *at) {
    size_t i;
    int hits = 0, site;
    *at = -1;
    for (i = 0; i + pn <= n; i++) {
        if (memcmp(data + i, pat, pn) != 0) continue;
        hits++;
        *at = (int)i;
    }
    if (hits == 1) return 1;
    if (spad < 0) return 0;
    site = find_jmp_to(data, 0, (int)n - 5, spad + hook);
    if (site < 0) return 0;
    *at = site;
    return 1;
}

int patch_stack_extras(uint8_t *data, size_t n, int cave, uint32_t stack, wchar_t *err, int errn) {
    static const uint8_t list_a0[] = {0x8B, 0x84, 0x84, 0xA0, 0x00, 0x00, 0x00, 0xFF, 0xC0};
    static const uint8_t list_110[] = {0x8B, 0x84, 0x84, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xC0};
    static const uint8_t fmt[] = {'%', 'd', 'x', ' ', '%', '6', 'd', 0};
    int sell, spad, imul, disp, la0, l110, npos, i;
    int pack, ppad, unpack, upad, c151;
    int c14, c110, cf3;
    int32_t rel;
    uint8_t sell_save[16], disp_save[28];
    uint8_t *p;

    if (!find_sell_site(data, n, &sell)) {
        StringCchCopyW(err, (size_t)errn, L"找不到卖出商品的位置。");
        return 0;
    }
    spad = find_sell_pad(data, n, sell);
    if (spad < 0) {
        StringCchCopyW(err, (size_t)errn, L"卖出函数后面没有空位，不能改堆叠。");
        return 0;
    }
    imul = find_sell_imul(data, n, sell, spad);
    if (imul < 0) {
        StringCchCopyW(err, (size_t)errn, L"找不到卖出时的数量计算。");
        return 0;
    }
    if (!find_disp_site(data, n, cave, &disp)) {
        StringCchCopyW(err, (size_t)errn, L"找不到舱位价格显示的位置。");
        return 0;
    }
    if (!find_list_inc(data, n, list_a0, sizeof list_a0, kListA0At, spad, &la0) ||
        !find_list_inc(data, n, list_110, sizeof list_110, kList110At, spad, &l110)) {
        StringCchCopyW(err, (size_t)errn, L"找不到交易所货物计数的位置。");
        return 0;
    }
    if (!find_pack_site(data, n, &pack) || (ppad = find_pack_pad(data, n, pack)) < 0) {
        StringCchCopyW(err, (size_t)errn, L"找不到关上交易行时写回船上货物的位置。");
        return 0;
    }
    if (!find_unpack_site(data, n, &unpack) || (upad = find_unpack_pad(data, n, unpack)) < 0) {
        StringCchCopyW(err, (size_t)errn, L"找不到打开交易行时读出船上货物的位置。");
        return 0;
    }
    if (data[pack] == 0xE8) c151 = slot_call_off(data, n, pack);
    else {
        memcpy(&rel, data + ppad + 1, 4);
        c151 = pack + 5 + rel;
    }
    if (c151 < 0) {
        StringCchCopyW(err, (size_t)errn, L"写回船上货物的调用对不上。");
        return 0;
    }

    if (data[sell] == 0x48 && data[sell + 5] == 0xE8)
        c14 = slot_call_off(data, n, sell + 5);
    else {
        memcpy(&rel, data + spad + 6, 4);
        c14 = sell + 10 + rel;
    }
    if (data[disp] == 0x48 && data[disp + 8] == 0xE8)
        cf3 = slot_call_off(data, n, disp + 8);
    else if ((size_t)cave + kSlotDispAt + 13 <= n && data[cave + kSlotDispAt + 8] == 0xE8) {
        memcpy(&rel, data + cave + kSlotDispAt + 9, 4);
        cf3 = disp + 13 + rel;
    } else cf3 = -1;
    if (disp >= 0x72 && data[disp - 0x72] == 0xE8)
        c110 = slot_call_off(data, n, disp - 0x72);
    else c110 = -1;
    if (c14 < 0 || cf3 < 0 || c110 < 0) {
        StringCchCopyW(err, (size_t)errn, L"卖出或显示里的调用对不上。");
        return 0;
    }

    if (data[sell] != 0xE9) memcpy(sell_save, data + sell, 16);
    else memcpy(sell_save, data + spad, 16);
    if (data[disp] != 0xE9) memcpy(disp_save, data + disp, 28);
    else memcpy(disp_save, data + cave + kSlotDispAt, 28);

    if (stack <= 1) {
        if (data[sell] == 0xE9) memcpy(data + sell, sell_save, 16);
        if (data[imul] == 0xE9)
            memcpy(data + imul, "\x8B\xD0\x48\x8B\x84\x24\x90\x00\x00\x00", 10);
        if (data[disp] == 0xE9) memcpy(data + disp, disp_save, 28);
        if (data[la0] == 0xE9) memcpy(data + la0, list_a0, sizeof list_a0);
        if (data[l110] == 0xE9) memcpy(data + l110, list_110, sizeof list_110);
        if (data[pack] == 0xE9) {
            data[pack] = 0xE8;
            slot_rel(data, pack, pack, c151);
        }
        if (data[unpack] == 0xE9) {
            memcpy(data + unpack, "\x45\x33\xC0\x48\x8B\x8C\x24\xA8\x00\x00\x00", 11);
        }
        memset(data + spad, 0xCC, kSellPadLen);
        memset(data + ppad, 0xCC, 84);
        memset(data + upad, 0xCC, 80);
        return 1;
    }

    memcpy(data + cave + kSlotFmtAt, fmt, sizeof fmt);
    memcpy(data + cave + kSlotDispAt, disp_save, 28);
    memset(data + spad, 0xCC, kSellPadLen);
    memcpy(data + spad, sell_save, 16);

    p = data + spad;
    npos = kSellHookAt;
    p[npos++] = 0x48; p[npos++] = 0x8B; p[npos++] = 0x44; p[npos++] = 0x24; p[npos++] = 0x28;
    p[npos++] = 0x44; p[npos++] = 0x8B; p[npos++] = 0x40; p[npos++] = 0x08;
    p[npos++] = 0x45; p[npos++] = 0x85; p[npos++] = 0xC0;
    p[npos++] = 0x75; p[npos++] = 0x06;
    p[npos++] = 0x41; p[npos++] = 0xB8; p[npos++] = 0x01; p[npos++] = 0x00; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0x44; p[npos++] = 0x89; p[npos++] = 0x44; p[npos++] = 0x24; p[npos++] = 0x4C;
    p[npos++] = 0x48; p[npos++] = 0x8B; p[npos++] = 0xC8;
    p[npos++] = 0xE8; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, spad + npos - 5, c14);
    p[npos++] = 0x44; p[npos++] = 0x8B; p[npos++] = 0x44; p[npos++] = 0x24; p[npos++] = 0x4C;
    p[npos++] = 0xE9; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, spad + npos - 5, sell + 16);
    slot_jmp(data, sell, spad + kSellHookAt);
    for (i = 5; i < 16; i++) data[sell + i] = 0x90;

    npos = kSellImulAt;
    p[npos++] = 0x8B; p[npos++] = 0xD0;
    p[npos++] = 0x0F; p[npos++] = 0xAF; p[npos++] = 0x54; p[npos++] = 0x24; p[npos++] = 0x4C;
    p[npos++] = 0x48; p[npos++] = 0x8B; p[npos++] = 0x84; p[npos++] = 0x24;
    p[npos++] = 0x90; p[npos++] = 0x00; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0xE9; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, spad + npos - 5, imul + 10);
    slot_jmp(data, imul, spad + kSellImulAt);
    for (i = 5; i < 10; i++) data[imul + i] = 0x90;

    npos = kListA0At;
    p[npos++] = 0x8B; p[npos++] = 0x84; p[npos++] = 0x84;
    p[npos++] = 0xA0; p[npos++] = 0x00; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0x48; p[npos++] = 0x8B; p[npos++] = 0x8C; p[npos++] = 0x24;
    p[npos++] = 0x68; p[npos++] = 0x04; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0x03; p[npos++] = 0x41; p[npos++] = 0x08;
    p[npos++] = 0xE9; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, spad + npos - 5, la0 + 9);
    slot_jmp(data, la0, spad + kListA0At);
    for (i = 5; i < 9; i++) data[la0 + i] = 0x90;

    npos = kList110At;
    p[npos++] = 0x8B; p[npos++] = 0x84; p[npos++] = 0x84;
    p[npos++] = 0x10; p[npos++] = 0x01; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0x48; p[npos++] = 0x8B; p[npos++] = 0x8C; p[npos++] = 0x24;
    p[npos++] = 0x78; p[npos++] = 0x04; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0x03; p[npos++] = 0x41; p[npos++] = 0x08;
    p[npos++] = 0xE9; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, spad + npos - 5, l110 + 9);
    slot_jmp(data, l110, spad + kList110At);
    for (i = 5; i < 9; i++) data[l110 + i] = 0x90;

    p = data + cave + kSlotDispAt + 28;
    npos = 0;
    p[npos++] = 0x48; p[npos++] = 0x8B; p[npos++] = 0x8C; p[npos++] = 0x24;
    p[npos++] = 0x68; p[npos++] = 0x03; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0xE8; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, cave + kSlotDispAt + 28 + npos - 5, c110);
    p[npos++] = 0x44; p[npos++] = 0x8B; p[npos++] = 0xC0;
    p[npos++] = 0x48; p[npos++] = 0x8B; p[npos++] = 0x8C; p[npos++] = 0x24;
    p[npos++] = 0x68; p[npos++] = 0x03; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0xE8; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, cave + kSlotDispAt + 28 + npos - 5, cf3);
    p[npos++] = 0x44; p[npos++] = 0x8B; p[npos++] = 0xC8;
    p[npos++] = 0x48; p[npos++] = 0x8D; p[npos++] = 0x15; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_lea32(p, npos - 7, cave + kSlotDispAt + 28 + npos - 7, cave + kSlotFmtAt);
    p[npos++] = 0x48; p[npos++] = 0x8D; p[npos++] = 0x4C; p[npos++] = 0x24; p[npos++] = 0x38;
    p[npos++] = 0xE9; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, cave + kSlotDispAt + 28 + npos - 5, disp + 28);
    slot_jmp(data, disp, cave + kSlotDispAt + 28);
    for (i = 5; i < 28; i++) data[disp + i] = 0x90;

    /* 关闭交易所：把 slot+8 复制到 8 字节船舱记录的 +3。 */
    if (data[pack] != 0xE9) memcpy(data + ppad, data + pack, 5);
    p = data + ppad;
    npos = kPackHookAt;
    p[npos++] = 0xE8; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, ppad + npos - 5, c151);
    p[npos++] = 0x48; p[npos++] = 0x8B; p[npos++] = 0x44; p[npos++] = 0x24; p[npos++] = 0x68;
    p[npos++] = 0x8B; p[npos++] = 0x48; p[npos++] = 0x08;
    p[npos++] = 0x85; p[npos++] = 0xC9;
    p[npos++] = 0x75; p[npos++] = 0x05;
    p[npos++] = 0xB9; p[npos++] = 0x01; p[npos++] = 0x00; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0x8B; p[npos++] = 0x44; p[npos++] = 0x24; p[npos++] = 0x54;
    p[npos++] = 0x48; p[npos++] = 0x63; p[npos++] = 0xC0;
    p[npos++] = 0x48; p[npos++] = 0x8B; p[npos++] = 0x54; p[npos++] = 0x24; p[npos++] = 0x38;
    p[npos++] = 0x88; p[npos++] = 0x4C; p[npos++] = 0xC2; p[npos++] = 0x5B;
    p[npos++] = 0xE9; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, ppad + npos - 5, pack + 5);
    slot_jmp(data, pack, ppad + kPackHookAt);

    /* 打开交易所：从船舱记录 +3 字节还原 slot+8。 */
    if (data[unpack] != 0xE9) memcpy(data + upad, data + unpack, 11);
    p = data + upad;
    npos = kUnpackHookAt;
    p[npos++] = 0x48; p[npos++] = 0x8B; p[npos++] = 0x8C; p[npos++] = 0x24;
    p[npos++] = 0xA8; p[npos++] = 0x00; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0x44; p[npos++] = 0x0F; p[npos++] = 0xB6; p[npos++] = 0x41; p[npos++] = 0x03;
    p[npos++] = 0x45; p[npos++] = 0x85; p[npos++] = 0xC0;
    p[npos++] = 0x75; p[npos++] = 0x06;
    p[npos++] = 0x41; p[npos++] = 0xB8; p[npos++] = 0x01; p[npos++] = 0x00; p[npos++] = 0x00; p[npos++] = 0x00;
    p[npos++] = 0xE9; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0; p[npos++] = 0;
    slot_rel(p, npos - 5, upad + npos - 5, unpack + 11);
    slot_jmp(data, unpack, upad + kUnpackHookAt);
    for (i = 5; i < 11; i++) data[unpack + i] = 0x90;
    return 1;
}
