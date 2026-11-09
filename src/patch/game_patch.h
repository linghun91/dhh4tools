/* 对游戏目录下各语言版主程序批量读取/写入补丁。 */
#ifndef DK4_PATCH_GAME_PATCH_H
#define DK4_PATCH_GAME_PATCH_H
#include "common/platform.h"
#include "patch/caps.h"

int read_game_exes(const wchar_t *dir, CapValues *v, wchar_t *status, int statusn);
int write_game_exes(const wchar_t *dir, const CapValues *v, wchar_t *status, int statusn);

#endif
