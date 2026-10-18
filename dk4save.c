/* DK4HD save editor, windowed.
 * Reads Documents\KoeiTecmo\DK4HD\Savedata\*\savedatXX.dk4
 * and edits the protagonist plus recruited sailors: six attributes,
 * the two experience tracks (the level shown in game), and the faction mark.
 *
 * The status screen reads a 0x30 sailor object. Those bytes live here
 * (magic "DAIKOUKAI4PKW9 1"), not in the later name block:
 *   +0x20                 protagonist roster id (0 拉斐尔 … 5 亚伯拉罕)
 *   +0x70                 cached total level of the protagonist
 *   roster 0x2C00, stride 0x19, sailor id i:
 *     +0x03 faction, +0x05 six stat bytes, +0x0B and +0x0F uint32 exp
 *   protagonist body, the copy the status screen actually shows:
 *     faction 0x35AA, stats 0x35AC, exp 0x35B2 and 0x35B6
 *   name block 0x35C3, stride 0x2E9: protagonist name only
 *     (record i is sailor id i-1; names of sailors are the table below)
 *   stat order: 体力 敏捷 魅力 智力 精神 运气
 *   one track's level = isqrt(exp/30), capped at 255
 */
#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
#include <windows.h>
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif
#include <commctrl.h>
#include <commdlg.h>
#include <strsafe.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAVE_MAGIC "DAIKOUKAI4PKW9 1"
#define REC_BASE 0x35C3
#define REC_STRIDE 0x2E9
#define SAILOR_COUNT 98
#define NAME_FIELD 17
#define STAT_COUNT 6
#define ROSTER_BASE 0x2C00
#define ROSTER_STRIDE 0x19
#define OFF_R_FACTION 0x03
#define OFF_R_STAT 0x05
#define OFF_R_EXP0 0x0B
#define OFF_R_EXP1 0x0F
#define HERO_FACTION 0x35AA
#define HERO_STAT 0x35AC
#define HERO_EXP0 0x35B2
#define HERO_EXP1 0x35B6
#define OFF_PROT_ID 0x20
#define OFF_CACHED_LEVEL 0x70
#define MAX_TRACK_LEVEL 255
#define MAX_TOTAL_LEVEL (MAX_TRACK_LEVEL * 2)
#define WM_APP_REFRESH (WM_APP + 1)

enum {
    IDC_PATH = 100,
    IDC_REFRESH,
    IDC_OPEN,
    IDC_SLOTS,
    IDC_PEOPLE,
    IDC_FILTER_REC,
    IDC_FILTER_ALL,
    IDC_NAME,
    IDC_TRACK,
    IDC_FACTION,
    IDC_LEVEL,
    IDC_STAT0,
    IDC_APPLY = 200,
    IDC_FILL_STAT,
    IDC_FILL_LEVEL,
    IDC_BATCH,
    IDC_SAVE,
    IDC_STATUS,
    IDC_HINT,
    IDC_GAME_CAP
};

enum {
    IDC_CAP_PATH = 400,
    IDC_CAP_BROWSE,
    IDC_CAP_TRACK,
    IDC_CAP_BASE,
    IDC_CAP_STAT,
    IDC_CAP_SKILL,
    IDC_CAP_PORT,
    IDC_CAP_DERIVED,
    IDC_CAP_READ,
    IDC_CAP_WRITE,
    IDC_CAP_VANILLA,
    IDC_CAP_UNLOCK,
    IDC_CAP_UNLOCK_PORT,
    IDC_CAP_NOTE,
    IDC_CAP_STATUS,
    IDC_CAP_LBL_TRACK = 420,
    IDC_CAP_LBL_BASE,
    IDC_CAP_LBL_STAT,
    IDC_CAP_LBL_SKILL,
    IDC_CAP_LBL_PORT
};

static const wchar_t *const kStatName[STAT_COUNT] = {
    L"体力", L"敏捷", L"魅力", L"智力", L"精神", L"运气"
};

static const wchar_t *const kSailorName[SAILOR_COUNT] = {
    L"拉斐尔·卡斯特罗", L"赫德拉姆·柏格斯通", L"丽璐·阿歌特", L"华梅·李",
    L"杏太郎·佐伯", L"亚伯拉罕·伍丁", L"蒂雅·恰斯卡", L"杰拿斯·帕沙",
    L"库拉乌迪·马奈乌斯", L"弗利奥·埃涅科", L"克莉丝汀娜·埃涅科", L"阿尔加迪斯·欧多西斯",
    L"卡米尔·奥芬埃西", L"希恩·杨", L"詹姆·鲁德韦", L"行久·白木",
    L"铁礼列·腾尼", L"埃米利奥·菲陛", L"安杰洛·普契尼", L"格尔哈特·阿迪肯",
    L"阿尔·西恩", L"查理·洛雪弗", L"科鲁罗·西奈特", L"费南德·迪阿斯",
    L"易安·杜可夫", L"塞维·汉", L"曼努埃尔·阿尔米达", L"塞拉·夏尔巴拉兹",
    L"梨花·薛", L"尤里安·罗佩斯", L"阿芝莎·努连纳哈尔", L"林·森",
    L"阿米娜·安奈富", L"德尼雅·伊蒂哈德", L"法娣玛·哈涅", L"哈希姆·阿尔奈迪尔",
    L"柳科·西萨", L"谢尔·内迪姆", L"塞西莉雅·梅卡德", L"詹姆兹·克利福德",
    L"马丁·舒派亚", L"迪斯尼·阿博科鲁克", L"彼德罗·巴鲁迪斯", L"帕欧罗·陈特利欧",
    L"赫德姆·巴夏", L"巴巴洛沙·海雷丁", L"菲南·西鲁韦拉", L"詹洛尼默·埃斯皮诺沙",
    L"格里高利·图鲁维", L"柴门林·纳哥普尔", L"道阿尔泰·普雷依拉", L"安东尼奥·库恩",
    L"索静·来岛", L"巴士科斯·玛尔德纳德", L"迪欧歌·埃斯康特", L"瑜·文",
    L"罗伯特·史特科", L"查理·吉尔邦", L"帕罗·莫拉依斯", L"舒伯特·格拉斯",
    L"西门·李纳列斯", L"阿伦索·罗莎", L"恩查·列吉欧尼", L"萨利德·阿加儒",
    L"莫沙里·希得拉", L"阿纳朱德·贾乌米", L"伊文·尼耶迪", L"达维易·弗拉芬",
    L"盖欧古·札图吉塔", L"纳哥兴·卡莫路", L"赫尔黑·阿本思", L"加洛沙·奥毕厄德",
    L"芬·布兰科", L"维路斯·格拉纳特", L"杰克斯·勃鲁姆", L"朱里安·菲米尔",
    L"尤里斯·休根", L"柏列特·佩罗", L"强·拉姆吉欧", L"伽布利厄·卡尔杜齐",
    L"雅可布·勃特朗", L"贝拉肯苏·阿基列", L"芝加诺斯·贝", L"维廉·克莱布",
    L"鄂路南·贝利欧", L"易斯卡·雅沙尔", L"理查·回森", L"汉斯·勒茨",
    L"米哈易尔·勒茨", L"阿科布·达迈", L"谢乌德·埃米", L"米瓦尔·根茨",
    L"夏洛特·米列", L"桑亨·韩", L"克丽雅·波福特", L"爱斯法妮雅·梅卡德",
    L"特欧贝尔德·梅卡德", L"宗九郎·泷田"
};

_Static_assert(sizeof kSailorName / sizeof kSailorName[0] == SAILOR_COUNT, "sailor names");

typedef struct Save {
    wchar_t path[MAX_PATH];
    uint8_t *data;
    size_t size;
    int dirty;
} Save;

typedef struct App {
    HINSTANCE inst;
    HWND hwnd;
    HWND pathLabel;
    HWND refreshBtn;
    HWND gameCapBtn;
    HWND openBtn;
    HWND slotCaption;
    HWND editCaption;
    HWND slots;
    HWND people;
    HWND filterRec;
    HWND filterAll;
    HWND nameLabel;
    HWND trackLabel;
    HWND fieldLabel[8];
    HWND faction;
    HWND level;
    HWND stat[STAT_COUNT];
    HWND applyBtn;
    HWND fillStatBtn;
    HWND fillLevelBtn;
    HWND batchBtn;
    HWND saveBtn;
    HWND status;
    HWND hint;
    HFONT font;
    HACCEL accel;
    int dpi;
    int slot;
    int person;
    int recruited_only;
    int suspend;
    Save save;
    wchar_t root[MAX_PATH];
} App;

static App g;

