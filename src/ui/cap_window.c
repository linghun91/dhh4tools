/* "游戏上限"窗口：编辑并写入主程序里的上限、堆叠和流行品参数。 */

#include "exe/game_exe.h"
#include "patch/caps.h"
#include "patch/stack/stack.h"
#include "patch/fashion.h"
#include "patch/game_patch.h"
#include "ui/app.h"
#include "ui/widgets.h"
#include "ui/cap_window.h"

enum {
    IDC_CAP_PATH = 400,
    IDC_CAP_BROWSE,
    IDC_CAP_TRACK,
    IDC_CAP_BASE,
    IDC_CAP_STAT,
    IDC_CAP_SKILL,
    IDC_CAP_PORT,
    IDC_CAP_STACK,
    IDC_CAP_DERIVED,
    IDC_CAP_READ,
    IDC_CAP_WRITE,
    IDC_CAP_VANILLA,
    IDC_CAP_UNLOCK,
    IDC_CAP_UNLOCK_PORT,
    IDC_CAP_NOTE,
    IDC_CAP_STATUS,
    IDC_CAP_LBL_TRACK = 420,
    IDC_CAP_LBL_BASE,
    IDC_CAP_LBL_STAT,
    IDC_CAP_LBL_SKILL,
    IDC_CAP_LBL_PORT,
    IDC_CAP_LBL_STACK,
    IDC_CAP_STACKMAX,
    IDC_CAP_RESTORE,
    IDC_CAP_LBL_FTHR,
    IDC_CAP_FTHR,
    IDC_CAP_LBL_FDIV,
    IDC_CAP_FDIV,
    IDC_CAP_LBL_FL1,
    IDC_CAP_FL1,
    IDC_CAP_LBL_FL2,
    IDC_CAP_FL2,
    IDC_CAP_FSCALE
};

CapWin capw;

static HWND cap_ctl(DWORD ex, const wchar_t *cls, const wchar_t *text, DWORD style, int id) {
    return CreateWindowExW(ex, cls, text, style | WS_CHILD | WS_VISIBLE,
        0, 0, 10, 10, capw.hwnd, (HMENU)(INT_PTR)id, g.inst, NULL);
}

static void cap_set_status(const wchar_t *text) {
    if (capw.status) SetWindowTextW(capw.status, text ? text : L"");
}

static void cap_refresh_derived(void) {
    wchar_t line[160];
    int track;
    unsigned xp;
    if (!capw.derived) return;
    track = edit_int(capw.track, 0, 255, NULL);
    xp = 30u * (unsigned)track * (unsigned)track;
    StringCchPrintfW(line, 160, L"两段合计 %d，经验上限 %u", track * 2, xp);
    SetWindowTextW(capw.derived, line);
}

static void cap_apply_values(const CapValues *v) {
    set_edit_int(capw.track, (int)v->track);
    set_edit_int(capw.base, (int)v->base);
    set_edit_int(capw.stat, (int)v->stat);
    set_edit_int(capw.skill, (int)v->skill);
    set_edit_int(capw.port, (int)v->port);
    set_edit_int(capw.stack, (int)v->stack);
    set_edit_int(capw.fthr, (int)v->fthr);
    set_edit_int(capw.fdiv, (int)v->fdiv);
    set_edit_int(capw.fl1, (int)v->fl1);
    set_edit_int(capw.fl2, (int)v->fl2);
    cap_refresh_derived();
}

static int cap_read_form(CapValues *v, int *clamped) {
    if (clamped) *clamped = 0;
    v->track = (uint32_t)edit_int(capw.track, 1, 255, clamped);
    v->base = (uint32_t)edit_int(capw.base, 0, 255, clamped);
    v->stat = (uint32_t)edit_int(capw.stat, 1, 65535, clamped);
    v->skill = (uint32_t)edit_int(capw.skill, 0, 65535, clamped);
    v->port = (uint32_t)edit_int(capw.port, 1, 65535, clamped);
    v->stack = (uint32_t)edit_int(capw.stack, 1, kStackMax, clamped);
    v->fthr = (uint32_t)edit_int(capw.fthr, 0, 65535, clamped);
    v->fdiv = (uint32_t)edit_int(capw.fdiv, 1, 9999999, clamped);
    v->fl1 = (uint32_t)edit_int(capw.fl1, 1, 9999998, clamped);
    v->fl2 = (uint32_t)edit_int(capw.fl2, 2, 9999999, clamped);
    if (v->fl2 <= v->fl1) {
        v->fl2 = v->fl1 + 1;
        if (clamped) *clamped = 1;
    }
    return 1;
}

