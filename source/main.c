// ════════════════════════════════════════════════════════════════════════
//  QU33PH GBA — the original marker throwing game, and the Olympics
//
//  CONTROLS: D-pad + A on the menus, B = back.
//  In a match: D-pad left/right aims, hold A and let go to throw (B cancels),
//  L/R how the marker lands, SELECT (held) looks up the table, START pause.
//  Progress saves to the cartridge's battery-backed save memory.
// ════════════════════════════════════════════════════════════════════════
#include "qu.h"

int screen = S_TITLE, mode = M_SINGLE;
int kDown, kHeld, kUp;
static int sel, confirmT, confirmWhich = -1, highTab, resultsNewBest, lastCoinsGain;
static Btn B[40];

void goScreen(int s) { screen = s; sel = 0; confirmWhich = -1; if (s == S_PLINKO) plinkoEnter(); saveWrite(); }

// ── starting and ending matches ───────────────────────────────────────────
static void beginTurn(void) { orient = sv.orient; startMatch(); screen = S_PLAY; }
static void startSingle(void) { mode = M_SINGLE; score2[0] = score2[1] = 0; player = 0; beginTurn(); }
static void startTwo(void) { mode = M_TWO; score2[0] = score2[1] = 0; player = 0; p2Round = 1; orient = sv.orient; startMatch(); screen = S_HANDOFF; }
void startOlympicMatch(void) { mode = M_OLYMPICS; score2[0] = 0; player = 0; beginTurn(); }

// name entry → where to go after it, and which table gets the score
static int nameReturn = S_TITLE, nameScore; static HighEntry *nameList; static char nameBuf[9];
void nameEntry(int ret, HighEntry *list, int s2) { nameReturn = ret; nameList = list; nameScore = s2; strcpy(nameBuf, sv.name); screen = S_NAME; sel = 0; }
// put a score on a Top 10 (asks for a name the first time)
static void submitHigh(HighEntry *list, int s2, int ret) {
    if (!highQualifies(list, s2)) { screen = ret; return; }
    if (!sv.name[0]) { nameEntry(ret, list, s2); return; }
    highInsert(list, sv.name, s2);
    toast("NEW HIGH SCORE!", list == sv.high1p ? "1 PLAYER TOP 10" : "OLYMPICS TOP 10");
    screen = ret;
}
void olympicsDone(void) { trackGameEnd(0); saveWrite(); goScreen(S_TITLE); submitHigh(sv.highOly, olyTournamentTotal(), S_TITLE); }

static void matchFinished(void) {
    if (mode == M_SINGLE) {
        int best = sv.high1p[0].name[0] ? sv.high1p[0].score2 : 0, before = sv.coins;
        trackGameEnd(score2[0]);
        resultsNewBest = score2[0] > best && score2[0] > 0;
        if (resultsNewBest) addCoins(25);                       // new personal best: +25 coins (as on the website)
        lastCoinsGain = sv.coins - before;
        saveWrite();
        goScreen(S_RESULTS);
        submitHigh(sv.high1p, score2[0], S_RESULTS);
    } else if (mode == M_OLYMPICS) {
        trackGameEnd(score2[0]);
        olyAfterMatch(score2[0]);
        goScreen(S_OLY_BRACKET);
    } else {
        if (player == 0) { player = 1; startMatch(); screen = S_HANDOFF; return; }
        if (p2Round >= sv.twoRounds) { trackGameEnd(score2[0] > score2[1] ? score2[0] : score2[1]); saveWrite(); goScreen(S_RESULTS); return; }
        p2Round++; player = 0; startMatch(); screen = S_HANDOFF;
    }
}

