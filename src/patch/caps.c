/* 等级/属性/技能/港口上限（原地改立即数），以及读写单个主程序映像中
 * 全部补丁的总入口 read_exe_image()/patch_exe_image()。 */

#include "common/bytes.h"
#include "patch/caps.h"
#include "patch/cave.h"
#include "patch/stack/stack.h"
#include "patch/fashion.h"
#include "patch/proxy_buy.h"

static int collect_fixed(const uint8_t *data, size_t n,
                         const uint8_t *pre, size_t pn,
                         const uint8_t *post, size_t qn,
                         int *at, uint32_t *val, int capn) {
    size_t i;
    int count = 0;
    if (pn + 4 + qn > n) return 0;
    for (i = 0; i + pn + 4 + qn <= n; i++) {
        if (memcmp(data + i, pre, pn) != 0) continue;
        if (memcmp(data + i + pn + 4, post, qn) != 0) continue;
        if (count < capn) {
            at[count] = (int)(i + pn);
            val[count] = ru32(data + i + pn);
        }
        count++;
    }
    return count;
}

/* 03 84 24 disp32  41 B8 imm32  8B D0 33 C9 */
static int collect_xp(const uint8_t *data, size_t n, int *at, uint32_t *val, int capn) {
    size_t i;
    int count = 0;
    for (i = 0; i + 17 <= n; i++) {
        if (data[i] != 0x03 || data[i + 1] != 0x84 || data[i + 2] != 0x24) continue;
        if (data[i + 7] != 0x41 || data[i + 8] != 0xB8) continue;
        if (data[i + 13] != 0x8B || data[i + 14] != 0xD0) continue;
        if (data[i + 15] != 0x33 || data[i + 16] != 0xC9) continue;
        if (count < capn) {
            at[count] = (int)(i + 9);
            val[count] = ru32(data + i + 9);
        }
        count++;
    }
    return count;
}

/* 81 7C 24 20 imm32  7E 08  C7 44 24 20 imm32 */
static int collect_skill(const uint8_t *data, size_t n, int *at, uint32_t *a, uint32_t *b, int capn) {
    size_t i;
    int count = 0;
    static const uint8_t mid[6] = {0x7E, 0x08, 0xC7, 0x44, 0x24, 0x20};
    for (i = 0; i + 18 <= n; i++) {
        if (data[i] != 0x81 || data[i + 1] != 0x7C || data[i + 2] != 0x24 || data[i + 3] != 0x20)
            continue;
        if (memcmp(data + i + 8, mid, 6) != 0) continue;
        if (count < capn) {
            at[count] = (int)(i + 4);
            a[count] = ru32(data + i + 4);
            b[count] = ru32(data + i + 14);
        }
        count++;
    }
    return count;
}

static int same_u32(const uint32_t *v, int n) {
    int i;
    for (i = 1; i < n; i++) if (v[i] != v[0]) return 0;
    return 1;
}

