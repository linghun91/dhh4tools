/* 写存档或主程序之前的备份。 */

#include "common/backup.h"

/* 备份放在原文件旁边：
 *   name.bak                      第一次改动前的原文件，之后不再覆盖
 *   name.YYYYMMDD-HHMMSS-mmm.bak  之后每次写入前的文件，只留最近 kKeepSteps 份 */
enum { kKeepSteps = 10, kStepStamp = 19 };

const wchar_t *file_part(const wchar_t *path) {
    const wchar_t *slash = wcsrchr(path, L'\\');
    return slash ? slash + 1 : path;
}

/* 是否为本文件的 name.<19 位时间戳>.bak */
static int is_step_backup(const wchar_t *base, const wchar_t *name) {
    size_t bn = wcslen(base), nn = wcslen(name), i;
    const wchar_t *stamp;
    if (nn != bn + 1 + kStepStamp + 4) return 0;
    if (_wcsnicmp(name, base, bn) != 0 || name[bn] != L'.') return 0;
    if (_wcsicmp(name + nn - 4, L".bak") != 0) return 0;
    stamp = name + bn + 1;
    for (i = 0; i < kStepStamp; i++) {
        if (i == 8 || i == 15) {
            if (stamp[i] != L'-') return 0;
        } else if (stamp[i] < L'0' || stamp[i] > L'9') {
            return 0;
        }
    }
    return 1;
}

static int cmp_wname(const void *a, const void *b) {
    return _wcsicmp((const wchar_t *)a, (const wchar_t *)b);
}

static void prune_step_backups(const wchar_t *path) {
    wchar_t pattern[MAX_PATH], dir[MAX_PATH], full[MAX_PATH];
    wchar_t (*names)[MAX_PATH] = NULL;
    const wchar_t *base = file_part(path);
    WIN32_FIND_DATAW fd;
    HANDLE h;
    int count = 0, cap = 0, i;
    StringCchCopyW(dir, MAX_PATH, path);
    dir[base - path] = 0;
    StringCchPrintfW(pattern, MAX_PATH, L"%s.*.bak", path);
    h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (!is_step_backup(base, fd.cFileName)) continue;
        if (count == cap) {
            int ncap = cap ? cap * 2 : 16;
            wchar_t (*grown)[MAX_PATH] = realloc(names, (size_t)ncap * sizeof *names);
            if (!grown) break;
            names = grown;
            cap = ncap;
        }
        StringCchCopyW(names[count++], MAX_PATH, fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    if (count > kKeepSteps) {
        qsort(names, (size_t)count, sizeof *names, cmp_wname);
        for (i = 0; i < count - kKeepSteps; i++) {
            StringCchPrintfW(full, MAX_PATH, L"%s%s", dir, names[i]);
            DeleteFileW(full);
        }
    }
    free(names);
}

/* 覆盖 path 之前调用。返回 1 = 生成了 name.bak，2 = 生成了带时间戳的备份，
 * 3 = 文件不存在无需备份，0 = 失败。 */
int backup_before_write(const wchar_t *path, wchar_t *made, int maden) {
    wchar_t bak[MAX_PATH];
    SYSTEMTIME t;
    if (made && maden > 0) made[0] = 0;
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) return 3;
    StringCchPrintfW(bak, MAX_PATH, L"%s.bak", path);
    if (GetFileAttributesW(bak) == INVALID_FILE_ATTRIBUTES) {
        if (!CopyFileW(path, bak, TRUE)) return 0;
        if (made) StringCchCopyW(made, (size_t)maden, bak);
        return 1;
    }
    GetLocalTime(&t);
    StringCchPrintfW(bak, MAX_PATH, L"%s.%04u%02u%02u-%02u%02u%02u-%03u.bak", path,
        t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    if (!CopyFileW(path, bak, TRUE)) return 0;
    if (made) StringCchCopyW(made, (size_t)maden, bak);
    prune_step_backups(path);
    return 2;
}
