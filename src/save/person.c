/* 已读入存档中的人物记录：六维、两段经验、势力、主角镜像、显示名。 */

#include "common/bytes.h"
#include "save/save.h"

/* 主角的三个 17 字节名字字段。 */
static uint8_t *hero_names(Save *s) {
    if (!s->data || s->size < (size_t)HERO_BASE + HERO_SIZE) return NULL;
    return s->data + HERO_NAME;
}

static unsigned level_of_exp(uint32_t xp) {
    uint32_t q, r;
    if (xp < 30) return 0;
    q = xp / 30;
    r = 0;
    while (r < 400 && (r + 1) * (r + 1) <= q) r++;
    return (unsigned)r;
}

uint32_t exp_of_level(unsigned level) {
    if (level > MAX_TRACK_LEVEL) level = MAX_TRACK_LEVEL;
    return 30u * level * level;
}

unsigned track_level(uint32_t xp) {
    unsigned lv = level_of_exp(xp);
    if (lv > MAX_TRACK_LEVEL || xp > exp_of_level(MAX_TRACK_LEVEL))
        return MAX_TRACK_LEVEL;
    return lv;
}

static void body_from(uint8_t *rec, Body *b) {
    b->faction = rec + OFF_P_FACTION;
    b->stat = rec + OFF_P_STAT;
    b->exp0 = rec + OFF_P_EXP0;
    b->exp1 = rec + OFF_P_EXP1;
}

/* 只支持人物 id 0..86，其余人物的六维和经验游戏不存档。 */
static int body_roster(Save *s, int person, Body *b) {
    if (!s->data || person < 0 || person >= FULL_PERSON_COUNT) return 0;
    if (s->size < (size_t)HERO_BASE + HERO_SIZE) return 0;
    body_from(s->data + PERSON_BASE + (size_t)person * PERSON_STRIDE, b);
    return 1;
}

static int body_hero(Save *s, Body *b) {
    if (!s->data || s->size < (size_t)HERO_BASE + HERO_SIZE) return 0;
    body_from(s->data + HERO_BASE, b);
    return 1;
}

/* index 0 是主角记录，1..87 对应人物 id index-1。 */
int body_at(Save *s, int index, Body *b) {
    if (index == 0) return body_hero(s, b);
    return body_roster(s, index - 1, b);
}

int person_exists(Save *s, int index) {
    Body b;
    return body_at(s, index, &b);
}

static int protagonist_id(Save *s) {
    uint32_t id;
    if (!s->data || s->size < (size_t)OFF_PROT_ID + 4) return -1;
    id = ru32(s->data + OFF_PROT_ID);
    if (id >= FULL_PERSON_COUNT) return -1;
    return (int)id;
}

int is_protagonist_card(Save *s, int index) {
    int id = protagonist_id(s);
    return index > 0 && id == index - 1;
}

int is_present(Save *s, int index) {
    Body b;
    int i;
    unsigned sum = 0;
    if (index <= 0 || index > FULL_PERSON_COUNT) return 0;
    if (!body_at(s, index, &b)) return 0;
    for (i = 0; i < STAT_COUNT; i++) sum += b.stat[i];
    return sum > 0;
}

int is_recruited(Save *s, int index) {
    Body player, mate;
    if (!is_present(s, index)) return 0;
    if (!body_hero(s, &player) || !body_at(s, index, &mate)) return 0;
    return *mate.faction == *player.faction;
}

void read_stats(const uint8_t *stat, int out[STAT_COUNT]) {
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

unsigned total_level(const uint8_t *exp0, const uint8_t *exp1) {
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

/* 写入状态画面上的字段。主角在名册里还有一行，同步写成一样，
 * 这样从哪一行改都会作用到游戏里的主角。 */
void apply_person(Save *s, int index, int faction, const int stats[STAT_COUNT],
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

void person_name(Save *s, int index, wchar_t *dst, int dstn) {
    uint8_t *rec = hero_names(s);
    wchar_t a[64], b[64], c[64];
    if (!rec || !person_exists(s, index)) {
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
    if (index >= 1 && index <= FULL_PERSON_COUNT)
        StringCchCopyW(dst, (size_t)dstn, kSailorName[index - 1]);
    else
        StringCchPrintfW(dst, (size_t)dstn, L"记录%d", index);
}

unsigned hero_level(Save *s) {
    Body b;
    if (!body_hero(s, &b)) return 0;
    return total_level(b.exp0, b.exp1);
}
