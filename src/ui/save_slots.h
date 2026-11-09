/* 存档栏列表，当前存档的打开、刷新、保存。 */
#ifndef DK4_UI_SAVE_SLOTS_H
#define DK4_UI_SAVE_SLOTS_H
#include "common/platform.h"

void refresh_slots(void);
int save_current(void);                 /* 当前存档写盘 */
int confirm_discard_or_save(void);      /* 有未保存修改时询问；返回 0 表示取消 */

/* 界面事件 */
void on_refresh(void);
void on_slot_clicked(int slot);
void on_open(void);
void startup_load(void);                /* 启动时打开第一个存档 */

#endif