// ── title: the logo, coins and the slot + PlinQu33ph on the left, the main menu down the right ──
#define TITLE_N 8
static const char *TITLE_ITEMS[TITLE_N] = { "1 PLAYER", "2 PLAYER", "OLYMPICS", "HIGH SCORES", "SETTINGS", "SLOT", "PLINQU33PH", "ARCADE" };
static void titleLayout(void) {
    for (int i = 0; i < 5; i++) { B[i] = (Btn){ 124, 8 + i * 30, 112, 25, "", 0, 0 }; strcpy(B[i].label, TITLE_ITEMS[i]); }
    for (int i = 5; i < 8; i++) { B[i] = (Btn){ 6, 88 + (i - 5) * 24, 112, 22, "", 0, 0 }; strcpy(B[i].label, TITLE_ITEMS[i]); }
    B[7].col = GOLD;
}
static void drawTitle(void) {
    fillScreen(DARK);
    drawLogo((124 - LOGO_W) / 2, 6);
    char s[32], a[10];
    if (sv.high1p[0].name[0]) {
        scoreStr(a, sv.high1p[0].score2);
        sprintf(s, "HIGH SCORE  %s  %s", a, sv.high1p[0].name); textS((124 - textSW(s)) / 2, 52, s, GREY);
    }
    sprintf(s, "%d", sv.coins);
    int w = COIN_W + 3 + textW(s, 1), x = (124 - w) / 2;
    coinCount(x, 68, 0);
    titleLayout();
    drawBtns(B, TITLE_N, sel);
}
// up/down within a column (wrapping), left/right hop between the columns
static void titleNav(void) {
    int right = sel < 5;
    if (kDown & KEY_DOWN) sel = right ? (sel + 1) % 5 : 5 + (sel - 4) % 3;
    if (kDown & KEY_UP) sel = right ? (sel + 4) % 5 : 5 + (sel - 3) % 3;
    if (kDown & (KEY_LEFT | KEY_RIGHT)) {
        int cy = B[sel].y + B[sel].h / 2, best = sel, bd = 1 << 30;
        for (int i = right ? 5 : 0; i < (right ? 8 : 5); i++) { int d = B[i].y + B[i].h / 2 - cy; if (d < 0) d = -d; if (d < bd) { bd = d; best = i; } }
        sel = best;
    }
}
static void inputTitle(void) {
    titleLayout();
    if (sel < 0 || sel >= TITLE_N) sel = 0;
    titleNav();
    switch ((kDown & KEY_A) ? sel : -1) {
        case 0: startSingle(); break;
        case 1: startTwo(); break;
        case 2: goScreen(S_OLY_SELECT); break;
        case 3: highTab = 0; goScreen(S_HIGHS); break;
        case 4: goScreen(S_SETTINGS); break;
        case 5: goScreen(S_SLOT); break;
        case 6: goScreen(S_PLINKO); break;
        case 7: goScreen(S_ARCADE); break;
    }
}

// ── high scores: 1 PLAYER / OLYMPICS / RECORD (L/R or left/right change page) ─
static const char *HIGH_TABS[3] = { "1 PLAYER", "OLYMPICS", "RECORD" };
static void drawHighs(void) {
    fillScreen(DARK);
    for (int t = 0; t < 3; t++) {                     // the three tabs along the top
        int x = 4 + t * 79, on = t == highTab;
        rect(x, 2, 74, 15, on ? C_GOLD : C_DIMEDGE); rect(x + 1, 3, 72, 13, on ? C_TOAST : C_PANEL);
        textS(x + (74 - textSW(HIGH_TABS[t])) / 2, 4, HIGH_TABS[t], on ? YELLOW : GREY);
    }
    if (highTab < 2) {
        HighEntry *l = highTab ? sv.highOly : sv.high1p;
        for (int i = 0; i < 10; i++) {
            char s[24], a[10]; int y = 20 + i * 14;
            sprintf(s, "%d", i + 1); text(24 - textW(s, 1), y, s, i < 3 ? GOLD : GREY, 1);
            text(40, y, l[i].name[0] ? l[i].name : "---", WHITE, 1);
            if (l[i].name[0]) { scoreStr(a, l[i].score2); text(SW - 24 - textW(a, 1), y, a, YELLOW, 1); }
        }
        if (highTab == 1) textS(SW - 4 - textSW("tournament total"), 150, "tournament total", GREY);
    } else {                                          // the career record and the coin record, side by side
        char s[24], a[10];
        scoreStr(a, sv.high1p[0].name[0] ? sv.high1p[0].score2 : 0);
        const char *L[12] = { "Games played", "High score", "QU33PHs landed", "MEGA QU33PHs", "PEEFs", "Games forfeited",
                              "Coins now", "Coins earned", "Coins spent", "Slot spins", "Slot wins", "Lost in slots" };
        int V[12] = { sv.games, 0, sv.qu33phs, sv.megas, sv.peefs, sv.forfeits, sv.coins, sv.coinsEarned, sv.coinsSpent, sv.slotSpins, sv.slotWins, sv.slotLost };
        for (int i = 0; i < 12; i++) {
            int x = i < 6 ? 6 : 124, y = 24 + (i % 6) * 15;
            textS(x, y, L[i], i < 6 ? WHITE : C_DIMWHITE);
            if (i == 1) strcpy(s, a); else sprintf(s, "%d", V[i]);
            textS(x + 110 - textSW(s), y, s, GOLD);
        }
        rect(120, 22, 1, 88, C_DIMEDGE);
        textC(118, "OLYMPIC MEDALS", GOLD, 1);
        const char *M[3] = { "Gold", "Silver", "Bronze" }; int MV[3] = { sv.gold, sv.silver, sv.bronze };
        const int MC[3] = { C_GOLD, C_SILVER, C_BRONZE };
        for (int i = 0; i < 3; i++) {
            int x = 8 + i * 78;
            for (int dy = -5; dy <= 5; dy++) for (int dx = -5; dx <= 5; dx++) if (dx * dx + dy * dy <= 25) pset(x + 5 + dx, 148 + dy, MC[i]);
            sprintf(s, "%s %d", M[i], MV[i]); textS(x + 14, 143, s, WHITE);
        }
    }
}
static void inputHighs(void) {
    if (kDown & (KEY_B | KEY_A)) { goScreen(S_TITLE); return; }
    if (kDown & (KEY_R | KEY_RIGHT)) highTab = (highTab + 1) % 3;
    if (kDown & (KEY_L | KEY_LEFT)) highTab = (highTab + 2) % 3;
}

