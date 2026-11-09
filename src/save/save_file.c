/* 存档读取与写入，写入时保持校验和有效。 */

#include "common/bytes.h"
#include "common/backup.h"
#include "save/save.h"

void free_save(Save *s) {
    free(s->data);
    free(s->orig);
    s->data = NULL;
    s->orig = NULL;
    s->size = 0;
    s->dirty = 0;
}

int load_save(Save *s, const wchar_t *path, wchar_t *err, int errn) {
    FILE *fp;
    long sz;
    uint8_t *data, *orig;
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
    if (sz < (long)(HERO_BASE + HERO_SIZE) || sz > 2L * 1024L * 1024L) {
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
    orig = (uint8_t *)malloc((size_t)sz);
    if (!orig) {
        free(data);
        if (err && errn > 0) StringCchCopyW(err, (size_t)errn, L"内存不足。");
        return 0;
    }
    memcpy(orig, data, (size_t)sz);
    free(s->data);
    free(s->orig);
    s->data = data;
    s->orig = orig;
    s->size = (size_t)sz;
    s->dirty = 0;
    StringCchCopyW(s->path, MAX_PATH, path);
    return 1;
}

/* off 所在正文字段的宽度（只认本工具会写的区域），*start 返回字段起点。
 * 0 = 不认识的位置。 */
static int field_at(size_t off, size_t *start) {
    size_t rec, rel;
    if (off >= PERSON_BASE && off < SHORT_BASE) {
        rel = (off - PERSON_BASE) % PERSON_STRIDE;
    } else if (off >= HERO_BASE && off < HERO_BASE + PERSON_STRIDE) {
        rel = off - HERO_BASE;
    } else if (off >= SHORT_BASE && off < HERO_BASE) {
        *start = off;
        return 1;
    } else {
        return 0;
    }
    rec = off - rel;
    if (rel >= OFF_P_EXP0 && rel < OFF_P_EXP0 + 4) { *start = rec + OFF_P_EXP0; return 4; }
    if (rel >= OFF_P_EXP1 && rel < OFF_P_EXP1 + 4) { *start = rec + OFF_P_EXP1; return 4; }
    if (rel >= OFF_P_HP && rel < OFF_P_HP + 2) { *start = rec + OFF_P_HP; return 2; }
    *start = off;
    return 1;
}

static uint32_t field_value(const uint8_t *p, int width) {
    if (width == 4) return ru32(p);
    if (width == 2) return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
    return p[0];
}

/* 游戏把正文每个字段（u8/u16 零扩展，u32 整值，模 2^32）累加到 +0x18。
 * 这里累加每个改动字段的差值；返回 0 表示有改动落在不认识的位置。 */
static int checksum_delta(const Save *s, uint32_t *delta, size_t *bad) {
    size_t i = SAVE_HEADER, start;
    uint32_t d = 0;
    int width;
    while (i < s->size) {
        if (s->data[i] == s->orig[i]) {
            i++;
            continue;
        }
        width = field_at(i, &start);
        if (!width) {
            *bad = i;
            return 0;
        }
        d += field_value(s->data + start, width) - field_value(s->orig + start, width);
        i = start + (size_t)width;
    }
    *delta = d;
    return 1;
}

int write_save(Save *s, wchar_t *msg, int msgn) {
    wchar_t bak[MAX_PATH];
    wchar_t extra[160];
    FILE *fp;
    uint32_t stored = 0, delta = 0;
    size_t bad = 0;
    int copied;
    if (!s->data) {
        StringCchCopyW(msg, (size_t)msgn, L"没有打开存档。");
        return 0;
    }
    if (!checksum_delta(s, &delta, &bad)) {
        StringCchPrintfW(msg, (size_t)msgn,
            L"偏移 0x%X 的改动不在已知字段里，算不出校验和，没有写入。", (unsigned)bad);
        return 0;
    }
    if (s->size >= 0x18) stored = ru32(s->data + 0x14);
    copied = backup_before_write(s->path, bak, MAX_PATH);
    if (!copied) {
        StringCchCopyW(msg, (size_t)msgn, L"备份失败，没有写入。");
        return 0;
    }
    fp = _wfopen(s->path, L"wb");
    if (!fp) {
        StringCchCopyW(msg, (size_t)msgn, L"无法写入。请先关掉游戏。");
        return 0;
    }
    wu32(s->data + OFF_CHECKSUM, ru32(s->data + OFF_CHECKSUM) + delta);
    if (fwrite(s->data, 1, s->size, fp) != s->size) {
        fclose(fp);
        wu32(s->data + OFF_CHECKSUM, ru32(s->data + OFF_CHECKSUM) - delta);
        StringCchCopyW(msg, (size_t)msgn, L"写入不完整。");
        return 0;
    }
    fclose(fp);
    memcpy(s->orig, s->data, s->size);
    s->dirty = 0;
    if (copied == 1)
        StringCchPrintfW(msg, (size_t)msgn, L"已写入。原文件备份为 %s", file_part(bak));
    else if (copied == 2)
        StringCchPrintfW(msg, (size_t)msgn, L"已写入。写入前的文件另存为 %s", file_part(bak));
    else
        StringCchCopyW(msg, (size_t)msgn, L"已写入。");
    if (stored != (uint32_t)s->size) {
        StringCchPrintfW(extra, 160, L" 长度字段是 %u，文件是 %u，未改长度字段。",
            stored, (unsigned)s->size);
        StringCchCatW(msg, (size_t)msgn, extra);
    }
    return 1;
}

/* 人物 87..234 只有势力字节和一个只增不减的字节。势力大于 0x1A（无）
 * 说明被旧版工具写坏过。在 status 后追加提示，返回坏掉的条数。 */
int warn_short_damage(Save *s, wchar_t *status, int statusn) {
    int id, bad = 0;
    wchar_t extra[200];
    if (!s->data || s->size < (size_t)HERO_BASE) return 0;
    for (id = FULL_PERSON_COUNT; id < PERSON_COUNT; id++) {
        uint8_t f = s->data[SHORT_BASE + (size_t)(id - FULL_PERSON_COUNT) * SHORT_STRIDE];
        if (f > FACTION_NONE) bad++;
    }
    if (bad > 0) {
        StringCchPrintfW(extra, 200,
            L" 注意：有 %d 个配角的势力值越界，像是旧版本工具写坏的。可用同目录 .bak 里的原值恢复。", bad);
        StringCchCatW(status, (size_t)statusn, extra);
    }
    return bad;
}
