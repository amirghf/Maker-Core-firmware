// پیاده‌سازی برنامه‌ریز حرکت (بند ۵.۷).
// هیچ هدر ST اینجا include نمی‌شود.
#include "planner.h"
#include "step_gen.h"
#include "hal_irq.h"
#include <math.h>

// متغیرهای debug بند ۵.۷ (شماره‌ها دائمی‌اند)
extern "C" {
volatile uint32_t mc_dbg_120_pl_jd_um     = 50;  // برای تست: نوشتنی. انحراف اتصال (junction deviation) به میکرومتر، بازه ۱ تا ۵۰۰۰. فقط روی بلوک‌هایی که بعد از نوشتن وارد صف می‌شوند اثر دارد.
volatile uint32_t mc_dbg_121_pl_sel       = 0;   // برای تست: نوشتنی. شماره بلوک منتظر در صف (۰ = اولین منتظر) برای خواندن متغیرهای mc_dbg_122 تا mc_dbg_126 و mc_dbg_131.
volatile uint32_t mc_dbg_122_pl_entry_hz  = 0;  // برای تست: خواندنی. نرخ ورود برنامه‌ریزی‌شده بلوک انتخاب‌شده (گام بر ثانیه برای محور اصلی همان بلوک). اگر بلوک نیست ۰.
volatile uint32_t mc_dbg_123_pl_exit_hz   = 0;  // برای تست: خواندنی. نرخ خروج برنامه‌ریزی‌شده بلوک انتخاب‌شده.
volatile uint32_t mc_dbg_124_pl_nacc      = 0;  // برای تست: خواندنی. تعداد گام فاز شتاب‌گیری بلوک انتخاب‌شده.
volatile uint32_t mc_dbg_125_pl_ndec      = 0;  // برای تست: خواندنی. تعداد گام فاز کاهش سرعت بلوک انتخاب‌شده.
volatile uint32_t mc_dbg_126_pl_junc_hz   = 0;  // برای تست: خواندنی. سقف سرعت اتصال بلوک انتخاب‌شده با بلوک قبلی، به نرخ محور اصلی همان بلوک (قبل از lookahead).
volatile uint32_t mc_dbg_127_pl_replans   = 0;  // برای تست: خواندنی. تعداد دفعاتی که lookahead با موفقیت اعمال شد.
volatile uint32_t mc_dbg_128_pl_retries   = 0;  // برای تست: خواندنی. تعداد دفعاتی که اعمال lookahead به‌خاطر شروع شدن یک بلوک تازه دوباره انجام شد.
}

