/* 补丁代码洞：放在 .text 增量链接跳板表后的 int3 空位里。
 * 开头是 8 字节魔数和头，末尾是各补丁点的原字节表，随时可完整还原原版。
 * 堆叠和卖流行品共用。 */

#include "exe/x64asm.h"
#include "patch/cave.h"

/* 按魔数查找代码洞；lo/hi 限定魔数后那个字节（堆叠上限）的取值范围。 */
int cave_find(const uint8_t *data, size_t n, const char *magic, int lo, int hi) {
    size_t i;
    for (i = 0x400; i + kCaveHead <= n; i++) {
        if (data[i] != 'D' || memcmp(data + i, magic, 8) != 0) continue;
        if (data[i + 8] < lo || data[i + 8] > hi) continue;
        return (int)i;
    }
    return -1;
}

/* 按原字节表放回原版字节；表不对时返回 0，不做任何修改。 */
int cave_restore(uint8_t *data, size_t n, int cave, int max) {
    int tab = ru16(data + cave + 10), used = ru16(data + cave + 12);
    int cnt = data[cave + 9], p, i;
    if (used <= kCaveHead || used > max || (size_t)cave + (size_t)used > n) return 0;
    if (tab < kCaveHead || tab >= used) return 0;
    for (p = cave + tab, i = 0; i < cnt; i++) {
        uint32_t off;
        int len;
        if (p + 5 > cave + used) return 0;
        memcpy(&off, data + p, 4);
        len = data[p + 4];
        if (len == 0 || p + 5 + len > cave + used || (size_t)off + (size_t)len > n) return 0;
        p += 5 + len;
    }
    for (p = cave + tab, i = 0; i < cnt; i++) {
        uint32_t off;
        int len;
        memcpy(&off, data + p, 4);
        len = data[p + 4];
        memcpy(data + off, data + p + 5, (size_t)len);
        p += 5 + len;
    }
    memset(data + cave, 0xCC, (size_t)used);
    return 1;
}
