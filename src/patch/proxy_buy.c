#include "patch/proxy_buy.h"

/* 地方舰队和玩家下属 NPC 舰队买货成本：
 *   cost * (1000 - ability + rand(100)) / 1000，ability 为船长交易能力。
 * 原版能力最高 500；超过 1000 时系数变负，再按无符号相除，一次买货记几百万支出。
 * 修正：把 mov ecx,1000 / sub ecx,eax / mov eax,ecx 换成 call 到函数结尾后的
 * 桩代码，先把能力封顶到 500（原版最大值）再算。 */
static const uint8_t kProxyBuyVanilla[] = {0xB9, 0xE8, 0x03, 0x00, 0x00, 0x2B, 0xC8, 0x8B, 0xC1};

static const uint8_t kProxyBuyTail[] = {
    0x89, 0x84, 0x24, 0x70, 0x03, 0x00, 0x00, 0xB9, 0x64, 0x00, 0x00, 0x00, 0xE8
};

static const uint8_t kProxyBuyEpi[] = {0x48, 0x81, 0xC4, 0xA0, 0x03, 0x00, 0x00, 0x5F, 0xC3};

static const uint8_t kProxyBuyStub[] = {
    0x3D, 0xF4, 0x01, 0x00, 0x00,   /* cmp eax,500 */
    0x7E, 0x05,                     /* jle +5 */
    0xB8, 0xF4, 0x01, 0x00, 0x00,   /* mov eax,500 */
    0xB9, 0xE8, 0x03, 0x00, 0x00,   /* mov ecx,1000 */
    0x2B, 0xC8,                     /* sub ecx,eax */
    0x8B, 0xC1,                     /* mov eax,ecx */
    0xC3                            /* ret */
};

/* 返回 0=未找到，1=原版，2=已装修正。 */
int find_proxy_buy(const uint8_t *data, size_t n, int *site, int *stub) {
    size_t i, k, end;
    int hits = 0, kind = 0, at = -1, pad = -1;
    for (i = 5; i + sizeof kProxyBuyVanilla + sizeof kProxyBuyTail <= n; i++) {
        int this_kind;
        if (data[i - 5] != 0xE8) continue;
        if (memcmp(data + i + sizeof kProxyBuyVanilla, kProxyBuyTail, sizeof kProxyBuyTail) != 0)
            continue;
        if (memcmp(data + i, kProxyBuyVanilla, sizeof kProxyBuyVanilla) == 0)
            this_kind = 1;
        else if (data[i] == 0xE8 && memcmp(data + i + 5, "\x90\x90\x90\x90", 4) == 0)
            this_kind = 2;
        else
            continue;
        hits++;
        kind = this_kind;
        at = (int)i;
    }
    if (hits != 1) return 0;
    end = (size_t)at + 0x800;
    if (end > n) end = n;
    for (i = (size_t)at; i + sizeof kProxyBuyEpi <= end; i++) {
        if (memcmp(data + i, kProxyBuyEpi, sizeof kProxyBuyEpi) == 0) {
            pad = (int)(i + sizeof kProxyBuyEpi);
            break;
        }
    }
    if (pad < 0 || (size_t)pad + sizeof kProxyBuyStub > n) return 0;
    if (kind == 1) {
        for (k = 0; k < sizeof kProxyBuyStub; k++)
            if (data[pad + k] != 0xCC) return 0;
    } else {
        int32_t rel;
        memcpy(&rel, data + at + 1, 4);
        if (at + 5 + rel != pad) return 0;
        if (memcmp(data + pad, kProxyBuyStub, sizeof kProxyBuyStub) != 0) return 0;
    }
    *site = at;
    *stub = pad;
    return kind;
}

int patch_proxy_buy(uint8_t *data, size_t n, wchar_t *err, int errn) {
    int site, stub, kind;
    int32_t rel;
    kind = find_proxy_buy(data, n, &site, &stub);
    if (!kind) {
        StringCchCopyW(err, (size_t)errn, L"找不到地方舰队买货的成本计算，不能修正舰队亏损。");
        return 0;
    }
    if (kind == 2) return 1;
    memcpy(data + stub, kProxyBuyStub, sizeof kProxyBuyStub);
    rel = (int32_t)(stub - (site + 5));
    data[site] = 0xE8;
    memcpy(data + site + 1, &rel, 4);
    memset(data + site + 5, 0x90, 4);
    return 1;
}
