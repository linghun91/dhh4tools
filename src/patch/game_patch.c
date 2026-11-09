/* 对游戏目录下各语言版主程序批量读取/写入补丁。 */

#include "exe/game_exe.h"
#include "patch/caps.h"
#include "patch/stack/stack.h"
#include "patch/proxy_buy.h"
#include "patch/game_patch.h"

int read_game_exes(const wchar_t *dir, CapValues *v, wchar_t *status, int statusn) {
    int i, found = 0, xp_off = 0, first_i = 0, no_fix = 0, old_stk = 0;
    CapValues first, cur;
    wchar_t path[MAX_PATH], err[240], names[120];
    memset(&first, 0, sizeof first);
    names[0] = 0;
    for (i = 0; i < 4; i++) {
        uint8_t *data = NULL;
        size_t n = 0;
        int bad = 0, site, stub;
        StringCchPrintfW(path, MAX_PATH, L"%s\\%s", dir, kGameExe[i]);
        if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) continue;
        if (!load_exe_bytes(path, &data, &n, err, 240) ||
            !read_exe_image(data, n, &cur, &bad, err, 240)) {
            free(data);
            StringCchPrintfW(status, (size_t)statusn, L"%s：%s", kGameExeName[i], err);
            return 0;
        }
        if (find_proxy_buy(data, n, &site, &stub) != 2) no_fix = 1;
        if (stack_outdated(data, n)) old_stk = 1;
        free(data);
        if (!found) {
            first = cur;
            first_i = i;
        } else if (cur.track != first.track || cur.base != first.base ||
                 cur.stat != first.stat || cur.skill != first.skill ||
                 cur.port != first.port || cur.stack != first.stack ||
                 cur.fthr != first.fthr || cur.fdiv != first.fdiv ||
                 cur.fl1 != first.fl1 || cur.fl2 != first.fl2) {
            *v = first;
            StringCchPrintfW(status, (size_t)statusn,
                L"%s和%s的封顶不一致。下面是先读到的数值，写入后四个版本会变成一样。",
                kGameExeName[first_i], kGameExeName[i]);
            return 1;
        }
        if (bad) xp_off = 1;
        if (names[0]) StringCchCatW(names, 120, L"、");
        StringCchCatW(names, 120, kGameExeName[i]);
        found++;
    }
    if (!found) {
        StringCchCopyW(status, (size_t)statusn, L"这个目录里没有 DK4HD 主程序。");
        return 0;
    }
    *v = first;
    StringCchPrintfW(status, (size_t)statusn,
        L"%s%s已读取%s。%s",
        (no_fix && first.stat > 500)
            ? L"还没有地方舰队亏损修正，属性结算超过 500 时舰队每月会巨亏，请再点一次“写入主程序”。"
            : L"",
        old_stk
            ? L"商品堆叠补丁是旧版：交易所和整理货物点一下总是整格，不能单件买卖、搬运，请再点一次“写入主程序”。"
            : L"",
        names,
        xp_off ? L"经验上限和单段等级对不上，写入时会按 30×等级² 重写。" : L"");
    return 1;
}

int write_game_exes(const wchar_t *dir, const CapValues *v, wchar_t *status, int statusn) {
    int i, found = 0;
    wchar_t path[MAX_PATH], err[240], names[120];
    names[0] = 0;
    for (i = 0; i < 4; i++) {
        uint8_t *data = NULL;
        size_t n = 0;
        StringCchPrintfW(path, MAX_PATH, L"%s\\%s", dir, kGameExe[i]);
        if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) continue;
        if (!load_exe_bytes(path, &data, &n, err, 240) ||
            !patch_exe_image(data, n, v, err, 240) ||
            !save_exe_bytes(path, data, n, err, 240)) {
            free(data);
            StringCchPrintfW(status, (size_t)statusn, L"%s没有写入。%s", kGameExeName[i], err);
            if (found)
                StringCchCatW(status, (size_t)statusn, L" 前面已经写入的版本还在。");
            return 0;
        }
        free(data);
        if (names[0]) StringCchCatW(names, 120, L"、");
        StringCchCatW(names, 120, kGameExeName[i]);
        found++;
    }
    if (!found) {
        StringCchCopyW(status, (size_t)statusn, L"这个目录里没有 DK4HD 主程序。");
        return 0;
    }
    StringCchPrintfW(status, (size_t)statusn,
        L"已写入%s。单段 %u，合计 %u，经验 %u，基础属性 %u，结算 %u，技能封顶 %u，港口 %u，商品堆叠 %u，流行品 %u/%u/%u/%u。已修正地方舰队买货亏损。",
        names, v->track, v->track * 2u, 30u * v->track * v->track,
        v->base, v->stat, v->skill, v->port, v->stack, v->fthr, v->fdiv, v->fl1, v->fl2);
    return 1;
}