int read_exe_image(const uint8_t *data, size_t n, CapValues *v, int *xp_off, wchar_t *err, int errn) {
    static const uint8_t pre_lv[] = {0x89, 0x44, 0x24, 0x20, 0x41, 0xB8};
    static const uint8_t post_lv[] = {0x8B, 0x54, 0x24, 0x20, 0x33, 0xC9};
    static const uint8_t pre_inc[] = {0x0F, 0xB7, 0x44, 0x41, 0x4A, 0x03, 0x44, 0x24, 0x60, 0xBA};
    static const uint8_t post_inc[] = {0x8B, 0xC8};
    static const uint8_t pre_asg[] = {0x41, 0xB8};
    static const uint8_t post_asg[] = {0x8B, 0x54, 0x24, 0x24, 0xB9, 0x19, 0x00, 0x00, 0x00};
    static const uint8_t post_res[] = {0x8B, 0x54, 0x24, 0x30, 0x33, 0xC9};
    static const uint8_t pre_base[] = {
        0x0F, 0xB6, 0x44, 0x01, 0x0F, 0x03, 0x44, 0x24, 0x70, 0x41, 0xB8
    };
    static const uint8_t post_base[] = {0x8B, 0xD0, 0x33, 0xC9};
    /* jmp $-213; mov edx,[rsp+0x54]; mov ecx,imm32; call min */
    static const uint8_t pre_port[] = {
        0xE9, 0x2B, 0xFF, 0xFF, 0xFF, 0x8B, 0x54, 0x24, 0x54, 0xB9
    };
    static const uint8_t post_port[] = {0xE8};
    int at[8], sat[4];
    uint32_t val[8], sa[4], sb[4];
    int c, xs;
    uint32_t stats[3];
    memset(v, 0, sizeof *v);
    if (xp_off) *xp_off = 0;
    c = collect_fixed(data, n, pre_lv, sizeof pre_lv, post_lv, sizeof post_lv, at, val, 8);
    if (c != 2 || !same_u32(val, 2)) {
        StringCchCopyW(err, (size_t)errn, L"找不到单段等级上限，或两处数值不一样。");
        return 0;
    }
    v->track = val[0];
    c = collect_xp(data, n, at, val, 8);
    if (c != 4 || !same_u32(val, 4)) {
        StringCchCopyW(err, (size_t)errn, L"找不到经验上限，或四处数值不一样。");
        return 0;
    }
    if (xp_off && val[0] != 30u * v->track * v->track) *xp_off = 1;
    c = collect_fixed(data, n, pre_inc, sizeof pre_inc, post_inc, sizeof post_inc, at, val, 8);
    if (c != 1) {
        StringCchCopyW(err, (size_t)errn, L"找不到属性增加上限。");
        return 0;
    }
    stats[0] = val[0];
    c = collect_fixed(data, n, pre_asg, sizeof pre_asg, post_asg, sizeof post_asg, at, val, 8);
    if (c != 1) {
        StringCchCopyW(err, (size_t)errn, L"找不到属性分配上限。");
        return 0;
    }
    stats[1] = val[0];
    c = collect_fixed(data, n, pre_asg, sizeof pre_asg, post_res, sizeof post_res, at, val, 8);
    if (c != 1) {
        StringCchCopyW(err, (size_t)errn, L"找不到属性结算上限。");
        return 0;
    }
    stats[2] = val[0];
    if (!same_u32(stats, 3)) {
        StringCchCopyW(err, (size_t)errn, L"三处属性结算上限不一致。");
        return 0;
    }
    v->stat = stats[0];
    c = collect_fixed(data, n, pre_base, sizeof pre_base, post_base, sizeof post_base, at, val, 8);
    if (c != 1) {
        StringCchCopyW(err, (size_t)errn, L"找不到基础属性成长上限。");
        return 0;
    }
    v->base = val[0];
    xs = collect_skill(data, n, sat, sa, sb, 4);
    if (xs != 1 || sa[0] != sb[0]) {
        StringCchCopyW(err, (size_t)errn, L"找不到技能等级封顶，或两处数值不一样。");
        return 0;
    }
    v->skill = sa[0];
    c = collect_fixed(data, n, pre_port, sizeof pre_port, post_port, sizeof post_port, at, val, 8);
    if (c != 1) {
        StringCchCopyW(err, (size_t)errn, L"找不到港口商业/军事上限。");
        return 0;
    }
    v->port = val[0];
    if (!fash_read(data, n, v)) {
        StringCchCopyW(err, (size_t)errn, L"已写入的流行品设置认不出来。");
        return 0;
    }
    {
        int free_at = 0, slot_at = 0, cave;
        uint32_t free_stack = 1, slot_stack = 1;
        int free_kind, slot_kind;
        free_kind = find_cargo_stack(data, n, &free_at, &free_stack);
        if (!free_kind || free_stack < 1 || free_stack > 10) {
            StringCchCopyW(err, (size_t)errn, L"找不到原来的舱位计算。");
            return 0;
        }
        slot_stack = stack_installed(data, n);
        if (slot_stack) {
            v->stack = slot_stack;
            return 1;
        }
        if (!find_slot_pat(data, n, &slot_at)) {
            StringCchCopyW(err, (size_t)errn, L"找不到购买商品占舱位的位置。");
            return 0;
        }
        cave = slot_cave_off(data, n, slot_at);
        slot_kind = read_slot_stack(data, n, slot_at, cave, &slot_stack);
        if (!slot_kind) {
            StringCchCopyW(err, (size_t)errn, L"购买占舱位的代码认不出来。");
            return 0;
        }
        /* 旧版写入只把空闲舱位乘了倍数；装上占格补丁前先显示这个倍数。 */
        v->stack = slot_kind == 2 ? slot_stack : free_stack;
    }
    return 1;
}