static void cap_read_clicked(void) {
    CapValues v;
    wchar_t status[360];
    if (!dir_has_game(capw.dir)) {
        cap_set_status(L"请先选择游戏主程序。");
        return;
    }
    if (!read_game_exes(capw.dir, &v, status, 360)) {
        cap_set_status(status);
        return;
    }
    cap_apply_values(&v);
    cap_set_status(status);
}

static void cap_write_clicked(void) {
    CapValues v;
    wchar_t ask[360], status[420];
    int clamped = 0;
    if (!dir_has_game(capw.dir)) {
        cap_set_status(L"请先选择游戏主程序。");
        return;
    }
    cap_read_form(&v, &clamped);
    cap_apply_values(&v);
    StringCchPrintfW(ask, 360,
        L"写入这个目录里的主程序？\n单段等级 %u（合计 %u，经验 %u）\n基础属性 %u\n属性结算 %u\n技能封顶 %u\n港口商业/军事 %u\n同商品堆叠 %u（同一个舱位堆 %u 件，满了才再占一格）\n"
        L"卖流行品：门槛 货舱格数×%u÷2 件，占有率 +件数÷%u+1，介绍信 %u 件，推荐信 %u 件\n请先完全退出游戏和启动器。",
        v.track, v.track * 2u, 30u * v.track * v.track, v.base, v.stat, v.skill, v.port,
        v.stack, v.stack, v.fthr, v.fdiv, v.fl1, v.fl2);
    if (MessageBoxW(capw.hwnd, ask, L"游戏上限", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;
    if (!write_game_exes(capw.dir, &v, status, 420)) {
        MessageBoxW(capw.hwnd, status, L"游戏上限", MB_ICONWARNING);
        cap_set_status(status);
        return;
    }
    if (clamped) StringCchCatW(status, 420, L" 超出范围的输入已按上下限处理。");
    cap_set_status(status);
    remember_game_dir(capw.dir);
}

static void cap_restore_clicked(void) {
    wchar_t status[420];
    CapValues v;
    if (!dir_has_game(capw.dir)) {
        cap_set_status(L"请先选择游戏主程序。");
        return;
    }
    if (MessageBoxW(capw.hwnd,
            L"把四个主程序还原成本工具第一次写入前的原文件（DK4HD_*.exe.bak）？\n"
            L"等级、属性、技能、港口上限和商品堆叠都会回到写入前的样子。\n"
            L"还原前的主程序会另存一份，存档不受影响。\n请先完全退出游戏和启动器。",
            L"还原原版主程序", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES)
        return;
    if (!restore_game_exes(capw.dir, status, 420)) {
        MessageBoxW(capw.hwnd, status, L"还原原版主程序", MB_ICONWARNING);
        cap_set_status(status);
        return;
    }
    {
        wchar_t tmp[360];
        if (read_game_exes(capw.dir, &v, tmp, 360)) cap_apply_values(&v);
    }
    cap_set_status(status);
}

static void cap_browse(void) {
    wchar_t file[MAX_PATH];
    OPENFILENAMEW ofn;
    file[0] = 0;
    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = capw.hwnd;
    ofn.lpstrFilter = L"主程序 (DK4HD_*.exe)\0DK4HD_*.exe\0可执行文件 (*.exe)\0*.exe\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = capw.dir[0] ? capw.dir : NULL;
    ofn.lpstrTitle = L"选择游戏主程序";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn)) return;
    parent_dir(file, capw.dir, MAX_PATH);
    SetWindowTextW(capw.path, capw.dir);
    remember_game_dir(capw.dir);
    cap_read_clicked();
}

static void cap_layout(void) {
    RECT rc;
    int m = S(16);
    int w, y, row, labW, editW, btnW;
    int gap = S(8);
    if (!capw.hwnd) return;
    GetClientRect(capw.hwnd, &rc);
    w = rc.right;
    row = S(28);
    labW = S(150);
    editW = S(88);
    btnW = S(128);
    y = m;
    MoveWindow(capw.path, m, y, w - m * 2 - btnW - gap, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_BROWSE), w - m - btnW, y, btnW, row, TRUE);
    y += row + S(14);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_TRACK), m, y, labW, row, TRUE);
    MoveWindow(capw.track, m + labW, y, editW, row, TRUE);
    MoveWindow(capw.derived, m + labW + editW + gap, y, w - (m + labW + editW + gap) - m, row, TRUE);
    y += row + gap;
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_BASE), m, y, labW, row, TRUE);
    MoveWindow(capw.base, m + labW, y, editW, row, TRUE);
    y += row + gap;
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_STAT), m, y, labW, row, TRUE);
    MoveWindow(capw.stat, m + labW, y, editW, row, TRUE);
    y += row + gap;
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_SKILL), m, y, labW, row, TRUE);
    MoveWindow(capw.skill, m + labW, y, editW, row, TRUE);
    y += row + gap;
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_PORT), m, y, labW, row, TRUE);
    MoveWindow(capw.port, m + labW, y, editW, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_UNLOCK_PORT), m + labW + editW + gap, y, btnW, row, TRUE);
    y += row + gap;
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_STACK), m, y, labW, row, TRUE);
    MoveWindow(capw.stack, m + labW, y, editW, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_STACKMAX), m + labW + editW + gap, y, btnW, row, TRUE);
    {
        int x2 = m + labW + editW + S(24);
        y += row + gap;
        MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_FTHR), m, y, labW, row, TRUE);
        MoveWindow(capw.fthr, m + labW, y, editW, row, TRUE);
        MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_FDIV), x2, y, labW, row, TRUE);
        MoveWindow(capw.fdiv, x2 + labW, y, editW, row, TRUE);
        MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_FSCALE), x2 + labW + editW + gap, y, btnW, row, TRUE);
        y += row + gap;
        MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_FL1), m, y, labW, row, TRUE);
        MoveWindow(capw.fl1, m + labW, y, editW, row, TRUE);
        MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_FL2), x2, y, labW, row, TRUE);
        MoveWindow(capw.fl2, x2 + labW, y, editW, row, TRUE);
    }
    y += row + S(16);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_READ), m, y, btnW, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_WRITE), m + (btnW + gap), y, btnW, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_VANILLA), m + (btnW + gap) * 2, y, btnW, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_UNLOCK), m + (btnW + gap) * 3, y, btnW, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_RESTORE), m + (btnW + gap) * 4, y, btnW, row, TRUE);
    y += row + S(12);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_NOTE), m, y, w - m * 2, rc.bottom - y - m - row - gap, TRUE);
    MoveWindow(capw.status, m, rc.bottom - m - row, w - m * 2, row, TRUE);
}

