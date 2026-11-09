/* 四个语言版主程序：定位游戏目录、读写主程序（带备份）、还原原版。 */

#include "common/backup.h"
#include "exe/game_exe.h"

const wchar_t *const kGameExe[4] = {
    L"DK4HD_sc.exe", L"DK4HD_tc.exe", L"DK4HD_jp.exe", L"DK4HD_kr.exe"
};

const wchar_t *const kGameExeName[4] = {
    L"简体", L"繁体", L"日文", L"韩文"
};

static void exe_ini_path(wchar_t *out, int outn) {
    wchar_t mod[MAX_PATH];
    wchar_t *slash;
    out[0] = 0;
    if (!GetModuleFileNameW(NULL, mod, MAX_PATH)) return;
    slash = wcsrchr(mod, L'\\');
    if (!slash) return;
    slash[1] = 0;
    StringCchPrintfW(out, (size_t)outn, L"%sdk4save.ini", mod);
}

void parent_dir(const wchar_t *file, wchar_t *dir, int dirn) {
    wchar_t tmp[MAX_PATH];
    wchar_t *slash;
    dir[0] = 0;
    StringCchCopyW(tmp, MAX_PATH, file);
    slash = wcsrchr(tmp, L'\\');
    if (!slash) return;
    *slash = 0;
    StringCchCopyW(dir, (size_t)dirn, tmp);
}

int dir_has_game(const wchar_t *dir) {
    int i;
    if (!dir || !dir[0]) return 0;
    for (i = 0; i < 4; i++) {
        wchar_t path[MAX_PATH];
        StringCchPrintfW(path, MAX_PATH, L"%s\\%s", dir, kGameExe[i]);
        if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) return 1;
    }
    return 0;
}

void remember_game_dir(const wchar_t *dir) {
    wchar_t ini[MAX_PATH];
    exe_ini_path(ini, MAX_PATH);
    if (ini[0] && dir && dir[0])
        WritePrivateProfileStringW(L"game", L"dir", dir, ini);
}

void find_game_dir(wchar_t *dir, int dirn) {
    wchar_t ini[MAX_PATH];
    wchar_t mod[MAX_PATH];
    wchar_t got[MAX_PATH];
    const wchar_t *fallback = L"E:\\Uncharted.Waters.IV.HD.Version.v1.0.2";
    dir[0] = 0;
    exe_ini_path(ini, MAX_PATH);
    if (ini[0]) {
        GetPrivateProfileStringW(L"game", L"dir", L"", got, MAX_PATH, ini);
        if (dir_has_game(got)) {
            StringCchCopyW(dir, (size_t)dirn, got);
            return;
        }
    }
    if (GetModuleFileNameW(NULL, mod, MAX_PATH)) {
        parent_dir(mod, got, MAX_PATH);
        if (dir_has_game(got)) {
            StringCchCopyW(dir, (size_t)dirn, got);
            return;
        }
    }
    if (dir_has_game(fallback))
        StringCchCopyW(dir, (size_t)dirn, fallback);
}

int load_exe_bytes(const wchar_t *path, uint8_t **out, size_t *outn, wchar_t *err, int errn) {
    HANDLE h;
    LARGE_INTEGER sz;
    DWORD got, done;
    uint8_t *buf;
    *out = NULL;
    *outn = 0;
    h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        StringCchPrintfW(err, (size_t)errn, L"打不开 %s", path);
        return 0;
    }
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart < 1024 * 1024 || sz.QuadPart > 20 * 1024 * 1024) {
        CloseHandle(h);
        StringCchCopyW(err, (size_t)errn, L"文件大小不像游戏主程序。");
        return 0;
    }
    buf = (uint8_t *)malloc((size_t)sz.QuadPart);
    if (!buf) {
        CloseHandle(h);
        StringCchCopyW(err, (size_t)errn, L"内存不足。");
        return 0;
    }
    done = 0;
    while (done < (DWORD)sz.QuadPart) {
        if (!ReadFile(h, buf + done, (DWORD)sz.QuadPart - done, &got, NULL) || got == 0) {
            CloseHandle(h);
            free(buf);
            StringCchCopyW(err, (size_t)errn, L"读取主程序中断。");
            return 0;
        }
        done += got;
    }
    CloseHandle(h);
    *out = buf;
    *outn = (size_t)sz.QuadPart;
    return 1;
}

