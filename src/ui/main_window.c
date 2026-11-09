/* 主窗口：创建控件、布局、窗口过程。 */

#include "save/save.h"
#include "ui/app.h"
#include "ui/widgets.h"
#include "ui/person_editor.h"
#include "ui/save_slots.h"
#include "ui/main_window.h"
#include "ui/cap_window.h"

static const wchar_t *const kStatName[STAT_COUNT] = {
    L"体力", L"敏捷", L"魅力", L"智力", L"精神", L"运气"
};

static void layout(int cw, int ch) {
    int m = S(12);
    int gap = S(10);
    int top = S(28);
    int bottom = S(28);
    int leftW = S(270);
    int rightW = S(318);
    int header = S(24);
    int bodyY = m + top + gap;
    int bodyH = ch - bodyY - bottom - m;
    int rx, cx, centerW;
    int x, y, row, labW, editW, half;
    int i;
    HWND edits[8];
    if (bodyH < S(200)) bodyH = S(200);
    MoveWindow(g.pathLabel, m, m, cw - m * 2 - S(72) - S(96) - S(88) - gap * 3, top, TRUE);
    MoveWindow(g.refreshBtn, cw - m - S(88) - gap - S(96) - gap - S(72), m, S(72), top, TRUE);
    MoveWindow(g.gameCapBtn, cw - m - S(88) - gap - S(96), m, S(96), top, TRUE);
    MoveWindow(g.openBtn, cw - m - S(88), m, S(88), top, TRUE);

    MoveWindow(g.slotCaption, m, bodyY, leftW, header, TRUE);
    MoveWindow(g.slots, m, bodyY + header, leftW, bodyH - header, TRUE);

    rx = cw - m - rightW;
    cx = m + leftW + gap;
    centerW = rx - gap - cx;
    if (centerW < S(180)) centerW = S(180);
    MoveWindow(g.filterRec, cx, bodyY, S(88), header, TRUE);
    MoveWindow(g.filterAll, cx + S(96), bodyY, S(100), header, TRUE);
    MoveWindow(g.people, cx, bodyY + header, centerW, bodyH - header, TRUE);

    MoveWindow(g.editCaption, rx, bodyY, rightW, header, TRUE);
    x = rx;
    y = bodyY + header;
    row = S(26);
    labW = S(72);
    editW = S(96);
    MoveWindow(g.nameLabel, x, y, rightW, S(36), TRUE);
    y += S(38);
    MoveWindow(g.trackLabel, x, y, rightW, S(22), TRUE);
    y += S(26);
    edits[0] = g.faction;
    edits[1] = g.level;
    for (i = 0; i < STAT_COUNT; i++) edits[2 + i] = g.stat[i];
    for (i = 0; i < 8; i++) {
        MoveWindow(g.fieldLabel[i], x, y, labW, row, TRUE);
        MoveWindow(edits[i], x + labW, y, editW, row, TRUE);
        y += row + S(6);
    }
    MoveWindow(g.applyBtn, x, y, rightW, row, TRUE);
    y += row + S(6);
    half = (rightW - S(8)) / 2;
    MoveWindow(g.fillStatBtn, x, y, half, row, TRUE);
    MoveWindow(g.fillLevelBtn, x + half + S(8), y, half, row, TRUE);
    y += row + S(6);
    MoveWindow(g.batchBtn, x, y, rightW, row, TRUE);
    y += row + S(8);
    {
        int hintH = bodyY + bodyH - y;
        if (hintH < S(72)) hintH = S(72);
        MoveWindow(g.hint, x, y, rightW, hintH, TRUE);
    }

    MoveWindow(g.saveBtn, m, ch - m - bottom + S(2), S(148), bottom - S(2), TRUE);
    MoveWindow(g.status, m + S(156), ch - m - bottom + S(2), cw - m * 2 - S(156), bottom - S(2), TRUE);
}