static int S(int x) {
    int dpi = g.dpi < 96 ? 96 : g.dpi;
    return MulDiv(x, dpi, 96);
}

static uint32_t ru32(const uint8_t *p) {
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24);
}

static void wu32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

/* The six bytes and two exp tracks the 0x30 sailor object is loaded from. */
typedef struct Body {
    uint8_t *faction;
    uint8_t *stat;
    uint8_t *exp0;
    uint8_t *exp1;
} Body;

static void free_save(Save *s) {
    free(s->data);
    s->data = NULL;
    s->size = 0;
    s->dirty = 0;
}

static uint8_t *record_at(Save *s, int index) {
    size_t off;
    if (!s->data || index < 0) return NULL;
    off = (size_t)REC_BASE + (size_t)index * (size_t)REC_STRIDE;
    if (off + (size_t)REC_STRIDE > s->size) return NULL;
    return s->data + off;
}

static unsigned level_of_exp(uint32_t xp) {
    uint32_t q, r;
    if (xp < 30) return 0;
    q = xp / 30;
    r = 0;
    while (r < 400 && (r + 1) * (r + 1) <= q) r++;
    return (unsigned)r;
}

static uint32_t exp_of_level(unsigned level) {
    if (level > MAX_TRACK_LEVEL) level = MAX_TRACK_LEVEL;
    return 30u * level * level;
}

static unsigned track_level(uint32_t xp) {
    unsigned lv = level_of_exp(xp);
    if (lv > MAX_TRACK_LEVEL || xp > exp_of_level(MAX_TRACK_LEVEL))
        return MAX_TRACK_LEVEL;
    return lv;
}

static int body_roster(Save *s, int sailor, Body *b) {
    size_t off;
    if (!s->data || sailor < 0 || sailor >= SAILOR_COUNT) return 0;
    off = (size_t)ROSTER_BASE + (size_t)sailor * (size_t)ROSTER_STRIDE;
    if (off + (size_t)ROSTER_STRIDE > s->size) return 0;
    b->faction = s->data + off + OFF_R_FACTION;
    b->stat = s->data + off + OFF_R_STAT;
    b->exp0 = s->data + off + OFF_R_EXP0;
    b->exp1 = s->data + off + OFF_R_EXP1;
    return 1;
}

static int body_hero(Save *s, Body *b) {
    if (!s->data || s->size < (size_t)HERO_EXP1 + 4) return 0;
    b->faction = s->data + HERO_FACTION;
    b->stat = s->data + HERO_STAT;
    b->exp0 = s->data + HERO_EXP0;
    b->exp1 = s->data + HERO_EXP1;
    return 1;
}

/* index 0 is the protagonist body. index 1..98 is roster sailor id index-1. */
static int body_at(Save *s, int index, Body *b) {
    if (index == 0) return body_hero(s, b);
    return body_roster(s, index - 1, b);
}

static int protagonist_id(Save *s) {
    uint32_t id;
    if (!s->data || s->size < (size_t)OFF_PROT_ID + 4) return -1;
    id = ru32(s->data + OFF_PROT_ID);
    if (id >= SAILOR_COUNT) return -1;
    return (int)id;
}

static int is_protagonist_card(Save *s, int index) {
    int id = protagonist_id(s);
    return index > 0 && id == index - 1;
}

static int is_present(Save *s, int index) {
    Body b;
    int i;
    unsigned sum = 0;
    if (index <= 0 || index > SAILOR_COUNT) return 0;
    if (!body_at(s, index, &b)) return 0;
    for (i = 0; i < STAT_COUNT; i++) sum += b.stat[i];
    return sum > 0;
}

static int is_recruited(Save *s, int index) {
    Body player, mate;
    if (!is_present(s, index)) return 0;
    if (!body_hero(s, &player) || !body_at(s, index, &mate)) return 0;
    return *mate.faction == *player.faction;
}

static void read_stats(const uint8_t *stat, int out[STAT_COUNT]) {
    int i;
    for (i = 0; i < STAT_COUNT; i++)
        out[i] = (int)stat[i];
}

static void write_stats(uint8_t *stat, const int in[STAT_COUNT]) {
    int i;
    for (i = 0; i < STAT_COUNT; i++) {
        int v = in[i];
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        stat[i] = (uint8_t)v;
    }
}

static unsigned total_level(const uint8_t *exp0, const uint8_t *exp1) {
    return track_level(ru32(exp0)) + track_level(ru32(exp1));
}

static void write_total_level(uint8_t *exp0, uint8_t *exp1, unsigned total) {
    unsigned a, track1;
    unsigned cur0 = level_of_exp(ru32(exp0));
    unsigned cur1 = level_of_exp(ru32(exp1));
    if (total > MAX_TOTAL_LEVEL) total = MAX_TOTAL_LEVEL;
    if (cur0 + cur1 > 0) {
        a = (unsigned)(((uint64_t)total * cur0) / (cur0 + cur1));
        track1 = total - a;
    } else {
        a = total / 2;
        track1 = total - a;
    }
    if (a > MAX_TRACK_LEVEL) {
        track1 += a - MAX_TRACK_LEVEL;
        a = MAX_TRACK_LEVEL;
    }
    if (track1 > MAX_TRACK_LEVEL) {
        a += track1 - MAX_TRACK_LEVEL;
        track1 = MAX_TRACK_LEVEL;
    }
    if (a > MAX_TRACK_LEVEL) a = MAX_TRACK_LEVEL;
    wu32(exp0, exp_of_level(a));
    wu32(exp1, exp_of_level(track1));
}

static void write_cached_level(Save *s) {
    Body hero;
    if (!body_hero(s, &hero) || s->size < (size_t)OFF_CACHED_LEVEL + 4) return;
    wu32(s->data + OFF_CACHED_LEVEL, total_level(hero.exp0, hero.exp1));
}

static void mirror_growth(Body *dst, const Body *src, int copy_faction) {
    memcpy(dst->stat, src->stat, STAT_COUNT);
    memcpy(dst->exp0, src->exp0, 4);
    memcpy(dst->exp1, src->exp1, 4);
    if (copy_faction) *dst->faction = *src->faction;
}

/* Writes the status-screen fields. The protagonist also has a roster row;
 * keep that row identical so either list entry changes the loaded character. */
static void apply_person(Save *s, int index, int faction, const int stats[STAT_COUNT],
                         unsigned level, int write_faction) {
    Body primary, other;
    int id;
    if (!body_at(s, index, &primary)) return;
    if (write_faction) *primary.faction = (uint8_t)faction;
    write_stats(primary.stat, stats);
    write_total_level(primary.exp0, primary.exp1, level);
    id = protagonist_id(s);
    if (index == 0 && id >= 0 && body_roster(s, id, &other))
        mirror_growth(&other, &primary, write_faction);
    else if (index > 0 && id == index - 1 && body_hero(s, &other))
        mirror_growth(&other, &primary, write_faction);
    if (index == 0 || (index > 0 && id == index - 1))
        write_cached_level(s);
}

static void gbk_field_to_wide(const uint8_t *src, int n, wchar_t *dst, int dstn) {
    char raw[40];
    int i;
    if (dstn <= 0) return;
    dst[0] = 0;
    if (n > 36) n = 36;
    memcpy(raw, src, (size_t)n);
    raw[n] = 0;
    for (i = 0; i < n; i++) {
        if (raw[i] == 0) break;
    }
    raw[i] = 0;
    if (raw[0] == 0) return;
    if (MultiByteToWideChar(936, 0, raw, -1, dst, dstn) <= 0)
        StringCchCopyW(dst, (size_t)dstn, L"?");
}

static void person_name(Save *s, int index, wchar_t *dst, int dstn) {
    uint8_t *rec = record_at(s, index);
    wchar_t a[64], b[64], c[64];
    if (!rec) {
        StringCchCopyW(dst, (size_t)dstn, L"(空)");
        return;
    }
    if (index == 0) {
        gbk_field_to_wide(rec + 0x00, NAME_FIELD, a, 64);
        gbk_field_to_wide(rec + 0x22, NAME_FIELD, b, 64);
        gbk_field_to_wide(rec + 0x11, NAME_FIELD, c, 64);
        if (b[0] && c[0])
            StringCchPrintfW(dst, (size_t)dstn, L"%s·%s·%s", a, b, c);
        else if (c[0])
            StringCchPrintfW(dst, (size_t)dstn, L"%s·%s", a, c);
        else
            StringCchPrintfW(dst, (size_t)dstn, L"%s", a[0] ? a : L"(无名)");
        return;
    }
    if (index >= 1 && index <= SAILOR_COUNT)
        StringCchCopyW(dst, (size_t)dstn, kSailorName[index - 1]);
    else
        StringCchPrintfW(dst, (size_t)dstn, L"记录%d", index);
}