/* 先写到旁边的临时文件，备份当前文件，再替换过去。 */
int save_exe_bytes(const wchar_t *path, const uint8_t *data, size_t n, wchar_t *err, int errn) {
    wchar_t tmp[MAX_PATH];
    HANDLE h;
    DWORD wrote, done;
    StringCchPrintfW(tmp, MAX_PATH, L"%s.writing", path);
    h = CreateFileW(tmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        StringCchCopyW(err, (size_t)errn, L"无法准备写入。请先完全退出游戏。");
        return 0;
    }
    done = 0;
    while (done < (DWORD)n) {
        if (!WriteFile(h, data + done, (DWORD)n - done, &wrote, NULL) || wrote == 0) {
            CloseHandle(h);
            DeleteFileW(tmp);
            StringCchCopyW(err, (size_t)errn, L"写入中断。原主程序没有改。");
            return 0;
        }
        done += wrote;
    }
    CloseHandle(h);
    if (!backup_before_write(path, NULL, 0)) {
        DeleteFileW(tmp);
        StringCchCopyW(err, (size_t)errn, L"备份失败，没有改主程序。");
        return 0;
    }
    if (!MoveFileExW(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(tmp);
        StringCchCopyW(err, (size_t)errn, L"替换主程序失败。请先完全退出游戏。");
        return 0;
    }
    return 1;
}

/* 用 DK4HD_*.exe.bak 还原主程序。被替换的文件先另存一份，还原也能撤销。 */
int restore_game_exes(const wchar_t *dir, wchar_t *status, int statusn) {
    int i, done = 0, same = 0, none = 0;
    wchar_t path[MAX_PATH], bak[MAX_PATH], err[240], names[120];
    names[0] = 0;
    for (i = 0; i < 4; i++) {
        uint8_t *orig = NULL, *cur = NULL;
        size_t on = 0, cn = 0;
        StringCchPrintfW(path, MAX_PATH, L"%s\\%s", dir, kGameExe[i]);
        StringCchPrintfW(bak, MAX_PATH, L"%s.bak", path);
        if (GetFileAttributesW(bak) == INVALID_FILE_ATTRIBUTES) {
            if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) none++;
            continue;
        }
        if (!load_exe_bytes(bak, &orig, &on, err, 240)) {
            StringCchPrintfW(status, (size_t)statusn, L"%s的备份读不出来，没有还原。%s", kGameExeName[i], err);
            if (done) StringCchCatW(status, (size_t)statusn, L" 前面的版本已经还原。");
            return 0;
        }
        if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES &&
            load_exe_bytes(path, &cur, &cn, err, 240) && cn == on && memcmp(cur, orig, on) == 0) {
            free(cur);
            free(orig);
            same++;
            continue;
        }
        free(cur);
        if (!save_exe_bytes(path, orig, on, err, 240)) {
            free(orig);
            StringCchPrintfW(status, (size_t)statusn, L"%s没有还原。%s", kGameExeName[i], err);
            if (done) StringCchCatW(status, (size_t)statusn, L" 前面的版本已经还原。");
            return 0;
        }
        free(orig);
        if (names[0]) StringCchCatW(names, 120, L"、");
        StringCchCatW(names, 120, kGameExeName[i]);
        done++;
    }
    if (!done && !same) {
        StringCchCopyW(status, (size_t)statusn, none
            ? L"主程序旁边没有 .bak，这个目录还没被本工具写过，主程序就是原文件。"
            : L"这个目录里没有 DK4HD 主程序。");
        return 0;
    }
    if (done)
        StringCchPrintfW(status, (size_t)statusn, L"已还原%s。还原前的主程序另存为 主程序名.日期-时间.bak。", names);
    else
        StringCchCopyW(status, (size_t)statusn, L"主程序已经是原文件，没有改动。");
    if (done && same) StringCchCatW(status, (size_t)statusn, L" 其余版本本来就是原文件。");
    if (none) StringCchCatW(status, (size_t)statusn, L" 有的版本没有 .bak，没动。");
    return 1;
}
