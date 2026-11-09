/* "游戏上限"窗口：编辑并写入主程序里的上限、堆叠和流行品参数。 */
#ifndef DK4_UI_CAP_WINDOW_H
#define DK4_UI_CAP_WINDOW_H
#include "common/platform.h"

typedef struct CapWin {
    HWND hwnd;
    HWND path;
    HWND track;
    HWND base;
    HWND stat;
    HWND skill;
    HWND port;
    HWND stack;
    HWND fthr;
    HWND fdiv;
    HWND fl1;
    HWND fl2;
    HWND derived;
    HWND status;
    int ready;
    wchar_t dir[MAX_PATH];
} CapWin;

extern CapWin capw;
void open_cap_window(void);

#endif