static int load_save(Save *s, const wchar_t *path, wchar_t *err, int errn) {
    FILE *fp;
    long sz;
    uint8_t *data;
    fp = _wfopen(path, L"rb");
    if (!fp) {
        if (err && errn > 0) StringCchPrintfW(err, (size_t)errn, L"打不开 %s", path);
        return 0;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        if (err && errn > 0) StringCchCopyW(err, (size_t)errn, L"无法读取文件长度。");
        return 0;
    }
    sz = ftell(fp);
    if (sz < (long)(REC_BASE + REC_STRIDE) || sz > 2L * 1024L * 1024L) {
        fclose(fp);
        if (err && errn > 0) StringCchCopyW(err, (size_t)errn, L"文件大小不像存档。");
        return 0;
    }
    rewind(fp);
    data = (uint8_t *)malloc((size_t)sz);
    if (!data || fread(data, 1, (size_t)sz, fp) != (size_t)sz) {
        fclose(fp);
        free(data);
        if (err && errn > 0) StringCchCopyW(err, (size_t)errn, L"读取中断。");
        return 0;
    }
    fclose(fp);
    if (memcmp(data, SAVE_MAGIC, 16) != 0) {
        free(data);
        if (err && errn > 0) StringCchCopyW(err, (size_t)errn, L"文件头不是 DAIKOUKAI4PKW9 1。");
        return 0;
    }
    free(s->data);
    s->data = data;
    s->size = (size_t)sz;
    s->dirty = 0;
    StringCchCopyW(s->path, MAX_PATH, path);
    return 1;
}

static int write_save(Save *s, wchar_t *msg, int msgn) {
    wchar_t bak[MAX_PATH];
    wchar_t extra[160];
    FILE *fp;
    uint32_t stored = 0;
    int copied = 0;
    if (!s->data) {
        StringCchCopyW(msg, (size_t)msgn, L"没有打开存档。");
        return 0;
    }
    if (s->size >= 0x18) stored = ru32(s->data + 0x14);
    StringCchPrintfW(bak, MAX_PATH, L"%s.bak", s->path);
    if (GetFileAttributesW(bak) == INVALID_FILE_ATTRIBUTES) {
        copied = CopyFileW(s->path, bak, TRUE) ? 1 : -1;
    }
    fp = _wfopen(s->path, L"wb");
    if (!fp) {
        StringCchCopyW(msg, (size_t)msgn, L"无法写入。请先关掉游戏。");
        return 0;
    }
    if (fwrite(s->data, 1, s->size, fp) != s->size) {
        fclose(fp);
        StringCchCopyW(msg, (size_t)msgn, L"写入不完整。");
        return 0;
    }
    fclose(fp);
    s->dirty = 0;
    if (copied == 1)
        StringCchPrintfW(msg, (size_t)msgn, L"已写入。备份 %s", bak);
    else if (copied == -1)
        StringCchCopyW(msg, (size_t)msgn, L"已写入。备份失败，原文件已经被覆盖。");
    else
        StringCchPrintfW(msg, (size_t)msgn, L"已写入。已有备份，没有覆盖 %s", bak);
    if (stored != (uint32_t)s->size) {
        StringCchPrintfW(extra, 160, L" 长度字段是 %u，文件是 %u，未改长度字段。",
            stored, (unsigned)s->size);
        StringCchCatW(msg, (size_t)msgn, extra);
    }
    return 1;
}

static int find_save_root(wchar_t *out, int outn) {
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

static void slot_path(const wchar_t *root, int slot, wchar_t *out, int outn) {
    StringCchPrintfW(out, (size_t)outn, L"%s\\savedat%02d.dk4", root, slot);
}

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

static void update_title(void) {
    SetWindowTextW(g.hwnd, g.save.dirty
        ? L"大航海时代4 HD 存档修改器 *"
        : L"大航海时代4 HD 存档修改器");
}

static void set_status(const wchar_t *text) {
    if (g.status) SetWindowTextW(g.status, text ? text : L"");
}

static void begin_update(void) { g.suspend++; }
static void end_update(void) { if (g.suspend > 0) g.suspend--; }

static HWND add_ctl(DWORD ex, const wchar_t *cls, const wchar_t *text, DWORD style, int id) {
    return CreateWindowExW(ex, cls, text, style | WS_CHILD | WS_VISIBLE,
        0, 0, 10, 10, g.hwnd, (HMENU)(INT_PTR)id, g.inst, NULL);
}

static void lv_col(HWND lv, int index, const wchar_t *text, int width) {
    LVCOLUMNW col;
    ZeroMemory(&col, sizeof col);
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;
    col.cx = width;
    col.pszText = (LPWSTR)text;
    ListView_InsertColumn(lv, index, &col);
}

static void lv_style(HWND lv) {
    ListView_SetExtendedListViewStyle(lv,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
}

static void lv_insert(HWND lv, int row, LPARAM param, const wchar_t **cols, int ncols) {
    LVITEMW it;
    int c;
    ZeroMemory(&it, sizeof it);
    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = row;
    it.pszText = (LPWSTR)cols[0];
    it.lParam = param;
    ListView_InsertItem(lv, &it);
    for (c = 1; c < ncols; c++)
        ListView_SetItemText(lv, row, c, (LPWSTR)cols[c]);
}

static void select_param(HWND lv, LPARAM param) {
    int i, n;
    begin_update();
    n = ListView_GetItemCount(lv);
    for (i = 0; i < n; i++) {
        LVITEMW it;
        UINT state;
        ZeroMemory(&it, sizeof it);
        it.mask = LVIF_PARAM;
        it.iItem = i;
        ListView_GetItem(lv, &it);
        state = (it.lParam == param) ? (LVIS_SELECTED | LVIS_FOCUSED) : 0;
        ListView_SetItemState(lv, i, state, LVIS_SELECTED | LVIS_FOCUSED);
        if (it.lParam == param)
            ListView_EnsureVisible(lv, i, FALSE);
    }
    end_update();
}

static BOOL CALLBACK set_font_cb(HWND hwnd, LPARAM lp) {
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)lp, TRUE);
    return TRUE;
}

static void set_edit_int(HWND ed, int value) {
    wchar_t buf[16];
    StringCchPrintfW(buf, 16, L"%d", value);
    SetWindowTextW(ed, buf);
}

static int edit_int(HWND ed, int lo, int hi, int *clamped) {
    wchar_t buf[32];
    int value;
    GetWindowTextW(ed, buf, 32);
    value = _wtoi(buf);
    if (value < lo) {
        value = lo;
        if (clamped) *clamped = 1;
    }
    if (value > hi) {
        value = hi;
        if (clamped) *clamped = 1;
    }
    return value;
}

static void read_form(int *faction, int *level, int stats[STAT_COUNT], int *clamped) {
    int i;
    if (clamped) *clamped = 0;
    *faction = edit_int(g.faction, 0, 255, clamped);
    *level = edit_int(g.level, 0, MAX_TOTAL_LEVEL, clamped);
    for (i = 0; i < STAT_COUNT; i++)
        stats[i] = edit_int(g.stat[i], 0, 255, clamped);
}