static LRESULT CALLBACK cap_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        DWORD editStyle = WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT | WS_BORDER;
        capw.hwnd = hwnd;
        capw.path = cap_ctl(0, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_PATHELLIPSIS, IDC_CAP_PATH);
        cap_ctl(0, L"BUTTON", L"选择主程序", WS_TABSTOP, IDC_CAP_BROWSE);
        cap_ctl(0, L"STATIC", L"单段等级上限", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_TRACK);
        capw.track = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"255", editStyle, IDC_CAP_TRACK);
        capw.derived = cap_ctl(0, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_ENDELLIPSIS, IDC_CAP_DERIVED);
        cap_ctl(0, L"STATIC", L"基础属性上限", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_BASE);
        capw.base = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"255", editStyle, IDC_CAP_BASE);
        cap_ctl(0, L"STATIC", L"属性结算上限", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_STAT);
        capw.stat = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"65535", editStyle, IDC_CAP_STAT);
        cap_ctl(0, L"STATIC", L"技能等级封顶", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_SKILL);
        capw.skill = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"510", editStyle, IDC_CAP_SKILL);
        cap_ctl(0, L"STATIC", L"港口商业/军事", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_PORT);
        capw.port = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"9999", editStyle, IDC_CAP_PORT);
        cap_ctl(0, L"BUTTON", L"解除港口上限", WS_TABSTOP, IDC_CAP_UNLOCK_PORT);
        cap_ctl(0, L"STATIC", L"同商品堆叠", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_STACK);
        capw.stack = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"99", editStyle, IDC_CAP_STACK);
        cap_ctl(0, L"BUTTON", L"堆叠 255", WS_TABSTOP, IDC_CAP_STACKMAX);
        cap_ctl(0, L"STATIC", L"流行品门槛(每格)", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_FTHR);
        capw.fthr = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"1", editStyle, IDC_CAP_FTHR);
        cap_ctl(0, L"STATIC", L"占有率涨幅除数", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_FDIV);
        capw.fdiv = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"6", editStyle, IDC_CAP_FDIV);
        cap_ctl(0, L"BUTTON", L"按堆叠等比", WS_TABSTOP, IDC_CAP_FSCALE);
        cap_ctl(0, L"STATIC", L"介绍信件数", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_FL1);
        capw.fl1 = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"10", editStyle, IDC_CAP_FL1);
        cap_ctl(0, L"STATIC", L"推荐信件数", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_FL2);
        capw.fl2 = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"16", editStyle, IDC_CAP_FL2);
        cap_ctl(0, L"BUTTON", L"读取当前值", WS_TABSTOP, IDC_CAP_READ);
        cap_ctl(0, L"BUTTON", L"写入主程序", WS_TABSTOP | BS_DEFPUSHBUTTON, IDC_CAP_WRITE);
        cap_ctl(0, L"BUTTON", L"填入原版", WS_TABSTOP, IDC_CAP_VANILLA);
        cap_ctl(0, L"BUTTON", L"解除上限", WS_TABSTOP, IDC_CAP_UNLOCK);
        cap_ctl(0, L"BUTTON", L"还原原版主程序", WS_TABSTOP, IDC_CAP_RESTORE);
        cap_ctl(WS_EX_CLIENTEDGE, L"EDIT",
            L"单段等级最高 255。再高时，游戏用 32 位计算平方根会溢出，等级会变成 0。经验上限按 30×单段等级² 自动写入。\r\n"
            L"基础属性存在一个字节里，最高 255。属性结算是升级、分配和结算三处共用的上限，原版 500，最高 65535。\r\n"
            L"技能按 (两项属性之和) × (不超过封顶的等级 + 100) ÷ 150 计算。封顶原版是 200。设成两段之和后，剑术、生命、交易会按真实等级上涨。\r\n"
            L"地方舰队和下属 NPC 舰队买货时，游戏按船长交易能力打折，只考虑了能力不超过 500 的情况。能力超过 1000 后每月会亏上亿金币。每次写入主程序都会顺带修正：这里的能力按 500 算，和原版满能力一样，其他地方不受影响。\r\n"
            L"港口商业和军事存在 16 位无符号数里，原版 9999，最高 65535。点“解除港口上限”填入 65535。海力上限保持 9999。商业度会参与商品定价，提到很高以后物价会跟着变。\r\n"
            L"同商品堆叠原版是 1：每买 1 件就占 1 个舱位，可填 1 到 255。填 N 以后，这次交易里买的同一个商品先堆在同一个舱位里，满 N 件才再占下一个舱位；船上原有的货不会并进来。交易所里左键点商品买 1 件，按住 Shift 点商品把这一格买满（钱、货或舱位不够时买到为止）；左键点船舱或卖出列表卖出、退货、撤回 1 件，按住 Shift 点是整格。"
            L"舱位价格放得下时显示成“件数x单价”，放不下时分两行：上行“件数x”，下行单价。卖出、退货按件数算钱，存档、读档保留件数。"
            L"玩家势力的舰队（地方舰队、下属 NPC 舰队）自动买货时也按 N 件一格装货，卖货按件数结算；其他势力的 NPC 保持原版。物价不改。"
            L"整理货物界面（整理货物、卖船或入坞时搬货、战后装战利品、广场发放交易品都用它）每格右下角显示“x件数”；左键点一格挪 1 件，按住 Shift 点整格挪，放回船舱时件数不变。"
            L"广场发放交易品按件数算：救济疫病、饥荒、缺货的城市，带出新流行（最多按 25 件算，和原版整支舰队 25 格一样），文化圈卖价回升，都按发出去的件数。"
            L"存过堆叠货物的存档不要再用原版主程序读：原版会把件数当成买入价的一部分。\r\n"
            L"卖流行品（在交易所确认交易时）：占有率不到 80 时，每种流行品要卖到 货舱格数×门槛÷2 件（向上取整），占有率才加 两种流行品件数÷除数+1（最多加到 100）；"
            L"占有率 80 以上时，卖到“介绍信件数”给介绍信，卖到“推荐信件数”给推荐信（已有介绍信时升级成推荐信）。原版是 1、6、10、16，是按一格一件定的。"
            L"“按堆叠等比”填入 N、6N、10N、16N（N 是上面的堆叠数），一满格等于原版一格。门槛填 0 表示卖一件就算。这些只管玩家自己在交易所卖货，舰队自动交易不受影响。\r\n"
            L"写入前请完全退出游戏。第一次写入时，原主程序存为 DK4HD_*.exe.bak，以后不再覆盖；之后每次写入前的主程序另存为 DK4HD_*.exe.日期-时间.bak，每个只留最近 10 份。\r\n"
            L"“还原原版主程序”用 .bak 把四个主程序还原成第一次写入前的样子，还原前的主程序同样另存一份。",
            ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, IDC_CAP_NOTE);
        capw.status = cap_ctl(0, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_ENDELLIPSIS, IDC_CAP_STATUS);
        SendMessageW(capw.track, EM_SETLIMITTEXT, 3, 0);
        SendMessageW(capw.base, EM_SETLIMITTEXT, 3, 0);
        SendMessageW(capw.stat, EM_SETLIMITTEXT, 5, 0);
        SendMessageW(capw.skill, EM_SETLIMITTEXT, 5, 0);
        SendMessageW(capw.port, EM_SETLIMITTEXT, 5, 0);
        SendMessageW(capw.stack, EM_SETLIMITTEXT, 3, 0);
        SendMessageW(capw.fthr, EM_SETLIMITTEXT, 5, 0);
        SendMessageW(capw.fdiv, EM_SETLIMITTEXT, 7, 0);
        SendMessageW(capw.fl1, EM_SETLIMITTEXT, 7, 0);
        SendMessageW(capw.fl2, EM_SETLIMITTEXT, 7, 0);
        EnumChildWindows(hwnd, set_font_cb, (LPARAM)g.font);
        SetWindowTextW(capw.path, capw.dir[0] ? capw.dir : L"未选择游戏目录");
        cap_refresh_derived();
        capw.ready = 1;
        if (dir_has_game(capw.dir)) cap_read_clicked();
        return 0;
    }
    case WM_SIZE:
        cap_layout();
        return 0;
    case WM_COMMAND:
        if (!capw.ready) return 0;
        if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_CAP_TRACK) {
            cap_refresh_derived();
            return 0;
        }
        switch (LOWORD(wParam)) {
        case IDC_CAP_BROWSE:
            cap_browse();
            break;
        case IDC_CAP_READ:
            cap_read_clicked();
            break;
        case IDC_CAP_WRITE:
            cap_write_clicked();
            break;
        case IDC_CAP_RESTORE:
            cap_restore_clicked();
            break;
        case IDC_CAP_VANILLA: {
            CapValues v;
            v.track = 100;
            v.base = 100;
            v.stat = 500;
            v.skill = 200;
            v.port = 9999;
            v.stack = 1;
            v.fthr = kFashThr;
            v.fdiv = kFashDiv;
            v.fl1 = kFashL1;
            v.fl2 = kFashL2;
            cap_apply_values(&v);
            cap_set_status(L"已填入原版：单段 100，基础属性 100，结算 500，技能封顶 200，港口 9999，堆叠 1，流行品 1/6/10/16。还没写入。");
            break;
        }
        case IDC_CAP_UNLOCK: {
            CapValues v;
            int port = edit_int(capw.port, 1, 65535, NULL);
            cap_read_form(&v, NULL);
            v.track = 255;
            v.base = 255;
            v.stat = 65535;
            v.skill = 510;
            v.port = (uint32_t)port;
            v.stack = (uint32_t)edit_int(capw.stack, 1, kStackMax, NULL);
            cap_apply_values(&v);
            cap_set_status(L"已填入解除值：单段 255，基础属性 255，结算 65535，技能封顶 510。堆叠和流行品保持当前输入。还没写入。");
            break;
        }
        case IDC_CAP_UNLOCK_PORT:
            set_edit_int(capw.port, 65535);
            cap_set_status(L"已填入港口上限 65535。点“写入主程序”后才会改主程序。");
            break;
        case IDC_CAP_FSCALE: {
            int n = edit_int(capw.stack, 1, kStackMax, NULL);
            wchar_t line[200];
            set_edit_int(capw.fthr, kFashThr * n);
            set_edit_int(capw.fdiv, kFashDiv * n);
            set_edit_int(capw.fl1, kFashL1 * n);
            set_edit_int(capw.fl2, kFashL2 * n);
            StringCchPrintfW(line, 200, L"已按堆叠 %d 填入流行品：门槛 %d，除数 %d，介绍信 %d 件，推荐信 %d 件。还没写入。",
                n, kFashThr * n, kFashDiv * n, kFashL1 * n, kFashL2 * n);
            cap_set_status(line);
            break;
        }
        case IDC_CAP_STACKMAX:
            set_edit_int(capw.stack, kStackMax);
            cap_set_status(L"已填入同商品堆叠 255。同一个舱位堆 255 件，第 256 件才再占一格。点“写入主程序”后才会改主程序。");
            break;
        default:
            break;
        }
        return 0;
    case DM_GETDEFID:
        return MAKELRESULT(IDC_CAP_WRITE, DC_HASDEFID);
    case WM_GETMINMAXINFO: {
        MINMAXINFO *mm = (MINMAXINFO *)lParam;
        mm->ptMinTrackSize.x = S(720);
        mm->ptMinTrackSize.y = S(680);
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        capw.hwnd = NULL;
        capw.ready = 0;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void open_cap_window(void) {
    WNDCLASSW wc;
    RECT rc;
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
    int x, y;
    if (capw.hwnd && IsWindow(capw.hwnd)) {
        ShowWindow(capw.hwnd, SW_SHOW);
        SetForegroundWindow(capw.hwnd);
        return;
    }
    if (!capw.dir[0]) find_game_dir(capw.dir, MAX_PATH);
    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = cap_proc;
    wc.hInstance = g.inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"DK4CapWindow";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassW(&wc);
    rc.left = 0;
    rc.top = 0;
    rc.right = S(760);
    rc.bottom = S(680);
    AdjustWindowRect(&rc, style, FALSE);
    x = CW_USEDEFAULT;
    y = CW_USEDEFAULT;
    if (g.hwnd) {
        RECT mainrc;
        GetWindowRect(g.hwnd, &mainrc);
        x = mainrc.left + S(36);
        y = mainrc.top + S(36);
    }
    CreateWindowExW(WS_EX_DLGMODALFRAME, L"DK4CapWindow", L"游戏上限",
        style, x, y, rc.right - rc.left, rc.bottom - rc.top,
        g.hwnd, NULL, g.inst, NULL);
    if (!capw.hwnd) return;
    ShowWindow(capw.hwnd, SW_SHOW);
    UpdateWindow(capw.hwnd);
}
