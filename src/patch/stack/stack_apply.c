/* 商品堆叠：拼出 DK4STK5 补丁代码并写入主程序。 */

#include "common/bytes.h"
#include "exe/pe.h"
#include "exe/x64asm.h"
#include "patch/cave.h"
#include "patch/stack/stack.h"

/* 调用前 data 的每个补丁点都须是原版字节（旧补丁已还原）。 */
int stack_apply(uint8_t *data, size_t n, uint32_t stack, wchar_t *err, int errn) {
    uint8_t code[kCaveMax];
    Asm a;
    PatchSite site[48];
    int ns = 0, i;
    int o_pool, o_font, o_draw, c_fc5e, c_4093, c_1e65, L_fmtx, L_cdraw;
    int d_relief, d_finc, d_fnew, d_dinc, d_dnew, d_kind;
    int slot, jne, sell, disp, la0, l110, f110b8, fc662, f14d80, cave, run, tx_raw, tx_size, tx_delta;
    int imul, unbuy, unbuy_after, sell_own, sell_all, unsell;
    int dispA, dispB, dispS, dispPA, dispPS, fmt6, c_draw, iat_fmt, iat_key;
    int c_bc6c, c_be92, c_b032, c_e9c6, c_price, c_quote, c_14d3a, c_833c, c_a3c6;
    int shift, c_buy, xfer[2], c_xfer[2], np_price, np_kind, np_fash, cap1, cap2, c_cap, np_load, c_mine;
    int q_cnt, q_take, q_cap, q_done, c_6474;
    int L_cnt, L_disp, L_fmt2, L_fmtn, L_ncnt, j1, j2, j3, j4, imm_at, loop, hits[4];
    int click, wd, c_click, c_wd, b_click, b_wd, c_ship, c_slot, c_sellrec, c_grade, c_item, c_sprice;
    int c_inc, c_dec, c_draw1, c_draw2, c_draw3, o_body, o_thunk, c_freepool, c_setcard, c_sort;
    int c_hov, c_cardrec, c_freehold, c_clrslot, c_clrrec, L_same, L_hover, one, jd[6];
    uint32_t key_rva;
    int32_t rel;

#define STK_ERR(msg) do { StringCchCopyW(err, (size_t)errn, msg); return 0; } while (0)
#define SITE(where, length, how) do { \
        if (ns >= (int)(sizeof site / sizeof site[0])) STK_ERR(L"商品堆叠的补丁拼不出来。"); \
        site[ns].at = (where); site[ns].len = (length); site[ns].kind = (how); \
        site[ns].entry = a.n; ns++; \
    } while (0)
    if (stack < 2 || stack > kStackMax) STK_ERR(L"商品堆叠只能填 1 到 255。");
    if (!find_slot_pat(data, n, &slot)) STK_ERR(L"找不到购买商品占舱位的位置。");
    jne = slot + kSlotJne;
    if (!pat_is(data, n, jne, "0F 85")) STK_ERR(L"购买占舱位的代码不是原版。");
    c_bc6c = slot_call_off(data, n, jne + kSlotCallBc6c);
    c_be92 = slot_call_off(data, n, jne + kSlotCallBe92);
    c_b032 = slot_call_off(data, n, jne + kSlotCallB032);
    c_e9c6 = slot_call_off(data, n, jne + kSlotCallE9c6);
    c_price = slot_call_off(data, n, jne + 0x4B);
    c_quote = slot_call_off(data, n, jne + 0x61);
    if (c_bc6c < 0 || c_be92 < 0 || c_b032 < 0 || c_e9c6 < 0 || c_price < 0 || c_quote < 0)
        STK_ERR(L"购买函数里的调用对不上。");

    if (!find_sell_site(data, n, &sell) || data[sell] != 0x48) STK_ERR(L"找不到退货的位置。");
    c_14d3a = slot_call_off(data, n, sell + 5);
    unbuy = sell - 0x7E;
    unbuy_after = sell - 0x41;
    imul = sell + 0x56;
    sell_own = sell + 0x289;
    sell_all = sell - 0x246;
    unsell = sell + 0x96A;
    if (c_14d3a < 0 ||
        !pat_is(data, n, unbuy, "45 33 C9 45 33 C0 8B 54 24 44 48 8B 84 24 90 00 00 00 48 8B 88 88 05 00 00 E8") ||
        !pat_is(data, n, unbuy + 0x1E, "8B 54 24 44 48 8B 84 24 90 00 00 00 48 8B 88 88 05 00 00 E8") ||
        !pat_is(data, n, unbuy_after, "C7 44 24 64 00 00 00 00") ||
        !pat_is(data, n, imul, "8B D0 48 8B 84 24 90 00 00 00 48 8B 88 88 05 00 00 E8") ||
        !pat_is(data, n, sell_own, "8B D0 48 8B 84 24 80 00 00 00 48 8B 88 88 05 00 00 E8") ||
        !pat_is(data, n, sell_all, "8B D0 48 8B 84 24 90 00 00 00 48 8B 88 88 05 00 00 E8") ||
        !pat_is(data, n, unsell, "8B D0 48 8B 84 24 80 00 00 00 48 8B 88 88 05 00 00 E8"))
        STK_ERR(L"卖出、退货的代码和原版对不上。");
    c_833c = slot_call_off(data, n, unbuy + 0x19);
    c_a3c6 = slot_call_off(data, n, unbuy + 0x31);
    if (c_833c < 0 || c_a3c6 < 0) STK_ERR(L"退货里的调用对不上。");

    /* 价格栏。每个补丁点共 36 字节，进入时价格已在 r8d：
     *   lea rdx,[fmt "%6d"] / lea rcx,[szBuff] / call [wsprintfA]
     *   lea rdx,[szBuff] / lea rcx,[writer] / call DrawCentered */
    if (!find_disp_site(data, n, -1, &disp) || !pat_is(data, n, disp, "48 8B 8C 24 68 03 00 00 E8"))
        STK_ERR(L"找不到舱位价格显示的位置。");
    dispA = disp - 0x26;
    dispB = disp + 0x10;
    dispS = disp + 0x3062;
    dispPA = disp - 0x176;      /* 左栏：原有货物的买入价 */
    dispPS = dispS - 0x119;     /* 卖出列表左栏：买入价 */
    if (!pat_is(data, n, dispPA - 0x10, "48 8B 8C 24 68 03 00 00 E8") ||
        !pat_is(data, n, dispPS - 0x10, "48 8B 8C 24 C8 02 00 00 E8") ||
        !pat_is(data, n, dispS - 0x43, "48 8B 8C 24 C8 02 00 00"))
        STK_ERR(L"价格显示的代码和原版对不上。");
    memcpy(&rel, data + dispB + 3, 4);
    fmt6 = dispB + 7 + rel;
    memcpy(&rel, data + dispB + 14, 4);
    iat_fmt = dispB + 18 + rel;
    c_draw = slot_call_off(data, n, dispB + 31);
    {
        static const char *const wr[5] = {
            "48 8D 8C 24 A0 03 00 00 E8", "48 8D 8C 24 A0 03 00 00 E8", "48 8D 8C 24 70 02 00 00 E8",
            "48 8D 8C 24 A0 03 00 00 E8", "48 8D 8C 24 70 02 00 00 E8"
        };
        int ds[5];
        ds[0] = dispA; ds[1] = dispB; ds[2] = dispS; ds[3] = dispPA; ds[4] = dispPS;
        for (i = 0; i < 5; i++) {
            int s = ds[i];
            if (!pat_is(data, n, s - 3, "44 8B C0 48 8D 15") || !pat_is(data, n, s + 7, "48 8D 4C 24 38 FF 15") ||
                !pat_is(data, n, s + 18, "48 8D 54 24 38") || !pat_is(data, n, s + 23, wr[i]))
                STK_ERR(L"价格显示的代码和原版对不上。");
            memcpy(&rel, data + s + 3, 4);
            if (s + 7 + rel != fmt6) STK_ERR(L"价格显示用的格式对不上。");
            memcpy(&rel, data + s + 14, 4);
            if (s + 18 + rel != iat_fmt) STK_ERR(L"价格显示用的格式化函数对不上。");
            if (c_draw < 0 || slot_call_off(data, n, s + 31) != c_draw) STK_ERR(L"价格显示用的绘制函数对不上。");
        }
    }

    if (!pat_find(data, n, "8B 84 84 A0 00 00 00 FF C0", &la0) ||
        !pat_find(data, n, "8B 84 84 10 01 00 00 FF C0", &l110) ||
        !pat_is(data, n, la0 + 0x5F, "48 8B 8C 24 98 05 00 00 03 84 8C 50 03 00 00") ||
        !pat_is(data, n, l110 + 0x6F, "48 8B 8C 24 A8 05 00 00 03 84 8C C0 03 00 00") ||
        !pat_is(data, n, l110 + 0xAC, "48 8B 84 24 B8 05 00 00 8B 00 FF C0 48 8B 8C 24 B8 05 00 00"))
        STK_ERR(L"找不到交易统计的位置。");

    if (!pat_find(data, n, "48 89 54 24 10 48 89 4C 24 08 57 48 8B 44 24 10 48 8B 4C 24 18 0F B6 09 88 08"
                          " 48 8B 44 24 10 48 8B 4C 24 18 0F B6 49 01 88 48 01", &f110b8) ||
        !pat_find(data, n, "48 8B 4C 24 40 88 41 01 48 8B 44 24 40 C6 40 02 62 48 8B 44 24 40"
                          " C7 40 04 00 00 00 00 48 83 C4 30 5F C3", &fc662) ||
        !pat_find(data, n, "48 8B 4C 24 30 88 41 02 48 8B 44 24 30 8B 4C 24 50 89 48 04 48 83 C4 20 5F C3", &f14d80))
        STK_ERR(L"找不到货物记录的读写函数。");

    /* 市场按钮 0x0F..0x13 -> BuyOne(this, button - 0x0F)。 */
    if (pat_findall(data, n, "48 83 E8 0F 48 8B 8C 24 C0 00 00 00 48 81 E9 98 08 00 00 8B D0 E8", hits, 4) != 1)
        STK_ERR(L"找不到交易所买入按钮的位置。");
    shift = hits[0] + 21;
    c_buy = slot_call_off(data, n, shift);
    if (c_buy < 0) STK_ERR(L"交易所买入按钮的调用对不上。");
    key_rva = pe_import(data, n, "USER32.dll", "GetAsyncKeyState");
    if (!pe_text(data, n, &tx_raw, &tx_size, &tx_delta) || key_rva == 0)
        STK_ERR(L"主程序里找不到 GetAsyncKeyState。");
    iat_key = (int)key_rva - tx_delta;

    /* 船只存/读档：船舱记录 j 的 4 字节买入价，
     * lea rax,[rcx+rax*8+0x54] / xor r8d,r8d / mov rdx,rax / mov rcx,[rsp+0x58] / call。 */
    if (pat_findall(data, n, "48 8D 44 C1 54 45 33 C0 48 8B D0 48 8B 4C 24 58 E8", hits, 4) != 2)
        STK_ERR(L"找不到船只存档里货物价格的位置。");
    for (i = 0; i < 2; i++) {
        xfer[i] = hits[i] + 16;
        c_xfer[i] = slot_call_off(data, n, xfer[i]);
        if (c_xfer[i] < 0) STK_ERR(L"船只存档的调用对不上。");
    }

    /* NPC 舰队自动交易。 */
    if (pat_findall(data, n, "89 84 24 94 02 00 00 8B 94 24 90 02 00 00", hits, 4) != 1)
        STK_ERR(L"找不到舰队自动卖货的位置。");
    np_price = hits[0];
    if (pat_findall(data, n, "8B 84 84 60 01 00 00 FF C0", hits, 4) != 1) STK_ERR(L"找不到舰队自动卖货的位置。");
    np_kind = hits[0];
    if (pat_findall(data, n, "8B 84 84 38 02 00 00 FF C0", hits, 4) != 1) STK_ERR(L"找不到舰队自动卖货的位置。");
    np_fash = hits[0];
    if (pat_findall(data, n, "48 8B 8C 24 B0 03 00 00 E8 ? ? ? ? 44 8B C8 4C 8D 84 24 38 02 00 00", hits, 4) != 1)
        STK_ERR(L"找不到舰队载货量的位置。");
    cap1 = hits[0] + 8;
    if (pat_findall(data, n, "48 8B 8C 24 B0 03 00 00 E8 ? ? ? ? 89 84 24 6C 03 00 00", hits, 4) != 1)
        STK_ERR(L"找不到舰队载货量的位置。");
    cap2 = hits[0] + 8;
    c_cap = slot_call_off(data, n, cap1);
    if (c_cap < 0 || slot_call_off(data, n, cap2) != c_cap) STK_ERR(L"舰队载货量的调用对不上。");
    if (np_kind - np_price != 0x33 || np_fash - np_price != 0xAD || cap1 - np_price != 0x14E ||
        cap2 - np_price != 0x3FE)
        STK_ERR(L"舰队自动买卖的代码和原版对不上。");
    if (pat_findall(data, n, "48 8B 4C 24 48 E8 ? ? ? ? 8B 84 24 B8 00 00 00 FF C8 89 84 24 B8 00 00 00", hits, 4) != 1)
        STK_ERR(L"找不到舰队装货的位置。");
    np_load = hits[0] + 10;
    /* 装货函数开头先判断舰队是否属于玩家。 */
    if (!pat_is(data, n, hits[0] - 0xCD, "48 8B 8C 24 90 00 00 00 E8") ||
        !pat_is(data, n, hits[0] - 0xC0, "85 C0 0F 84"))
        STK_ERR(L"舰队装货的代码和原版对不上。");
    c_mine = slot_call_off(data, n, hits[0] - 0xC5);
    if (c_mine < 0) STK_ERR(L"舰队装货的调用对不上。");

    /* 委托、剧情、事件脚本经两个舰队函数查货/扣货：count(fleet, good) 和
     * add(fleet, good, ..., n)，n < 0 为扣除。原版对玩家舰队按格计数、整格扣除，现都按件。
     * count 内层循环：船在 [rsp+0x48]，j 在 [rsp+0x54]，结果在 [rsp+0x20]。 */
    if (pat_findall(data, n, "48 8B 4C 24 48 E8 ? ? ? ? 48 8B C8 E8 ? ? ? ? 3B 84 24 88 00 00 00 75 0A"
                            " 8B 44 24 20 FF C0 89 44 24 20 EB", hits, 4) != 1)
        STK_ERR(L"找不到统计舰队货物的位置。");
    q_cnt = hits[0] + 0x1B;
    /* 扣货，玩家分支：船在 [rsp+0x2D8]，j 在 [rsp+0x2E4]，n 在 [rsp+0x358]
     *   clear(ship, j) / n-- / if (n > 0) next / else jmp done */
    if (pat_findall(data, n, "8B 94 24 E4 02 00 00 48 8B 8C 24 D8 02 00 00 E8 ? ? ? ? 8B 84 24 58 03 00 00 FF C8"
                            " 89 84 24 58 03 00 00 83 BC 24 58 03 00 00 00 7F 05 E9", hits, 4) != 1)
        STK_ERR(L"找不到扣除舰队货物的位置。");
    q_take = hits[0];
    c_6474 = slot_call_off(data, n, q_take + 15);
    memcpy(&rel, data + q_take + 0x2F, 4);
    q_done = q_take + 0x33 + rel;
    /* 扣货：n = min(25, -n)，25 是舰队最多的格数。 */
    if (pat_findall(data, n, "8B 84 24 58 03 00 00 F7 D8 8B D0 B9 19 00 00 00 E8", hits, 4) != 1 ||
        q_take - hits[0] != 0xD6 || c_6474 < 0)
        STK_ERR(L"扣除舰队货物的代码和原版对不上。");
    q_cap = hits[0] + 12;

    /* 整理货物界面（COrganizeLoadView）：整理货物、卖船/入坞挪货、装战利品、广场发放交易品
     * 都用它。船舱格和 25 格待处理区整条复制 8 字节记录，但放回船舱的卡片由记录构造函数
     * 按字段重建，+3 被写成 0。卡片在 [rsp+0x58]，构造函数在 rax 返回船舱记录。 */
    if (pat_findall(data, n, "8B 94 24 F8 00 00 00 89 54 24 20 8B 94 24 FC 00 00 00 44 8B CA 8B 94 24 00 01 00 00"
                            " 44 8B C2 8B D0 E8 ? ? ? ? 48 63 44 24 64 48 8D 54 24 58", hits, 4) != 1)
        STK_ERR(L"找不到整理货物放回船舱的位置。");
    o_pool = hits[0] + 0x26;
    if (call_body(data, n, hits[0] + 0x21) != f14d80 - 0x59 ||
        !pat_is(data, n, f14d80 - 0x59, "44 89 4C 24 20 44 89 44 24 18"))
        STK_ERR(L"整理货物放回船舱的代码和原版对不上。");

    /* 格子绘制：先画图标，再经 [rsp+0xE0] 的文字绘制器在右上框写等级（或星号）；
     * 卡片在 [rsp+0x200]。件数画在等级之后，位于右下角。 */
    if (pat_findall(data, n, "48 8D 94 24 98 00 00 00 48 8D 8C 24 E0 00 00 00 E8 ? ? ? ? C7 84 24 A0 01 00 00 0B 00 00 00"
                            " C7 84 24 A4 01 00 00 16 00 00 00 BA 01 00 00 00 48 8B 8C 24 A0 01 00 00 E8", hits, 4) != 1)
        STK_ERR(L"找不到整理货物格子的绘制位置。");
    o_font = hits[0];
    o_draw = o_font + 248 + 19;
    if (!pat_is(data, n, o_font + 0x5A, "48 8B 84 24 00 02 00 00 48 05 C8 00 00 00") ||
        !pat_is(data, n, o_font + 248, "45 33 C0 48 8B D0 48 8D 8C 24 E0 00 00 00 E8") ||
        !pat_is(data, n, o_draw, "48 8D 8C 24 E0 00 00 00 E8") ||
        !pat_is(data, n, o_draw + 13, "90 48 8D 8C 24 E0 00 00 00 E8"))
        STK_ERR(L"整理货物格子的绘制代码和原版对不上。");
    c_fc5e = slot_call_off(data, n, o_font + 16);
    c_4093 = slot_call_off(data, n, o_font + 56);
    c_1e65 = slot_call_off(data, n, o_font + 248 + 14);
    if (c_fc5e < 0 || c_4093 < 0 || c_1e65 < 0) STK_ERR(L"整理货物格子绘制里的调用对不上。");

    /* 广场发放交易品（传教士）：界面返回所选卡片（最多 25 条记录），三个函数各按一张卡计 1。
     * 救济：疫病、饥荒、缺货的城市，所需那类货达到 5 + x/500 件（x 为商业发展度）即解除。
     * 卡片在 [rsp+0x78]，i 在 [rsp+0x38]，匹配数在 [rsp+0x34]。 */
    if (pat_findall(data, n, "E8 ? ? ? ? 3B 44 24 2C 0F 85 ? ? ? ? 8B 44 24 34 FF C0 89 44 24 34 8B 44 24 28 39 44 24 34",
                   hits, 4) != 1 ||
        !pat_is(data, n, hits[0] - 0x27, "48 63 44 24 38 48 8B 4C 24 78 48 8D 04 C1"))
        STK_ERR(L"找不到广场发放交易品的位置。");
    d_relief = hits[0] + 15;
    /* 可能在别的城市引发的流行：(good, amount) 对；amount 既是对 rand(25) 的判定加成，
     * 也是新流行等级（一个字节），所以同原版最多 25。卡片在 [rsp+0x58]，i 在 [rsp+0x24]。 */
    if (pat_findall(data, n, "48 63 44 24 30 48 8B 4C 24 50 8B 44 C1 04 FF C0 48 63 4C 24 30 48 8B 54 24 50 89 44 CA 04",
                   hits, 4) != 1 ||
        !pat_is(data, n, hits[0] - 0x53, "48 63 44 24 24 48 8B 4C 24 58 48 8D 04 C1") ||
        !pat_is(data, n, hits[0] + 66, "48 63 44 24 20 48 8B 4C 24 50 C7 44 C1 04 01 00 00 00 8B 44 24 20"))
        STK_ERR(L"找不到广场发放交易品的位置。");
    d_finc = hits[0] + 10;
    d_fnew = hits[0] + 66 + 10;
    /* 文化圈卖价与该类需求：(good, grade, amount) 三元组。栈帧同上。 */
    if (pat_findall(data, n, "48 6B C0 0C 48 8B 4C 24 50 8B 44 01 08 FF C0 48 63 4C 24 2C 48 6B C9 0C 48 8B 54 24 50 89 44 0A 08",
                   hits, 4) != 1 ||
        !pat_is(data, n, hits[0] - 0xAF, "48 63 44 24 24 48 8B 4C 24 58 48 8D 04 C1") ||
        !pat_is(data, n, hits[0] + 139, "48 6B C0 0C 48 8B 4C 24 50 C7 44 01 08 01 00 00 00 8B 44 24 20"))
        STK_ERR(L"找不到广场发放交易品的位置。");
    d_dinc = hits[0] + 9;
    d_dnew = hits[0] + 139 + 9;
    /* 该类需求 = (加权均价 / 简单均价) * 200 * 件数 / 100，之后夹到 1..25。简单均价
     * = 单价之和 / 件数，件数多于单价之和时为 0（作除数会崩），乘积也会溢出 32 位。 */
    if (pat_findall(data, n, "33 D2 8B 8C 24 DC 01 00 00 8B C1 8B 8C 24 E0 01 00 00 F7 F1 0F AF 84 24 9C 01 00 00"
                            " 33 D2 B9 64 00 00 00 F7 F1 89 84 24 C8 01 00 00 41 B8 19 00 00 00", hits, 4) != 1)
        STK_ERR(L"找不到广场发放交易品的位置。");
    d_kind = hits[0];

    /* 一件或整格。交易所窗口把按钮 5..9 交给 OnClickSlot(this, button - 5)，
     * 0x0A..0x0E 交给 WithdrawSell(this, button - 0x0A)，代码形式同上面的市场按钮。 */
    if (pat_findall(data, n, "48 83 E8 05 48 8B 8C 24 C0 00 00 00 48 81 E9 98 08 00 00 8B D0 E8", hits, 4) != 1)
        STK_ERR(L"找不到交易所点舱位的位置。");
    click = hits[0] + 21;
    if (pat_findall(data, n, "48 83 E8 0A 48 8B 8C 24 C0 00 00 00 48 81 E9 98 08 00 00 8B D0 E8", hits, 4) != 1)
        STK_ERR(L"找不到交易所撤回卖出的位置。");
    wd = hits[0] + 21;
    c_click = slot_call_off(data, n, click);
    c_wd = slot_call_off(data, n, wd);
    b_click = call_body(data, n, click);
    b_wd = call_body(data, n, wd);
    if (click - shift != 0x3A || wd - shift != 0x74 || c_click < 0 || c_wd < 0 || b_click < 0 || b_wd < 0)
        STK_ERR(L"交易所按钮的代码和原版对不上。");
    /* OnClickSlot：依次调用当前船、交易所舱位、卖出记录、等级、商品、卖价、收入，
     * 最后三次重绘。 */
    if (!pat_is(data, n, b_click, "89 54 24 10 48 89 4C 24 08 57 48 83 EC 70") ||
        !pat_is(data, n, b_click + 0x25, "48 8B 8C 24 80 00 00 00 E8") ||
        !pat_is(data, n, b_click + 0x41, "44 8B 84 24 88 00 00 00 8B 54 24 20 48 8B 84 24 80 00 00 00 48 8B 88 88 05 00 00 E8") ||
        !pat_is(data, n, b_click + 0xB4, "44 8B 44 24 44 8B 54 24 20 48 8B 84 24 80 00 00 00 48 8B 88 88 05 00 00 E8") ||
        !pat_is(data, n, b_click + 0x102, "48 8B 4C 24 28 E8") ||
        !pat_is(data, n, b_click + 0x10C, "48 8B 4C 24 58 E8") ||
        !pat_is(data, n, b_click + 0x11A, "48 8B 4C 24 58 E8") ||
        !pat_is(data, n, b_click + 0x124, "8B 4C 24 60 44 8B C1 8B D0 48 8B 84 24 80 00 00 00 8B 88 80 05 00 00 E8") ||
        !pat_is(data, n, b_click + 0x140, "8B D0 48 8B 84 24 80 00 00 00 48 8B 88 88 05 00 00 E8") ||
        !pat_is(data, n, b_click + 0x156, "48 8B 8C 24 80 00 00 00 E8") ||
        !pat_is(data, n, b_click + 0x163, "48 8B 8C 24 80 00 00 00 E8") ||
        !pat_is(data, n, b_click + 0x170, "48 8B 8C 24 80 00 00 00 E8"))
        STK_ERR(L"点舱位的代码和原版对不上。");
    c_ship = slot_call_off(data, n, b_click + 0x2D);
    c_slot = slot_call_off(data, n, b_click + 0x5C);
    c_sellrec = slot_call_off(data, n, b_click + 0xCC);
    c_clrslot = slot_call_off(data, n, b_click + 0x107);
    c_grade = slot_call_off(data, n, b_click + 0x111);
    c_item = slot_call_off(data, n, b_click + 0x11F);
    c_sprice = slot_call_off(data, n, b_click + 0x13B);
    c_inc = slot_call_off(data, n, b_click + 0x151);
    c_draw1 = slot_call_off(data, n, b_click + 0x15E);
    c_draw2 = slot_call_off(data, n, b_click + 0x16B);
    c_draw3 = slot_call_off(data, n, b_click + 0x178);
    /* WithdrawSell 用同一组辅助函数，并把收入扣回。 */
    if (!pat_is(data, n, b_wd, "89 54 24 10 48 89 4C 24 08 57 48 83 EC 70") ||
        !pat_is(data, n, b_wd + 0x113, "48 8B 4C 24 60 E8") ||
        !pat_is(data, n, b_wd + 0x146, "8B 88 80 05 00 00 E8") ||
        !pat_is(data, n, b_wd + 0x15B, "48 8B 88 88 05 00 00 E8") ||
        slot_call_off(data, n, b_wd + 0x2D) != c_ship ||
        slot_call_off(data, n, b_wd + 0xB5) != c_slot ||
        slot_call_off(data, n, b_wd + 0xFA) != c_sellrec ||
        slot_call_off(data, n, b_wd + 0x122) != c_grade ||
        slot_call_off(data, n, b_wd + 0x130) != c_item ||
        slot_call_off(data, n, b_wd + 0x14C) != c_sprice ||
        slot_call_off(data, n, b_wd + 0x16F) != c_draw2 ||
        slot_call_off(data, n, b_wd + 0x17C) != c_draw1 ||
        slot_call_off(data, n, b_wd + 0x189) != c_draw3)
        STK_ERR(L"撤回卖出的代码和原版对不上。");
    c_dec = slot_call_off(data, n, b_wd + 0x162);
    c_clrrec = slot_call_off(data, n, b_wd + 0x118);
    if (c_ship < 0 || c_slot < 0 || c_sellrec < 0 || c_grade < 0 || c_item < 0 || c_sprice < 0 ||
        c_inc < 0 || c_dec < 0 || c_draw1 < 0 || c_draw2 < 0 || c_draw3 < 0 ||
        c_clrslot != c_14d3a || c_clrrec < 0)
        STK_ERR(L"交易所里的调用对不上。");

    /* 整理货物界面虚函数 13（按钮处理，o_pool 就在其中）。r8 = 按钮：5..9 为当前船
     * （[this+0x1C4]）的船舱卡 j，记录在 this+0x1C8+ship*0x28+j*8，卡片在 [this+0x290+j*8]；
     * 0x0B..0x23 为待处理区卡 i，在 [this+0x2B8+i*8]，记录在 card+0xC8。只经由跳板进入。 */
    o_body = o_pool - 0x306;
    if (!pat_is(data, n, o_body, "4C 89 4C 24 20 4C 89 44 24 18 48 89 54 24 10 48 89 4C 24 08 56 57 48 81 EC 48 01 00 00") ||
        !pat_is(data, n, o_body + 0x46, "48 83 BC 24 70 01 00 00 05") ||
        !pat_is(data, n, o_body + 0x55, "48 83 BC 24 70 01 00 00 0A") ||
        !pat_is(data, n, o_body + 0x7C, "48 63 80 C4 01 00 00 48 6B C0 28 48 8B 8C 24 60 01 00 00 48 8D 84 01 C8 01 00 00") ||
        !pat_is(data, n, o_body + 0xB8, "48 8B 8C 24 60 01 00 00 E8") ||
        !pat_is(data, n, o_body + 0xE6, "48 8B 8C C1 B8 02 00 00 E8") ||
        !pat_is(data, n, o_body + 0xF3, "48 8B 4C 24 38 E8") ||
        !pat_is(data, n, o_body + 0x10F, "48 8B 8C C1 90 02 00 00 E8") ||
        !pat_is(data, n, o_body + 0x11C, "48 8B 8C 24 60 01 00 00 E8") ||
        !pat_is(data, n, o_body + 0x14E, "48 8B 80 90 00 00 00 48 63 40 08 48 8B 8C 24 E0 00 00 00 48 8D 84 01 90 00 00 00 48 8B C8 E8") ||
        !pat_is(data, n, o_body + 0x1E0, "BA 01 00 00 00 48 8B C8 48 8B 84 24 F0 00 00 00 FF 50 78") ||
        !pat_is(data, n, o_body + 0x1F8, "48 83 BC 24 70 01 00 00 0B") ||
        !pat_is(data, n, o_body + 0x207, "48 83 BC 24 70 01 00 00 24") ||
        !pat_is(data, n, o_body + 0x233, "48 8B 8C C1 B8 02 00 00 E8") ||
        !pat_is(data, n, o_body + 0x25B, "48 8B 8C 24 60 01 00 00 E8"))
        STK_ERR(L"整理货物按钮的代码和原版对不上。");
    c_freepool = slot_call_off(data, n, o_body + 0xC0);
    c_setcard = slot_call_off(data, n, o_body + 0xEE);
    c_sort = slot_call_off(data, n, o_body + 0x124);
    c_hov = slot_call_off(data, n, o_body + 0x16C);
    c_cardrec = slot_call_off(data, n, o_body + 0x23B);
    c_freehold = slot_call_off(data, n, o_body + 0x263);
    if (c_freepool < 0 || c_setcard < 0 || c_sort < 0 || c_hov < 0 || c_cardrec < 0 || c_freehold < 0 ||
        slot_call_off(data, n, o_body + 0x117) != c_setcard ||
        slot_call_off(data, n, o_body + 0xF8) != c_clrrec)
        STK_ERR(L"整理货物按钮里的调用对不上。");
    o_thunk = -1;
    for (i = tx_raw, j1 = 0; i + 5 <= tx_raw + tx_size; i++) {
        if (data[i] != 0xE9) continue;
        memcpy(&rel, data + i + 1, 4);
        if (i + 5 + rel != o_body) continue;
        o_thunk = i;
        j1++;
    }
    if (j1 != 1 || o_thunk >= o_body) STK_ERR(L"找不到整理货物按钮的跳板。");

    /* 代码洞：跳板表后面的 int3 空位。 */
    cave = -1;
    for (i = tx_raw; i < tx_raw + tx_size; ) {
        int j = i;
        if (data[i] != 0xCC) { i++; continue; }
        while (j < tx_raw + tx_size && data[j] == 0xCC) j++;
        if (j - i >= 0x8000) {
            cave = (i + 0x40 + 15) & ~15;
            run = j - cave;
            break;
        }
        i = j;
    }
    if (cave < 0) STK_ERR(L"主程序里找不到放补丁代码的空位。");

    a.b = code; a.n = 0; a.base = cave; a.cap = kCaveMax; a.bad = 0;
    memset(code, 0xCC, sizeof code);
    memcpy(code, STK5_MAGIC, 8);
    code[8] = (uint8_t)stack;
    a.n = kCaveHead;

    /* cnt: rcx = 记录 -> ecx = 件数（+3 字节，0 视为 1）。只改 ecx 和标志位。 */
    L_cnt = a.n;
    asm_hex(&a, "0F B6 49 03  85 C9  75 02  FF C1  C3");

    L_fmt2 = a.n;
    asm_hex(&a, "25 64 78 25 64 00");            /* "%dx%d" */
    L_fmtn = a.n;
    asm_hex(&a, "25 35 64 78 00");               /* "%5dx" */

    /* disp: rcx = 记录，rdx = 绘制器，r8d = 价格，r9d = 1 表示左栏。在绘制函数自身栈帧里
     * 调用，压栈后 szBuff 在 rsp+0x88。1 件显示一行 "%6d"；放得进原版 6 字符栏就显示
     * "NxPRICE"，否则两行："   22x" 在上、"  1200" 在下。右栏上下各扩半行，左栏向上扩
     * 一整行，两栏不相碰。绘制器：+0x28/+0x2C 笔位 x/y，+0x4C 字高。 */
    L_disp = a.n;
    asm_hex(&a, "53 56 57 41 54  48 83 EC 28");
    asm_hex(&a, "48 89 D3  44 89 C6  45 89 CC");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "89 CF  83 FF 01");
    j1 = asm_jcc(&a, 0x86);                      /* jbe one */
    asm_hex(&a, "B8 10 27 00 00  83 FF 0A");
    j2 = asm_jcc(&a, 0x82);                      /* jb fit */
    asm_hex(&a, "B8 E8 03 00 00  83 FF 64");
    j3 = asm_jcc(&a, 0x82);                      /* jb fit */
    asm_hex(&a, "B8 64 00 00 00");
    asm_fix(&a, j2, a.n);
    asm_fix(&a, j3, a.n);
    asm_hex(&a, "39 C6");
    j2 = asm_jcc(&a, 0x83);                      /* jae two */
    asm_hex(&a, "48 8D 8C 24 88 00 00 00");
    asm_lea_rdx(&a, cave + L_fmt2);
    asm_hex(&a, "41 89 F8  41 89 F1");
    asm_calli(&a, iat_fmt);
    asm_jmp(&a, 0);
    j3 = a.n - 5;                               /* jmp draw */
    asm_fix(&a, j1, a.n);                        /* one: */
    asm_hex(&a, "48 8D 8C 24 88 00 00 00");
    asm_lea_rdx(&a, fmt6);
    asm_hex(&a, "41 89 F0");
    asm_calli(&a, iat_fmt);
    rel = (int32_t)(a.n - (j3 + 5));            /* draw: */
    memcpy(code + j3 + 1, &rel, 4);
    asm_hex(&a, "48 8D 94 24 88 00 00 00  48 89 D9");
    asm_call(&a, c_draw);
    asm_jmp(&a, 0);
    j4 = a.n - 5;                               /* jmp done */
    asm_fix(&a, j2, a.n);                        /* two: */
    asm_hex(&a, "8B 43 4C  45 85 E4  75 02  D1 E8  41 89 C4  29 43 2C");
    asm_hex(&a, "48 8D 8C 24 88 00 00 00");
    asm_lea_rdx(&a, cave + L_fmtn);
    asm_hex(&a, "41 89 F8");
    asm_calli(&a, iat_fmt);
    asm_hex(&a, "48 8D 94 24 88 00 00 00  48 89 D9");
    asm_call(&a, c_draw);
    asm_hex(&a, "8B 43 4C  01 43 2C");
    asm_hex(&a, "48 8D 8C 24 88 00 00 00");
    asm_lea_rdx(&a, fmt6);
    asm_hex(&a, "41 89 F0");
    asm_calli(&a, iat_fmt);
    asm_hex(&a, "48 8D 94 24 88 00 00 00  48 89 D9");
    asm_call(&a, c_draw);
    asm_hex(&a, "8B 43 4C  44 29 E0  29 43 2C");
    rel = (int32_t)(a.n - (j4 + 5));            /* done: */
    memcpy(code + j4 + 1, &rel, 4);
    asm_hex(&a, "48 83 C4 28  41 5C 5F 5E 5B C3");

    /* 价格栏：原有货物、本次买入、卖出列表，以及价格切换后左栏显示的买入价。 */
