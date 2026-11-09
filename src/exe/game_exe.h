/* 四个语言版主程序：定位游戏目录、读写主程序（带备份）、还原原版。 */
#ifndef DK4_EXE_GAME_EXE_H
#define DK4_EXE_GAME_EXE_H
#include "common/platform.h"

extern const wchar_t *const kGameExe[4];
extern const wchar_t *const kGameExeName[4];
void parent_dir(const wchar_t *file, wchar_t *dir, int dirn);
int dir_has_game(const wchar_t *dir);
void remember_game_dir(const wchar_t *dir);
void find_game_dir(wchar_t *dir, int dirn);
int load_exe_bytes(const wchar_t *path, uint8_t **out, size_t *outn, wchar_t *err, int errn);
int save_exe_bytes(const wchar_t *path, const uint8_t *data, size_t n, wchar_t *err, int errn);
int restore_game_exes(const wchar_t *dir, wchar_t *status, int statusn);

#endif
