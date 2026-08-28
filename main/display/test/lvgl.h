// Minimal LVGL stub: just enough surface to compile and drive kitt_visualizer.cc on the host.
#pragma once
#include <cstdint>
#include <vector>

struct lv_obj_t { int x=0, y=0, w=0, h=0; uint8_t bg_opa=0; uint32_t bg_color=0; };
struct lv_timer_t;
typedef struct { uint32_t v; } lv_color_t;
typedef uint8_t lv_opa_t;

enum { LV_OPA_TRANSP = 0, LV_OPA_COVER = 255 };
enum { LV_SCROLLBAR_MODE_OFF = 0 };
enum { LV_OBJ_FLAG_SCROLLABLE = 1, LV_OBJ_FLAG_CLICKABLE = 2, LV_OBJ_FLAG_HIDDEN = 4 };

typedef void (*lv_timer_cb_t)(lv_timer_t*);
struct lv_timer_t { lv_timer_cb_t cb; void* user_data; };

inline lv_color_t lv_color_hex(uint32_t c) { return lv_color_t{c}; }
inline lv_color_t lv_color_black() { return lv_color_t{0}; }

extern std::vector<lv_obj_t*> g_objs;
inline lv_obj_t* lv_obj_create(lv_obj_t*) { auto* o = new lv_obj_t(); g_objs.push_back(o); return o; }
inline void lv_obj_delete(lv_obj_t* o) { delete o; }
inline void lv_obj_set_size(lv_obj_t* o, int w, int h) { o->w = w; o->h = h; }
inline void lv_obj_set_pos(lv_obj_t* o, int x, int y) { o->x = x; o->y = y; }
inline void lv_obj_center(lv_obj_t*) {}
inline void lv_obj_set_scrollbar_mode(lv_obj_t*, int) {}
inline void lv_obj_remove_flag(lv_obj_t*, int) {}
inline void lv_obj_set_style_radius(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_border_width(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_pad_all(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_bg_color(lv_obj_t* o, lv_color_t c, int) { o->bg_color = c.v; }
inline void lv_obj_set_style_bg_opa(lv_obj_t* o, lv_opa_t a, int) { o->bg_opa = a; }

extern lv_timer_t* g_timer;
inline lv_timer_t* lv_timer_create(lv_timer_cb_t cb, uint32_t, void* ud) {
    g_timer = new lv_timer_t{cb, ud};
    return g_timer;
}
inline void lv_timer_delete(lv_timer_t* t) { delete t; }
inline void* lv_timer_get_user_data(lv_timer_t* t) { return t->user_data; }
