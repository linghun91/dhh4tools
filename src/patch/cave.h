/* 补丁代码洞：放在 .text 增量链接跳板表后的 int3 空位里。
 * 开头是 8 字节魔数和头，末尾是各补丁点的原字节表，随时可完整还原原版。
 * 堆叠和卖流行品共用。 */
#ifndef DK4_PATCH_CAVE_H
#define DK4_PATCH_CAVE_H
#include "common/platform.h"

enum { kCaveHead = 16, kCaveMaxStk2 = 1200, kCaveMax = 8192 };

int cave_find(const uint8_t *data, size_t n, const char *magic, int lo, int hi);
int cave_restore(uint8_t *data, size_t n, int cave, int max);

#endif