static void create_controls(HWND hwnd) {
    const wchar_t *labels[8];
    labels[0] = L"所属";
    labels[1] = L"等级";
    int i;
    DWORD editStyle = WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT | WS_BORDER;
    DWORD listStyle = WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;
    g.hwnd = hwnd;
    g.font = CreateFontW(-MulDiv(15, g.dpi < 96 ? 96 : g.dpi, 96),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    g.pathLabel = add_ctl(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_PATHELLIPSIS, IDC_PATH);
    g.refreshBtn = add_ctl(0, L"BUTTON", L"刷新", WS_TABSTOP, IDC_REFRESH);
    g.gameCapBtn = add_ctl(0, L"BUTTON", L"游戏上限", WS_TABSTOP, IDC_GAME_CAP);
    g.openBtn = add_ctl(0, L"BUTTON", L"打开", WS_TABSTOP, IDC_OPEN);
    g.slotCaption = add_ctl(0, L"STATIC", L"存档栏", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_STATIC);
    g.slots = add_ctl(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", listStyle, IDC_SLOTS);
    g.filterRec = add_ctl(0, L"BUTTON", L"已招收", WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON, IDC_FILTER_REC);
    g.filterAll = add_ctl(0, L"BUTTON", L"全部名册", WS_TABSTOP | BS_AUTORADIOBUTTON, IDC_FILTER_ALL);
    g.people = add_ctl(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", listStyle | WS_GROUP, IDC_PEOPLE);
    g.editCaption = add_ctl(0, L"STATIC", L"编辑", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_STATIC);
    g.nameLabel = add_ctl(0, L"STATIC", L"未选择角色", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_ENDELLIPSIS, IDC_NAME);
    g.trackLabel = add_ctl(0, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS, IDC_TRACK);
    for (i = 0; i < STAT_COUNT; i++) labels[2 + i] = kStatName[i];
    for (i = 0; i < 8; i++) {
        g.fieldLabel[i] = add_ctl(0, L"STATIC", labels[i], SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_STATIC);
    }
    g.faction = add_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"", editStyle, IDC_FACTION);
    g.level = add_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"", editStyle, IDC_LEVEL);
    for (i = 0; i < STAT_COUNT; i++) {
        g.stat[i] = add_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"", editStyle, IDC_STAT0 + i);
        SendMessageW(g.stat[i], EM_SETLIMITTEXT, 3, 0);
    }
    SendMessageW(g.faction, EM_SETLIMITTEXT, 3, 0);
    SendMessageW(g.level, EM_SETLIMITTEXT, 3, 0);
    g.applyBtn = add_ctl(0, L"BUTTON", L"应用到此人", WS_TABSTOP | BS_DEFPUSHBUTTON, IDC_APPLY);
    g.fillStatBtn = add_ctl(0, L"BUTTON", L"六维 255", WS_TABSTOP, IDC_FILL_STAT);
    g.fillLevelBtn = add_ctl(0, L"BUTTON", L"等级 510", WS_TABSTOP, IDC_FILL_LEVEL);
    g.batchBtn = add_ctl(0, L"BUTTON", L"应用到主角和已招收", WS_TABSTOP, IDC_BATCH);
    g.hint = add_ctl(0, L"STATIC",
        L"六维上限 255。等级是两段之和，每段最高 255，合计 510。改等级时按当前两段的比例分配。健康、心情和技能没有单独字段，由游戏按六维和等级计算。改动先留在内存里。点“保存到原文件”或按 Ctrl+S 才写回。第一次保存把原存档存为 .bak，之后每次保存前的存档另存为 .日期-时间.bak，只留最近 10 份。",
        SS_LEFT | SS_NOPREFIX, IDC_HINT);
    g.saveBtn = add_ctl(0, L"BUTTON", L"保存到原文件", WS_TABSTOP, IDC_SAVE);
    g.status = add_ctl(0, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_ENDELLIPSIS, IDC_STATUS);

    lv_style(g.slots);
    lv_style(g.people);
    lv_col(g.slots, 0, L"栏", S(40));
    lv_col(g.slots, 1, L"主角", S(150));
    lv_col(g.slots, 2, L"等级", S(52));
    lv_col(g.people, 0, L"编号", S(48));
    lv_col(g.people, 1, L"姓名", S(150));
    lv_col(g.people, 2, L"等级", S(52));
    lv_col(g.people, 3, L"所属", S(48));
    lv_col(g.people, 4, L"状态", S(64));
    lv_col(g.people, 5, L"六维", S(150));

    EnumChildWindows(hwnd, set_font_cb, (LPARAM)g.font);
    {
        HWND hdr = ListView_GetHeader(g.slots);
        if (hdr) SendMessageW(hdr, WM_SETFONT, (WPARAM)g.font, TRUE);
        hdr = ListView_GetHeader(g.people);
        if (hdr) SendMessageW(hdr, WM_SETFONT, (WPARAM)g.font, TRUE);
    }
    begin_update();
    CheckRadioButton(hwnd, IDC_FILTER_REC, IDC_FILTER_ALL, IDC_FILTER_REC);
    end_update();
    g.recruited_only = 1;
    sync_buttons();
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        create_controls(hwnd);
        return 0;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
            layout(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_GETMINMAXINFO: {
        MINMAXINFO *mm = (MINMAXINFO *)lParam;
        mm->ptMinTrackSize.x = S(1020);
        mm->ptMinTrackSize.y = S(720);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (g.suspend && (id == IDC_FILTER_REC || id == IDC_FILTER_ALL))
            return 0;
        switch (id) {
        case IDC_REFRESH:
            on_refresh();
            break;
        case IDC_GAME_CAP:
            open_cap_window();
            break;
        case IDC_OPEN:
            on_open();
            break;
        case IDC_FILTER_REC:
            on_filter(1);
            break;
        case IDC_FILTER_ALL:
            on_filter(0);
            break;
        case IDC_APPLY:
            on_apply();
            break;
        case IDC_FILL_STAT:
            if (g.person >= 0) {
                int i;
                for (i = 0; i < STAT_COUNT; i++) set_edit_int(g.stat[i], 255);
                set_status(L"六维已填入 255。点“应用到此人”后写入内存。");
            }
            break;
        case IDC_FILL_LEVEL:
            if (g.person >= 0) {
                set_edit_int(g.level, MAX_TOTAL_LEVEL);
                set_status(L"等级已填入 510。点“应用到此人”后写入内存。");
            }
            break;
        case IDC_BATCH:
            on_batch();
            break;
        case IDC_SAVE:
            save_current();
            break;
        default:
            break;
        }
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR *hdr = (NMHDR *)lParam;
        NMLISTVIEW *nm;
        if (g.suspend || hdr->code != LVN_ITEMCHANGED) break;
        nm = (NMLISTVIEW *)lParam;
        if (!(nm->uChanged & LVIF_STATE)) break;
        if ((nm->uNewState & LVIS_SELECTED) == 0 || (nm->uOldState & LVIS_SELECTED) != 0)
            break;
        if (hdr->hwndFrom == g.slots) on_slot_clicked((int)nm->lParam);
        else if (hdr->hwndFrom == g.people) on_person_clicked((int)nm->lParam);
        return 0;
    }
    case WM_APP_REFRESH:
        fill_people();
        refresh_slots();
        set_status(L"已写入内存。还没保存到文件。");
        return 0;
    case DM_GETDEFID:
        return MAKELRESULT(IDC_APPLY, DC_HASDEFID);
    case WM_CLOSE:
        if (!confirm_discard_or_save()) return 0;
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        free_save(&g.save);
        if (g.font) DeleteObject(g.font);
        if (g.accel) DestroyAcceleratorTable(g.accel);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
