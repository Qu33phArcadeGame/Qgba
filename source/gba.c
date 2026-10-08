// gba.c — the hardware: screen, sprites, buttons, sound, save memory, the refresh interrupt.
// (sim.c stands in for this file in the PC test harness.)
#ifndef GBA_SIM
#include "qu.h"

typedef volatile u16 vu16; typedef volatile u32 vu32;
#define REG16(a) (*(vu16 *)(a))
#define REG32(a) (*(vu32 *)(a))
#define REG_DISPCNT   REG16(0x04000000)
#define REG_DISPSTAT  REG16(0x04000004)
#define REG_BG2PA     REG16(0x04000020)
#define REG_BG2PB     REG16(0x04000022)
#define REG_BG2PC     REG16(0x04000024)
#define REG_BG2PD     REG16(0x04000026)
#define REG_BG2X      REG32(0x04000028)
#define REG_BG2Y      REG32(0x0400002C)
#define REG_WIN0H     REG16(0x04000040)
#define REG_WIN0V     REG16(0x04000044)
#define REG_WININ     REG16(0x04000048)
#define REG_WINOUT    REG16(0x0400004A)
#define REG_BLDCNT    REG16(0x04000050)
#define REG_BLDALPHA  REG16(0x04000052)
#define REG_SOUNDCNT_L REG16(0x04000080)
#define REG_SOUNDCNT_H REG16(0x04000082)
#define REG_SOUNDCNT_X REG16(0x04000084)
#define REG_FIFO_A    0x040000A0
#define REG_FIFO_B    0x040000A4
#define REG_DMA1SAD   REG32(0x040000BC)
#define REG_DMA1DAD   REG32(0x040000C0)
#define REG_DMA1CNT_H REG16(0x040000C6)
#define REG_DMA2SAD   REG32(0x040000C8)
#define REG_DMA2DAD   REG32(0x040000CC)
#define REG_DMA2CNT_H REG16(0x040000D2)
#define REG_DMA3SAD   REG32(0x040000D4)
#define REG_DMA3DAD   REG32(0x040000D8)
#define REG_DMA3CNT   REG32(0x040000DC)
#define REG_TM0CNT_L  REG16(0x04000100)
#define REG_TM0CNT_H  REG16(0x04000102)
#define REG_TM1CNT_L  REG16(0x04000104)
#define REG_TM1CNT_H  REG16(0x04000106)
#define REG_KEYINPUT  REG16(0x04000130)
#define REG_IE        REG16(0x04000200)
#define REG_IF        REG16(0x04000202)
#define REG_WAITCNT   REG16(0x04000204)
#define REG_IME       REG16(0x04000208)
#define BIOS_IF       (*(vu16 *)0x03007FF8)
#define IRQ_VECTOR    (*(void (**)(void))0x03007FFC)
#define MEM_PAL       ((u16 *)0x05000000)
#define MEM_VRAM      ((u16 *)0x06000000)
#define MEM_OAM       ((u16 *)0x07000000)
#define MEM_SRAM      ((vu8 *)0x0E000000)
typedef volatile u8 vu8;

#define DMA_ON        0x8000
#define DMA_32        0x0400
#define DMA_REPEAT    0x0200
#define DMA_SPECIAL   0x3000
#define DMA_DST_FIXED 0x0040
#define DMA_SRC_FIXED 0x0100

u16 *page;
u16 bgPal[256] EWRAM_BSS, objPal[256] EWRAM_BSS;
Obj oam[128] EWRAM_BSS;
volatile u32 vbCount;
static int shown;                     // the page on screen (0 or 1)
static int gameView;

// ── copying (DMA channel 3) ───────────────────────────────────────────────
void copy32(void *dst, const void *src, int words) {
    if (words <= 0) return;
    REG_DMA3SAD = (u32)src; REG_DMA3DAD = (u32)dst; REG_DMA3CNT = (u32)words | ((u32)(DMA_ON | DMA_32) << 16);
}
void fill32(void *dst, u32 v, int words) {
    static volatile u32 src;
    if (words <= 0) return;
    src = v;
    REG_DMA3SAD = (u32)&src; REG_DMA3DAD = (u32)dst; REG_DMA3CNT = (u32)words | ((u32)(DMA_ON | DMA_32 | DMA_SRC_FIXED) << 16);
}

