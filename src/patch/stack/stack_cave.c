/* 堆叠：按魔数查找任一版本已装的堆叠代码洞，并读回堆叠上限。 */

#include "exe/x64asm.h"
#include "patch/cave.h"
#include "patch/stack/stack.h"

int stk2_cave(const uint8_t *data, size_t n) { return cave_find(data, n, STK2_MAGIC, 2, 10); }

int stk3_cave(const uint8_t *data, size_t n) { return cave_find(data, n, STK3_MAGIC, 2, kStackMax); }

int stk4_cave(const uint8_t *data, size_t n) { return cave_find(data, n, STK4_MAGIC, 2, kStackMax); }

int stk5_cave(const uint8_t *data, size_t n) { return cave_find(data, n, STK5_MAGIC, 2, kStackMax); }

/* call 的实际目标：若落在增量链接 jmp 跳板上则再跟一跳。 */
int call_body(const uint8_t *data, size_t n, int at) {
    int t = slot_call_off(data, n, at);
    int32_t rel;
    if (t < 0 || (size_t)t + 5 > n || data[t] != 0xE9) return t;
    memcpy(&rel, data + t + 1, 4);
    return t + 5 + rel;
}

/* 未装返回 0，否则返回堆叠上限。依次找 DK4STK5、旧版 DK4STK4/DK4STK3
 * （头相同）和 DK4STK2。 */
uint32_t stack_installed(const uint8_t *data, size_t n) {
    int cave = stk5_cave(data, n), imm;
    uint32_t s;
    if (cave < 0) cave = stk4_cave(data, n);
    if (cave < 0) cave = stk3_cave(data, n);
    if (cave >= 0) {
        imm = ru16(data + cave + 14);
        if (imm <= kCaveHead || (size_t)cave + (size_t)imm + 4 > n) return 0;
        memcpy(&s, data + cave + imm, 4);
        if (s != data[cave + 8]) return 0;
        return s;
    }
    cave = stk2_cave(data, n);
    if (cave < 0) return 0;
    imm = ru16(data + cave + 14);
    if (imm <= kCaveHead || (size_t)cave + (size_t)imm >= n) return 0;
    if (data[cave + imm] != data[cave + 8]) return 0;
    return data[cave + 8];
}

/* 装的是旧版堆叠补丁（DK4STK4/3/2），点击总是移动整格。 */
int stack_outdated(const uint8_t *data, size_t n) {
    return stk5_cave(data, n) < 0 &&
        (stk4_cave(data, n) >= 0 || stk3_cave(data, n) >= 0 || stk2_cave(data, n) >= 0);
}
