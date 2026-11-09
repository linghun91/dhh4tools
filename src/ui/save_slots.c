/* 存档栏列表，当前存档的打开、刷新、保存。 */

#include "save/save.h"
#include "ui/app.h"
#include "ui/widgets.h"
#include "ui/person_editor.h"
#include "ui/save_slots.h"

static int slot_of_path(const wchar_t *path) {
    int i;
    if (!g.root[0] || !path || !path[0]) return -1;
    for (i = 0; i <= 10; i++) {
        wchar_t slot[MAX_PATH];
        slot_path(g.root, i, slot, MAX_PATH);
        if (_wcsicmp(slot, path) == 0) return i;
    }
    return -1;
}

static int peek_level(const wchar_t *path, wchar_t *name, int namen, unsigned *level) {
    Save tmp;
    if (g.save.data && _wcsicmp(path, g.save.path) == 0) {
        person_name(&g.save, 0, name, namen);
        *level = hero_level(&g.save);
        return 1;
    }
    ZeroMemory(&tmp, sizeof tmp);
    if (!load_save(&tmp, path, NULL, 0)) return 0;
    person_name(&tmp, 0, name, namen);
    *level = hero_level(&tmp);
    free_save(&tmp);
    return 1;
}

void refresh_slots(void) {
    int slot, row = 0;
    if (!g.slots) return;
    begin_update();
    SendMessageW(g.slots, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g.slots);
    if (g.root[0]) {
        for (slot = 0; slot <= 10; slot++) {
            wchar_t path[MAX_PATH];
            wchar_t name[160], idx[8], level[16];
            const wchar_t *cols[3];
            unsigned lv = 0;
            slot_path(g.root, slot, path, MAX_PATH);
            if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) continue;
            StringCchPrintfW(idx, 8, L"%d", slot);
            if (!peek_level(path, name, 160, &lv)) {
                StringCchCopyW(name, 160, L"无法读取");
                StringCchCopyW(level, 16, L"-");
            } else {
                StringCchPrintfW(level, 16, L"%u", lv);
            }
            cols[0] = idx;
            cols[1] = name;
            cols[2] = level;
            lv_insert(g.slots, row++, slot, cols, 3);
        }
    }
    SendMessageW(g.slots, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g.slots, NULL, TRUE);
    end_update();
    select_param(g.slots, g.slot);
}

int save_current(void) {
    wchar_t msg[560];
    if (!g.save.data) return 0;
    if (g.person >= 0 && editor_differs()) commit_editor(NULL);
    if (!write_save(&g.save, msg, 560)) {
        MessageBoxW(g.hwnd, msg, L"保存失败", MB_ICONWARNING);
        return 0;
    }
    update_title();
    fill_people();
    refresh_slots();
    set_status(msg);
    return 1;
}

int confirm_discard_or_save(void) {
    int answer;
    if (!g.save.data) return 1;
    if (!g.save.dirty && !editor_differs()) return 1;
    answer = MessageBoxW(g.hwnd,
        L"还有未保存的修改。要先保存吗？",
        L"大航海时代4 HD 存档修改器",
        MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL) return 0;
    if (answer == IDNO) return 1;
    return save_current();
}

static int replace_save(const wchar_t *path) {
    Save next;
    wchar_t err[360];
    wchar_t name[160];
    wchar_t status[360];
    ZeroMemory(&next, sizeof next);
    if (!load_save(&next, path, err, 360)) {
        MessageBoxW(g.hwnd, err, L"打不开存档", MB_ICONWARNING);
        return 0;
    }
    free_save(&g.save);
    g.save = next;
    g.slot = slot_of_path(g.save.path);
    g.person = 0;
    update_title();
    SetWindowTextW(g.pathLabel, g.save.path);
    fill_people();
    person_name(&g.save, 0, name, 160);
    StringCchPrintfW(status, 360, L"已打开。主角 %s，等级 %u。",
        name, hero_level(&g.save));
    warn_short_damage(&g.save, status, 360);
    set_status(status);
    return 1;
}

