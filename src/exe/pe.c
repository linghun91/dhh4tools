/* 最简 PE 解析：.text 范围、RVA 转文件偏移、导入表查找。 */

#include "common/bytes.h"
#include "exe/pe.h"

/* .text 的文件偏移范围，以及 VA 与文件偏移之差。 */
int pe_text(const uint8_t *data, size_t n, int *raw, int *size, int *delta) {
    uint32_t pe, so, k, nsec;
    if (n < 0x40) return 0;
    pe = ru32(data + 0x3C);
    if ((size_t)pe + 24 > n || memcmp(data + pe, "PE\0\0", 4) != 0) return 0;
    nsec = (uint32_t)(data[pe + 6] | (data[pe + 7] << 8));
    so = pe + 24 + (uint32_t)(data[pe + 20] | (data[pe + 21] << 8));
    for (k = 0; k < nsec; k++) {
        const uint8_t *s = data + so + 40 * k;
        if ((size_t)(so + 40 * k + 40) > n) return 0;
        if (memcmp(s, ".text\0", 6) != 0) continue;
        *raw = (int)ru32(s + 20);
        *size = (int)ru32(s + 16);
        *delta = (int)(ru32(s + 12) - ru32(s + 20));
        if (*size > (int)ru32(s + 8)) *size = (int)ru32(s + 8);
        return (size_t)*raw + (size_t)*size <= n;
    }
    return 0;
}

static int pe_rva_off(const uint8_t *data, size_t n, uint32_t rva) {
    uint32_t pe = ru32(data + 0x3C), so, k, nsec;
    nsec = (uint32_t)(data[pe + 6] | (data[pe + 7] << 8));
    so = pe + 24 + (uint32_t)(data[pe + 20] | (data[pe + 21] << 8));
    for (k = 0; k < nsec; k++) {
        const uint8_t *s = data + so + 40 * k;
        uint32_t va = ru32(s + 12), vs = ru32(s + 8), rs = ru32(s + 16), ra = ru32(s + 20);
        uint32_t sz = vs > rs ? vs : rs;
        if (rva >= va && rva < va + sz) {
            if (rva - va >= rs || (size_t)ra + (rva - va) >= n) return -1;
            return (int)(ra + (rva - va));
        }
    }
    return -1;
}

/* dll!fn 导入地址槽的 RVA（PE32+），找不到返回 0。 */
uint32_t pe_import(const uint8_t *data, size_t n, const char *dll, const char *fn) {
    uint32_t pe, opt, dir;
    int o;
    if (n < 0x40) return 0;
    pe = ru32(data + 0x3C);
    opt = pe + 24;
    if ((size_t)opt + 128 > n || data[opt] != 0x0B || data[opt + 1] != 0x02) return 0;
    dir = ru32(data + opt + 112 + 8);
    o = pe_rva_off(data, n, dir);
    for (; o >= 0 && (size_t)o + 20 <= n; o += 20) {
        uint32_t oft = ru32(data + o), name = ru32(data + o + 12), ft = ru32(data + o + 16);
        int no, to, k;
        if (name == 0) break;
        no = pe_rva_off(data, n, name);
        if (no < 0 || _stricmp((const char *)data + no, dll) != 0) continue;
        to = pe_rva_off(data, n, oft ? oft : ft);
        for (k = 0; to >= 0 && (size_t)to + 8 * (size_t)k + 8 <= n; k++) {
            uint64_t e;
            int eo;
            memcpy(&e, data + to + 8 * k, 8);
            if (e == 0) break;
            if (e >> 63) continue;
            eo = pe_rva_off(data, n, (uint32_t)e);
            if (eo >= 0 && strcmp((const char *)data + eo + 2, fn) == 0)
                return ft + 8u * (uint32_t)k;
        }
    }
    return 0;
}