namespace mc {

namespace {

constexpr uint8_t  kMask      = static_cast<uint8_t>(kPlanQueueSize - 1U);
constexpr uint32_t kMaxTries  = 3U;

// یک بلوک در صف برنامه‌ریز: بخش عدد صحیح برای وقفه + بخش اعشاری فقط برای برنامه‌ریز
struct PlanBlock {
    PlanExec ex;
    float    len_mm;       // طول حرکت (میلی‌متر)
    float    rho;          // میلی‌متر به ازای هر گام محور اصلی
    float    nom_mm;       // سرعت cruise (میلی‌متر بر ثانیه)
    float    acc_mm;       // شتاب (میلی‌متر بر ثانیه‌مربع)
    float    rest_mm;      // سرعت شروع و پایان از حالت سکون (میلی‌متر بر ثانیه)
    float    max_in_sq;    // سقف مربع سرعت ورود (اتصال با بلوک قبلی)
    float    v_in_sq;      // مربع سرعت ورود برنامه‌ریزی‌شده (میلی‌متر بر ثانیه)^2
    float    unit[3];      // بردار یکه جهت XYZ
    bool     has_xyz;      // آیا بلوک حرکت XYZ دارد؟
    uint32_t rest_hz;      // start_hz بلوک (نرخ محور اصلی)
};

// اطلاعات آخرین بلوک واردشده (برای محاسبه اتصال با بلوک بعدی)
struct LastInfo {
    bool  valid;
    float unit[3];
    bool  has_xyz;
    float nom_mm;
    float acc_mm;
    float rest_mm;
};

PlanBlock g_q[kPlanQueueSize];
volatile uint8_t  g_head  = 0U;   // جای نوشتن بعدی (فقط حلقه اصلی)
volatile uint8_t  g_tail  = 0U;   // جای خواندن بعدی (فقط وقفه، یا حلقه اصلی داخل قفل)
volatile uint32_t g_epoch = 0U;   // با هر برداشتن بلوک توسط وقفه یک واحد زیاد می‌شود
LastInfo g_last = {};
bool     g_dirty = false;

inline float sq(float x) { return x * x; }

inline uint32_t to_u32(float x) {
    if (x < 1.0f) { return 1U; }
    if (x > 4.0e9f) { return 4000000000U; }
    return static_cast<uint32_t>(x + 0.5f);
}

// حساب گام‌های شتاب‌گیری و کاهش سرعت از روی نرخ ورود و خروج (فقط عدد صحیح)
void calc_trapezoid(PlanExec& e) {
    const uint64_t two_a = 2ULL * e.accel;
    const uint64_t nom2  = static_cast<uint64_t>(e.nominal_hz) * e.nominal_hz;
    uint64_t na = (nom2 > e.entry_sq) ? ((nom2 - e.entry_sq) / two_a) : 0ULL;
    uint64_t nd = (nom2 > e.exit_sq)  ? ((nom2 - e.exit_sq) / two_a)  : 0ULL;
    if ((na + nd) > e.total) {   // مثلثی: به cruise نمی‌رسد
        const int64_t num = static_cast<int64_t>(two_a * e.total) +
                            static_cast<int64_t>(e.exit_sq) - static_cast<int64_t>(e.entry_sq);
        int64_t n = (num > 0) ? (num / static_cast<int64_t>(2ULL * two_a)) : 0;
        if (n > static_cast<int64_t>(e.total)) { n = static_cast<int64_t>(e.total); }
        na = static_cast<uint64_t>(n);
        nd = e.total - na;
    }
    e.n_acc = static_cast<uint32_t>(na);
    e.n_dec = static_cast<uint32_t>(nd);
}

// هندسه بلوک را از روی گام‌ها می‌سازد (میلی‌متر، یکه، ...)
void build_block(const MotionBlock& mb, PlanBlock& nb) {
    uint8_t master = 0U;
    for (uint8_t i = 1U; i < 4U; ++i) {
        if (mb.steps[i] > mb.steps[master]) { master = i; }
    }
    PlanExec& e = nb.ex;
    for (uint8_t i = 0U; i < 4U; ++i) { e.steps[i] = mb.steps[i]; }
    e.dir_mask   = static_cast<uint8_t>(mb.dir_mask & 0x0FU);
    e.master     = master;
    e.total      = mb.steps[master];
    e.nominal_hz = mb.cruise_hz;
    e.accel      = mb.accel_sps2;
    nb.rest_hz   = mb.start_hz;

    float d[3];
    float len2 = 0.0f;
    for (uint8_t i = 0U; i < 3U; ++i) {
        const float mm = static_cast<float>(mb.steps[i]) / kPlanDefaultStepsPerMm;
        d[i] = (((mb.dir_mask >> i) & 1U) != 0U) ? mm : -mm;
        len2 += mm * mm;
    }
    if (len2 > 0.0f) {
        nb.len_mm  = sqrtf(len2);
        nb.has_xyz = true;
        for (uint8_t i = 0U; i < 3U; ++i) { nb.unit[i] = d[i] / nb.len_mm; }
    } else {
        nb.len_mm  = static_cast<float>(mb.steps[3]) / kPlanDefaultStepsPerMm;   // فقط E
        nb.has_xyz = false;
        for (uint8_t i = 0U; i < 3U; ++i) { nb.unit[i] = 0.0f; }
    }
    nb.rho     = nb.len_mm / static_cast<float>(e.total);
    nb.nom_mm  = static_cast<float>(mb.cruise_hz) * nb.rho;
    nb.acc_mm  = static_cast<float>(mb.accel_sps2) * nb.rho;
    nb.rest_mm = static_cast<float>(mb.start_hz) * nb.rho;
}

// سقف مربع سرعت ورود بلوک cur با بلوک قبلی (g_last) به روش junction deviation
float junction_limit_sq(const PlanBlock& cur) {
    const float floor_mm = fminf(g_last.rest_mm, cur.rest_mm);
    const float ceil_mm  = fminf(cur.nom_mm, g_last.nom_mm);
    float v = floor_mm;   // بلوک بدون XYZ یا برگشت کامل: در حد سرعت سکون
    if (g_last.has_xyz && cur.has_xyz) {
        float dot = 0.0f;
        for (uint8_t i = 0U; i < 3U; ++i) { dot += g_last.unit[i] * cur.unit[i]; }
        const float cos_t = -dot;   // زاویه بین دو مسیر؛ خط راست = منهای ۱
        if (cos_t < -0.999999f) {
            v = ceil_mm;            // ادامه در یک خط
        } else if (cos_t > 0.999999f) {
            v = floor_mm;           // برگشت ۱۸۰ درجه
        } else {
            uint32_t jd_um = mc_dbg_120_pl_jd_um;
            if (jd_um < kPlanMinJunctionUm) { jd_um = kPlanMinJunctionUm; }
            if (jd_um > kPlanMaxJunctionUm) { jd_um = kPlanMaxJunctionUm; }
            const float jd_mm    = static_cast<float>(jd_um) * 0.001f;
            const float sin_half = sqrtf(0.5f * (1.0f - cos_t));
            const float a        = fminf(g_last.acc_mm, cur.acc_mm);
            v = sqrtf(a * jd_mm * sin_half / (1.0f - sin_half));
        }
    }
    if (v > ceil_mm)  { v = ceil_mm; }
    if (v < floor_mm) { v = floor_mm; }
    return v * v;
}

// یک بار lookahead (عبور معکوس و رو‌به‌جلو) روی بلوک‌های منتظر. true = اعمال شد (یا کاری نبود).
bool replan_once() {
    const uint32_t ep   = g_epoch;
    const uint8_t  tail = g_tail;
    const uint8_t  head = g_head;
    const uint8_t  n    = static_cast<uint8_t>((head - tail) & kMask);
    if (n == 0U) { return true; }
    const bool fixed0 = mc_stepgen_plan_active();   // بلوک قبل از اولین منتظر در حال اجراست: ورود اولین منتظر ثابت است

    float    v[kPlanQueueSize];
    uint32_t out_entry[kPlanQueueSize];
    uint32_t out_exit[kPlanQueueSize];
    uint32_t out_nacc[kPlanQueueSize];
    uint32_t out_ndec[kPlanQueueSize];

    // سرعت ورود اولین بلوک
    {
        const PlanBlock& b0 = g_q[tail];
        v[0] = fixed0 ? b0.v_in_sq : sq(b0.rest_mm);
    }
    // عبور معکوس: از آخر به اول، سرعت ورود نباید بیشتر از چیزی باشد که تا سرعت خروج بشود ترمز کرد
    for (uint8_t i = static_cast<uint8_t>(n - 1U); i >= 1U; --i) {
        const PlanBlock& b = g_q[(tail + i) & kMask];
        const float exit_sq = (i == static_cast<uint8_t>(n - 1U)) ? sq(b.rest_mm) : v[i + 1U];
        const float limit   = exit_sq + 2.0f * b.acc_mm * b.len_mm;
        v[i] = fminf(b.max_in_sq, limit);
    }
    // عبور رو‌به‌جلو: سرعت ورود بلوک بعدی نباید بیشتر از چیزی باشد که با شتاب بشود به آن رسید
    for (uint8_t i = 0U; (i + 1U) < n; ++i) {
        const PlanBlock& b = g_q[(tail + i) & kMask];
        const float reach = v[i] + 2.0f * b.acc_mm * b.len_mm;
        if (v[i + 1U] > reach) { v[i + 1U] = reach; }
    }
    // تبدیل به عدد صحیح (گام بر ثانیه) و حساب دوزنقه
    for (uint8_t i = 0U; i < n; ++i) {
        const PlanBlock& b = g_q[(tail + i) & kMask];
        const float rho2    = b.rho * b.rho;
        const float exit_sq = ((i + 1U) < n) ? v[i + 1U] : sq(b.rest_mm);
        PlanExec e = b.ex;
        const uint32_t nom_sq = e.nominal_hz * e.nominal_hz;
        e.entry_sq = to_u32(v[i] / rho2);
        e.exit_sq  = to_u32(exit_sq / rho2);
        if (e.entry_sq > nom_sq) { e.entry_sq = nom_sq; }
        if (e.exit_sq  > nom_sq) { e.exit_sq  = nom_sq; }
        calc_trapezoid(e);
        out_entry[i] = e.entry_sq;
        out_exit[i]  = e.exit_sq;
        out_nacc[i]  = e.n_acc;
        out_ndec[i]  = e.n_dec;
    }

    // اعمال: اگر در این مدت وقفه بلوکی برداشته بود، نتیجه کهنه است
    const uint32_t irq = mc_irq_save();
    if ((g_epoch != ep) || (g_tail != tail)) {
        mc_irq_restore(irq);
        return false;
    }
    for (uint8_t i = 0U; i < n; ++i) {
        PlanBlock& b = g_q[(tail + i) & kMask];
        b.v_in_sq     = v[i];
        b.ex.entry_sq = out_entry[i];
        b.ex.exit_sq  = out_exit[i];
        b.ex.n_acc    = out_nacc[i];
        b.ex.n_dec    = out_ndec[i];
    }
    mc_irq_restore(irq);
    return true;
}

void replan() {
    for (uint32_t t = 0U; t < kMaxTries; ++t) {
        if (replan_once()) {
            g_dirty = false;
            mc_dbg_127_pl_replans += 1U;
            return;
        }
        mc_dbg_128_pl_retries += 1U;
    }
    g_dirty = true;   // در وظیفه بعدی دوباره امتحان می‌شود
}

uint32_t isqrt_f(uint32_t x) {
    return static_cast<uint32_t>(sqrtf(static_cast<float>(x)) + 0.5f);
}

void update_debug() {
    const uint8_t n = static_cast<uint8_t>((g_head - g_tail) & kMask);
    const uint32_t sel = mc_dbg_121_pl_sel;
    if (sel < n) {
        const uint32_t irq = mc_irq_save();
        const PlanBlock b = g_q[(g_tail + sel) & kMask];
        mc_irq_restore(irq);
        mc_dbg_122_pl_entry_hz = isqrt_f(b.ex.entry_sq);
        mc_dbg_123_pl_exit_hz  = isqrt_f(b.ex.exit_sq);
        mc_dbg_124_pl_nacc     = b.ex.n_acc;
        mc_dbg_125_pl_ndec     = b.ex.n_dec;
        mc_dbg_126_pl_junc_hz  = to_u32(sqrtf(b.max_in_sq) / b.rho);
    } else {
        mc_dbg_122_pl_entry_hz = 0U;
        mc_dbg_123_pl_exit_hz  = 0U;
        mc_dbg_124_pl_nacc     = 0U;
        mc_dbg_125_pl_ndec     = 0U;
        mc_dbg_126_pl_junc_hz  = 0U;
    }
}

}  // namespace

void mc_plan_init() {
    g_head  = 0U;
    g_tail  = 0U;
    g_epoch = 0U;
    g_last  = LastInfo{};
    g_dirty = false;
    mc_dbg_120_pl_jd_um    = kPlanDefaultJunctionUm;
    mc_dbg_121_pl_sel      = 0U;
    mc_dbg_122_pl_entry_hz = 0U;
    mc_dbg_123_pl_exit_hz  = 0U;
    mc_dbg_124_pl_nacc     = 0U;
    mc_dbg_125_pl_ndec     = 0U;
    mc_dbg_126_pl_junc_hz  = 0U;
    mc_dbg_127_pl_replans  = 0U;
    mc_dbg_128_pl_retries  = 0U;
}

bool mc_plan_push(const MotionBlock& block) {
    PlanBlock nb = {};
    build_block(block, nb);
    const float junction_sq = g_last.valid ? junction_limit_sq(nb) : 0.0f;

    const uint32_t irq  = mc_irq_save();
    const uint8_t  head = g_head;
    const uint8_t  next = static_cast<uint8_t>((head + 1U) & kMask);
    if (next == g_tail) {
        mc_irq_restore(irq);
        return false;
    }
    const bool chain = g_last.valid && (mc_stepgen_plan_active() || (head != g_tail));
    const float rest_sq = sq(nb.rest_mm);
    if (chain) {
        nb.max_in_sq = junction_sq;
        nb.v_in_sq   = fminf(sq(g_last.rest_mm), sq(nb.nom_mm));   // سرعت خروج بلوک قبلی که آخر صف بوده
    } else {
        nb.max_in_sq = rest_sq;
        nb.v_in_sq   = rest_sq;
    }
    const float rho2 = nb.rho * nb.rho;
    nb.ex.entry_sq = to_u32(nb.v_in_sq / rho2);
    nb.ex.exit_sq  = to_u32(rest_sq / rho2);
    calc_trapezoid(nb.ex);
    g_q[head] = nb;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    g_head = next;

    g_last.valid   = true;
    g_last.has_xyz = nb.has_xyz;
    g_last.nom_mm  = nb.nom_mm;
    g_last.acc_mm  = nb.acc_mm;
    g_last.rest_mm = nb.rest_mm;
    for (uint8_t i = 0U; i < 3U; ++i) { g_last.unit[i] = nb.unit[i]; }
    mc_irq_restore(irq);

    replan();
    return true;
}

void mc_plan_clear() {
    const uint32_t irq = mc_irq_save();
    g_tail = g_head;
    g_last.valid = false;
    g_dirty = false;
    mc_irq_restore(irq);
}

uint16_t mc_plan_count() {
    return static_cast<uint16_t>((g_head - g_tail) & kMask);
}

bool mc_plan_isr_take(PlanExec& out) {
    const uint8_t t = g_tail;
    if (t == g_head) { return false; }
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    out = g_q[t].ex;
    g_tail  = static_cast<uint8_t>((t + 1U) & kMask);
    g_epoch = g_epoch + 1U;
    return true;
}

void mc_plan_poll() {
    if (g_dirty) {
        replan();
    }
    update_debug();
}

}  // namespace mc