/* 存档栏里当前文件的等级取自内存，刷新必须重读磁盘，否则游戏里新招的人不显示。 */
void on_refresh(void) {
    wchar_t path[MAX_PATH];
    wchar_t err[360];
    wchar_t name[160];
    wchar_t status[360];
    Save next;
    int person;
    if (!g.save.data || !g.save.path[0]) {
        refresh_slots();
        set_status(L"已刷新存档栏。");
        return;
    }
    if (g.save.dirty || editor_differs()) {
        int answer = MessageBoxW(g.hwnd,
            L"内存里还有未保存的修改。重新读取会丢掉这些修改，改用磁盘上的存档。继续吗？",
            L"刷新",
            MB_YESNO | MB_ICONQUESTION);
        if (answer != IDYES) {
            refresh_slots();
            set_status(L"未重新读取。内存里的修改还在。");
            return;
        }
    }
    StringCchCopyW(path, MAX_PATH, g.save.path);
    person = g.person;
    ZeroMemory(&next, sizeof next);
    if (!load_save(&next, path, err, 360)) {
        MessageBoxW(g.hwnd, err, L"打不开存档", MB_ICONWARNING);
        set_status(L"重新读取失败，仍显示内存里的存档。");
        return;
    }
    free_save(&g.save);
    g.save = next;
    g.slot = slot_of_path(g.save.path);
    g.person = person;
    update_title();
    SetWindowTextW(g.pathLabel, g.save.path);
    fill_people();
    refresh_slots();
    person_name(&g.save, 0, name, 160);
    StringCchPrintfW(status, 360, L"已从磁盘重新读取。主角 %s，等级 %u。",
        name, hero_level(&g.save));
    warn_short_damage(&g.save, status, 360);
    set_status(status);
}

void on_slot_clicked(int slot) {
    wchar_t path[MAX_PATH];
    if (!g.root[0]) return;
    slot_path(g.root, slot, path, MAX_PATH);
    if (g.save.data && _wcsicmp(path, g.save.path) == 0 &&
        !g.save.dirty && !editor_differs()) {
        return;
    }
    if (!confirm_discard_or_save()) {
        select_param(g.slots, g.slot);
        return;
    }
    if (!replace_save(path)) {
        select_param(g.slots, g.slot);
        return;
    }
    refresh_slots();
}

void on_open(void) {
    wchar_t file[MAX_PATH];
    OPENFILENAMEW ofn;
    if (!confirm_discard_or_save()) return;
    file[0] = 0;
    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = g.hwnd;
    ofn.lpstrFilter = L"DK4 存档 (*.dk4)\0*.dk4\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = g.root[0] ? g.root : NULL;
    ofn.lpstrTitle = L"打开存档";
    ofn.lpstrDefExt = L"dk4";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn)) return;
    if (replace_save(file)) refresh_slots();
}

void startup_load(void) {
    if (g.root[0]) {
        SetWindowTextW(g.pathLabel, g.root);
        refresh_slots();
        if (ListView_GetItemCount(g.slots) > 0) {
            LVITEMW it;
            wchar_t path[MAX_PATH];
            ZeroMemory(&it, sizeof it);
            it.mask = LVIF_PARAM;
            it.iItem = 0;
            if (ListView_GetItem(g.slots, &it)) {
                slot_path(g.root, (int)it.lParam, path, MAX_PATH);
                if (replace_save(path)) refresh_slots();
            }
        } else {
            set_status(L"存档目录里没有 savedat00 到 savedat10。");
        }
    } else {
        SetWindowTextW(g.pathLabel, L"未找到存档目录");
        set_status(L"没有找到 Documents\\KoeiTecmo\\DK4HD\\Savedata。用“打开”选择 savedatXX.dk4。");
    }
}
