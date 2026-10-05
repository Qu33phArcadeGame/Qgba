// gfx.h - made by tools/make_gba_assets.py from the DS art (do not edit)
#pragma once
#include <stdint.h>
#define FIELD_W 176
#define FIELD_H 720
#define MM_X0 30
#define MM_W 24
#define MM_H 160
#define LOGO_W 111
#define LOGO_H 40
extern const uint8_t field8[FIELD_W * FIELD_H];   // palette 64-255
extern const uint16_t field_pal[192];
extern const uint8_t mini8[MM_W * MM_H];
extern const uint8_t logo8[LOGO_W * LOGO_H];      // 0 = see-through, else palette 64-255
extern const uint16_t logo_pal[192];
// 64x64 4bpp sprite tiles and their 16-colour palettes
extern const uint8_t obj_mk_green[2048];
extern const uint16_t obj_mk_green_pal[16];
extern const uint8_t obj_mk_red[2048];
extern const uint16_t obj_mk_red_pal[16];
extern const uint8_t obj_mk_blue[2048];
extern const uint16_t obj_mk_blue_pal[16];
extern const uint8_t obj_chair[2048];
extern const uint16_t obj_chair_pal[16];
extern const uint8_t glow_mk_green[2048];
extern const uint8_t glow_mk_red[2048];
extern const uint8_t glow_mk_blue[2048];
extern const uint16_t font_rows[1330];
extern const uint16_t fonts_rows[950];
extern const uint8_t font_w[95];
extern const uint8_t fonts_w[95];
#define FONT_H 14
#define FONTS_H 10