#define STK_DISP(siteoff, load, writer, left) do { \
        SITE(siteoff, 12, 0); \
        asm_hex(&a, load); \
        asm_hex(&a, writer); \
        asm_hex(&a, left ? "41 B9 01 00 00 00" : "41 B9 00 00 00 00"); \
        asm_call(&a, cave + L_disp); \
        asm_jmp(&a, (siteoff) + 36); \
    } while (0)
    STK_DISP(dispA, "48 8B 8C 24 68 03 00 00", "48 8D 94 24 A0 03 00 00", 0);
    STK_DISP(dispB, "48 8B 8C 24 68 03 00 00", "48 8D 94 24 A0 03 00 00", 0);
    STK_DISP(dispS, "48 8B 8C 24 C8 02 00 00", "48 8D 94 24 70 02 00 00", 0);
    STK_DISP(dispPA, "48 8B 8C 24 68 03 00 00", "48 8D 94 24 A0 03 00 00", 1);
    STK_DISP(dispPS, "48 8B 8C 24 C8 02 00 00", "48 8D 94 24 70 02 00 00", 1);
#undef STK_DISP

    /* 买入：舱位已占用。同一商品、本次买入且未达上限时，该格加 1 件，按原版买入计价。 */
    SITE(jne, 6, 1);
    asm_hex(&a, "48 8B 84 24 D0 00 00 00  48 8B 80 88 05 00 00  48 89 44 24 28");
    asm_hex(&a, "48 8B 4C 24 68");
    asm_call(&a, c_bc6c);
    asm_hex(&a, "89 44 24 20  8B 94 24 D8 00 00 00  48 8B 4C 24 28");
    asm_call(&a, c_be92);
    asm_hex(&a, "3B 44 24 20");
    j1 = asm_jcc(&a, 0x85);
    asm_hex(&a, "48 8B 44 24 68  83 78 08 00");
    j2 = asm_jcc(&a, 0x84);
    asm_hex(&a, "48 8B C8");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "81 F9");                        /* cmp ecx, imm32 */
    imm_at = a.n;
    asm_u32(&a, stack);
    j3 = asm_jcc(&a, 0x83);
    asm_hex(&a, "FF C1  88 48 03");
    asm_hex(&a, "41 B8 01 00 00 00  8B 94 24 D8 00 00 00  48 8B 4C 24 28");
    asm_call(&a, c_b032);
    asm_hex(&a, "8B 94 24 D8 00 00 00  48 8B 4C 24 28");
    asm_call(&a, c_price);
    asm_hex(&a, "8B D0  48 8B 4C 24 28");
    asm_call(&a, c_quote);
    asm_hex(&a, "8B D0  48 8B 4C 24 28");
    asm_call(&a, c_e9c6);
    asm_jmp(&a, jne + kSlotUi);
    asm_fix(&a, j1, a.n);
    asm_fix(&a, j2, a.n);
    asm_fix(&a, j3, a.n);
    asm_jmp(&a, jne + kSlotSkip);

    /* 退货：每件的记账各执行一次。 */
    SITE(unbuy, 6, 0);
    asm_hex(&a, "48 8B 4C 24 28");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "89 4C 24 4C");
    loop = a.n;
    asm_hex(&a, "45 33 C9  45 33 C0  8B 54 24 44  48 8B 84 24 90 00 00 00  48 8B 88 88 05 00 00");
    asm_call(&a, c_833c);
    asm_hex(&a, "8B 54 24 44  48 8B 84 24 90 00 00 00  48 8B 88 88 05 00 00");
    asm_call(&a, c_a3c6);
    asm_hex(&a, "FF 4C 24 4C");
    j1 = asm_jcc(&a, 0x85);
    asm_fix(&a, j1, loop);
    asm_jmp(&a, unbuy_after);

    /* 退货：每件都退回供货并退款。 */
    SITE(sell, 16, 0);
    asm_hex(&a, "48 8B 4C 24 28");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "89 4C 24 4C  48 8B 4C 24 28");
    asm_call(&a, c_14d3a);
    asm_hex(&a, "44 8B 44 24 4C");
    asm_jmp(&a, sell + 16);
    SITE(imul, 10, 0);
    asm_hex(&a, "8B D0  0F AF 54 24 4C  48 8B 84 24 90 00 00 00");
    asm_jmp(&a, imul + 10);

    /* 卖出、全部卖出、撤回卖出：价格乘以件数。 */