int patch_exe_image(uint8_t *data, size_t n, const CapValues *v, wchar_t *err, int errn) {
    static const uint8_t pre_lv[] = {0x89, 0x44, 0x24, 0x20, 0x41, 0xB8};
    static const uint8_t post_lv[] = {0x8B, 0x54, 0x24, 0x20, 0x33, 0xC9};
    static const uint8_t pre_inc[] = {0x0F, 0xB7, 0x44, 0x41, 0x4A, 0x03, 0x44, 0x24, 0x60, 0xBA};
    static const uint8_t post_inc[] = {0x8B, 0xC8};
    static const uint8_t pre_asg[] = {0x41, 0xB8};
    static const uint8_t post_asg[] = {0x8B, 0x54, 0x24, 0x24, 0xB9, 0x19, 0x00, 0x00, 0x00};
    static const uint8_t post_res[] = {0x8B, 0x54, 0x24, 0x30, 0x33, 0xC9};
    static const uint8_t pre_base[] = {
        0x0F, 0xB6, 0x44, 0x01, 0x0F, 0x03, 0x44, 0x24, 0x70, 0x41, 0xB8
    };
    static const uint8_t post_base[] = {0x8B, 0xD0, 0x33, 0xC9};
    static const uint8_t pre_port[] = {
        0xE9, 0x2B, 0xFF, 0xFF, 0xFF, 0x8B, 0x54, 0x24, 0x54, 0xB9
    };
    static const uint8_t post_port[] = {0xE8};
    int at[8], sat[4];
    uint32_t val[8], sa[4], sb[4];
    int c, i;
    uint32_t xp;
    CapValues cur;
    if (!read_exe_image(data, n, &cur, NULL, err, errn)) return 0;
    (void)cur;
    c = collect_fixed(data, n, pre_lv, sizeof pre_lv, post_lv, sizeof post_lv, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->track);
    xp = 30u * v->track * v->track;
    c = collect_xp(data, n, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], xp);
    c = collect_fixed(data, n, pre_inc, sizeof pre_inc, post_inc, sizeof post_inc, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->stat);
    c = collect_fixed(data, n, pre_asg, sizeof pre_asg, post_asg, sizeof post_asg, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->stat);
    c = collect_fixed(data, n, pre_asg, sizeof pre_asg, post_res, sizeof post_res, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->stat);
    c = collect_fixed(data, n, pre_base, sizeof pre_base, post_base, sizeof post_base, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->base);
    c = collect_skill(data, n, sat, sa, sb, 4);
    for (i = 0; i < c; i++) {
        wu32(data + sat[i], v->skill);
        wu32(data + sat[i] + 10, v->skill);
    }
    c = collect_fixed(data, n, pre_port, sizeof pre_port, post_port, sizeof post_port, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->port);
    {
        int cargo_at = 0, slot_at = 0, cave, kind, slot_kind;
        uint32_t cargo_now = 1, slot_now = 1;
        int bc6c, be92, b032, e9c6;
        int32_t rel;
        if (!fash_remove(data, n)) {
            StringCchCopyW(err, (size_t)errn, L"已写入的流行品设置认不出来，不能改。");
            return 0;
        }
        kind = find_cargo_stack(data, n, &cargo_at, &cargo_now);
        if (!kind) {
            StringCchCopyW(err, (size_t)errn, L"找不到原来的舱位计算。");
            return 0;
        }
        memcpy(data + cargo_at, kCargoVanilla, sizeof kCargoVanilla);
        if (kind == 2 && (size_t)cargo_at + 34 <= n)
            data[cargo_at + (int)sizeof kCargoVanilla] = 0xCC;
        {
            int c7 = stk5_cave(data, n), c6, c5, c4;
            if (c7 >= 0 && !cave_restore(data, n, c7, kCaveMax)) {
                StringCchCopyW(err, (size_t)errn, L"已写入的商品堆叠认不出来，不能改。");
                return 0;
            }
            c6 = stk4_cave(data, n);
            if (c6 >= 0 && !cave_restore(data, n, c6, kCaveMax)) {
                StringCchCopyW(err, (size_t)errn, L"已写入的商品堆叠认不出来，不能改。");
                return 0;
            }
            c5 = stk3_cave(data, n);
            if (c5 >= 0 && !cave_restore(data, n, c5, kCaveMax)) {
                StringCchCopyW(err, (size_t)errn, L"已写入的商品堆叠认不出来，不能改。");
                return 0;
            }
            c4 = stk2_cave(data, n);
            if (c4 >= 0 && !cave_restore(data, n, c4, kCaveMaxStk2)) {
                StringCchCopyW(err, (size_t)errn, L"已写入的商品堆叠认不出来，不能改。");
                return 0;
            }
        }
        if (!find_slot_pat(data, n, &slot_at)) {
            StringCchCopyW(err, (size_t)errn, L"找不到购买商品占舱位的位置。");
            return 0;
        }
        cave = slot_cave_off(data, n, slot_at);
        if (cave < 0 || (size_t)cave + kSlotPadLen > n) {
            StringCchCopyW(err, (size_t)errn, L"购买函数后面没有空位，不能改堆叠。");
            return 0;
        }
        slot_kind = read_slot_stack(data, n, slot_at, cave, &slot_now);
        if (!slot_kind) {
            StringCchCopyW(err, (size_t)errn, L"购买占舱位的代码认不出来。");
            return 0;
        }
        if (slot_kind == 1) {
            size_t c;
            for (c = 0; c < kSlotPadLen; c++) {
                if (data[cave + c] != 0xCC) {
                    StringCchCopyW(err, (size_t)errn, L"购买函数后面的空位被占用，不能改堆叠。");
                    return 0;
                }
            }
        }
        bc6c = slot_call_off(data, n, slot_at + kSlotJne + kSlotCallBc6c);
        be92 = slot_call_off(data, n, slot_at + kSlotJne + kSlotCallBe92);
        b032 = slot_call_off(data, n, slot_at + kSlotJne + kSlotCallB032);
        e9c6 = slot_call_off(data, n, slot_at + kSlotJne + kSlotCallE9c6);
        if (bc6c < 0 || be92 < 0 || b032 < 0 || e9c6 < 0) {
            StringCchCopyW(err, (size_t)errn, L"购买函数里的调用对不上。");
            return 0;
        }
        /* 先移除旧版布局（1150da3 和 v3.1）：它们把件数放在 [slot+8]，
         * 占用了游戏的"本次买入"标志。 */
        if (!patch_stack_extras(data, n, cave, 1, err, errn)) return 0;
        rel = (int32_t)(kSlotSkip - 6);
        data[slot_at + kSlotJne] = 0x0F;
        data[slot_at + kSlotJne + 1] = 0x85;
        memcpy(data + slot_at + kSlotJne + 2, &rel, 4);
        if (slot_kind == 2)
            memset(data + cave, 0xCC, kSlotPadLen);
        if (v->stack > 1 && !stack_apply(data, n, v->stack, err, errn)) return 0;
        if (!fash_apply(data, n, v, err, errn)) return 0;
    }
    /* 原版上限下无副作用，因此每次写入都装。 */
    if (!patch_proxy_buy(data, n, err, errn)) return 0;
    {
        int site, stub;
        if (find_proxy_buy(data, n, &site, &stub) != 2) {
            StringCchCopyW(err, (size_t)errn, L"地方舰队亏损修正写入后校验失败。");
            return 0;
        }
    }
    if (!read_exe_image(data, n, &cur, NULL, err, errn)) return 0;
    if (cur.track != v->track || cur.base != v->base || cur.stat != v->stat ||
        cur.skill != v->skill || cur.port != v->port || cur.stack != v->stack ||
        cur.fthr != v->fthr || cur.fdiv != v->fdiv || cur.fl1 != v->fl1 || cur.fl2 != v->fl2) {
        StringCchCopyW(err, (size_t)errn, L"写入后的数值和输入不一致。");
        return 0;
    }
    return 1;
}
