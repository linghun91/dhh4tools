/* 主程序里可改的数值，以及读/写单个主程序映像的总入口
 * read_exe_image()/patch_exe_image()。patch_exe_image() 依次调用各补丁：
 *   caps.c 等级/属性/技能/港口上限（原地改）；stack/ 商品堆叠（DK4STK5）；
 *   fashion.c 卖流行品（DK4FSH1）；proxy_buy.c 地方舰队亏损修正。
 * cave.c 是堆叠和卖流行品共用的代码洞格式。 */
/* 主程序上限。剑术、生命、交易不进存档，由主程序计算并按这些立即数封顶：
 *   单段等级 = isqrt(xp/30)，再 min(等级, 单段上限)，2 处；
 *   获得经验时写入 min(xp, 30*上限^2)，4 处；等级最高 255（整数开方移位 16，再大溢出）；
 *   基础属性成长为一字节，min(值, 基础上限)，1 处；
 *   属性结算在增加、赋值、结果三处 min(值, 属性上限)；
 *   技能 = (属性A + 属性B) * (min(总等级, 技能上限) + 100) / 150；
 *   港口商业/军事 min(值, 港口上限)，uint16 存于 +0x1e、之后 movzx 读取，1 处；
 *   制海权 9999 是另一个立即数（mov r8d, 9999），不改。
 * 堆叠 N：同商品买入时共用一格，满 N 件再占下一格；价格不变。 */
#ifndef DK4_PATCH_CAPS_H
#define DK4_PATCH_CAPS_H
#include "common/platform.h"

typedef struct CapValues {
    uint32_t track;
    uint32_t base;
    uint32_t stat;
    uint32_t skill;
    uint32_t port;
    /* 每格可堆叠的件数，原版 1。 */
    uint32_t stack;
    /* 交易所卖流行品（原版 1、6、10、16）：件数 >= 船舱格数 * fthr / 2 才加占有率，
     * 涨幅 件数 / fdiv + 1；fl1..fl2-1 件给介绍信，fl2 件给推荐信。 */
    uint32_t fthr;
    uint32_t fdiv;
    uint32_t fl1;
    uint32_t fl2;
} CapValues;

int read_exe_image(const uint8_t *data, size_t n, CapValues *v, int *xp_off, wchar_t *err, int errn);
int patch_exe_image(uint8_t *data, size_t n, const CapValues *v, wchar_t *err, int errn);

#endif
