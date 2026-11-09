/* 子控件小工具：创建控件、列表视图列与行、数字输入框。 */
#ifndef DK4_UI_WIDGETS_H
#define DK4_UI_WIDGETS_H
#include "common/platform.h"

HWND add_ctl(DWORD ex, const wchar_t *cls, const wchar_t *text, DWORD style, int id);
void lv_col(HWND lv, int index, const wchar_t *text, int width);
void lv_style(HWND lv);
void lv_insert(HWND lv, int row, LPARAM param, const wchar_t **cols, int ncols);
void select_param(HWND lv, LPARAM param);
BOOL CALLBACK set_font_cb(HWND hwnd, LPARAM lp);
void set_edit_int(HWND ed, int value);
int edit_int(HWND ed, int lo, int hi, int *clamped);

#endif
