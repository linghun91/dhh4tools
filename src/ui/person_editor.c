/* 人物列表与右侧编辑表单。 */

#include "common/bytes.h"
#include "save/save.h"
#include "ui/app.h"
#include "ui/widgets.h"
#include "ui/person_editor.h"
#include "ui/save_slots.h"

static void read_form(int *faction, int *level, int stats[STAT_COUNT], int *clamped) {
    int i;
    if (clamped) *clamped = 0;
    *faction = edit_int(g.faction, 0, 255, clamped);
    *level = edit_int(g.level, 0, MAX_TOTAL_LEVEL, clamped);
    for (i = 0; i < STAT_COUNT; i++)
        stats[i] = edit_int(g.stat[i], 0, 255, clamped);
}

void sync_buttons(void) {
    int has = g.save.data != NULL;
    int person = has && g.person >= 0;
    int i;
    EnableWindow(g.saveBtn, has);
    EnableWindow(g.batchBtn, person);
    EnableWindow(g.applyBtn, person);
    EnableWindow(g.fillStatBtn, person);
    EnableWindow(g.fillLevelBtn, person);
    EnableWindow(g.faction, person);
    EnableWindow(g.level, person);
    EnableWindow(g.filterRec, has);
    EnableWindow(g.filterAll, has);
    for (i = 0; i < STAT_COUNT; i++)
        EnableWindow(g.stat[i], person);
}

static int want_person(int index) {
    if (!g.save.data) return 0;
    if (index == 0) return person_exists(&g.save, 0);
    if (!is_present(&g.save, index)) return 0;
    if (g.recruited_only && !is_recruited(&g.save, index)) return 0;
    return 1;
}

static void show_person(int index) {
    Body b;
    wchar_t name[160], line[220], track[220];
    int stats[STAT_COUNT];
    int i;
    unsigned lv0, lv1;
    const wchar_t *kind;
    int ok;
    g.person = index;
    ok = g.save.data && index >= 0 && body_at(&g.save, index, &b);
    if (!ok) {
        SetWindowTextW(g.nameLabel, L"未选择角色");
        SetWindowTextW(g.trackLabel, L"");
        SetWindowTextW(g.faction, L"");
        SetWindowTextW(g.level, L"");
        for (i = 0; i < STAT_COUNT; i++) SetWindowTextW(g.stat[i], L"");
        sync_buttons();
        return;
    }
    person_name(&g.save, index, name, 160);
    if (index == 0 || is_protagonist_card(&g.save, index)) kind = L"主角";
    else if (is_recruited(&g.save, index)) kind = L"已招收";
    else kind = L"名册";
    StringCchPrintfW(line, 220, L"%s    %s", kind, name);
    SetWindowTextW(g.nameLabel, line);
    lv0 = track_level(ru32(b.exp0));
    lv1 = track_level(ru32(b.exp1));
    StringCchPrintfW(track, 220, L"等级 %u（第一段 %u + 第二段 %u）", lv0 + lv1, lv0, lv1);
    if (ru32(b.exp0) > exp_of_level(MAX_TRACK_LEVEL) ||
        ru32(b.exp1) > exp_of_level(MAX_TRACK_LEVEL)) {
        StringCchCatW(track, 220, L"，经验超过画面上限");
    }
    SetWindowTextW(g.trackLabel, track);
    set_edit_int(g.faction, (int)(*b.faction));
    set_edit_int(g.level, (int)(lv0 + lv1));
    read_stats(b.stat, stats);
    for (i = 0; i < STAT_COUNT; i++) set_edit_int(g.stat[i], stats[i]);
    sync_buttons();
}

static void add_person_row(int row, int index) {
    Body b;
    wchar_t idx[16], name[160], level[16], faction[16], state[16], numbers[64];
    int stats[STAT_COUNT];
    const wchar_t *cols[6];
    if (!body_at(&g.save, index, &b)) return;
    StringCchPrintfW(idx, 16, L"%d", index);
    person_name(&g.save, index, name, 160);
    StringCchPrintfW(level, 16, L"%u", total_level(b.exp0, b.exp1));
    StringCchPrintfW(faction, 16, L"%u", (unsigned)(*b.faction));
    if (index == 0 || is_protagonist_card(&g.save, index)) StringCchCopyW(state, 16, L"主角");
    else if (is_recruited(&g.save, index)) StringCchCopyW(state, 16, L"已招收");
    else StringCchCopyW(state, 16, L"名册");
    read_stats(b.stat, stats);
    StringCchPrintfW(numbers, 64, L"%d %d %d %d %d %d",
        stats[0], stats[1], stats[2], stats[3], stats[4], stats[5]);
    cols[0] = idx;
    cols[1] = name;
    cols[2] = level;
    cols[3] = faction;
    cols[4] = state;
    cols[5] = numbers;
    lv_insert(g.people, row, index, cols, 6);
}