static void sync_buttons(void) {
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
    if (index == 0) return record_at(&g.save, 0) != NULL;
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
    ok = g.save.data && index >= 0 && record_at(&g.save, index) && body_at(&g.save, index, &b);
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
    if (!record_at(&g.save, index) || !body_at(&g.save, index, &b)) return;
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

static void fill_people(void) {
    int keep = g.person;
    int i, row = 0;
    if (!g.people) return;
    begin_update();
    SendMessageW(g.people, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g.people);
    if (g.save.data) {
        if (want_person(0)) add_person_row(row++, 0);
        for (i = 1; i <= SAILOR_COUNT; i++) {
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

static unsigned hero_level(Save *s) {
    Body b;
    if (!body_hero(s, &b)) return 0;
    return total_level(b.exp0, b.exp1);
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

static void refresh_slots(void) {
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

static int editor_differs(void) {
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

static void commit_editor(int *clamped_out) {
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

static int save_current(void) {
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

static int confirm_discard_or_save(void) {
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
    set_status(status);
    return 1;
}

/* The slot list keeps the open file's level in memory. Refresh has to
 * read that file again, or recruits saved by the game never show up. */
static void on_refresh(void) {
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
    set_status(status);
}

static void on_slot_clicked(int slot) {
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

static void on_person_clicked(int index) {
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

static void on_apply(void) {
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

static void on_batch(void) {
    int stats[STAT_COUNT];
    int faction, level, clamped = 0;
    int i, n = 0;
    wchar_t text[320];
    if (!g.save.data || g.person < 0) return;
    read_form(&faction, &level, stats, &clamped);
    (void)faction;
    for (i = 0; i <= SAILOR_COUNT; i++) {
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
    for (i = 0; i <= SAILOR_COUNT; i++) {
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

static void on_open(void) {
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

static void on_filter(int recruited_only) {
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

static void startup_load(void) {
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

/* Game-exe ceilings. The save file does not store swordsmanship, life, or
 * trade; the exe computes them and clamps these immediates.
 *   one track level = isqrt(xp/30), then min(level, track cap). Two sites.
 *   xp written on gain is min(xp, 30*track^2). Four sites. Track max is 255:
 *   the integer square root shifts by 16 and overflows past that.
 *   base attribute growth is one byte, min(value, base cap). One site.
 *   stat settlement min(value, stat cap) at increase, assign, and result.
 *   skill = (statA + statB) * (min(total level, skill cap) + 100) / 150.
 *   port commerce/military min(value, port cap), stored uint16 at +0x1e,
 *   later loaded with movzx. One site. Sea-power 9999 is a different
 *   immediate (mov r8d, 9999) and is left alone. */
static const wchar_t *const kGameExe[4] = {
    L"DK4HD_sc.exe", L"DK4HD_tc.exe", L"DK4HD_jp.exe", L"DK4HD_kr.exe"
};
static const wchar_t *const kGameExeName[4] = {
    L"简体", L"繁体", L"日文", L"韩文"
};

typedef struct CapValues {
    uint32_t track;
    uint32_t base;
    uint32_t stat;
    uint32_t skill;
    uint32_t port;
} CapValues;

typedef struct CapWin {
    HWND hwnd;
    HWND path;
    HWND track;
    HWND base;
    HWND stat;
    HWND skill;
    HWND port;
    HWND derived;
    HWND status;
    int ready;
    wchar_t dir[MAX_PATH];
} CapWin;

static CapWin capw;

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

static void parent_dir(const wchar_t *file, wchar_t *dir, int dirn) {
    wchar_t tmp[MAX_PATH];
    wchar_t *slash;
    dir[0] = 0;
    StringCchCopyW(tmp, MAX_PATH, file);
    slash = wcsrchr(tmp, L'\\');
    if (!slash) return;
    *slash = 0;
    StringCchCopyW(dir, (size_t)dirn, tmp);
}

static int dir_has_game(const wchar_t *dir) {
    int i;
    if (!dir || !dir[0]) return 0;
    for (i = 0; i < 4; i++) {
        wchar_t path[MAX_PATH];
        StringCchPrintfW(path, MAX_PATH, L"%s\\%s", dir, kGameExe[i]);
        if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) return 1;
    }
    return 0;
}

static void remember_game_dir(const wchar_t *dir) {
    wchar_t ini[MAX_PATH];
    exe_ini_path(ini, MAX_PATH);
    if (ini[0] && dir && dir[0])
        WritePrivateProfileStringW(L"game", L"dir", dir, ini);
}

static void find_game_dir(wchar_t *dir, int dirn) {
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

static int collect_fixed(const uint8_t *data, size_t n,
                         const uint8_t *pre, size_t pn,
                         const uint8_t *post, size_t qn,
                         int *at, uint32_t *val, int capn) {
    size_t i;
    int count = 0;
    if (pn + 4 + qn > n) return 0;
    for (i = 0; i + pn + 4 + qn <= n; i++) {
        if (memcmp(data + i, pre, pn) != 0) continue;
        if (memcmp(data + i + pn + 4, post, qn) != 0) continue;
        if (count < capn) {
            at[count] = (int)(i + pn);
            val[count] = ru32(data + i + pn);
        }
        count++;
    }
    return count;
}

/* 03 84 24 disp32  41 B8 imm32  8B D0 33 C9 */
static int collect_xp(const uint8_t *data, size_t n, int *at, uint32_t *val, int capn) {
    size_t i;
    int count = 0;
    for (i = 0; i + 17 <= n; i++) {
        if (data[i] != 0x03 || data[i + 1] != 0x84 || data[i + 2] != 0x24) continue;
        if (data[i + 7] != 0x41 || data[i + 8] != 0xB8) continue;
        if (data[i + 13] != 0x8B || data[i + 14] != 0xD0) continue;
        if (data[i + 15] != 0x33 || data[i + 16] != 0xC9) continue;
        if (count < capn) {
            at[count] = (int)(i + 9);
            val[count] = ru32(data + i + 9);
        }
        count++;
    }
    return count;
}

/* 81 7C 24 20 imm32  7E 08  C7 44 24 20 imm32 */
static int collect_skill(const uint8_t *data, size_t n, int *at, uint32_t *a, uint32_t *b, int capn) {
    size_t i;
    int count = 0;
    static const uint8_t mid[6] = {0x7E, 0x08, 0xC7, 0x44, 0x24, 0x20};
    for (i = 0; i + 18 <= n; i++) {
        if (data[i] != 0x81 || data[i + 1] != 0x7C || data[i + 2] != 0x24 || data[i + 3] != 0x20)
            continue;
        if (memcmp(data + i + 8, mid, 6) != 0) continue;
        if (count < capn) {
            at[count] = (int)(i + 4);
            a[count] = ru32(data + i + 4);
            b[count] = ru32(data + i + 14);
        }
        count++;
    }
    return count;
}

static int same_u32(const uint32_t *v, int n) {
    int i;
    for (i = 1; i < n; i++) if (v[i] != v[0]) return 0;
    return 1;
}

static int read_cap_values(const uint8_t *data, size_t n, CapValues *v, int *xp_off, wchar_t *err, int errn) {
    static const uint8_t pre_lv[] = {0x89, 0x44, 0x24, 0x20, 0x41, 0xB8};
    static const uint8_t post_lv[] = {0x8B, 0x54, 0x24, 0x20, 0x33, 0xC9};
    static const uint8_t pre_inc[] = {0x0F, 0xB7, 0x44, 0x41, 0x4A, 0x03, 0x44, 0x24, 0x60, 0xBA};
    static const uint8_t post_inc[] = {0x8B, 0xC8};
    static const uint8_t pre_asg[] = {0x41, 0xB8};
    static const uint8_t post_asg[] = {0x8B, 0x54, 0x24, 0x24, 0xB9, 0x19, 0x00, 0x00, 0x00};
    static const uint8_t post_res[] = {0x8B, 0x54, 0x24, 0x30, 0x33, 0xC9};
    static const uint8_t pre_base[] = {
        0x0F, 0xB6, 0x44, 0x01, 0x0F, 0x03, 0x44, 0x24, 0x70, 0x41, 0xB8
    };
    static const uint8_t post_base[] = {0x8B, 0xD0, 0x33, 0xC9};
    /* jmp $-213; mov edx,[rsp+0x54]; mov ecx,imm32; call min */
    static const uint8_t pre_port[] = {
        0xE9, 0x2B, 0xFF, 0xFF, 0xFF, 0x8B, 0x54, 0x24, 0x54, 0xB9
    };
    static const uint8_t post_port[] = {0xE8};
    int at[8], sat[4];
    uint32_t val[8], sa[4], sb[4];
    int c, xs;
    uint32_t stats[3];
    memset(v, 0, sizeof *v);
    if (xp_off) *xp_off = 0;
    c = collect_fixed(data, n, pre_lv, sizeof pre_lv, post_lv, sizeof post_lv, at, val, 8);
    if (c != 2 || !same_u32(val, 2)) {
        StringCchCopyW(err, (size_t)errn, L"找不到单段等级上限，或两处数值不一样。");
        return 0;
    }
    v->track = val[0];
    c = collect_xp(data, n, at, val, 8);
    if (c != 4 || !same_u32(val, 4)) {
        StringCchCopyW(err, (size_t)errn, L"找不到经验上限，或四处数值不一样。");
        return 0;
    }
    if (xp_off && val[0] != 30u * v->track * v->track) *xp_off = 1;
    c = collect_fixed(data, n, pre_inc, sizeof pre_inc, post_inc, sizeof post_inc, at, val, 8);
    if (c != 1) {
        StringCchCopyW(err, (size_t)errn, L"找不到属性增加上限。");
        return 0;
    }
    stats[0] = val[0];
    c = collect_fixed(data, n, pre_asg, sizeof pre_asg, post_asg, sizeof post_asg, at, val, 8);
    if (c != 1) {
        StringCchCopyW(err, (size_t)errn, L"找不到属性分配上限。");
        return 0;
    }
    stats[1] = val[0];
    c = collect_fixed(data, n, pre_asg, sizeof pre_asg, post_res, sizeof post_res, at, val, 8);
    if (c != 1) {
        StringCchCopyW(err, (size_t)errn, L"找不到属性结算上限。");
        return 0;
    }
    stats[2] = val[0];
    if (!same_u32(stats, 3)) {
        StringCchCopyW(err, (size_t)errn, L"三处属性结算上限不一致。");
        return 0;
    }
    v->stat = stats[0];
    c = collect_fixed(data, n, pre_base, sizeof pre_base, post_base, sizeof post_base, at, val, 8);
    if (c != 1) {
        StringCchCopyW(err, (size_t)errn, L"找不到基础属性成长上限。");
        return 0;
    }
    v->base = val[0];
    xs = collect_skill(data, n, sat, sa, sb, 4);
    if (xs != 1 || sa[0] != sb[0]) {
        StringCchCopyW(err, (size_t)errn, L"找不到技能等级封顶，或两处数值不一样。");
        return 0;
    }
    v->skill = sa[0];
    c = collect_fixed(data, n, pre_port, sizeof pre_port, post_port, sizeof post_port, at, val, 8);
    if (c != 1) {
        StringCchCopyW(err, (size_t)errn, L"找不到港口商业/军事上限。");
        return 0;
    }
    v->port = val[0];
    return 1;
}

static int patch_cap_buffer(uint8_t *data, size_t n, const CapValues *v, wchar_t *err, int errn) {
    static const uint8_t pre_lv[] = {0x89, 0x44, 0x24, 0x20, 0x41, 0xB8};
    static const uint8_t post_lv[] = {0x8B, 0x54, 0x24, 0x20, 0x33, 0xC9};
    static const uint8_t pre_inc[] = {0x0F, 0xB7, 0x44, 0x41, 0x4A, 0x03, 0x44, 0x24, 0x60, 0xBA};
    static const uint8_t post_inc[] = {0x8B, 0xC8};
    static const uint8_t pre_asg[] = {0x41, 0xB8};
    static const uint8_t post_asg[] = {0x8B, 0x54, 0x24, 0x24, 0xB9, 0x19, 0x00, 0x00, 0x00};
    static const uint8_t post_res[] = {0x8B, 0x54, 0x24, 0x30, 0x33, 0xC9};
    static const uint8_t pre_base[] = {
        0x0F, 0xB6, 0x44, 0x01, 0x0F, 0x03, 0x44, 0x24, 0x70, 0x41, 0xB8
    };
    static const uint8_t post_base[] = {0x8B, 0xD0, 0x33, 0xC9};
    static const uint8_t pre_port[] = {
        0xE9, 0x2B, 0xFF, 0xFF, 0xFF, 0x8B, 0x54, 0x24, 0x54, 0xB9
    };
    static const uint8_t post_port[] = {0xE8};
    int at[8], sat[4];
    uint32_t val[8], sa[4], sb[4];
    int c, i;
    uint32_t xp;
    CapValues cur;
    if (!read_cap_values(data, n, &cur, NULL, err, errn)) return 0;
    (void)cur;
    c = collect_fixed(data, n, pre_lv, sizeof pre_lv, post_lv, sizeof post_lv, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->track);
    xp = 30u * v->track * v->track;
    c = collect_xp(data, n, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], xp);
    c = collect_fixed(data, n, pre_inc, sizeof pre_inc, post_inc, sizeof post_inc, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->stat);
    c = collect_fixed(data, n, pre_asg, sizeof pre_asg, post_asg, sizeof post_asg, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->stat);
    c = collect_fixed(data, n, pre_asg, sizeof pre_asg, post_res, sizeof post_res, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->stat);
    c = collect_fixed(data, n, pre_base, sizeof pre_base, post_base, sizeof post_base, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->base);
    c = collect_skill(data, n, sat, sa, sb, 4);
    for (i = 0; i < c; i++) {
        wu32(data + sat[i], v->skill);
        wu32(data + sat[i] + 10, v->skill);
    }
    c = collect_fixed(data, n, pre_port, sizeof pre_port, post_port, sizeof post_port, at, val, 8);
    for (i = 0; i < c; i++) wu32(data + at[i], v->port);
    if (!read_cap_values(data, n, &cur, NULL, err, errn)) return 0;
    if (cur.track != v->track || cur.base != v->base || cur.stat != v->stat ||
        cur.skill != v->skill || cur.port != v->port) {
        StringCchCopyW(err, (size_t)errn, L"写入后的数值和输入不一致。");
        return 0;
    }
    return 1;
}

static int load_exe_bytes(const wchar_t *path, uint8_t **out, size_t *outn, wchar_t *err, int errn) {
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

static int save_exe_bytes(const wchar_t *path, const uint8_t *data, size_t n, wchar_t *err, int errn) {
    wchar_t bak[MAX_PATH], tmp[MAX_PATH];
    HANDLE h;
    DWORD wrote, done;
    StringCchPrintfW(bak, MAX_PATH, L"%s.bak", path);
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
    if (GetFileAttributesW(bak) == INVALID_FILE_ATTRIBUTES) {
        if (!CopyFileW(path, bak, TRUE)) {
            DeleteFileW(tmp);
            StringCchCopyW(err, (size_t)errn, L"备份失败，没有改主程序。");
            return 0;
        }
    }
    if (!MoveFileExW(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(tmp);
        StringCchCopyW(err, (size_t)errn, L"替换主程序失败。请先完全退出游戏。");
        return 0;
    }
    return 1;
}

static int read_game_caps(const wchar_t *dir, CapValues *v, wchar_t *status, int statusn) {
    int i, found = 0, xp_off = 0, first_i = 0;
    CapValues first, cur;
    wchar_t path[MAX_PATH], err[240], names[120];
    memset(&first, 0, sizeof first);
    names[0] = 0;
    for (i = 0; i < 4; i++) {
        uint8_t *data = NULL;
        size_t n = 0;
        int bad = 0;
        StringCchPrintfW(path, MAX_PATH, L"%s\\%s", dir, kGameExe[i]);
        if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) continue;
        if (!load_exe_bytes(path, &data, &n, err, 240) ||
            !read_cap_values(data, n, &cur, &bad, err, 240)) {
            free(data);
            StringCchPrintfW(status, (size_t)statusn, L"%s：%s", kGameExeName[i], err);
            return 0;
        }
        free(data);
        if (!found) {
            first = cur;
            first_i = i;
        } else if (cur.track != first.track || cur.base != first.base ||
                 cur.stat != first.stat || cur.skill != first.skill ||
                 cur.port != first.port) {
            *v = first;
            StringCchPrintfW(status, (size_t)statusn,
                L"%s和%s的封顶不一致。下面是先读到的数值，写入后四个版本会变成一样。",
                kGameExeName[first_i], kGameExeName[i]);
            return 1;
        }
        if (bad) xp_off = 1;
        if (names[0]) StringCchCatW(names, 120, L"、");
        StringCchCatW(names, 120, kGameExeName[i]);
        found++;
    }
    if (!found) {
        StringCchCopyW(status, (size_t)statusn, L"这个目录里没有 DK4HD 主程序。");
        return 0;
    }
    *v = first;
    StringCchPrintfW(status, (size_t)statusn,
        xp_off
            ? L"已读取%s。经验上限和单段等级对不上，写入时会按 30×等级² 重写。"
            : L"已读取%s。",
        names);
    return 1;
}

static int write_game_caps(const wchar_t *dir, const CapValues *v, wchar_t *status, int statusn) {
    int i, found = 0;
    wchar_t path[MAX_PATH], err[240], names[120];
    names[0] = 0;
    for (i = 0; i < 4; i++) {
        uint8_t *data = NULL;
        size_t n = 0;
        StringCchPrintfW(path, MAX_PATH, L"%s\\%s", dir, kGameExe[i]);
        if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) continue;
        if (!load_exe_bytes(path, &data, &n, err, 240) ||
            !patch_cap_buffer(data, n, v, err, 240) ||
            !save_exe_bytes(path, data, n, err, 240)) {
            free(data);
            StringCchPrintfW(status, (size_t)statusn, L"%s没有写入。%s", kGameExeName[i], err);
            if (found)
                StringCchCatW(status, (size_t)statusn, L" 前面已经写入的版本还在。");
            return 0;
        }
        free(data);
        if (names[0]) StringCchCatW(names, 120, L"、");
        StringCchCatW(names, 120, kGameExeName[i]);
        found++;
    }
    if (!found) {
        StringCchCopyW(status, (size_t)statusn, L"这个目录里没有 DK4HD 主程序。");
        return 0;
    }
    StringCchPrintfW(status, (size_t)statusn,
        L"已写入%s。单段 %u，合计 %u，经验 %u，基础属性 %u，结算 %u，技能封顶 %u，港口 %u。",
        names, v->track, v->track * 2u, 30u * v->track * v->track, v->base, v->stat, v->skill, v->port);
    return 1;
}

static HWND cap_ctl(DWORD ex, const wchar_t *cls, const wchar_t *text, DWORD style, int id) {
    return CreateWindowExW(ex, cls, text, style | WS_CHILD | WS_VISIBLE,
        0, 0, 10, 10, capw.hwnd, (HMENU)(INT_PTR)id, g.inst, NULL);
}

static void cap_set_status(const wchar_t *text) {
    if (capw.status) SetWindowTextW(capw.status, text ? text : L"");
}

static void cap_refresh_derived(void) {
    wchar_t line[160];
    int track;
    unsigned xp;
    if (!capw.derived) return;
    track = edit_int(capw.track, 0, 255, NULL);
    xp = 30u * (unsigned)track * (unsigned)track;
    StringCchPrintfW(line, 160, L"两段合计 %d，经验上限 %u", track * 2, xp);
    SetWindowTextW(capw.derived, line);
}

static void cap_apply_values(const CapValues *v) {
    set_edit_int(capw.track, (int)v->track);
    set_edit_int(capw.base, (int)v->base);
    set_edit_int(capw.stat, (int)v->stat);
    set_edit_int(capw.skill, (int)v->skill);
    set_edit_int(capw.port, (int)v->port);
    cap_refresh_derived();
}

static int cap_read_form(CapValues *v, int *clamped) {
    if (clamped) *clamped = 0;
    v->track = (uint32_t)edit_int(capw.track, 1, 255, clamped);
    v->base = (uint32_t)edit_int(capw.base, 0, 255, clamped);
    v->stat = (uint32_t)edit_int(capw.stat, 1, 65535, clamped);
    v->skill = (uint32_t)edit_int(capw.skill, 0, 65535, clamped);
    v->port = (uint32_t)edit_int(capw.port, 1, 65535, clamped);
    return 1;
}

static void cap_read_clicked(void) {
    CapValues v;
    wchar_t status[360];
    if (!dir_has_game(capw.dir)) {
        cap_set_status(L"请先选择游戏主程序。");
        return;
    }
    if (!read_game_caps(capw.dir, &v, status, 360)) {
        cap_set_status(status);
        return;
    }
    cap_apply_values(&v);
    cap_set_status(status);
}

static void cap_write_clicked(void) {
    CapValues v;
    wchar_t ask[360], status[420];
    int clamped = 0;
    if (!dir_has_game(capw.dir)) {
        cap_set_status(L"请先选择游戏主程序。");
        return;
    }
    cap_read_form(&v, &clamped);
    cap_apply_values(&v);
    StringCchPrintfW(ask, 360,
        L"写入这个目录里的主程序？\n单段等级 %u（合计 %u，经验 %u）\n基础属性 %u\n属性结算 %u\n技能封顶 %u\n港口商业/军事 %u\n请先完全退出游戏。",
        v.track, v.track * 2u, 30u * v.track * v.track, v.base, v.stat, v.skill, v.port);
    if (MessageBoxW(capw.hwnd, ask, L"游戏上限", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;
    if (!write_game_caps(capw.dir, &v, status, 420)) {
        MessageBoxW(capw.hwnd, status, L"游戏上限", MB_ICONWARNING);
        cap_set_status(status);
        return;
    }
    if (clamped) StringCchCatW(status, 420, L" 超出范围的输入已按上下限处理。");
    cap_set_status(status);
    remember_game_dir(capw.dir);
}

static void cap_browse(void) {
    wchar_t file[MAX_PATH];
    OPENFILENAMEW ofn;
    file[0] = 0;
    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = capw.hwnd;
    ofn.lpstrFilter = L"主程序 (DK4HD_*.exe)\0DK4HD_*.exe\0可执行文件 (*.exe)\0*.exe\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = capw.dir[0] ? capw.dir : NULL;
    ofn.lpstrTitle = L"选择游戏主程序";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn)) return;
    parent_dir(file, capw.dir, MAX_PATH);
    SetWindowTextW(capw.path, capw.dir);
    remember_game_dir(capw.dir);
    cap_read_clicked();
}

static void cap_layout(void) {
    RECT rc;
    int m = S(16);
    int w, y, row, labW, editW, btnW;
    int gap = S(8);
    if (!capw.hwnd) return;
    GetClientRect(capw.hwnd, &rc);
    w = rc.right;
    row = S(28);
    labW = S(150);
    editW = S(88);
    btnW = S(128);
    y = m;
    MoveWindow(capw.path, m, y, w - m * 2 - btnW - gap, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_BROWSE), w - m - btnW, y, btnW, row, TRUE);
    y += row + S(14);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_TRACK), m, y, labW, row, TRUE);
    MoveWindow(capw.track, m + labW, y, editW, row, TRUE);
    MoveWindow(capw.derived, m + labW + editW + gap, y, w - (m + labW + editW + gap) - m, row, TRUE);
    y += row + gap;
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_BASE), m, y, labW, row, TRUE);
    MoveWindow(capw.base, m + labW, y, editW, row, TRUE);
    y += row + gap;
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_STAT), m, y, labW, row, TRUE);
    MoveWindow(capw.stat, m + labW, y, editW, row, TRUE);
    y += row + gap;
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_SKILL), m, y, labW, row, TRUE);
    MoveWindow(capw.skill, m + labW, y, editW, row, TRUE);
    y += row + gap;
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_LBL_PORT), m, y, labW, row, TRUE);
    MoveWindow(capw.port, m + labW, y, editW, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_UNLOCK_PORT), m + labW + editW + gap, y, btnW, row, TRUE);
    y += row + S(16);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_READ), m, y, btnW, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_WRITE), m + (btnW + gap), y, btnW, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_VANILLA), m + (btnW + gap) * 2, y, btnW, row, TRUE);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_UNLOCK), m + (btnW + gap) * 3, y, btnW, row, TRUE);
    y += row + S(12);
    MoveWindow(GetDlgItem(capw.hwnd, IDC_CAP_NOTE), m, y, w - m * 2, rc.bottom - y - m - row - gap, TRUE);
    MoveWindow(capw.status, m, rc.bottom - m - row, w - m * 2, row, TRUE);
}

