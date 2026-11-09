/* 大航海时代4 HD 存档修改器，程序入口。
 *
 * 源码目录（详见 README.md）：
 *   common/  编译设置、字节读写、备份
 *   save/    存档格式与人物记录
 *   exe/     主程序文件、PE 解析、x64 机器码拼装
 *   patch/   主程序补丁（上限、堆叠、流行品、舰队亏损修正）
 *   ui/      主窗口与"游戏上限"窗口 */

#include "save/save.h"
#include "ui/app.h"
#include "ui/save_slots.h"
#include "ui/main_window.h"
#include "ui/cap_window.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show) {
    WNDCLASSW wc;
    RECT rc;
    HWND hwnd;
    MSG msg;
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    INITCOMMONCONTROLSEX icc;
    ACCEL acc;
    (void)hPrev;
    (void)cmd;
    (void)show;

    SetProcessDPIAware();
    {
        HDC hdc = GetDC(NULL);
        g.dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSX) : 96;
        if (hdc) ReleaseDC(NULL, hdc);
    }
    if (g.dpi < 96) g.dpi = 96;
    g.inst = hInst;
    g.slot = -1;
    g.person = -1;
    g.recruited_only = 1;
    find_save_root(g.root, MAX_PATH);

    icc.dwSize = sizeof icc;
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"DK4SaveEditor";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassW(&wc)) return 1;

    rc.left = 0;
    rc.top = 0;
    rc.right = S(1100);
    rc.bottom = S(720);
    AdjustWindowRect(&rc, style, FALSE);
    hwnd = CreateWindowW(L"DK4SaveEditor", L"大航海时代4 HD 存档修改器",
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInst, NULL);
    if (!hwnd) return 1;

    acc.fVirt = FCONTROL | FVIRTKEY;
    acc.key = 'S';
    acc.cmd = IDC_SAVE;
    g.accel = CreateAcceleratorTableW(&acc, 1);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    startup_load();

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (capw.hwnd && (msg.hwnd == capw.hwnd || IsChild(capw.hwnd, msg.hwnd))) {
            if (IsDialogMessageW(capw.hwnd, &msg))
                continue;
        } else if (g.accel && TranslateAcceleratorW(g.hwnd, g.accel, &msg)) {
            continue;
        }
        if (IsDialogMessageW(g.hwnd, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
