/* 写存档或主程序之前的备份。 */
#ifndef DK4_COMMON_BACKUP_H
#define DK4_COMMON_BACKUP_H
#include "common/platform.h"

const wchar_t *file_part(const wchar_t *path);
int backup_before_write(const wchar_t *path, wchar_t *made, int maden);

#endif
