// Host-only check for the KITT visualizer's animation logic. Not part of the
// firmware build; lvgl.h and application.h here are stubs, so the maths can be
// iterated on without flashing hardware.
//
//   clang++ -std=c++17 -Wall -Wextra -Imain/display/test -Imain/display \
//       -o /tmp/test_kitt main/display/test/test_kitt_visualizer.cc \
//       main/display/kitt_visualizer.cc && /tmp/test_kitt
//
#include <cassert>
#include <cstdio>
#include <vector>
#include "kitt_visualizer.h"
#include "application.h"

lv_timer_t* g_timer = nullptr;
std::vector<lv_obj_t*> g_objs;

static constexpr int kBars = 21;

// g_objs[0] is the root; the 21 bars follow in left-to-right order.
static std::vector<int> Opas() {
    std::vector<int> v;
    for (int i = 1; i <= kBars; i++) v.push_back(g_objs[i]->bg_opa);
    return v;
}
static int Brightest(const std::vector<int>& v) {
    int best = 0;
    for (int i = 1; i < (int)v.size(); i++)
        if (v[i] > v[best]) best = i;
    return best;
}
static int Total(const std::vector<int>& v) {
    int t = 0;
    for (int x : v) t += x;
    return t;
}

int main() {
    lv_obj_t parent;
    KittVisualizer vis(&parent, 360, 120);
    assert(g_timer != nullptr);
    assert(g_objs.size() == kBars + 1);

    auto tick = [] { g_timer->cb(g_timer); };
    auto& app = Application::GetInstance();

    // --- scanner: the eye moves right, hits the wall, and comes back ---
    app.state = kDeviceStateIdle;
    tick();
    int start = Brightest(Opas());
    for (int i = 0; i < 5; i++) tick();
    int moved = Brightest(Opas());
    assert(moved > start);  // travelling right

    int peak = moved;
    for (int i = 0; i < 40; i++) {  // long enough to reach the far end and turn
        tick();
        peak = std::max(peak, Brightest(Opas()));
    }
    assert(peak == kBars - 1);                 // reached the far end
    assert(Brightest(Opas()) < kBars - 1);     // and bounced back

    // --- voice box: loud speech lights far more than silence ---
    app.state = kDeviceStateSpeaking;
    app.audio.out = 0;
    for (int i = 0; i < 60; i++) tick();
    auto quiet = Opas();
    int quiet_total = Total(quiet);

    app.audio.out = 255;
    for (int i = 0; i < 10; i++) tick();
    auto loud = Opas();
    assert(Total(loud) > quiet_total * 2);

    // Each cluster is symmetric about its centre and brightest there.
    for (int c = 0; c < 3; c++) {
        const int base = c * 7;
        for (int i = 0; i < 3; i++)
            assert(loud[base + i] == loud[base + 6 - i]);
        for (int i = 0; i < 7; i++)
            assert(loud[base + 3] >= loud[base + i]);
    }

    // Silence decays back down, but the centre segments stay lit.
    app.audio.out = 0;
    for (int i = 0; i < 80; i++) tick();
    auto decayed = Opas();
    assert(Total(decayed) < Total(loud));
    for (int c = 0; c < 3; c++) assert(decayed[c * 7 + 3] > decayed[c * 7]);

    // Listening reads the mic instead of the speaker.
    app.state = kDeviceStateListening;
    app.audio.out = 0;
    app.audio.in = 255;
    for (int i = 0; i < 10; i++) tick();
    assert(Total(Opas()) > Total(decayed));

    printf("kitt visualizer: all checks passed\n");
    return 0;
}