// ── settings ──────────────────────────────────────────────────────────────
static const char *ORIENT_SHORT[3] = { "VERTICAL", "ANGLED", "FLAT" };
static void settingsLayout(void) {
    for (int i = 0; i < 10; i++) { B[i].x = 4 + (i % 2) * 118; B[i].y = 20 + (i / 2) * 28; B[i].w = 114; B[i].h = 24; B[i].col = 0; B[i].dim = 0; }
    sprintf(B[0].label, "MUSIC  %s", sv.musicOn ? "ON" : "OFF");
    sprintf(B[1].label, "SOUND FX  %s", sv.sfxOn ? "ON" : "OFF");
    sprintf(B[2].label, "1P TIMER  %ds", sv.timer1p);
    sprintf(B[3].label, "2P TIMER  %ds", sv.timer2p);
    sprintf(B[4].label, "2P ROUNDS  %d", sv.twoRounds);
    sprintf(B[5].label, "START  %s", ORIENT_SHORT[sv.orient]);
    sprintf(B[6].label, "NAME  %s", sv.name[0] ? sv.name : "---");
    strcpy(B[7].label, "BACK");
    strcpy(B[8].label, confirmWhich == 8 ? "SURE? PRESS A" : "RESET SCORES"); B[8].col = RED;
    strcpy(B[9].label, confirmWhich == 9 ? "SURE? PRESS A" : "RESET ALL"); B[9].col = RED;
}
static void drawSettings(void) {
    settingsLayout();
    fillScreen(DARK);
    textC(2, "SETTINGS", GOLD, 1);
    drawBtns(B, 10, sel);
}
static void inputSettings(void) {
    settingsLayout();
    if (confirmT > 0 && --confirmT == 0) confirmWhich = -1;
    if (kDown & KEY_B) { saveWrite(); goScreen(S_TITLE); return; }
    int h = btnInput(B, 10, &sel, 2);
    if (h < 0) return;
    switch (h) {
        case 0: musicToggle(); break;
        case 1: sv.sfxOn = !sv.sfxOn; break;
        case 2: sv.timer1p = sv.timer1p == 30 ? 45 : sv.timer1p == 45 ? 60 : 30; break;
        case 3: sv.timer2p = sv.timer2p == 15 ? 30 : sv.timer2p == 30 ? 60 : 15; break;
        case 4: sv.twoRounds = sv.twoRounds == 3 ? 5 : sv.twoRounds == 5 ? 10 : 3; break;
        case 5: sv.orient = (sv.orient + 1) % 3; break;
        case 6: nameEntry(S_SETTINGS, 0, 0); return;
        case 7: saveWrite(); goScreen(S_TITLE); return;
        case 8: case 9:
            if (confirmWhich == h) {
                if (h == 8) resetHighScores(); else { resetEverything(); musicStop(); musicStart(); }
                toast("DONE", h == 8 ? "high scores cleared" : "everything reset"); confirmWhich = -1;
            } else { confirmWhich = h; confirmT = 180; }
            break;
    }
    saveWrite();
}

