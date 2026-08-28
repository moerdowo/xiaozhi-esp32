#include "kitt_visualizer.h"

#include <algorithm>
#include <cmath>

#include "application.h"

namespace {

// The one colour knob: KITT is red in every state, idle sweep and speech alike.
constexpr uint32_t kKittRed = 0xFF1A00;

constexpr uint32_t kFrameIntervalMs = 33;  // ~30 fps
constexpr uint32_t kScanPeriodMs = 900;    // one end-to-end sweep, as on the show's prop

// Unlit segments stay faintly visible, the way real LEDs do behind a red lens.
constexpr float kFloorOpa = 28.0f;

}  // namespace

KittVisualizer::KittVisualizer(lv_obj_t* parent, int width, int height) {
    root_ = lv_obj_create(parent);
    lv_obj_set_size(root_, width, height);
    lv_obj_center(root_);
    lv_obj_set_style_radius(root_, height / 8, 0);
    lv_obj_set_style_bg_color(root_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(root_, LV_OBJ_FLAG_SCROLLABLE);

    // Gaps between clusters are wider than gaps between segments, as on the voice box.
    const int pad = std::max(2, width / 60);
    const int cluster_gap = pad * 3;
    const int spacing =
        2 * pad + (kClusters - 1) * cluster_gap + kClusters * (kBarsPerCluster - 1) * pad;
    const int bar_w = std::max(2, (width - spacing) / kBarCount);
    const int bar_h = std::max(2, height - 2 * pad);
    const int used = spacing + bar_w * kBarCount;

    int x = pad + (width - used) / 2;
    for (int c = 0; c < kClusters; c++) {
        for (int i = 0; i < kBarsPerCluster; i++) {
            lv_obj_t* bar = lv_obj_create(root_);
            lv_obj_set_size(bar, bar_w, bar_h);
            lv_obj_set_pos(bar, x, pad);
            lv_obj_set_style_radius(bar, bar_w / 3, 0);
            lv_obj_set_style_border_width(bar, 0, 0);
            lv_obj_set_style_pad_all(bar, 0, 0);
            lv_obj_set_style_bg_color(bar, lv_color_hex(kKittRed), 0);
            lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
            lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);
            lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
            bars_[c * kBarsPerCluster + i] = bar;
            x += bar_w + pad;
        }
        x += cluster_gap - pad;
    }

    timer_ = lv_timer_create(
        [](lv_timer_t* timer) {
            static_cast<KittVisualizer*>(lv_timer_get_user_data(timer))->Render();
        },
        kFrameIntervalMs, this);
}

KittVisualizer::~KittVisualizer() {
    if (timer_ != nullptr) {
        lv_timer_delete(timer_);
    }
    if (root_ != nullptr) {
        lv_obj_delete(root_);  // deletes the bars with it
    }
}

void KittVisualizer::Render() {
    auto& app = Application::GetInstance();
    auto state = app.GetDeviceState();
    bool speaking = state == kDeviceStateSpeaking;
    bool listening = state == kDeviceStateListening;

    if (!speaking && !listening) {
        RenderScanner();
        return;
    }

    // While speaking the mic still hears the speaker, so pick the source that
    // actually represents who is talking rather than whichever is loudest.
    uint8_t raw =
        speaking ? app.GetAudioService().GetOutputLevel() : app.GetAudioService().GetInputLevel();

    // Square root keeps quiet speech visible; drop the exponent for a flatter meter.
    RenderVoiceBox(std::sqrt(raw / 255.0f));
}

void KittVisualizer::RenderScanner() {
    scan_pos_ += scan_dir_ * (kBarCount - 1) * static_cast<float>(kFrameIntervalMs) / kScanPeriodMs;
    if (scan_pos_ >= kBarCount - 1) {
        scan_pos_ = kBarCount - 1;
        scan_dir_ = -1.0f;
    } else if (scan_pos_ <= 0.0f) {
        scan_pos_ = 0.0f;
        scan_dir_ = 1.0f;
    }

    for (int i = 0; i < kBarCount; i++) {
        // Four-segment trail behind the eye.
        SetBar(i, 1.0f - std::fabs(i - scan_pos_) / 4.0f);
    }

    // Start the next utterance from rest instead of from a stale level.
    for (float& level : cluster_level_) {
        level = 0.0f;
    }
}

void KittVisualizer::RenderVoiceBox(float level) {
    // KITT's three clusters were driven by different frequency bands. Staggered
    // attack/decay fakes that shimmer without an FFT.
    // ponytail: per-cluster envelopes, not real bands. Swap in esp-dsp if it ever
    // needs to track actual bass/mid/treble.
    static constexpr float kAttack[kClusters] = {0.55f, 0.75f, 0.55f};
    static constexpr float kDecay[kClusters] = {0.10f, 0.18f, 0.10f};
    static constexpr float kGain[kClusters] = {0.80f, 1.00f, 0.80f};
    constexpr float kCentre = (kBarsPerCluster - 1) / 2.0f;

    for (int c = 0; c < kClusters; c++) {
        float target = level * kGain[c];
        float k = target > cluster_level_[c] ? kAttack[c] : kDecay[c];
        cluster_level_[c] += (target - cluster_level_[c]) * k;

        // Lit segments bloom outward from the centre of the cluster.
        float radius = cluster_level_[c] * (kCentre + 1.0f);
        for (int i = 0; i < kBarsPerCluster; i++) {
            float brightness = radius - std::fabs(i - kCentre);
            if (i == static_cast<int>(kCentre)) {
                // Keep the centre segment alive during silence so the panel never looks dead.
                brightness = std::max(brightness, 0.18f);
            }
            SetBar(c * kBarsPerCluster + i, brightness);
        }
    }
}

void KittVisualizer::SetBar(int index, float brightness) {
    brightness = std::clamp(brightness, 0.0f, 1.0f);
    auto opa = static_cast<uint8_t>(std::lround(brightness * (255.0f - kFloorOpa) + kFloorOpa));
    if (opa == last_opa_[index]) {
        return;  // skip the redraw LVGL would otherwise queue
    }
    last_opa_[index] = opa;
    lv_obj_set_style_bg_opa(bars_[index], opa, 0);
}
