/* 字节缓冲区的小端读写。 */
#ifndef DK4_COMMON_BYTES_H
#define DK4_COMMON_BYTES_H
#include "common/platform.h"

uint32_t ru32(const uint8_t *p);
void wu32(uint8_t *p, uint32_t v);

#endif
