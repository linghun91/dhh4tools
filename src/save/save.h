/* 存档：格式定义、人物记录、读取、校验和、写入。
 *   save_file.c     读取、校验和增量、写入、旧版损坏检查
 *   person.c        人物记录：六维、等级、势力、名字
 *   sailor_names.c  人物 0..86 的显示名
 *   save_dir.c      存档目录与存档栏文件名 */
/* 存档格式（魔数 "DAIKOUKAI4PKW9 1"，写入方 CPerson::vfunc_06，
 * DK4HD_sc.exe 中函数体 0x1401cb480）：
 *   +0x00..0xFF  文件头，不计入校验和
 *     +0x14      文件长度；+0x18 正文校验和
 *     +0x20      主角人物 id（0 拉斐尔 … 6 蒂雅）
 *     +0x70      主角总等级缓存
 *   0x2C03       235 条人物记录，按 ePerson 顺序、无标签：
 *     id < 87    每条 25 字节：+0 势力，+1 只增不减的字节，+2 六维，
 *                +8/+0xC 两段 uint32 经验，+0x10 uint16 HP，+0x12 身体，
 *                +0x13 精神，+0x14 五个装备字节
 *     id 87..234 0x3482 起每条 2 字节：势力与只增不减的字节，六维和经验不存档
 *   0x35AA       主角记录 103 字节：同样的 25 字节，之后是三个 17 字节 GBK 名字
 *                （0x35C3 / 0x35D4 / 0x35E5）
 *   六维顺序：体力 敏捷 魅力 智力 精神 运气
 *   单段等级 = isqrt(经验 / 30)，最高 255
 *
 * 校验和：游戏写盘时把正文每个字段加到 +0x18（u8/u16 零扩展，u32 整值）。
 * 不符时读档会问"所储存的进度有可能损坏了，还要继续吗？"。本工具按改动字段的
 * 差值修正 +0x18，游戏存过的档改完仍然有效。
 *
 * 旧版按 0x2C00 起 98 行、步长 0x19 读写：第 0..86 行字段正确，第 87..97 行
 * 在存档里并不存在，会写坏人物 88..219 的势力字节，见 warn_short_damage()。 */
#ifndef DK4_SAVE_SAVE_H
#define DK4_SAVE_SAVE_H
#include "common/platform.h"

#define SAVE_MAGIC "DAIKOUKAI4PKW9 1"
#define SAVE_HEADER 0x100
#define OFF_CHECKSUM 0x18
#define NAME_FIELD 17
#define STAT_COUNT 6
#define PERSON_COUNT 235
#define FULL_PERSON_COUNT 87
#define PERSON_BASE 0x2C03
#define PERSON_STRIDE 25
#define SHORT_BASE (PERSON_BASE + FULL_PERSON_COUNT * PERSON_STRIDE)
#define SHORT_STRIDE 2
#define HERO_BASE 0x35AA
#define HERO_SIZE 103
#define HERO_NAME (HERO_BASE + PERSON_STRIDE)
#define OFF_P_FACTION 0x00
#define OFF_P_STAT 0x02
#define OFF_P_EXP0 0x08
#define OFF_P_EXP1 0x0C
#define OFF_P_HP 0x10
#define FACTION_NONE 0x1A
#define OFF_PROT_ID 0x20
#define OFF_CACHED_LEVEL 0x70
#define MAX_TRACK_LEVEL 255
#define MAX_TOTAL_LEVEL (MAX_TRACK_LEVEL * 2)

_Static_assert(SHORT_BASE + (PERSON_COUNT - FULL_PERSON_COUNT) * SHORT_STRIDE == HERO_BASE,
    "person records end where the protagonist record starts");

/* sailor_names.c */
extern const wchar_t *const kSailorName[FULL_PERSON_COUNT];

typedef struct Save {
    wchar_t path[MAX_PATH];
    uint8_t *data;
    uint8_t *orig;   /* 最近一次读入或写出时的内容，用于算校验和增量 */
    size_t size;
    int dirty;
} Save;

/* 指向一条人物记录里的势力、六维和两段经验（游戏据此生成 0x30 字节的海员对象）。 */
typedef struct Body {
    uint8_t *faction;
    uint8_t *stat;
    uint8_t *exp0;
    uint8_t *exp1;
} Body;

/* save_file.c */
int load_save(Save *s, const wchar_t *path, wchar_t *err, int errn);
int write_save(Save *s, wchar_t *msg, int msgn);
void free_save(Save *s);
int warn_short_damage(Save *s, wchar_t *status, int statusn);

/* person.c：index 0 为主角，1..87 为人物 id index-1 */
int body_at(Save *s, int index, Body *b);
int person_exists(Save *s, int index);
int is_protagonist_card(Save *s, int index);
int is_present(Save *s, int index);
int is_recruited(Save *s, int index);
void read_stats(const uint8_t *stat, int out[STAT_COUNT]);
uint32_t exp_of_level(unsigned level);
unsigned track_level(uint32_t xp);
unsigned total_level(const uint8_t *exp0, const uint8_t *exp1);
unsigned hero_level(Save *s);
void apply_person(Save *s, int index, int faction, const int stats[STAT_COUNT],
                  unsigned level, int write_faction);
void person_name(Save *s, int index, wchar_t *dst, int dstn);

/* save_dir.c */
int find_save_root(wchar_t *out, int outn);
void slot_path(const wchar_t *root, int slot, wchar_t *out, int outn);

#endif