void fill_people(void) {
    int keep = g.person;
    int i, row = 0;
    if (!g.people) return;
    begin_update();
    SendMessageW(g.people, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g.people);
    if (g.save.data) {
        if (want_person(0)) add_person_row(row++, 0);
        for (i = 1; i <= FULL_PERSON_COUNT; i++) {
            if (!want_person(i)) continue;
            add_person_row(row++, i);
        }
    }
    SendMessageW(g.people, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g.people, NULL, TRUE);
    end_update();
    if (!(keep >= 0 && want_person(keep)))
        keep = (g.save.data && want_person(0)) ? 0 : -1;
    select_param(g.people, keep);
    show_person(keep);
}

int editor_differs(void) {
    Body b;
    int faction, level, stats[STAT_COUNT], old[STAT_COUNT], i;
    if (!g.save.data || g.person < 0) return 0;
    if (!body_at(&g.save, g.person, &b)) return 0;
    read_form(&faction, &level, stats, NULL);
    if (faction != (int)(*b.faction)) return 1;
    if ((unsigned)level != total_level(b.exp0, b.exp1)) return 1;
    read_stats(b.stat, old);
    for (i = 0; i < STAT_COUNT; i++) {
        if (stats[i] != old[i]) return 1;
    }
    return 0;
}

void commit_editor(int *clamped_out) {
    int faction, level, stats[STAT_COUNT], clamped = 0;
    Body b;
    if (g.person < 0) return;
    if (!body_at(&g.save, g.person, &b)) return;
    read_form(&faction, &level, stats, &clamped);
    apply_person(&g.save, g.person, faction, stats, (unsigned)level, 1);
    g.save.dirty = 1;
    update_title();
    if (clamped_out) *clamped_out = clamped;
}

void on_person_clicked(int index) {
    int answer;
    if (index == g.person) return;
    if (g.person >= 0 && editor_differs()) {
        answer = MessageBoxW(g.hwnd,
            L"这个角色的输入还没应用。要应用后再切换吗？",
            L"大航海时代4 HD 存档修改器",
            MB_YESNOCANCEL | MB_ICONQUESTION);
        if (answer == IDCANCEL) {
            select_param(g.people, g.person);
            return;
        }
        if (answer == IDYES) {
            commit_editor(NULL);
            PostMessageW(g.hwnd, WM_APP_REFRESH, 0, 0);
        }
    }
    show_person(index);
}

void on_apply(void) {
    int clamped = 0;
    if (g.person < 0) return;
    commit_editor(&clamped);
    fill_people();
    refresh_slots();
    if (clamped)
        set_status(L"有数值超出范围，已按上下限写入内存。还没保存到文件。");
    else
        set_status(L"已写入内存。还没保存到文件。");
}

void on_batch(void) {
    int stats[STAT_COUNT];
    int faction, level, clamped = 0;
    int i, n = 0;
    wchar_t text[320];
    if (!g.save.data || g.person < 0) return;
    read_form(&faction, &level, stats, &clamped);
    (void)faction;
    for (i = 0; i <= FULL_PERSON_COUNT; i++) {
        Body probe;
        if (is_protagonist_card(&g.save, i)) continue;
        if (i != 0 && !is_recruited(&g.save, i)) continue;
        if (!body_at(&g.save, i, &probe)) continue;
        n++;
    }
    if (n <= 0) {
        set_status(L"没有可以修改的角色。");
        return;
    }
    StringCchPrintfW(text, 320,
        L"把主角和已招收海员共 %d 人的六维设为 %d %d %d %d %d %d，等级设为 %d？\n所属标记不会改。",
        n, stats[0], stats[1], stats[2], stats[3], stats[4], stats[5], level);
    if (MessageBoxW(g.hwnd, text, L"批量修改", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;
    n = 0;
    for (i = 0; i <= FULL_PERSON_COUNT; i++) {
        Body probe;
        if (is_protagonist_card(&g.save, i)) continue;
        if (i != 0 && !is_recruited(&g.save, i)) continue;
        if (!body_at(&g.save, i, &probe)) continue;
        apply_person(&g.save, i, 0, stats, (unsigned)level, 0);
        n++;
    }
    g.save.dirty = 1;
    update_title();
    fill_people();
    refresh_slots();
    if (clamped)
        StringCchPrintfW(text, 320, L"已把 %d 人写入内存，超出范围的数值已按上下限处理。还没保存到文件。", n);
    else
        StringCchPrintfW(text, 320, L"已把 %d 人写入内存。还没保存到文件。", n);
    set_status(text);
}

void on_filter(int recruited_only) {
    if (g.suspend) return;
    if (editor_differs()) {
        int answer = MessageBoxW(g.hwnd,
            L"这个角色的输入还没应用。要应用吗？",
            L"大航海时代4 HD 存档修改器",
            MB_YESNOCANCEL | MB_ICONQUESTION);
        if (answer == IDCANCEL) {
            begin_update();
            CheckRadioButton(g.hwnd, IDC_FILTER_REC, IDC_FILTER_ALL,
                g.recruited_only ? IDC_FILTER_REC : IDC_FILTER_ALL);
            end_update();
            return;
        }
        if (answer == IDYES) commit_editor(NULL);
    }
    g.recruited_only = recruited_only;
    fill_people();
    refresh_slots();
}
