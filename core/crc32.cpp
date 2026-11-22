// محاسبه CRC32 (بند ۷.۲). بدون جدول، تا حافظه کم مصرف شود.
#include "crc32.h"

namespace mc {

uint32_t mc_crc32(const uint8_t* data, uint16_t len) {
    uint32_t crc = 0xFFFFFFFFU;
    for (uint16_t i = 0U; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0U; b < 8U; b++) {
            crc = ((crc & 1U) != 0U) ? ((crc >> 1U) ^ 0xEDB88320U) : (crc >> 1U);
        }
    }
    return ~crc;
}

}  // namespace mc
