/* 主窗口全局状态、控件 id，以及 DPI 缩放、标题、状态栏、更新保护等小工具。 */
#ifndef DK4_UI_APP_H
#define DK4_UI_APP_H
#include "common/platform.h"
#include "save/save.h"

#define WM_APP_REFRESH (WM_APP + 1)

enum {
    IDC_PATH = 100,
    IDC_REFRESH,
    IDC_OPEN,
    IDC_SLOTS,
    IDC_PEOPLE,
    IDC_FILTER_REC,
    IDC_FILTER_ALL,
    IDC_NAME,
    IDC_TRACK,
    IDC_FACTION,
    IDC_LEVEL,
    IDC_STAT0,
    IDC_APPLY = 200,
    IDC_FILL_STAT,
    IDC_FILL_LEVEL,
    IDC_BATCH,
    IDC_SAVE,
    IDC_STATUS,
    IDC_HINT,
    IDC_GAME_CAP
};

typedef struct App {
    HINSTANCE inst;
    HWND hwnd;
    HWND pathLabel;
    HWND refreshBtn;
    HWND gameCapBtn;
    HWND openBtn;
    HWND slotCaption;
    HWND editCaption;
    HWND slots;
    HWND people;
    HWND filterRec;
    HWND filterAll;
    HWND nameLabel;
    HWND trackLabel;
    HWND fieldLabel[8];
    HWND faction;
    HWND level;
    HWND stat[STAT_COUNT];
    HWND applyBtn;
    HWND fillStatBtn;
    HWND fillLevelBtn;
    HWND batchBtn;
    HWND saveBtn;
    HWND status;
    HWND hint;
    HFONT font;
    HACCEL accel;
    int dpi;
    int slot;
    int person;
    int recruited_only;
    int suspend;
    Save save;
    wchar_t root[MAX_PATH];
} App;

extern App g;                           /* 主窗口状态，全程唯一 */

int S(int x);                           /* 按当前 DPI 缩放像素值 */
void update_title(void);                /* 标题栏随"未保存"状态加 * */
void set_status(const wchar_t *text);   /* 设置底部状态栏文字 */

/* 程序自己改列表选中时包一层，期间忽略选中变化通知；可嵌套 */
void begin_update(void);
void end_update(void);

#endif
