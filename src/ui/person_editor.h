/* 人物列表与右侧编辑表单。 */
#ifndef DK4_UI_PERSON_EDITOR_H
#define DK4_UI_PERSON_EDITOR_H
#include "common/platform.h"

void sync_buttons(void);                /* 按是否打开存档、是否选中人物启用控件 */
void fill_people(void);
int editor_differs(void);               /* 表单与内存中的记录是否不同 */
void commit_editor(int *clamped_out);   /* 表单写入内存（尚未存盘） */

/* 界面事件 */
void on_person_clicked(int index);
void on_apply(void);
void on_batch(void);
void on_filter(int recruited_only);

#endif
