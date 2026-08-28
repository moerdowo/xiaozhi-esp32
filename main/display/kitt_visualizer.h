#ifndef KITT_VISUALIZER_H
#define KITT_VISUALIZER_H

#include <lvgl.h>

#include <cstdint>

/*
 * Knight Rider (KITT) style audio visualizer, used in place of the emoji face.
 *
 * A single row of vertical LED strips serves both of KITT's signature looks:
 *  - idle: the nose scanner, one bright segment sweeping left/right with a trail
 *  - listening/speaking: the dashboard voice modulator, three clusters blooming
 *    outward from their centres with the audio level
 */
class KittVisualizer {
public:
    KittVisualizer(lv_obj_t* parent, int width, int height);
    ~KittVisualizer();

    KittVisualizer(const KittVisualizer&) = delete;
    KittVisualizer& operator=(const KittVisualizer&) = delete;

    // Base colour of the lit segments. Defaults to KITT red.
    void SetColor(lv_color_t color);

private:
    static constexpr int kClusters = 3;
    static constexpr int kBarsPerCluster = 7;
    static constexpr int kBarCount = kClusters * kBarsPerCluster;

    void Render();
    void RenderScanner();
    void RenderVoiceBox(float level);
    void SetBar(int index, float brightness);

    lv_obj_t* root_ = nullptr;
    lv_obj_t* bars_[kBarCount] = {};
    lv_timer_t* timer_ = nullptr;
    lv_color_t color_;

    uint8_t last_opa_[kBarCount] = {};
    float cluster_level_[kClusters] = {};
    float scan_pos_ = 0.0f;
    float scan_dir_ = 1.0f;
};

#endif  // KITT_VISUALIZER_H