// ── sound ─────────────────────────────────────────────────────────────────
// Two direct-sound channels, each fed straight from the cartridge by its own DMA: A plays the
// music (timer 1, 10000 a second), B the effects (timer 0, 16000 a second). Each sound has
// silence after it, so the refresh interrupt only has to count samples and stop (or, for the
// music, restart) the channel once it has played the lot.
#define FX_RELOAD  1049               // 16777216 / 1049 = 15994 samples a second
#define MUS_RELOAD 1678               // 16777216 / 1678 = 9998 samples a second
#define FX_STEP    (280896 * 256 / FX_RELOAD)    // samples per refresh, x256
#define MUS_STEP   (280896 * 256 / MUS_RELOAD)
static const s8 *musData; static u32 musLen, musPos; static int musOn;
static u32 fxLen, fxPos; static int fxOn;
static u16 sndBase;
IWRAM_CODE static void musRestart(void) {
    REG_DMA1CNT_H = 0;
    REG_SOUNDCNT_H = sndBase | 0x0800;                       // empty channel A's queue
    REG_DMA1SAD = (u32)musData; REG_DMA1DAD = REG_FIFO_A;
    REG_DMA1CNT_H = DMA_ON | DMA_SPECIAL | DMA_32 | DMA_REPEAT | DMA_DST_FIXED;
    musPos = 0; musOn = 1;
}
void sndMusic(const s8 *d, int len) { musData = d; musLen = (u32)len << 8; musRestart(); }
void sndMusicStop(void) { REG_DMA1CNT_H = 0; musOn = 0; REG_SOUNDCNT_H = sndBase | 0x0800; }
void sndMusicMute(int mute) {
    if (mute) sndBase &= ~0x0300; else sndBase |= 0x0300;    // channel A to both speakers, or to neither
    REG_SOUNDCNT_H = sndBase;
}
void sndFx(const s8 *d, int len) {
    REG_DMA2CNT_H = 0;
    REG_SOUNDCNT_H = sndBase | 0x8000;                       // empty channel B's queue
    REG_DMA2SAD = (u32)d; REG_DMA2DAD = REG_FIFO_B;
    REG_DMA2CNT_H = DMA_ON | DMA_SPECIAL | DMA_32 | DMA_REPEAT | DMA_DST_FIXED;
    fxLen = (u32)len << 8; fxPos = 0; fxOn = 1;
}
static void sndInit(void) {
    REG_SOUNDCNT_X = 0x0080;                                 // sound on
    REG_SOUNDCNT_L = 0;
    // A: full volume, both speakers, timer 1. B: full volume, both speakers, timer 0.
    sndBase = 0x0004 | 0x0008 | 0x0300 | 0x0400 | 0x3000;
    REG_SOUNDCNT_H = sndBase | 0x0800 | 0x8000;
    REG_TM0CNT_H = 0; REG_TM0CNT_L = (u16)(65536 - FX_RELOAD); REG_TM0CNT_H = 0x0080;
    REG_TM1CNT_H = 0; REG_TM1CNT_L = (u16)(65536 - MUS_RELOAD); REG_TM1CNT_H = 0x0080;
}

// ── the refresh interrupt (runs as ARM code from fast RAM) ───────────────
IWRAM_CODE void isr(void) {
    u16 f = REG_IF & REG_IE;
    if (f & 1) {
        vbCount++;
        if (musOn && (musPos += MUS_STEP) >= musLen) musRestart();          // the music loops
        if (fxOn && (fxPos += FX_STEP) >= fxLen) { REG_DMA2CNT_H = 0; fxOn = 0; }
    }
    REG_IF = f;
    BIOS_IF |= f;
}

// ── screen ────────────────────────────────────────────────────────────────
void platInit(void) {
    REG_WAITCNT = 0x4317;                                     // faster cartridge reads, with prefetch
    REG_DISPCNT = 0x0080;                                     // blank while setting up
    REG_BG2PA = 256; REG_BG2PB = 0; REG_BG2PC = 0; REG_BG2PD = 256; REG_BG2X = 0; REG_BG2Y = 0;
    for (int i = 0; i < 128; i++) { oam[i].a0 = 0x0200; oam[i].a1 = oam[i].a2 = oam[i].a3 = 0; }
    copy32(MEM_OAM, oam, 256);
    fill32(MEM_VRAM, 0, 0x14000 / 4);
    shown = 0; page = (u16 *)((u8 *)MEM_VRAM + 0xA000);     // draw on page 1 while page 0 shows
    // windows: during a match, sprites only over the field (window 0); the panel is outside it
    REG_WIN0H = (0 << 8) | VIEW_W; REG_WIN0V = (0 << 8) | SH;
    REG_WININ = 0x0004 | 0x0010 | 0x0020;                     // window 0: the picture, sprites, blending
    REG_WINOUT = 0x0004;                                      // outside: the picture only
    REG_BLDCNT = 0x0400 | 0x0040;                             // see-through sprites blend over the picture
    REG_BLDALPHA = 7 | (9 << 8);
    sndInit();
    IRQ_VECTOR = isr;
    REG_DISPSTAT = 0x0008;                                    // interrupt at each refresh
    REG_IE = 1; REG_IF = 0xFFFF; REG_IME = 1;
    REG_DISPCNT = 0x0004 | 0x0400 | 0x1000 | 0x0040;          // mode 4, the picture, sprites (1D)
}
void platGameView(int w) {               // window 0 = the left w columns: sprites (and blending) only there
    if (w && w != gameView) REG_WIN0H = (0 << 8) | w;
    gameView = w;
}
static void vblankWait(void) {
#if defined(__thumb__)
    __asm__ volatile("swi 0x05" ::: "r0", "r1", "r2", "r3", "memory");
#else
    __asm__ volatile("swi 0x050000" ::: "r0", "r1", "r2", "r3", "memory");
#endif
}
void platFlip(void) { platShow(1); }
void platShow(int swap) {
    vblankWait();
    shown ^= swap;
    REG_DISPCNT = 0x0004 | 0x0400 | 0x1000 | 0x0040 | (shown ? 0x0010 : 0) | (gameView ? 0x2000 : 0);
    copy32(MEM_OAM, oam, 256);
    copy32(MEM_PAL, bgPal, 128);
    copy32(MEM_PAL + 256, objPal, 128);
    page = (u16 *)((u8 *)MEM_VRAM + (shown ? 0 : 0xA000));
}
void platObjTiles(int tile, const void *src, int bytes) { copy32((u8 *)MEM_VRAM + 0x10000 + tile * 32, src, bytes / 4); }
u16 platKeys(void) { return (u16)(~REG_KEYINPUT & 0x03FF); }

// ── save memory: battery-backed SRAM, one byte at a time (it's on an 8-bit bus) ──
// These run from fast RAM: while the save memory is being read the cartridge's program
// can't be fetched.
IWRAM_CODE void sramRead(void *dst, int off, int n) { u8 *d = dst; for (int i = 0; i < n; i++) d[i] = MEM_SRAM[off + i]; }
IWRAM_CODE void sramWrite(int off, const void *src, int n) { const u8 *s = src; for (int i = 0; i < n; i++) MEM_SRAM[off + i] = s[i]; }
#endif
