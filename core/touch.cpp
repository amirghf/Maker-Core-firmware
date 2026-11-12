// کالیبراسیون سه‌نقطه‌ای لمس: x_صفحه = A*rx + B*ry + C و y_صفحه = D*rx + E*ry + F
// این روش جابه‌جایی یا برعکس بودن محورهای لمس را خودش درست می‌کند.
#include "touch.h"
#include "hal_display.h"

// متغیر دیباگ؛ ضرایب A تا F در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile float mc_dbg_touch_coef[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
}

namespace mc {
namespace {

float g_coef[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
bool  g_calibrated = false;

double abs_d(double v) { return (v < 0.0) ? -v : v; }

// حل A*rx + B*ry + C = s برای سه نقطه با قانون کرامر. خروجی: out[0]=A، out[1]=B، out[2]=C
void solve(const double rx[3], const double ry[3], const double s[3], double det, float* out) {
    const double da = (s[0] * (ry[1] - ry[2])) - (ry[0] * (s[1] - s[2])) + ((s[1] * ry[2]) - (s[2] * ry[1]));
    const double db = (rx[0] * (s[1] - s[2])) - (s[0] * (rx[1] - rx[2])) + ((rx[1] * s[2]) - (rx[2] * s[1]));
    const double dc = (rx[0] * ((ry[1] * s[2]) - (ry[2] * s[1])))
                    - (ry[0] * ((rx[1] * s[2]) - (rx[2] * s[1])))
                    + (s[0] * ((rx[1] * ry[2]) - (rx[2] * ry[1])));
    out[0] = static_cast<float>(da / det);
    out[1] = static_cast<float>(db / det);
    out[2] = static_cast<float>(dc / det);
}

}  // namespace

bool mc_touch_calibrate_3pt(const uint16_t raw_x[3], const uint16_t raw_y[3],
                            const uint16_t scr_x[3], const uint16_t scr_y[3]) {
    double rx[3];
    double ry[3];
    double sx[3];
    double sy[3];
    for (int i = 0; i < 3; ++i) {
        rx[i] = static_cast<double>(raw_x[i]);
        ry[i] = static_cast<double>(raw_y[i]);
        sx[i] = static_cast<double>(scr_x[i]);
        sy[i] = static_cast<double>(scr_y[i]);
    }
    const double det = (rx[0] * (ry[1] - ry[2])) - (ry[0] * (rx[1] - rx[2])) + ((rx[1] * ry[2]) - (rx[2] * ry[1]));
    if (abs_d(det) < 100000.0) {   // نقطه‌ها تقریباً روی یک خط‌اند
        g_calibrated = false;
        return false;
    }
    float cx[3];
    float cy[3];
    solve(rx, ry, sx, det, cx);
    solve(rx, ry, sy, det, cy);
    for (int i = 0; i < 3; ++i) {
        g_coef[i]     = cx[i];
        g_coef[i + 3] = cy[i];
    }
    for (int i = 0; i < 6; ++i) {
        mc_dbg_touch_coef[i] = g_coef[i];
    }
    g_calibrated = true;
    return true;
}

bool mc_touch_is_calibrated() {
    return g_calibrated;
}

bool mc_touch_apply(uint16_t raw_x, uint16_t raw_y, uint16_t* scr_x, uint16_t* scr_y) {
    if ((!g_calibrated) || (scr_x == nullptr) || (scr_y == nullptr)) { return false; }
    const float rx = static_cast<float>(raw_x);
    const float ry = static_cast<float>(raw_y);
    float fx = (g_coef[0] * rx) + (g_coef[1] * ry) + g_coef[2];
    float fy = (g_coef[3] * rx) + (g_coef[4] * ry) + g_coef[5];
    const float max_x = static_cast<float>(mc_display_width() - 1U);
    const float max_y = static_cast<float>(mc_display_height() - 1U);
    if (fx < 0.0f) { fx = 0.0f; }
    if (fy < 0.0f) { fy = 0.0f; }
    if (fx > max_x) { fx = max_x; }
    if (fy > max_y) { fy = max_y; }
    *scr_x = static_cast<uint16_t>(fx + 0.5f);
    *scr_y = static_cast<uint16_t>(fy + 0.5f);
    return true;
}

}  // namespace mc
