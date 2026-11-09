/* 交易所卖流行品（DK4FSH1）。 */
#ifndef DK4_PATCH_FASHION_H
#define DK4_PATCH_FASHION_H
#include "common/platform.h"
#include "patch/caps.h"

enum { kFashThr = 1, kFashDiv = 6, kFashL1 = 10, kFashL2 = 16 };

int fash_read(const uint8_t *data, size_t n, CapValues *v);
int fash_remove(uint8_t *data, size_t n);
int fash_apply(uint8_t *data, size_t n, const CapValues *v, wchar_t *err, int errn);

#endif
