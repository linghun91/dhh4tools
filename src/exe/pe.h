/* 最简 PE 解析：.text 范围、RVA 转文件偏移、导入表查找。 */
#ifndef DK4_EXE_PE_H
#define DK4_EXE_PE_H
#include "common/platform.h"

/* 取 .text 的文件偏移、大小及 VA 与文件偏移之差。 */
int pe_text(const uint8_t *data, size_t n, int *raw, int *size, int *delta);
/* dll!fn 导入地址槽的 RVA（PE32+），找不到返回 0。 */
uint32_t pe_import(const uint8_t *data, size_t n, const char *dll, const char *fn);

#endif