#define STK_PRICE(siteoff, rec, frame) do { \
        SITE(siteoff, 10, 0); \
        asm_hex(&a, "8B D0"); \
        asm_hex(&a, rec); \
        asm_call(&a, cave + L_cnt); \
        asm_hex(&a, "0F AF D1"); \
        asm_hex(&a, frame); \
        asm_jmp(&a, (siteoff) + 10); \
    } while (0)
    STK_PRICE(sell_own, "48 8B 4C 24 58", "48 8B 84 24 80 00 00 00");
    STK_PRICE(sell_all, "48 8B 4C 24 70", "48 8B 84 24 90 00 00 00");
    STK_PRICE(unsell, "48 8B 4C 24 58", "48 8B 84 24 80 00 00 00");
#undef STK_PRICE

    /* 确认交易时的交易统计按件数而非格数。 */
    SITE(la0, 9, 0);
    asm_hex(&a, "8B 84 84 A0 00 00 00  48 8B 8C 24 68 04 00 00");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "03 C1");
    asm_jmp(&a, la0 + 9);
    SITE(la0 + 0x5F, 15, 0);
    asm_hex(&a, "48 8B 8C 24 68 04 00 00");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "0F AF C1  48 8B 8C 24 98 05 00 00  03 84 8C 50 03 00 00");
    asm_jmp(&a, la0 + 0x5F + 15);
    SITE(l110, 9, 0);
    asm_hex(&a, "8B 84 84 10 01 00 00  48 8B 8C 24 78 04 00 00");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "03 C1");
    asm_jmp(&a, l110 + 9);
    SITE(l110 + 0x6F, 15, 0);
    asm_hex(&a, "48 8B 8C 24 78 04 00 00");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "0F AF C1  48 8B 8C 24 A8 05 00 00  03 84 8C C0 03 00 00");
    asm_jmp(&a, l110 + 0x6F + 15);
    SITE(l110 + 0xAC, 12, 0);
    asm_hex(&a, "48 8B 8C 24 78 04 00 00");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "48 8B 84 24 B8 05 00 00  8B 00  03 C1");
    asm_jmp(&a, l110 + 0xAC + 12);

    /* same: rcx、rdx = 两条记录 -> 除件数外相同时 ZF=1。 */
    L_same = a.n;
    asm_hex(&a, "48 8B 01  48 33 02  49 BA FF FF FF 00 FF FF FF FF  4C 85 D0  C3");

    /* hover: rcx = 整理卡片。仿虚函数 13 对刚改动卡片的处理：界面判定需要时，
     * 以参数 1 调用卡片 +0x78 处的虚函数。 */
    L_hover = a.n;
    asm_hex(&a, "53  48 83 EC 20  48 8B 81 90 00 00 00  48 63 40 08  48 8D 9C 01 90 00 00 00  48 89 D9");
    asm_call(&a, c_hov);
    asm_hex(&a, "85 C0");
    j1 = asm_jcc(&a, 0x84);
    asm_hex(&a, "48 8B 03  BA 01 00 00 00  48 89 D9  FF 50 78");
    asm_fix(&a, j1, a.n);
    asm_hex(&a, "48 83 C4 20  5B C3");

    /* 市场商品。左键：BuyOne 一次。Shift 点击：反复 BuyOne，直到第一件落进的那格满了，
     * 或某次没买到（没钱、没货、没舱位）。第一件总落进接收它的船的首个可用格，
     * 所以比较买前买后五格的件数就能找到它。 */
    SITE(shift, 5, 2);
    asm_hex(&a, "53 56 57  41 54 41 55 41 56  48 83 EC 48  48 89 CB  89 D6  B9 10 00 00 00");
    asm_calli(&a, iat_key);
    asm_hex(&a, "66 85 C0");
    j1 = asm_jcc(&a, 0x88);                      /* js fill */
    one = a.n;
    asm_hex(&a, "48 89 D9  89 F2");
    asm_call(&a, c_buy);
    jd[0] = asm_jmpf(&a);                        /* jmp done */
    asm_fix(&a, j1, a.n);                        /* fill: */
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_ship);
    asm_hex(&a, "41 89 C4  41 83 FC 06");
    j1 = asm_jcc(&a, 0x84);
    asm_fix(&a, j1, one);                        /* 无船：转 one */
    asm_hex(&a, "31 FF");
    loop = a.n;                                 /* 记录买前 counts[edi] */
    asm_hex(&a, "48 8B 8B 88 05 00 00  44 89 E2  41 89 F8");
    asm_call(&a, c_slot);
    asm_hex(&a, "31 C9  80 38 6A");
    j1 = asm_jcc(&a, 0x84);
    asm_hex(&a, "48 89 C1");
    asm_call(&a, cave + L_cnt);
    asm_fix(&a, j1, a.n);
    asm_hex(&a, "89 4C BC 20  FF C7  83 FF 05");
    j1 = asm_jcc(&a, 0x82);
    asm_fix(&a, j1, loop);
    asm_hex(&a, "48 89 D9  89 F2");
    asm_call(&a, c_buy);
    asm_hex(&a, "31 FF");
    loop = a.n;                                 /* 找件数增加的那格 */
    asm_hex(&a, "48 8B 8B 88 05 00 00  44 89 E2  41 89 F8");
    asm_call(&a, c_slot);
    asm_hex(&a, "49 89 C5  31 C9  80 38 6A");
    j1 = asm_jcc(&a, 0x84);
    asm_hex(&a, "48 89 C1");
    asm_call(&a, cave + L_cnt);
    asm_fix(&a, j1, a.n);
    asm_hex(&a, "3B 4C BC 20");
    j2 = asm_jcc(&a, 0x87);                      /* ja found */
    asm_hex(&a, "FF C7  83 FF 05");
    j1 = asm_jcc(&a, 0x82);
    asm_fix(&a, j1, loop);
    jd[1] = asm_jmpf(&a);
    asm_fix(&a, j2, a.n);                        /* found: r13 = 格，ecx = 件数 */
    asm_hex(&a, "41 BE");
    asm_u32(&a, stack);
    loop = a.n;
    asm_hex(&a, "81 F9");
    asm_u32(&a, stack);
    jd[2] = asm_jcc(&a, 0x83);                   /* 已满：转 done */
    asm_hex(&a, "89 CF  48 89 D9  89 F2");
    asm_call(&a, c_buy);
    asm_hex(&a, "4C 89 E9  80 39 6A");
    jd[3] = asm_jcc(&a, 0x84);
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "39 F9");
    jd[4] = asm_jcc(&a, 0x86);                   /* 没买到：转 done */
    asm_hex(&a, "41 FF CE");
    j1 = asm_jcc(&a, 0x85);
    asm_fix(&a, j1, loop);
    asm_fixj(&a, jd[0], a.n);                    /* done: */
    asm_fixj(&a, jd[1], a.n);
    asm_fix(&a, jd[2], a.n);
    asm_fix(&a, jd[3], a.n);
    asm_fix(&a, jd[4], a.n);
    asm_hex(&a, "48 83 C4 48  41 5E 41 5D 41 5C 5F 5E 5B C3");

    /* 船舱格（OnClickSlot）。Shift、空格或无船：走原版；否则只处理 1 件。
     * 非本次买入：并进本船同记录且未满的卖出记录（若是最后一件则按原版清空该格），
     * 1 件卖价计入收入。没有可并记录时，1 件的格走原版；多件的格及本次买入的格（退 1 件）
     * 把件数临时设为 1 调用原版点击，之后补回其余件数。 */
    SITE(click, 5, 2);
    asm_hex(&a, "53 56 57  41 54 41 55 41 56  48 83 EC 48  48 89 CB  89 D6  B9 10 00 00 00");
    asm_calli(&a, iat_key);
    asm_hex(&a, "66 85 C0");
    jd[0] = asm_jcc(&a, 0x88);                   /* js whole */
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_ship);
    asm_hex(&a, "83 F8 06");
    jd[1] = asm_jcc(&a, 0x84);
    asm_hex(&a, "41 89 C4  48 8B 8B 88 05 00 00  44 89 E2  41 89 F0");
    asm_call(&a, c_slot);
    asm_hex(&a, "49 89 C5  41 80 7D 00 6A");
    jd[2] = asm_jcc(&a, 0x84);
    asm_hex(&a, "4C 89 E9");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "89 CF  41 83 7D 08 00");
    j3 = asm_jcc(&a, 0x85);                      /* 本次买入 */
    asm_hex(&a, "45 31 F6");
    loop = a.n;
    asm_hex(&a, "48 8B 8B 88 05 00 00  44 89 E2  45 89 F0");
    asm_call(&a, c_sellrec);
    asm_hex(&a, "80 38 6A");
    j1 = asm_jcc(&a, 0x84);
    asm_hex(&a, "48 89 44 24 28  48 89 C1  4C 89 EA");
    asm_call(&a, cave + L_same);
    j2 = asm_jcc(&a, 0x85);
    asm_hex(&a, "48 8B 4C 24 28");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "81 F9");
    asm_u32(&a, stack);
    j4 = asm_jcc(&a, 0x83);
    asm_hex(&a, "48 8B 44 24 28  FF C1  88 48 03  83 FF 01");
    jd[3] = asm_jcc(&a, 0x87);                   /* ja part */
    asm_hex(&a, "4C 89 E9");
    asm_call(&a, c_clrslot);
    jd[4] = asm_jmpf(&a);                        /* jmp money */
    asm_fix(&a, jd[3], a.n);                     /* part: */
    asm_hex(&a, "8D 4F FF  41 88 4D 03");
    asm_fixj(&a, jd[4], a.n);                    /* money: */
    asm_hex(&a, "48 8B 4C 24 28");
    asm_call(&a, c_grade);
    asm_hex(&a, "41 89 C6  48 8B 4C 24 28");
    asm_call(&a, c_item);
    asm_hex(&a, "45 89 F0  89 C2  8B 8B 80 05 00 00");
    asm_call(&a, c_sprice);
    asm_hex(&a, "89 C2  48 8B 8B 88 05 00 00");
    asm_call(&a, c_inc);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_draw1);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_draw2);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_draw3);
    jd[4] = asm_jmpf(&a);                        /* jmp done */
    asm_fix(&a, j1, a.n);                        /* 下一条卖出记录 */
    asm_fix(&a, j2, a.n);
    asm_fix(&a, j4, a.n);
    asm_hex(&a, "41 FF C6  41 83 FE 05");
    j1 = asm_jcc(&a, 0x82);
    asm_fix(&a, j1, loop);
    asm_fix(&a, j3, a.n);                        /* 本次买入 / 无可并记录： */
    asm_hex(&a, "83 FF 01");
    jd[3] = asm_jcc(&a, 0x86);                   /* 只有 1 件：转 whole */
    asm_hex(&a, "49 8B 45 00  48 89 44 24 30  41 8B 45 08  89 44 24 38  41 C6 45 03 01  48 89 D9  89 F2");
    asm_call(&a, c_click);
    asm_hex(&a, "41 80 7D 00 6A");
    j1 = asm_jcc(&a, 0x85);                      /* 格未被取走：转 keep */
    asm_hex(&a, "48 8B 44 24 30  49 89 45 00  8B 44 24 38  41 89 45 08  8D 4F FF  41 88 4D 03  48 89 D9");
    asm_call(&a, c_draw1);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_draw2);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_draw3);
    jd[5] = asm_jmpf(&a);
    asm_fix(&a, j1, a.n);                        /* keep: 恢复原件数 */
    asm_hex(&a, "41 88 7D 03");
    j2 = asm_jmpf(&a);
    asm_fix(&a, jd[0], a.n);                     /* whole: */
    asm_fix(&a, jd[1], a.n);
    asm_fix(&a, jd[2], a.n);
    asm_fix(&a, jd[3], a.n);
    asm_hex(&a, "48 89 D9  89 F2");
    asm_call(&a, c_click);
    asm_fixj(&a, jd[4], a.n);                    /* done: */
    asm_fixj(&a, jd[5], a.n);
    asm_fixj(&a, j2, a.n);
    asm_hex(&a, "48 83 C4 48  41 5E 41 5D 41 5C 5F 5E 5B C3");

    /* 卖出列表（WithdrawSell）。Shift、空记录或无船：走原版；否则撤回 1 件：并进本船
     * 非本次买入、同记录且未满的格（若是最后一件则清空该卖出记录），1 件卖价从收入扣回。
     * 没有可并的格时，1 件的记录走原版；多件的记录把件数设为 1 调用原版撤回，其余留在卖出列表。 */
    SITE(wd, 5, 2);
    asm_hex(&a, "53 56 57  41 54 41 55 41 56  48 83 EC 48  48 89 CB  89 D6  B9 10 00 00 00");
    asm_calli(&a, iat_key);
    asm_hex(&a, "66 85 C0");
    jd[0] = asm_jcc(&a, 0x88);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_ship);
    asm_hex(&a, "83 F8 06");
    jd[1] = asm_jcc(&a, 0x84);
    asm_hex(&a, "41 89 C4  48 8B 8B 88 05 00 00  44 89 E2  41 89 F0");
    asm_call(&a, c_sellrec);
    asm_hex(&a, "49 89 C5  41 80 7D 00 6A");
    jd[2] = asm_jcc(&a, 0x84);
    asm_hex(&a, "4C 89 E9");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "89 CF  45 31 F6");
    loop = a.n;
    asm_hex(&a, "48 8B 8B 88 05 00 00  44 89 E2  45 89 F0");
    asm_call(&a, c_slot);
    asm_hex(&a, "80 38 6A");
    j1 = asm_jcc(&a, 0x84);
    asm_hex(&a, "83 78 08 00");
    j2 = asm_jcc(&a, 0x85);
    asm_hex(&a, "48 89 44 24 28  48 89 C1  4C 89 EA");
    asm_call(&a, cave + L_same);
    j3 = asm_jcc(&a, 0x85);
    asm_hex(&a, "48 8B 4C 24 28");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "81 F9");
    asm_u32(&a, stack);
    j4 = asm_jcc(&a, 0x83);
    asm_hex(&a, "48 8B 44 24 28  FF C1  88 48 03  83 FF 01");
    jd[3] = asm_jcc(&a, 0x87);                   /* ja part */
    asm_hex(&a, "4C 89 E9");
    asm_call(&a, c_clrrec);
    jd[4] = asm_jmpf(&a);                        /* jmp money */
    asm_fix(&a, jd[3], a.n);                     /* part: */
    asm_hex(&a, "8D 4F FF  41 88 4D 03");
    asm_fixj(&a, jd[4], a.n);                    /* money: */
    asm_hex(&a, "48 8B 4C 24 28");
    asm_call(&a, c_grade);
    asm_hex(&a, "41 89 C6  48 8B 4C 24 28");
    asm_call(&a, c_item);
    asm_hex(&a, "45 89 F0  89 C2  8B 8B 80 05 00 00");
    asm_call(&a, c_sprice);
    asm_hex(&a, "89 C2  48 8B 8B 88 05 00 00");
    asm_call(&a, c_dec);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_draw2);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_draw1);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_draw3);
    jd[4] = asm_jmpf(&a);
    asm_fix(&a, j1, a.n);                        /* 下一格 */
    asm_fix(&a, j2, a.n);
    asm_fix(&a, j3, a.n);
    asm_fix(&a, j4, a.n);
    asm_hex(&a, "41 FF C6  41 83 FE 05");
    j1 = asm_jcc(&a, 0x82);
    asm_fix(&a, j1, loop);
    asm_hex(&a, "83 FF 01");
    jd[3] = asm_jcc(&a, 0x86);                   /* 只有 1 件：转 whole */
    asm_hex(&a, "49 8B 45 00  48 89 44 24 30  41 C6 45 03 01  48 89 D9  89 F2");
    asm_call(&a, c_wd);
    asm_hex(&a, "41 80 7D 00 6A");
    j1 = asm_jcc(&a, 0x85);
    asm_hex(&a, "48 8B 44 24 30  49 89 45 00  8D 4F FF  41 88 4D 03  48 89 D9");
    asm_call(&a, c_draw2);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_draw1);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_draw3);
    jd[5] = asm_jmpf(&a);
    asm_fix(&a, j1, a.n);
    asm_hex(&a, "41 88 7D 03");
    j2 = asm_jmpf(&a);
    asm_fix(&a, jd[0], a.n);
    asm_fix(&a, jd[1], a.n);
    asm_fix(&a, jd[2], a.n);
    asm_fix(&a, jd[3], a.n);
    asm_hex(&a, "48 89 D9  89 F2");
    asm_call(&a, c_wd);
    asm_fixj(&a, jd[4], a.n);
    asm_fixj(&a, jd[5], a.n);
    asm_fixj(&a, j2, a.n);
    asm_hex(&a, "48 83 C4 48  41 5E 41 5D 41 5C 5F 5E 5B C3");

    /* 整理货物界面：其跳板跳到这里。非卡片按钮、Shift 点击和空卡走原版。
     * 船舱卡 -> 待处理区：1 件并进同记录未满的待处理卡，否则（多于 1 件时）占一张空闲
     * 待处理卡；没有空闲则什么都不做（同原版）。待处理卡 -> 船舱：1 件并进当前船同记录
     * 未满的船舱，否则（多于 1 件时）占船舱数以内第一个空船舱；没有则不做。无处可并的
     * 单件卡走原版。来源少 1 件，或按原版清空；然后按原版顺序整理待处理区并恢复被点卡片的悬停状态。 */
    SITE(o_thunk, 5, 2);
    asm_hex(&a, "49 8D 40 FB  48 83 F8 05");
    j1 = asm_jcc(&a, 0x82);
    asm_hex(&a, "49 8D 40 F5  48 83 F8 19");
    j2 = asm_jcc(&a, 0x82);
    asm_jmp(&a, o_body);
    asm_fix(&a, j1, a.n);
    asm_fix(&a, j2, a.n);
    asm_hex(&a, "53 56 57  41 54 41 55 41 56 41 57  48 83 EC 60");
    asm_hex(&a, "48 89 4C 24 20  48 89 54 24 28  4C 89 44 24 30  4C 89 4C 24 38  48 89 CB  B9 10 00 00 00");
    asm_calli(&a, iat_key);
    asm_hex(&a, "66 85 C0");
    jd[0] = asm_jcc(&a, 0x88);                   /* js pass */
    asm_hex(&a, "48 63 83 C4 01 00 00  48 6B C0 28  4C 8D BC 03 C8 01 00 00  48 8B 44 24 30  48 83 F8 0B");
    j4 = asm_jcc(&a, 0x83);                      /* jae from pool */
    asm_hex(&a, "48 8D 70 FB  49 8D 3C F7  80 3F 6A");
    jd[1] = asm_jcc(&a, 0x84);
    asm_hex(&a, "48 89 F9");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "41 89 CE  45 31 E4");
    loop = a.n;
    asm_hex(&a, "4A 8B 8C E3 B8 02 00 00");
    asm_call(&a, c_cardrec);
    asm_hex(&a, "80 38 6A");
    j1 = asm_jcc(&a, 0x84);
    asm_hex(&a, "49 89 C5  48 89 C1  48 89 FA");
    asm_call(&a, cave + L_same);
    j2 = asm_jcc(&a, 0x85);
    asm_hex(&a, "4C 89 E9");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "81 F9");
    asm_u32(&a, stack);
    j3 = asm_jcc(&a, 0x83);
    asm_hex(&a, "49 8B 45 00  48 89 44 24 48  FF C1  88 4C 24 4B");
    one = asm_jmpf(&a);                          /* jmp set */
    asm_fix(&a, j1, a.n);
    asm_fix(&a, j2, a.n);
    asm_fix(&a, j3, a.n);
    asm_hex(&a, "41 FF C4  41 83 FC 19");
    j1 = asm_jcc(&a, 0x82);
    asm_fix(&a, j1, loop);
    asm_hex(&a, "41 83 FE 01");
    jd[2] = asm_jcc(&a, 0x86);                   /* 只有 1 件：转 pass */
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_freepool);
    asm_hex(&a, "83 F8 FF");
    jd[3] = asm_jcc(&a, 0x84);                   /* 无空闲卡：转 done */
    asm_hex(&a, "4C 63 E0  48 8B 07  48 89 44 24 48  C6 44 24 4B 01");
    asm_fixj(&a, one, a.n);                      /* set: */
    asm_hex(&a, "4A 8B 8C E3 B8 02 00 00  48 8D 54 24 48");
    asm_call(&a, c_setcard);
    asm_hex(&a, "41 83 FE 01");
    j1 = asm_jcc(&a, 0x87);                      /* ja part */
    asm_hex(&a, "48 89 F9");
    asm_call(&a, c_clrrec);
    j2 = asm_jmpf(&a);
    asm_fix(&a, j1, a.n);                        /* part: */
    asm_hex(&a, "41 8D 46 FF  88 47 03");
    asm_fixj(&a, j2, a.n);
    asm_hex(&a, "48 8B 8C F3 90 02 00 00  48 89 FA");
    asm_call(&a, c_setcard);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_sort);
    asm_hex(&a, "48 8B 8C F3 90 02 00 00");
    asm_call(&a, cave + L_hover);
    jd[4] = asm_jmpf(&a);
    asm_fix(&a, j4, a.n);                        /* from pool: */
    asm_hex(&a, "48 8D 70 F5  48 8B 8C F3 B8 02 00 00");
    asm_call(&a, c_cardrec);
    asm_hex(&a, "48 89 C7  80 3F 6A");
    jd[5] = asm_jcc(&a, 0x84);
    asm_hex(&a, "48 89 F9");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "41 89 CE  45 31 E4");
    loop = a.n;
    asm_hex(&a, "4F 8D 2C E7  41 80 7D 00 6A");
    j1 = asm_jcc(&a, 0x84);
    asm_hex(&a, "4C 89 E9  48 89 FA");
    asm_call(&a, cave + L_same);
    j2 = asm_jcc(&a, 0x85);
    asm_hex(&a, "4C 89 E9");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "81 F9");
    asm_u32(&a, stack);
    j3 = asm_jcc(&a, 0x83);
    asm_hex(&a, "FF C1  41 88 4D 03");
    one = asm_jmpf(&a);                          /* jmp set */
    asm_fix(&a, j1, a.n);
    asm_fix(&a, j2, a.n);
    asm_fix(&a, j3, a.n);
    asm_hex(&a, "41 FF C4  41 83 FC 05");
    j1 = asm_jcc(&a, 0x82);
    asm_fix(&a, j1, loop);
    asm_hex(&a, "41 83 FE 01");
    j4 = asm_jcc(&a, 0x86);                      /* 只有 1 件：转 pass */
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_freehold);
    asm_hex(&a, "83 F8 06");
    j1 = asm_jcc(&a, 0x84);                      /* 无空船舱：转 done */
    asm_hex(&a, "4C 63 E0  4F 8D 2C E7  48 8B 07  49 89 45 00  41 C6 45 03 01");
    asm_fixj(&a, one, a.n);                      /* set: */
    asm_hex(&a, "4A 8B 8C E3 90 02 00 00  4C 89 EA");
    asm_call(&a, c_setcard);
    asm_hex(&a, "48 8B 07  48 89 44 24 48  41 83 FE 01");
    j2 = asm_jcc(&a, 0x87);                      /* ja part */
    asm_hex(&a, "48 8D 4C 24 48");
    asm_call(&a, c_clrrec);
    one = asm_jmpf(&a);
    asm_fix(&a, j2, a.n);                        /* part: */
    asm_hex(&a, "41 8D 46 FF  88 44 24 4B");
    asm_fixj(&a, one, a.n);
    asm_hex(&a, "48 8B 8C F3 B8 02 00 00  48 8D 54 24 48");
    asm_call(&a, c_setcard);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, c_sort);
    asm_hex(&a, "48 8B 8C F3 B8 02 00 00");
    asm_call(&a, cave + L_hover);
    j2 = asm_jmpf(&a);
    asm_fix(&a, jd[0], a.n);                     /* pass: */
    asm_fix(&a, jd[1], a.n);
    asm_fix(&a, jd[2], a.n);
    asm_fix(&a, jd[5], a.n);
    asm_fix(&a, j4, a.n);
    asm_hex(&a, "48 8B 4C 24 20  48 8B 54 24 28  4C 8B 44 24 30  4C 8B 4C 24 38");
    asm_hex(&a, "48 83 C4 60  41 5F 41 5E 41 5D 41 5C 5F 5E 5B");
    asm_jmp(&a, o_body);
    asm_fix(&a, jd[3], a.n);                     /* done: */
    asm_fixj(&a, jd[4], a.n);
    asm_fix(&a, j1, a.n);
    asm_fixj(&a, j2, a.n);
    asm_hex(&a, "48 83 C4 60  41 5F 41 5E 41 5D 41 5C 5F 5E 5B C3");

    /* 船只存/读档，rdx = &record.price。存档流里没有 +3 这个字节，件数借买入价最高字节
     * 传递（价格远小于 2^24），传完放回 +3。存档后字节恢复原样；读档时 +3 取自文件，
     * 内存里上一局残留的件数不会混入。原版存档此处为 0，即 1 件。 */
    for (i = 0; i < 2; i++) {
        SITE(xfer[i], 5, 2);
        asm_hex(&a, "53  48 83 EC 20  48 89 D3  8A 43 FF  88 43 03");
        asm_call(&a, c_xfer[i]);
        asm_hex(&a, "0F B6 4B 03  88 4B FF  C6 43 03 00  48 83 C4 20  5B C3");
    }

    /* 舰队自动交易（同一函数，栈帧 F，舰队在 [F+0x3B0]）。只有玩家势力的舰队有真实船舱；
     * 其他势力只存一条记录加数量，保持原版。
     * ncnt: rcx = 记录 -> ecx = 玩家舰队为件数，否则为 1。保留 rax。从 F 调用，故舰队在 rsp+0x3F0。 */
    L_ncnt = a.n;
    asm_hex(&a, "53 56  48 83 EC 28  48 89 CB  48 89 C6  48 8B 8C 24 F0 03 00 00");
    asm_call(&a, c_mine);
    asm_hex(&a, "B9 01 00 00 00  85 C0");
    j1 = asm_jcc(&a, 0x84);
    asm_hex(&a, "48 89 D9");
    asm_call(&a, cave + L_cnt);
    asm_fix(&a, j1, a.n);
    asm_hex(&a, "48 89 F0  48 83 C4 28  5E 5B C3");

    /* 卖出：单价乘件数，计入收入和各类合计。 */
    SITE(np_price, 7, 0);
    asm_hex(&a, "89 84 24 94 02 00 00  48 63 8C 24 84 02 00 00  48 8D 4C CC 70");
    asm_call(&a, cave + L_ncnt);
    asm_hex(&a, "0F AF C1  89 84 24 94 02 00 00");
    asm_jmp(&a, np_price + 7);
    /* 卖出：各类及各流行组按件数而非记录数计。 */
    SITE(np_kind, 9, 0);
    asm_hex(&a, "48 63 8C 24 84 02 00 00  48 8D 4C CC 70");
    asm_call(&a, cave + L_ncnt);
    asm_hex(&a, "8B 84 84 60 01 00 00  01 C8");
    asm_jmp(&a, np_kind + 9);
    SITE(np_fash, 9, 0);
    asm_hex(&a, "48 63 8C 24 84 02 00 00  48 8D 4C CC 70");
    asm_call(&a, cave + L_ncnt);
    asm_hex(&a, "8B 84 84 38 02 00 00  01 C8");
    asm_jmp(&a, np_fash + 9);

    /* 载货量按件：玩家舰队为船舱格数乘上限。买货量和“卖够一半载货量”的流行判定都用它，
     * 与卖出件数保持同一量纲。 */
    {
        int L_cap = a.n;
        asm_hex(&a, "53 56  48 83 EC 28  48 89 CB");
        asm_call(&a, c_cap);
        asm_hex(&a, "89 C6  48 89 D9");
        asm_call(&a, c_mine);
        asm_hex(&a, "85 C0  89 F0");
        j1 = asm_jcc(&a, 0x84);
        asm_hex(&a, "69 C0");
        asm_u32(&a, stack);
        asm_fix(&a, j1, a.n);
        asm_hex(&a, "48 83 C4 28  5E 5B C3");
        SITE(cap1, 5, 2);
        site[ns - 1].entry = L_cap;
        SITE(cap2, 5, 2);
        site[ns - 1].entry = L_cap;
    }

    /* 玩家舰队装货：每格最多装 `stack` 件而非 1 件。原版：amount--。 */
    SITE(np_load, 16, 0);
    asm_hex(&a, "8B 84 24 B8 00 00 00  3D");
    asm_u32(&a, stack);
    j1 = asm_jcc(&a, 0x86);
    asm_hex(&a, "B8");
    asm_u32(&a, stack);
    asm_fix(&a, j1, a.n);
    asm_hex(&a, "48 8B 4C 24 48  48 63 54 24 64  88 44 D1 5B  29 84 24 B8 00 00 00");
    asm_jmp(&a, np_load + 16);

    /* 委托与事件，计数：累加匹配格的件数。 */
    SITE(q_cnt, 10, 0);
    asm_hex(&a, "48 8B 4C 24 48  48 63 44 24 54  48 8D 4C C1 58");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "01 4C 24 20");
    asm_jmp(&a, q_cnt + 10);

    /* 委托与事件，扣货：件数不超过尚欠数的格按原版清空；否则只扣欠的件数。 */
    SITE(q_take, 7, 0);
    asm_hex(&a, "48 8B 8C 24 D8 02 00 00  48 63 84 24 E4 02 00 00  48 8D 4C C1 58");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "8B 84 24 58 03 00 00  39 C1");
    j1 = asm_jcc(&a, 0x8F);                      /* jg part */
    asm_hex(&a, "29 C8  89 84 24 58 03 00 00");
    asm_hex(&a, "8B 94 24 E4 02 00 00  48 8B 8C 24 D8 02 00 00");
    asm_call(&a, c_6474);
    asm_hex(&a, "83 BC 24 58 03 00 00 00");
    j2 = asm_jcc(&a, 0x8F);                      /* jg 下一格 */
    asm_fix(&a, j2, q_take + 0x33 - cave);
    asm_jmp(&a, q_done);
    asm_fix(&a, j1, a.n);                        /* part: */
    asm_hex(&a, "29 C1  48 8B 94 24 D8 02 00 00  48 63 84 24 E4 02 00 00  88 4C C2 5B");
    asm_jmp(&a, q_done);

    /* 整理货物界面，卡片放回船舱：连件数一起复制。 */
    SITE(o_pool, 5, 0);
    asm_hex(&a, "8A 4C 24 5B  88 48 03  48 63 44 24 64");
    asm_jmp(&a, o_pool + 5);

    /* 整理货物界面卡片绘制：多于 1 件时显示 "x<件数>"，居中于 96x96 卡片右下 60x38 区域
     * （等级框是右上 42x42），缩放同等级框。cdraw: rcx = 绘制器，rdx = 记录。 */
    L_fmtx = a.n;
    asm_hex(&a, "78 25 64 00");                  /* "x%d" */
    L_cdraw = a.n;
    asm_hex(&a, "53 56  48 83 EC 58  48 89 CB  48 89 D1");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "83 F9 01");
    j1 = asm_jcc(&a, 0x86);                      /* jbe done */
    asm_hex(&a, "89 CE  48 B9");
    asm_u32(&a, 36);
    asm_u32(&a, 58);
    asm_hex(&a, "BA 01 00 00 00");
    asm_call(&a, c_4093);
    asm_hex(&a, "48 89 44 24 30  48 B9");
    asm_u32(&a, 60);
    asm_u32(&a, 38);
    asm_hex(&a, "BA 01 00 00 00");
    asm_call(&a, c_4093);
    asm_hex(&a, "48 89 44 24 40");
    asm_hex(&a, "8B 44 24 30  03 44 24 40  89 44 24 38");
    asm_hex(&a, "8B 44 24 34  03 44 24 44  89 44 24 3C");
    asm_hex(&a, "48 8D 54 24 30  48 89 D9");
    asm_call(&a, c_fc5e);
    asm_hex(&a, "48 8D 4C 24 48");
    asm_lea_rdx(&a, cave + L_fmtx);
    asm_hex(&a, "41 89 F0");
    asm_calli(&a, iat_fmt);
    asm_hex(&a, "48 C7 44 24 20 00 00 00 00  45 31 C9  45 31 C0  48 8D 54 24 48  48 89 D9");
    asm_call(&a, c_1e65);
    asm_fix(&a, j1, a.n);                        /* done: */
    asm_hex(&a, "48 83 C4 58  5E 5B C3");
    SITE(o_draw, 8, 0);
    asm_hex(&a, "48 8D 8C 24 E0 00 00 00  48 8B 94 24 00 02 00 00  48 81 C2 C8 00 00 00");
    asm_call(&a, cave + L_cdraw);
    asm_hex(&a, "48 8D 8C 24 E0 00 00 00");
    asm_jmp(&a, o_draw + 8);

    /* 广场发放交易品，救济：matches += 件数。 */
    SITE(d_relief, 10, 0);
    asm_hex(&a, "48 63 4C 24 38  48 8B 44 24 78  48 8D 0C C8");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "8B 44 24 34  01 C8  89 44 24 34");
    asm_jmp(&a, d_relief + 10);
    /* 流行对：amount += 件数 / = 件数，最多 25。 */
    SITE(d_finc, 6, 0);
    asm_hex(&a, "8B 44 C1 04  48 63 54 24 24  48 8B 4C 24 58  48 8D 0C D1");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "01 C8  83 F8 19  76 05  B8 19 00 00 00");
    asm_jmp(&a, d_finc + 6);
    SITE(d_fnew, 8, 0);
    asm_hex(&a, "48 8D 14 C1  48 63 44 24 24  48 8B 4C 24 58  48 8D 0C C1");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "83 F9 19  76 05  B9 19 00 00 00  89 4A 04");
    asm_jmp(&a, d_fnew + 8);
    /* 文化圈卖价三元组：amount += 件数 / = 件数。 */
    SITE(d_dinc, 6, 0);
    asm_hex(&a, "8B 44 01 08  48 63 54 24 24  48 8B 4C 24 58  48 8D 0C D1");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "01 C8");
    asm_jmp(&a, d_dinc + 6);
    SITE(d_dnew, 8, 0);
    asm_hex(&a, "48 8D 54 01 08  48 63 44 24 24  48 8B 4C 24 58  48 8D 0C C1");
    asm_call(&a, cave + L_cnt);
    asm_hex(&a, "89 0A");
    asm_jmp(&a, d_dnew + 8);
    /* 该类需求：除数为 0 时取封顶值，乘积用 64 位，超过 25 取 25（后面的夹取也会这样）。
     * 原版范围内的输入结果与原版相同。 */
    SITE(d_kind, 44, 0);
    asm_hex(&a, "8B 84 24 DC 01 00 00  8B 8C 24 E0 01 00 00  85 C9");
    j1 = asm_jcc(&a, 0x84);                      /* jz cap */
    asm_hex(&a, "33 D2  F7 F1  8B 8C 24 9C 01 00 00  48 0F AF C1  33 D2  B9 64 00 00 00  48 F7 F1  48 83 F8 19");
    j2 = asm_jcc(&a, 0x86);                      /* jbe store */
    asm_fix(&a, j1, a.n);                        /* cap: */
    asm_hex(&a, "B8 19 00 00 00");
    asm_fix(&a, j2, a.n);                        /* store: */
    asm_hex(&a, "89 84 24 C8 01 00 00");
    asm_jmp(&a, d_kind + 44);

    {
        int jumps = ns;
        /* 以下是原地修改的补丁点，没有代码洞入口。 */
        SITE(f110b8 + 11, 18, 3);
        SITE(fc662 + 8, 21, 3);
        SITE(f14d80, 20, 3);
        SITE(q_cap, 4, 3);

        /* 原字节表。 */
        code[10] = (uint8_t)(a.n & 0xFF);
        code[11] = (uint8_t)(a.n >> 8);
        code[9] = (uint8_t)ns;
        for (i = 0; i < ns; i++) {
            uint32_t off = (uint32_t)site[i].at;
            if (a.n + 5 + site[i].len > kCaveMax) { a.bad = 1; break; }
            memcpy(code + a.n, &off, 4);
            code[a.n + 4] = (uint8_t)site[i].len;
            memcpy(code + a.n + 5, data + site[i].at, (size_t)site[i].len);
            a.n += 5 + site[i].len;
        }
        code[12] = (uint8_t)(a.n & 0xFF);
        code[13] = (uint8_t)(a.n >> 8);
        code[14] = (uint8_t)(imm_at & 0xFF);
        code[15] = (uint8_t)(imm_at >> 8);
        if (a.bad || a.n > run) STK_ERR(L"放补丁代码的空位不够，不能改堆叠。");

        memcpy(data + cave, code, (size_t)a.n);
        for (i = 0; i < jumps; i++) {
            int at = site[i].at, k;
            if (site[i].kind == 1) {            /* jcc rel32 跳入代码洞 */
                rel = (int32_t)(cave + site[i].entry - (at + 6));
                memcpy(data + at + 2, &rel, 4);
            } else if (site[i].kind == 2) {     /* call rel32 跳入代码洞 */
                rel = (int32_t)(cave + site[i].entry - (at + 5));
                memcpy(data + at + 1, &rel, 4);
            } else {
                slot_jmp(data, at, cave + site[i].entry);
                for (k = 5; k < site[i].len; k++) data[at + k] = 0x90;
            }
        }
    }
    /* 记录复制：完整 8 字节。 */
    memcpy(data + f110b8 + 11,
        "\x48\x8B\x44\x24\x10\x48\x8B\x4C\x24\x18\x48\x8B\x09\x48\x89\x08\x5F\xC3", 18);
    /* 记录清空：写 +2 = 0x62 时一并把 +3 置 0。 */
    memcpy(data + fc662 + 8,
        "\x48\x8B\x44\x24\x40\x66\xC7\x40\x02\x62\x00\xC7\x40\x04\x00\x00\x00\x00\x90\x90\x90", 21);
    /* 记录构造：+3 字节置 0（al 来自 movzx），rax 仍返回记录。 */
    memcpy(data + f14d80,
        "\x48\x8B\x4C\x24\x30\x66\x89\x41\x02\x8B\x44\x24\x50\x89\x41\x04\x48\x8B\xC1\x90", 20);
    /* 扣货：上限为 25 格的件数，“全部扣除”仍能清空满载舰队。 */
    wu32(data + q_cap, 25u * stack);
#undef SITE
#undef STK_ERR
    return 1;
}
