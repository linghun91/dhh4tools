/* 地方舰队买货亏损修正。 */
#ifndef DK4_PATCH_PROXY_BUY_H
#define DK4_PATCH_PROXY_BUY_H
#include "common/platform.h"

int find_proxy_buy(const uint8_t *data, size_t n, int *site, int *stub);
int patch_proxy_buy(uint8_t *data, size_t n, wchar_t *err, int errn);

#endif
