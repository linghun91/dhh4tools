/* 主窗口全局状态、控件 id，以及 DPI 缩放、标题、状态栏、更新保护等小工具。 */

#include "ui/app.h"

App g;

int S(int x) {
    int dpi = g.dpi < 96 ? 96 : g.dpi;
    return MulDiv(x, dpi, 96);
}

void update_title(void) {
    SetWindowTextW(g.hwnd, g.save.dirty
        ? L"大航海时代4 HD 存档修改器 *"
        : L"大航海时代4 HD 存档修改器");
}

void set_status(const wchar_t *text) {
    if (g.status) SetWindowTextW(g.status, text ? text : L"");
}

void begin_update(void) { g.suspend++; }

void end_update(void) { if (g.suspend > 0) g.suspend--; }
