/* 商品堆叠。
 *   stack_slot.c   买/卖代码里的补丁点，以及旧版布局
 *   stack_cave.c   查找、读取已装的堆叠代码洞
 *   stack_apply.c  stack_apply()：生成并安装 DK4STK5 补丁
 */
/* 堆叠（"DK4STK5"）要点：
 * - 件数存在每条 8 字节货物记录（交易所格、船舱、卖出列表）的 +3 字节，原版不读写它；
 *   0 表示 1 件。[slot+8] 保持原版"本次买入"含义，点击、退货、全部卖出照原版分流。
 * - 原地改：记录复制时连 +3 一起复制，清记录和建记录时写 0。
 * - 钩子：买入（并进本次买的同货格）、Shift 买满一格、退货、卖出、全部卖出、撤回卖出、
 *   五个价格栏、确认交易的统计、存/读档（件数借存买入价最高字节）、玩家势力舰队
 *   自动交易、委托/剧情/事件的货物计数与扣除（按件）、整理货物界面及广场发放交易品。
 * - 单件/整格：普通点击船舱格、卖出列表或整理货物格移 1 件，Shift 移整格（原版）。
 *   单件先并进记录相同（除件数外）且未满的格；否则按件数 1 走原版代码，余下放回原处，
 *   钱、退款、供应仍是原版记账。
 * - 新代码放在增量链接跳板表后的 int3 空位；代码洞开头是头，末尾是各补丁点的
 *   原字节表，写堆叠 1 即可完整还原原版。
 * - 旧版 DK4STK4/DK4STK3（同布局、补丁点更少）和 DK4STK2（上限 9，代码在货物
 *   列表绘制函数之后）会被识别并在写入前移除。 */
#ifndef DK4_PATCH_STACK_STACK_H
#define DK4_PATCH_STACK_STACK_H
#include "common/platform.h"

extern const uint8_t kCargoVanilla[33];
int find_cargo_stack(const uint8_t *data, size_t n, int *at, uint32_t *stack);

enum {
    kSlotJne = 17,
    kSlotCallB032 = 0x30,
    kSlotCallBe92 = 0xD6,
    kSlotCallE9c6 = 0x136,
    kSlotUi = 0x13B,
    kSlotCallBc6c = 0x1B5,
    kSlotSkip = 0x239,
    kSlotBuyLen = 130,
    kSlotFmtAt = 130,
    kSlotDispAt = 140,
    kSlotPadLen = 218,
    /* cmp dword [rax+8], imm8；上限字节在操作码后第 3 字节。 */
    kSlotImmAt = 69,
    kSlotOldImmAt = 107,
    kSellPadLen = 147,
    kSellHookAt = 16,
    kSellImulAt = 64,
    kListA0At = 88,
    kList110At = 116,
    kPackHookAt = 5,
    kUnpackHookAt = 16
};

int find_slot_pat(const uint8_t *data, size_t n, int *at);
int slot_call_off(const uint8_t *data, size_t n, int at);
int slot_cave_off(const uint8_t *data, size_t n, int pat);
void slot_jmp(uint8_t *data, int from, int to);
int read_slot_stack(const uint8_t *data, size_t n, int pat, int cave, uint32_t *stack);
int find_sell_site(const uint8_t *data, size_t n, int *at);
int find_disp_site(const uint8_t *data, size_t n, int cave, int *at);
int patch_stack_extras(uint8_t *data, size_t n, int cave, uint32_t stack, wchar_t *err, int errn);

#define STK2_MAGIC "DK4STK2"
#define STK3_MAGIC "DK4STK3"
#define STK4_MAGIC "DK4STK4"
#define STK5_MAGIC "DK4STK5"

/* 件数只占一个字节。 */
enum { kStackMax = 255 };

int stk2_cave(const uint8_t *data, size_t n);
int stk3_cave(const uint8_t *data, size_t n);
int stk4_cave(const uint8_t *data, size_t n);
int stk5_cave(const uint8_t *data, size_t n);
/* call 的实际目标（穿过增量链接跳板）。 */
int call_body(const uint8_t *data, size_t n, int at);
int stack_apply(uint8_t *data, size_t n, uint32_t stack, wchar_t *err, int errn);
/* 已装堆叠上限，未装返回 0。 */
uint32_t stack_installed(const uint8_t *data, size_t n);
/* 装的是旧版堆叠补丁（DK4STK4/3/2）。 */
int stack_outdated(const uint8_t *data, size_t n);

#endif