// ── name entry: on-screen keyboard (D-pad + A; B deletes; START done) ─────
static const char KEYS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static void nameLayout(void) {
    for (int i = 0; i < 38; i++) { B[i].x = 5 + (i % 10) * 23; B[i].y = 58 + (i / 10) * 25; B[i].w = 22; B[i].h = 23; B[i].col = 0; B[i].dim = 0; }
    for (int i = 0; i < 36; i++) { B[i].label[0] = KEYS[i]; B[i].label[1] = 0; }
    strcpy(B[36].label, "DEL"); strcpy(B[37].label, "OK");
    B[36].w = 36; B[37].x = B[36].x + 40; B[37].w = 36; B[37].col = LIME;
}
static void drawName(void) {
    nameLayout();
    fillScreen(DARK);
    textS(5, 3, nameList ? "NEW HIGH SCORE!" : "YOUR NAME (up to 8)", GOLD);
    textS(SW - 5 - textSW("B delete  START done"), 3, "B delete  START done", GREY);
    box(56, 17, 128, 36, BLACK, WHITE);
    char s[12]; sprintf(s, "%s%s", nameBuf, (frameCount / 20) % 2 ? "_" : " "); textC(21, s, WHITE, 2);
    drawBtns(B, 38, sel);
}
static void inputName(void) {
    nameLayout();
    int len = strlen(nameBuf), done = 0;
    int h = btnInput(B, 38, &sel, 10);
    if (h >= 0 && h < 36 && len < 8) { nameBuf[len] = KEYS[h]; nameBuf[len + 1] = 0; }
    if ((h == 36 || (kDown & KEY_B)) && len > 0) nameBuf[len - 1] = 0;
    if (h == 37 || (kDown & KEY_START)) done = 1;
    if (done && nameBuf[0]) {
        strcpy(sv.name, nameBuf);
        if (nameList) { highInsert(nameList, sv.name, nameScore); toast("NEW HIGH SCORE!", sv.name); }
        saveWrite();
        int r = nameReturn; nameList = 0; screen = r; sel = 0;
    }
}

// ── results / hand-over / pause ───────────────────────────────────────────
static void drawResults(void) {
    char s[40], a[10], b[10];
    fillScreen(DARK);
    drawLogo((SW - LOGO_W) / 2, 6);
    if (mode == M_SINGLE) {
        scoreStr(a, score2[0]); sprintf(s, "FINAL SCORE  %s", a); textC(54, s, YELLOW, 2);
        if (resultsNewBest) textC(88, "NEW PERSONAL BEST!", LIME, 1);
        if (lastCoinsGain > 0) { sprintf(s, "+%d coins", lastCoinsGain); textC(104, s, GOLD, 1); }
    } else {
        scoreStr(a, score2[0]); scoreStr(b, score2[1]);
        sprintf(s, "P1  %s     P2  %s", a, b); textC(52, s, WHITE, 1);
        textC(72, score2[0] > score2[1] ? "PLAYER 1 WINS!" : score2[1] > score2[0] ? "PLAYER 2 WINS!" : "IT'S A TIE!", YELLOW, 2);
    }
    B[0] = (Btn){ 10, 120, 110, 28, "PLAY AGAIN", 0, 0 }; B[1] = (Btn){ 126, 120, 104, 28, "MENU", 0, 0 };
    drawBtns(B, 2, sel);
}
static void inputResults(void) {
    int h = btnInput(B, 2, &sel, 2);
    if (kDown & KEY_B) h = 1;
    if (h == 0) { if (mode == M_TWO) startTwo(); else startSingle(); }
    if (h == 1) goScreen(S_TITLE);
}
static void drawHandoff(void) {
    char s[32];
    fillScreen(DARK);
    drawLogo((SW - LOGO_W) / 2, 10);
    sprintf(s, "PLAYER %d", player + 1); textC(60, s, YELLOW, 2);
    sprintf(s, "ROUND %d / %d", p2Round, sv.twoRounds); textC(94, s, WHITE, 1);
    textC(118, "Pass the GBA", WHITE, 1);
    textC(136, "A to start", YELLOW, 1);
}

