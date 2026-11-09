/* 游戏存档目录与存档栏文件名。 */

#include "save/save.h"

int find_save_root(wchar_t *out, int outn) {
    wchar_t home[MAX_PATH];
    const wchar_t *langs[] = {L"zh-CN", L"zh-TW", L"ja-JP", L"ko-KR", L"ja", L"ko", NULL};
    int i;
    if (!GetEnvironmentVariableW(L"USERPROFILE", home, MAX_PATH)) return 0;
    for (i = 0; langs[i]; i++) {
        wchar_t dir[MAX_PATH];
        wchar_t probe[MAX_PATH];
        StringCchPrintfW(dir, MAX_PATH,
            L"%s\\Documents\\KoeiTecmo\\DK4HD\\Savedata\\%s", home, langs[i]);
        StringCchPrintfW(probe, MAX_PATH, L"%s\\savedat00.dk4", dir);
        if (GetFileAttributesW(probe) != INVALID_FILE_ATTRIBUTES) {
            StringCchCopyW(out, (size_t)outn, dir);
            return 1;
        }
    }
    return 0;
}

void slot_path(const wchar_t *root, int slot, wchar_t *out, int outn) {
    StringCchPrintfW(out, (size_t)outn, L"%s\\savedat%02d.dk4", root, slot);
}
