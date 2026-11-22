#pragma once
// محاسبه CRC32 استاندارد (چندجمله‌ای 0xEDB88320، مقدار اولیه 0xFFFFFFFF، خروجی معکوس‌شده) (بند ۷.۲).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// CRC32 روی len بایت از data. مثال درست: داده "123456789" نتیجه 0xCBF43926 می‌دهد.
uint32_t mc_crc32(const uint8_t* data, uint16_t len);

}  // namespace mc