// ── main loop ─────────────────────────────────────────────────────────────
// Frames really shown are counted by the screen's own refresh (60 a second). If a busy frame
// takes longer than 1/60 s, the match catches up by running extra steps, so it keeps full speed.
int main(void) {
    platInit();
    uiPalette();
    saveInit();
#ifdef GBA_SIM
    if (getenv("SIM_COINS")) sv.coins = atoi(getenv("SIM_COINS"));     // (test harness only)
#endif
    gameInit();
    srand(0x51A);
    u32 vbLast = vbCount; u16 prev = 0;
    while (1) {
        frameCount++;
        u32 vbNow = vbCount; int steps = (int)(vbNow - vbLast); vbLast = vbNow;
        if (steps < 1) steps = 1;
        if (steps > 3) steps = 3;                                   // (never more than 3 catch-up steps)
        u16 k = platKeys();
        kDown = k & ~prev; kUp = ~k & prev; kHeld = k; prev = k;
        if (kDown) srand(rand() ^ frameCount);
        switch (screen) {
            case S_TITLE: inputTitle(); break;
            case S_PLAY:
                if (kDown & KEY_START) { screen = S_PAUSE; break; }
                matchInput(kDown, kHeld, kUp); for (int s = 0; s < steps && !matchOver; s++) matchUpdate();
                if (matchOver) matchFinished();
                break;
            case S_PAUSE:
                if (kDown & (KEY_START | KEY_A)) screen = S_PLAY;
                if (kDown & KEY_SELECT) { sv.forfeits++; saveWrite(); goScreen(S_TITLE); }
                break;
            case S_HANDOFF: if (kDown & (KEY_A | KEY_START)) screen = S_PLAY; break;
            case S_RESULTS: inputResults(); break;
            case S_HIGHS: inputHighs(); break;
            case S_SETTINGS: inputSettings(); break;
            case S_OLY_SELECT: inputOlySelect(); break;
            case S_OLY_BRACKET: inputOlyBracket(); break;
            case S_NAME: inputName(); break;
            case S_SLOT: inputSlot(); updateSlot(); break;
            case S_PLINKO: inputPlinko(); for (int s = 0; s < steps && screen == S_PLINKO; s++) updatePlinko(); break;
            case S_ARCADE: inputArcade(); break;
            case S_MINI_MENU: inputMiniMenu(); break;
            case S_MINI: case S_MINI_PAUSE: inputMini(); for (int s = 0; s < steps && screen == S_MINI; s++) updateMini(); break;
            case S_MINI_OVER: inputMiniOver(); break;
        }
        int inMatch = screen == S_PLAY || screen == S_PAUSE;
        int inMini = screen >= S_MINI_MENU && screen <= S_MINI_OVER, onTable = screen >= S_MINI && screen <= S_MINI_OVER;
        scenePalette(inMatch ? PAL_FIELD : screen == S_SLOT ? PAL_SLOT : onTable ? PAL_MINI : PAL_MENU);
        objSet(onTable ? OBJ_MINI : OBJ_MAIN);
        musicSet(inMini ? MUS_MINI : MUS_MAIN);              // the arcade game has its own music, menu to results
        platGameView(inMatch);
        if (!inMatch) hideSprites();
        clipAll();
        switch (screen) {
            case S_TITLE: drawTitle(); break;
            case S_PLAY: matchDraw(); break;
            case S_PAUSE:
                matchDraw(); hideSprites();
                box(28, 44, 120, 72, C_TOAST, GOLD);
                textCW(28, 120, 52, "PAUSED", YELLOW, 2);
                textS(40, 84, "START  resume", WHITE); textS(40, 98, "SELECT  quit to menu", WHITE);
                break;
            case S_HANDOFF: drawHandoff(); break;
            case S_RESULTS: drawResults(); break;
            case S_HIGHS: drawHighs(); break;
            case S_SETTINGS: drawSettings(); break;
            case S_OLY_SELECT: drawOlySelect(); break;
            case S_OLY_BRACKET: drawOlyBracket(); break;
            case S_NAME: drawName(); break;
            case S_SLOT: drawSlot(); break;
            case S_PLINKO: drawPlinko(); break;
            case S_ARCADE: drawArcade(); break;
            case S_MINI_MENU: drawMiniMenu(); break;
            case S_MINI: case S_MINI_PAUSE: drawMini(); break;
            case S_MINI_OVER: drawMiniOver(); break;
        }
        drawToast();
        platFlip();
    }
    return 0;
}
