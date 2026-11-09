/* 子控件小工具：创建控件、列表视图列与行、数字输入框。 */

#include "ui/app.h"
#include "ui/widgets.h"

HWND add_ctl(DWORD ex, const wchar_t *cls, const wchar_t *text, DWORD style, int id) {
    return CreateWindowExW(ex, cls, text, style | WS_CHILD | WS_VISIBLE,
        0, 0, 10, 10, g.hwnd, (HMENU)(INT_PTR)id, g.inst, NULL);
}

void lv_col(HWND lv, int index, const wchar_t *text, int width) {
    LVCOLUMNW col;
    ZeroMemory(&col, sizeof col);
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;
    col.cx = width;
    col.pszText = (LPWSTR)text;
    ListView_InsertColumn(lv, index, &col);
}

void lv_style(HWND lv) {
    ListView_SetExtendedListViewStyle(lv,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
}

void lv_insert(HWND lv, int row, LPARAM param, const wchar_t **cols, int ncols) {
    LVITEMW it;
    int c;
    ZeroMemory(&it, sizeof it);
    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = row;
    it.pszText = (LPWSTR)cols[0];
    it.lParam = param;
    ListView_InsertItem(lv, &it);
    for (c = 1; c < ncols; c++)
        ListView_SetItemText(lv, row, c, (LPWSTR)cols[c]);
}

void select_param(HWND lv, LPARAM param) {
    int i, n;
    begin_update();
    n = ListView_GetItemCount(lv);
    for (i = 0; i < n; i++) {
        LVITEMW it;
        UINT state;
        ZeroMemory(&it, sizeof it);
        it.mask = LVIF_PARAM;
        it.iItem = i;
        ListView_GetItem(lv, &it);
        state = (it.lParam == param) ? (LVIS_SELECTED | LVIS_FOCUSED) : 0;
        ListView_SetItemState(lv, i, state, LVIS_SELECTED | LVIS_FOCUSED);
        if (it.lParam == param)
            ListView_EnsureVisible(lv, i, FALSE);
    }
    end_update();
}

BOOL CALLBACK set_font_cb(HWND hwnd, LPARAM lp) {
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)lp, TRUE);
    return TRUE;
}

void set_edit_int(HWND ed, int value) {
    wchar_t buf[16];
    StringCchPrintfW(buf, 16, L"%d", value);
    SetWindowTextW(ed, buf);
}

int edit_int(HWND ed, int lo, int hi, int *clamped) {
    wchar_t buf[32];
    int value;
    GetWindowTextW(ed, buf, 32);
    value = _wtoi(buf);
    if (value < lo) {
        value = lo;
        if (clamped) *clamped = 1;
    }
    if (value > hi) {
        value = hi;
        if (clamped) *clamped = 1;
    }
    return value;
}