static LRESULT CALLBACK cap_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        DWORD editStyle = WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT | WS_BORDER;
        capw.hwnd = hwnd;
        capw.path = cap_ctl(0, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_PATHELLIPSIS, IDC_CAP_PATH);
        cap_ctl(0, L"BUTTON", L"选择主程序", WS_TABSTOP, IDC_CAP_BROWSE);
        cap_ctl(0, L"STATIC", L"单段等级上限", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_TRACK);
        capw.track = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"255", editStyle, IDC_CAP_TRACK);
        capw.derived = cap_ctl(0, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_ENDELLIPSIS, IDC_CAP_DERIVED);
        cap_ctl(0, L"STATIC", L"基础属性上限", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_BASE);
        capw.base = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"255", editStyle, IDC_CAP_BASE);
        cap_ctl(0, L"STATIC", L"属性结算上限", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_STAT);
        capw.stat = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"65535", editStyle, IDC_CAP_STAT);
        cap_ctl(0, L"STATIC", L"技能等级封顶", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_SKILL);
        capw.skill = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"510", editStyle, IDC_CAP_SKILL);
        cap_ctl(0, L"STATIC", L"港口商业/军事", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_CAP_LBL_PORT);
        capw.port = cap_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"9999", editStyle, IDC_CAP_PORT);
        cap_ctl(0, L"BUTTON", L"解除港口上限", WS_TABSTOP, IDC_CAP_UNLOCK_PORT);
        cap_ctl(0, L"BUTTON", L"读取当前值", WS_TABSTOP, IDC_CAP_READ);
        cap_ctl(0, L"BUTTON", L"写入主程序", WS_TABSTOP | BS_DEFPUSHBUTTON, IDC_CAP_WRITE);
        cap_ctl(0, L"BUTTON", L"填入原版", WS_TABSTOP, IDC_CAP_VANILLA);
        cap_ctl(0, L"BUTTON", L"解除上限", WS_TABSTOP, IDC_CAP_UNLOCK);
        cap_ctl(WS_EX_CLIENTEDGE, L"EDIT",
            L"单段等级最高 255。再高时，游戏用 32 位计算平方根会溢出，等级会变成 0。经验上限按 30×单段等级² 自动写入。\r\n"
            L"基础属性存在一个字节里，最高 255。属性结算是升级、分配和结算三处共用的上限，原版 500，最高 65535。\r\n"
            L"技能按 (两项属性之和) × (不超过封顶的等级 + 100) ÷ 150 计算。封顶原版是 200。设成两段之和后，剑术、生命、交易会按真实等级上涨。\r\n"
            L"港口商业和军事存在 16 位无符号数里，原版 9999，最高 65535。点“解除港口上限”填入 65535。海力上限保持 9999。商业度会参与商品定价，提到很高以后物价会跟着变。\r\n"
            L"写入前请完全退出游戏。主程序旁边已经有的 .bak 不会覆盖。",
            ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, IDC_CAP_NOTE);
        capw.status = cap_ctl(0, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_ENDELLIPSIS, IDC_CAP_STATUS);
        SendMessageW(capw.track, EM_SETLIMITTEXT, 3, 0);
        SendMessageW(capw.base, EM_SETLIMITTEXT, 3, 0);
        SendMessageW(capw.stat, EM_SETLIMITTEXT, 5, 0);
        SendMessageW(capw.skill, EM_SETLIMITTEXT, 5, 0);
        SendMessageW(capw.port, EM_SETLIMITTEXT, 5, 0);
        EnumChildWindows(hwnd, set_font_cb, (LPARAM)g.font);
        SetWindowTextW(capw.path, capw.dir[0] ? capw.dir : L"未选择游戏目录");
        cap_refresh_derived();
        capw.ready = 1;
        if (dir_has_game(capw.dir)) cap_read_clicked();
        return 0;
    }
    case WM_SIZE:
        cap_layout();
        return 0;
    case WM_COMMAND:
        if (!capw.ready) return 0;
        if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_CAP_TRACK) {
            cap_refresh_derived();
            return 0;
        }
        switch (LOWORD(wParam)) {
        case IDC_CAP_BROWSE:
            cap_browse();
            break;
        case IDC_CAP_READ:
            cap_read_clicked();
            break;
        case IDC_CAP_WRITE:
            cap_write_clicked();
            break;
        case IDC_CAP_VANILLA: {
            CapValues v;
            v.track = 100;
            v.base = 100;
            v.stat = 500;
            v.skill = 200;
            v.port = 9999;
            cap_apply_values(&v);
            cap_set_status(L"已填入原版：单段 100，基础属性 100，结算 500，技能封顶 200，港口 9999。还没写入。");
            break;
        }
        case IDC_CAP_UNLOCK: {
            CapValues v;
            int port = edit_int(capw.port, 1, 65535, NULL);
            v.track = 255;
            v.base = 255;
            v.stat = 65535;
            v.skill = 510;
            v.port = (uint32_t)port;
            cap_apply_values(&v);
            cap_set_status(L"已填入解除值：单段 255，基础属性 255，结算 65535，技能封顶 510。还没写入。");
            break;
        }
        case IDC_CAP_UNLOCK_PORT:
            set_edit_int(capw.port, 65535);
            cap_set_status(L"已填入港口上限 65535。点“写入主程序”后才会改主程序。");
            break;
        default:
            break;
        }
        return 0;
    case DM_GETDEFID:
        return MAKELRESULT(IDC_CAP_WRITE, DC_HASDEFID);
    case WM_GETMINMAXINFO: {
        MINMAXINFO *mm = (MINMAXINFO *)lParam;
        mm->ptMinTrackSize.x = S(720);
        mm->ptMinTrackSize.y = S(500);
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        capw.hwnd = NULL;
        capw.ready = 0;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void open_cap_window(void) {
    WNDCLASSW wc;
    RECT rc;
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
    int x, y;
    if (capw.hwnd && IsWindow(capw.hwnd)) {
        ShowWindow(capw.hwnd, SW_SHOW);
        SetForegroundWindow(capw.hwnd);
        return;
    }
    if (!capw.dir[0]) find_game_dir(capw.dir, MAX_PATH);
    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = cap_proc;
    wc.hInstance = g.inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"DK4CapWindow";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassW(&wc);
    rc.left = 0;
    rc.top = 0;
    rc.right = S(760);
    rc.bottom = S(520);
    AdjustWindowRect(&rc, style, FALSE);
    x = CW_USEDEFAULT;
    y = CW_USEDEFAULT;
    if (g.hwnd) {
        RECT mainrc;
        GetWindowRect(g.hwnd, &mainrc);
        x = mainrc.left + S(36);
        y = mainrc.top + S(36);
    }
    CreateWindowExW(WS_EX_DLGMODALFRAME, L"DK4CapWindow", L"游戏上限",
        style, x, y, rc.right - rc.left, rc.bottom - rc.top,
        g.hwnd, NULL, g.inst, NULL);
    if (!capw.hwnd) return;
    ShowWindow(capw.hwnd, SW_SHOW);
    UpdateWindow(capw.hwnd);
}

static void layout(int cw, int ch) {
    int m = S(12);
    int gap = S(10);
    int top = S(28);
    int bottom = S(28);
    int leftW = S(270);
    int rightW = S(318);
    int header = S(24);
    int bodyY = m + top + gap;
    int bodyH = ch - bodyY - bottom - m;
    int rx, cx, centerW;
    int x, y, row, labW, editW, half;
    int i;
    HWND edits[8];
    if (bodyH < S(200)) bodyH = S(200);
    MoveWindow(g.pathLabel, m, m, cw - m * 2 - S(72) - S(96) - S(88) - gap * 3, top, TRUE);
    MoveWindow(g.refreshBtn, cw - m - S(88) - gap - S(96) - gap - S(72), m, S(72), top, TRUE);
    MoveWindow(g.gameCapBtn, cw - m - S(88) - gap - S(96), m, S(96), top, TRUE);
    MoveWindow(g.openBtn, cw - m - S(88), m, S(88), top, TRUE);

    MoveWindow(g.slotCaption, m, bodyY, leftW, header, TRUE);
    MoveWindow(g.slots, m, bodyY + header, leftW, bodyH - header, TRUE);

    rx = cw - m - rightW;
    cx = m + leftW + gap;
    centerW = rx - gap - cx;
    if (centerW < S(180)) centerW = S(180);
    MoveWindow(g.filterRec, cx, bodyY, S(88), header, TRUE);
    MoveWindow(g.filterAll, cx + S(96), bodyY, S(100), header, TRUE);
    MoveWindow(g.people, cx, bodyY + header, centerW, bodyH - header, TRUE);

    MoveWindow(g.editCaption, rx, bodyY, rightW, header, TRUE);
    x = rx;
    y = bodyY + header;
    row = S(26);
    labW = S(72);
    editW = S(96);
    MoveWindow(g.nameLabel, x, y, rightW, S(36), TRUE);
    y += S(38);
    MoveWindow(g.trackLabel, x, y, rightW, S(22), TRUE);
    y += S(26);
    edits[0] = g.faction;
    edits[1] = g.level;
    for (i = 0; i < STAT_COUNT; i++) edits[2 + i] = g.stat[i];
    for (i = 0; i < 8; i++) {
        MoveWindow(g.fieldLabel[i], x, y, labW, row, TRUE);
        MoveWindow(edits[i], x + labW, y, editW, row, TRUE);
        y += row + S(6);
    }
    MoveWindow(g.applyBtn, x, y, rightW, row, TRUE);
    y += row + S(6);
    half = (rightW - S(8)) / 2;
    MoveWindow(g.fillStatBtn, x, y, half, row, TRUE);
    MoveWindow(g.fillLevelBtn, x + half + S(8), y, half, row, TRUE);
    y += row + S(6);
    MoveWindow(g.batchBtn, x, y, rightW, row, TRUE);
    y += row + S(8);
    {
        int hintH = bodyY + bodyH - y;
        if (hintH < S(72)) hintH = S(72);
        MoveWindow(g.hint, x, y, rightW, hintH, TRUE);
    }

    MoveWindow(g.saveBtn, m, ch - m - bottom + S(2), S(148), bottom - S(2), TRUE);
    MoveWindow(g.status, m + S(156), ch - m - bottom + S(2), cw - m * 2 - S(156), bottom - S(2), TRUE);
}

static void create_controls(HWND hwnd) {
    const wchar_t *labels[8];
    labels[0] = L"所属";
    labels[1] = L"等级";
    int i;
    DWORD editStyle = WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT | WS_BORDER;
    DWORD listStyle = WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;
    g.hwnd = hwnd;
    g.font = CreateFontW(-MulDiv(15, g.dpi < 96 ? 96 : g.dpi, 96),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    g.pathLabel = add_ctl(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_PATHELLIPSIS, IDC_PATH);
    g.refreshBtn = add_ctl(0, L"BUTTON", L"刷新", WS_TABSTOP, IDC_REFRESH);
    g.gameCapBtn = add_ctl(0, L"BUTTON", L"游戏上限", WS_TABSTOP, IDC_GAME_CAP);
    g.openBtn = add_ctl(0, L"BUTTON", L"打开", WS_TABSTOP, IDC_OPEN);
    g.slotCaption = add_ctl(0, L"STATIC", L"存档栏", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_STATIC);
    g.slots = add_ctl(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", listStyle, IDC_SLOTS);
    g.filterRec = add_ctl(0, L"BUTTON", L"已招收", WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON, IDC_FILTER_REC);
    g.filterAll = add_ctl(0, L"BUTTON", L"全部名册", WS_TABSTOP | BS_AUTORADIOBUTTON, IDC_FILTER_ALL);
    g.people = add_ctl(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", listStyle | WS_GROUP, IDC_PEOPLE);
    g.editCaption = add_ctl(0, L"STATIC", L"编辑", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_STATIC);
    g.nameLabel = add_ctl(0, L"STATIC", L"未选择角色", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_ENDELLIPSIS, IDC_NAME);
    g.trackLabel = add_ctl(0, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS, IDC_TRACK);
    for (i = 0; i < STAT_COUNT; i++) labels[2 + i] = kStatName[i];
    for (i = 0; i < 8; i++) {
        g.fieldLabel[i] = add_ctl(0, L"STATIC", labels[i], SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, IDC_STATIC);
    }
    g.faction = add_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"", editStyle, IDC_FACTION);
    g.level = add_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"", editStyle, IDC_LEVEL);
    for (i = 0; i < STAT_COUNT; i++) {
        g.stat[i] = add_ctl(WS_EX_CLIENTEDGE, L"EDIT", L"", editStyle, IDC_STAT0 + i);
        SendMessageW(g.stat[i], EM_SETLIMITTEXT, 3, 0);
    }
    SendMessageW(g.faction, EM_SETLIMITTEXT, 3, 0);
    SendMessageW(g.level, EM_SETLIMITTEXT, 3, 0);
    g.applyBtn = add_ctl(0, L"BUTTON", L"应用到此人", WS_TABSTOP | BS_DEFPUSHBUTTON, IDC_APPLY);
    g.fillStatBtn = add_ctl(0, L"BUTTON", L"六维 255", WS_TABSTOP, IDC_FILL_STAT);
    g.fillLevelBtn = add_ctl(0, L"BUTTON", L"等级 510", WS_TABSTOP, IDC_FILL_LEVEL);
    g.batchBtn = add_ctl(0, L"BUTTON", L"应用到主角和已招收", WS_TABSTOP, IDC_BATCH);
    g.hint = add_ctl(0, L"STATIC",
        L"六维上限 255。等级是两段之和，每段最高 255，合计 510。改等级时按当前两段的比例分配。健康、心情和技能没有单独字段，由游戏按六维和等级计算。改动先留在内存里。点“保存到原文件”或按 Ctrl+S 才写回，第一次保存会生成 .bak。",
        SS_LEFT | SS_NOPREFIX, IDC_HINT);
    g.saveBtn = add_ctl(0, L"BUTTON", L"保存到原文件", WS_TABSTOP, IDC_SAVE);
    g.status = add_ctl(0, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE | SS_ENDELLIPSIS, IDC_STATUS);

    lv_style(g.slots);
    lv_style(g.people);
    lv_col(g.slots, 0, L"栏", S(40));
    lv_col(g.slots, 1, L"主角", S(150));
    lv_col(g.slots, 2, L"等级", S(52));
    lv_col(g.people, 0, L"编号", S(48));
    lv_col(g.people, 1, L"姓名", S(150));
    lv_col(g.people, 2, L"等级", S(52));
    lv_col(g.people, 3, L"所属", S(48));
    lv_col(g.people, 4, L"状态", S(64));
    lv_col(g.people, 5, L"六维", S(150));

    EnumChildWindows(hwnd, set_font_cb, (LPARAM)g.font);
    {
        HWND hdr = ListView_GetHeader(g.slots);
        if (hdr) SendMessageW(hdr, WM_SETFONT, (WPARAM)g.font, TRUE);
        hdr = ListView_GetHeader(g.people);
        if (hdr) SendMessageW(hdr, WM_SETFONT, (WPARAM)g.font, TRUE);
    }
    begin_update();
    CheckRadioButton(hwnd, IDC_FILTER_REC, IDC_FILTER_ALL, IDC_FILTER_REC);
    end_update();
    g.recruited_only = 1;
    sync_buttons();
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        create_controls(hwnd);
        return 0;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
            layout(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_GETMINMAXINFO: {
        MINMAXINFO *mm = (MINMAXINFO *)lParam;
        mm->ptMinTrackSize.x = S(1020);
        mm->ptMinTrackSize.y = S(720);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (g.suspend && (id == IDC_FILTER_REC || id == IDC_FILTER_ALL))
            return 0;
        switch (id) {
        case IDC_REFRESH:
            on_refresh();
            break;
        case IDC_GAME_CAP:
            open_cap_window();
            break;
        case IDC_OPEN:
            on_open();
            break;
        case IDC_FILTER_REC:
            on_filter(1);
            break;
        case IDC_FILTER_ALL:
            on_filter(0);
            break;
        case IDC_APPLY:
            on_apply();
            break;
        case IDC_FILL_STAT:
            if (g.person >= 0) {
                int i;
                for (i = 0; i < STAT_COUNT; i++) set_edit_int(g.stat[i], 255);
                set_status(L"六维已填入 255。点“应用到此人”后写入内存。");
            }
            break;
        case IDC_FILL_LEVEL:
            if (g.person >= 0) {
                set_edit_int(g.level, MAX_TOTAL_LEVEL);
                set_status(L"等级已填入 510。点“应用到此人”后写入内存。");
            }
            break;
        case IDC_BATCH:
            on_batch();
            break;
        case IDC_SAVE:
            save_current();
            break;
        default:
            break;
        }
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR *hdr = (NMHDR *)lParam;
        NMLISTVIEW *nm;
        if (g.suspend || hdr->code != LVN_ITEMCHANGED) break;
        nm = (NMLISTVIEW *)lParam;
        if (!(nm->uChanged & LVIF_STATE)) break;
        if ((nm->uNewState & LVIS_SELECTED) == 0 || (nm->uOldState & LVIS_SELECTED) != 0)
            break;
        if (hdr->hwndFrom == g.slots) on_slot_clicked((int)nm->lParam);
        else if (hdr->hwndFrom == g.people) on_person_clicked((int)nm->lParam);
        return 0;
    }
    case WM_APP_REFRESH:
        fill_people();
        refresh_slots();
        set_status(L"已写入内存。还没保存到文件。");
        return 0;
    case DM_GETDEFID:
        return MAKELRESULT(IDC_APPLY, DC_HASDEFID);
    case WM_CLOSE:
        if (!confirm_discard_or_save()) return 0;
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        free_save(&g.save);
        if (g.font) DeleteObject(g.font);
        if (g.accel) DestroyAcceleratorTable(g.accel);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show) {
    WNDCLASSW wc;
    RECT rc;
    HWND hwnd;
    MSG msg;
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    INITCOMMONCONTROLSEX icc;
    ACCEL acc;
    (void)hPrev;
    (void)cmd;
    (void)show;

    SetProcessDPIAware();
    {
        HDC hdc = GetDC(NULL);
        g.dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSX) : 96;
        if (hdc) ReleaseDC(NULL, hdc);
    }
    if (g.dpi < 96) g.dpi = 96;
    g.inst = hInst;
    g.slot = -1;
    g.person = -1;
    g.recruited_only = 1;
    find_save_root(g.root, MAX_PATH);

    icc.dwSize = sizeof icc;
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"DK4SaveEditor";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassW(&wc)) return 1;

    rc.left = 0;
    rc.top = 0;
    rc.right = S(1100);
    rc.bottom = S(720);
    AdjustWindowRect(&rc, style, FALSE);
    hwnd = CreateWindowW(L"DK4SaveEditor", L"大航海时代4 HD 存档修改器",
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInst, NULL);
    if (!hwnd) return 1;

    acc.fVirt = FCONTROL | FVIRTKEY;
    acc.key = 'S';
    acc.cmd = IDC_SAVE;
    g.accel = CreateAcceleratorTableW(&acc, 1);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    startup_load();

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (capw.hwnd && (msg.hwnd == capw.hwnd || IsChild(capw.hwnd, msg.hwnd))) {
            if (IsDialogMessageW(capw.hwnd, &msg))
                continue;
        } else if (g.accel && TranslateAcceleratorW(g.hwnd, g.accel, &msg)) {
            continue;
        }
        if (IsDialogMessageW(g.hwnd, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
