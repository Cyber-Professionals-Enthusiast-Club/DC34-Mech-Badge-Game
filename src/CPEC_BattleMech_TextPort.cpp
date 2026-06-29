/*
  CPEC Mech Fighter Badge - Text Only Port v0.2
  Target: ESP32-S3 badge + Crystalfontz CFAF240320A0-024S
  Display: Adafruit ST7789 over SPI

  Confirmed badge pins:
    TFT_SCLK 36
    TFT_MOSI 35
    TFT_DC   26
    TFT_CS   38
    TFT_RST  21
    TFT_BL   47

    BTN_UP      7
    BTN_DOWN    10
    BTN_LEFT    8
    BTN_RIGHT   9
    BTN_SELECT  1
    BTN_START   2
    BTN_A       11
    BTN_B       14
*/

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <LittleFS.h>
#include <TJpg_Decoder.h>
#include "BadgeDisplay.h"

// ===================== HARDWARE CONFIG =====================
#define TFT_SCLK 36
#define TFT_MOSI 35
#define TFT_DC   26
#define TFT_CS   38
#define TFT_RST  21
#define TFT_BL   47

#define BTN_UP      7
#define BTN_DOWN    10
#define BTN_LEFT    8
#define BTN_RIGHT   9
#define BTN_SELECT  1
#define BTN_START   2
#define BTN_A       11
#define BTN_B       14

// Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

SPIClass displaySpi(FSPI);
Adafruit_ST7789 tft(&displaySpi, TFT_CS, TFT_DC, TFT_RST);

// Landscape after setRotation(1)
static const int SCREEN_W = 320;
static const int SCREEN_H = 240;

// ===================== INPUT =====================
enum ButtonEvent {
  EV_NONE,
  EV_UP,
  EV_DOWN,
  EV_LEFT,
  EV_RIGHT,
  EV_A,
  EV_B,
  EV_SELECT,
  EV_START
};

struct ButtonDef {
  int pin;
  ButtonEvent ev;
  const char* name;
  bool last;
  unsigned long lastChange;
};

ButtonDef buttons[] = {
  {BTN_UP,     EV_UP,     "UP",     true, 0},
  {BTN_DOWN,   EV_DOWN,   "DOWN",   true, 0},
  {BTN_LEFT,   EV_LEFT,   "LEFT",   true, 0},
  {BTN_RIGHT,  EV_RIGHT,  "RIGHT",  true, 0},
  {BTN_A,      EV_A,      "A",      true, 0},
  {BTN_B,      EV_B,      "B",      true, 0},
  {BTN_SELECT, EV_SELECT, "SELECT", true, 0},
  {BTN_START,  EV_START,  "START",  true, 0}
};

ButtonEvent readButtonEvent() {
  const unsigned long debounceMs = 45;
  unsigned long now = millis();

  for (unsigned int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
    bool cur = digitalRead(buttons[i].pin); // HIGH idle, LOW pressed
    if (cur != buttons[i].last && (now - buttons[i].lastChange) > debounceMs) {
      buttons[i].last = cur;
      buttons[i].lastChange = now;
      if (cur == LOW) {
        Serial.print("Button: ");
        Serial.println(buttons[i].name);
        return buttons[i].ev;
      }
    }
  }
  return EV_NONE;
}

// ===================== GAME DATA / ENGINE TYPES =====================
typedef enum { RANGE_MELEE = 0, RANGE_SHORT = 1, RANGE_MED = 2, RANGE_LONG = 3 } RangeBand;
typedef enum { ACT_CLOSE = 0, ACT_HOLD = 1, ACT_OPEN = 2 } RangeAction;

typedef enum {
  LOC_HEAD = 0, LOC_CT = 1, LOC_RT = 2, LOC_LT = 3,
  LOC_RA = 4, LOC_LA = 5, LOC_RL = 6, LOC_LL = 7, LOC_COUNT = 8
} Loc;

typedef struct { uint64_t state; } Rng;

static uint64_t splitmix64(uint64_t* x) {
  uint64_t z = (*x += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}
static void rng_seed(Rng* r, uint64_t seed) { r->state = seed; }
static uint32_t rng_u32(Rng* r) {
  uint64_t x = r->state;
  uint64_t z = splitmix64(&x);
  r->state = x;
  return (uint32_t)(z & 0xFFFFFFFFu);
}
static int rng_int(Rng* r, int lo, int hi) {
  if (lo > hi) { int t = lo; lo = hi; hi = t; }
  uint32_t span = (uint32_t)(hi - lo + 1);
  return lo + (int)(rng_u32(r) % span);
}
static int roll_d6(Rng* r) { return rng_int(r, 1, 6); }
static int roll_2d6(Rng* r) { return roll_d6(r) + roll_d6(r); }
static uint64_t make_seed(uint32_t idA, uint32_t idB, uint32_t salt) {
  uint64_t lo = (idA < idB) ? idA : idB;
  uint64_t hi = (idA < idB) ? idB : idA;
  uint64_t x = (hi << 32) ^ lo ^ ((uint64_t)salt << 1);
  uint64_t tmp = x;
  return splitmix64(&tmp);
}

typedef struct {
  char name[32];
  int gunnery;
  int piloting;
} Pilot;

typedef struct {
  char name[32];
  int damage;
  int heat;
  RangeBand minRange;
  RangeBand maxRange;
} Weapon;

typedef struct {
  char chassis[32];
  int speed;
  int heatSinks;
  int heat;
  int shutdown;
  int movePenalty;
  int armor[LOC_COUNT];
  int internal[LOC_COUNT];
  Weapon hardpoints[3];
} Mech;

typedef struct {
  RangeAction rangeAction;
  int fire[3];
} PlayerChoice;

typedef struct {
  char callsign[16];
  char mech[32];
  int rssi;
} Contact;

static const char* range_name(RangeBand r) {
  switch (r) {
    case RANGE_MELEE: return "MELEE";
    case RANGE_SHORT: return "SHORT";
    case RANGE_MED:   return "MEDIUM";
    case RANGE_LONG:  return "LONG";
    default: return "?";
  }
}
static const char* loc_name(Loc l) {
  switch (l) {
    case LOC_HEAD: return "Head";
    case LOC_CT: return "Center Torso";
    case LOC_RT: return "Right Torso";
    case LOC_LT: return "Left Torso";
    case LOC_RA: return "Right Arm";
    case LOC_LA: return "Left Arm";
    case LOC_RL: return "Right Leg";
    case LOC_LL: return "Left Leg";
    default: return "?";
  }
}
static int weapon_in_range(const Weapon* w, RangeBand range) {
  return ((int)range >= (int)w->minRange) && ((int)range <= (int)w->maxRange);
}
static int range_mod(RangeBand r) {
  switch (r) {
    case RANGE_LONG:  return 4;
    case RANGE_MED:   return 2;
    case RANGE_SHORT: return 0;
    case RANGE_MELEE: return -1;
    default: return 0;
  }
}
static int heat_to_hit_mod(int heat) {
  if (heat <= 4) return 0;
  if (heat <= 8) return 1;
  if (heat <= 12) return 2;
  return 3;
}
static int mech_alive(const Mech* m) {
  return (m->internal[LOC_CT] > 0) && (m->internal[LOC_HEAD] > 0);
}
static RangeBand shift_range(RangeBand cur, int delta) {
  int r = (int)cur + delta;
  if (r < (int)RANGE_MELEE) r = (int)RANGE_MELEE;
  if (r > (int)RANGE_LONG)  r = (int)RANGE_LONG;
  return (RangeBand)r;
}
static Loc roll_hit_location(Rng* rng) {
  int roll = roll_2d6(rng);
  switch (roll) {
    case 2: return LOC_HEAD;
    case 3:
    case 4: return LOC_RA;
    case 5: return LOC_RL;
    case 6: return LOC_RT;
    case 7: return LOC_CT;
    case 8: return LOC_LT;
    case 9: return LOC_LL;
    case 10:
    case 11: return LOC_LA;
    case 12: return LOC_CT;
    default: return LOC_CT;
  }
}
static int clamp_int(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}
static int mech_effective_speed(const Mech* m) {
  if (m->shutdown) return 0;
  int eff = m->speed - m->movePenalty;
  if (eff < 1) eff = 1;
  return eff;
}
static void apply_damage(Rng* rng, Mech* def, Loc loc, int dmg) {
  int idx = (int)loc;
  if (def->armor[idx] > 0) {
    int take = (def->armor[idx] < dmg) ? def->armor[idx] : dmg;
    def->armor[idx] -= take;
    dmg -= take;
  }
  if (dmg > 0) {
    def->internal[idx] -= dmg;
    int critRoll = roll_2d6(rng);
    if (critRoll >= 8) {
      if (loc == LOC_CT || loc == LOC_RT || loc == LOC_LT) {
        if (def->heatSinks > 0) def->heatSinks -= 1;
      } else if (loc == LOC_RL || loc == LOC_LL) {
        if (def->speed > 1) def->speed -= 1;
      } else if (loc == LOC_HEAD) {
        def->internal[LOC_HEAD] -= 1;
      }
    }
  }
}
static void sink_heat(Mech* m) {
  m->heat -= m->heatSinks;
  if (m->heat < 0) m->heat = 0;
}
static void heat_phase(Mech* m, const Pilot* p, Rng* rng) {
  if (m->shutdown) {
    int roll = roll_2d6(rng) + (6 - p->piloting);
    if (roll >= 8) m->shutdown = 0;
  }
  if (m->heat >= 13)      m->movePenalty = 2;
  else if (m->heat >= 10) m->movePenalty = 1;
  else                    m->movePenalty = 0;

  if (!m->shutdown) {
    if (m->heat >= 19) {
      m->shutdown = 1;
    } else if (m->heat >= 14) {
      int target = clamp_int(m->heat - 12, 2, 12);
      int roll = roll_2d6(rng);
      if (roll <= target) m->shutdown = 1;
    }
  }
}
static int determine_rear_advantage(int yourMoveRoll, int enemyMoveRoll, RangeAction yourAction, RangeAction enemyAction, Rng* rng) {
  int margin = yourMoveRoll - enemyMoveRoll;
  int youManeuver = (yourAction == ACT_CLOSE || yourAction == ACT_OPEN);
  int enManeuver = (enemyAction == ACT_CLOSE || enemyAction == ACT_OPEN);
  if (youManeuver && margin >= 5) {
    int d6 = roll_d6(rng);
    int needed = (margin >= 8) ? 3 : 4;
    if (d6 >= needed) return +1;
  }
  if (enManeuver && margin <= -5) {
    int d6 = roll_d6(rng);
    int needed = (margin <= -8) ? 3 : 4;
    if (d6 >= needed) return -1;
  }
  return 0;
}

static Weapon W_MED_LASER(void) { Weapon w; strcpy(w.name, "Medium Laser"); w.damage = 5; w.heat = 3; w.minRange = RANGE_SHORT; w.maxRange = RANGE_MED; return w; }
static Weapon W_PPC(void) { Weapon w; strcpy(w.name, "PPC"); w.damage = 10; w.heat = 10; w.minRange = RANGE_MED; w.maxRange = RANGE_LONG; return w; }
static Weapon W_AC5(void) { Weapon w; strcpy(w.name, "AC/5"); w.damage = 5; w.heat = 1; w.minRange = RANGE_SHORT; w.maxRange = RANGE_LONG; return w; }
static Weapon W_LRM10(void) { Weapon w; strcpy(w.name, "LRM-10"); w.damage = 10; w.heat = 4; w.minRange = RANGE_MED; w.maxRange = RANGE_LONG; return w; }
static Weapon W_SRM4(void) { Weapon w; strcpy(w.name, "SRM-4"); w.damage = 8; w.heat = 3; w.minRange = RANGE_MELEE; w.maxRange = RANGE_SHORT; return w; }

static Mech make_loki(void) {
  Mech m;
  strcpy(m.chassis, "SUMMONER [PRIME]");
  m.speed = 3; m.heatSinks = 6; m.heat = 0; m.shutdown = 0; m.movePenalty = 0;
  int armor[LOC_COUNT] = { 3, 18, 12, 12, 8, 8, 10, 10 };
  int internal[LOC_COUNT] = { 3, 12, 8, 8, 6, 6, 7, 7 };
  memcpy(m.armor, armor, sizeof(armor));
  memcpy(m.internal, internal, sizeof(internal));
  m.hardpoints[0] = W_PPC();
  m.hardpoints[1] = W_MED_LASER();
  m.hardpoints[2] = W_SRM4();
  return m;
}
static Mech make_bushwacker(void) {
  Mech m;
  strcpy(m.chassis, "BUSHWACKER [BSW-X1]");
  m.speed = 2; m.heatSinks = 5; m.heat = 0; m.shutdown = 0; m.movePenalty = 0;
  int armor[LOC_COUNT] = { 3, 20, 13, 13, 9, 9, 11, 11 };
  int internal[LOC_COUNT] = { 3, 13, 9, 9, 6, 6, 8, 8 };
  memcpy(m.armor, armor, sizeof(armor));
  memcpy(m.internal, internal, sizeof(internal));
  m.hardpoints[0] = W_AC5();
  m.hardpoints[1] = W_LRM10();
  m.hardpoints[2] = W_MED_LASER();
  return m;
}

// ===================== DISPLAY HELPERS =====================
void clearScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
}
void header(const char* title) {
  tft.fillRect(0, 0, SCREEN_W, 26, ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
  tft.setTextSize(2);
  tft.setCursor(6, 6);
  tft.print(title);
}
void footer(const char* text) {
  tft.fillRect(0, SCREEN_H - 18, SCREEN_W, 18, ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, SCREEN_H - 14);
  tft.print(text);
}
void printLine(int x, int y, const char* text, uint16_t color = ST77XX_WHITE, int size = 1) {
  tft.setTextColor(color, ST77XX_BLACK);
  tft.setTextSize(size);
  tft.setCursor(x, y);
  tft.print(text);
}
void drawMenuList(const char* title, const char* items[], int count, int selected, const char* foot) {
  clearScreen();
  header(title);
  tft.setTextSize(2);
  for (int i = 0; i < count; i++) {
    int y = 42 + i * 28;
    tft.setCursor(12, y);
    if (i == selected) {
      tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
      tft.print("> ");
      tft.print(items[i]);
      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(items[i], 0, 0, &x1, &y1, &w, &h);
    } else {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft.print("  ");
      tft.print(items[i]);
    }
  }
  footer(foot);
}

// ===================== UI STATE =====================
enum ScreenState {
  SCREEN_MAIN_MENU,
  SCREEN_RADAR,
  SCREEN_CONTACT,
  SCREEN_MECH_STATUS,
  SCREEN_PILOT_STATUS,
  SCREEN_OPTIONS,
  SCREEN_BATTLE_RANGE,
  SCREEN_BATTLE_WEAPON,
  SCREEN_BATTLE_RESULT,
  SCREEN_BATTLE_END
};

ScreenState screen = SCREEN_MAIN_MENU;
int menuIndex = 0;
int radarIndex = 0;
int contactIndex = 0;
int battleRangeIndex = 1; // Hold
int battleWeaponIndex = 3; // Alpha
int selectedContact = 0;

Contact contacts[] = {
  {"FOOFER", "BUSHWACKER [BSW-X1]", -62},
  {"HEXW0LF", "LOKI [PRIME]", -71}
};
const int CONTACT_COUNT = sizeof(contacts) / sizeof(contacts[0]);

Pilot youPilot;
Pilot enemyPilot;
Mech yourMech;
Mech enemyMech;
Rng battleRng;
RangeBand battleRange = RANGE_LONG;
int battleTurn = 0;
PlayerChoice pendingChoice;
char resultLines[8][38];
int resultCount = 0;

void addResult(const char* s) {
  if (resultCount < 8) {
    strncpy(resultLines[resultCount], s, 37);
    resultLines[resultCount][37] = '\0';
    resultCount++;
  }
}
void addResultFmt(const char* prefix, const char* a, const char* b = NULL) {
  char buf[38];
  if (b) snprintf(buf, sizeof(buf), "%s%s%s", prefix, a, b);
  else snprintf(buf, sizeof(buf), "%s%s", prefix, a);
  addResult(buf);
}

void drawMainMenu() {
  const char* items[] = {"RADAR", "Mech Status", "Pilot Status", "Options"};
  drawMenuList("IDLE SCREEN", items, 4, menuIndex, "UP/DOWN move  A select");
}
void drawRadar() {
  clearScreen();
  header("RADAR CONTACTS");
  tft.setTextSize(2);
  for (int i = 0; i < CONTACT_COUNT; i++) {
    int y = 44 + i * 42;
    tft.setCursor(10, y);
    if (i == radarIndex) tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
    else tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print(i == radarIndex ? "> " : "  ");
    tft.print(contacts[i].callsign);
    tft.setCursor(34, y + 18);
    if (i == radarIndex) tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
    else tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.print(contacts[i].mech);
    tft.print(" ");
    tft.print(contacts[i].rssi);
    tft.print("dBm");
  }
  footer("A contact  B back");
}
void drawContact() {
  const char* items[] = {"Engage", "Communicate", "Back"};
  clearScreen();
  header("CONTACT SELECTED");
  printLine(12, 36, contacts[selectedContact].callsign, ST77XX_YELLOW, 2);
  printLine(12, 58, contacts[selectedContact].mech, ST77XX_CYAN, 1);
  tft.setTextSize(2);
  for (int i = 0; i < 3; i++) {
    tft.setCursor(20, 92 + i * 30);
    if (i == contactIndex) tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
    else tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print(i == contactIndex ? "> " : "  ");
    tft.print(items[i]);
  }
  footer("A select  B back");
}
void drawMechStatus() {
  clearScreen();
  header("MECH STATUS");
  printLine(10, 42, "SUMMONER PRIME", ST77XX_YELLOW, 2);
  printLine(10, 76, "REACTOR: ONLINE", ST77XX_GREEN, 2);
  printLine(10, 104, "SENSORS: ONLINE", ST77XX_GREEN, 2);
  printLine(10, 132, "WEAPONS: ONLINE", ST77XX_GREEN, 2);
  printLine(10, 166, "ALL SYSTEMS NOMINAL", ST77XX_CYAN, 2);
  footer("B back");
}
void drawPilotStatus() {
  clearScreen();
  header("PILOT STATUS");
  printLine(10, 42, "Name: Jordan", ST77XX_WHITE, 2);
  printLine(10, 68, "Callsign: Wildcat", ST77XX_YELLOW, 2);
  printLine(10, 100, "Piloting: 1", ST77XX_CYAN, 2);
  printLine(10, 126, "Gunnery: 1", ST77XX_CYAN, 2);
  printLine(10, 158, "Faction: Clan Jade Falcon", ST77XX_GREEN, 1);
  printLine(10, 178, "XP: 69/420", ST77XX_GREEN, 2);
  footer("B back");
}
void drawOptions() {
  clearScreen();
  header("OPTIONS");
  printLine(10, 52, "Options Go Here...", ST77XX_WHITE, 2);
  printLine(10, 92, "Future:", ST77XX_YELLOW, 2);
  printLine(20, 122, "BLE callsign", ST77XX_CYAN, 1);
  printLine(20, 140, "QR leaderboard", ST77XX_CYAN, 1);
  printLine(20, 158, "SAO weapon scan", ST77XX_CYAN, 1);
  footer("B back");
}

void battleSetup(const char* target_callsign) {
  uint32_t badgeId = 0xA11CE001u;
  uint32_t oppId = 0;
  for (size_t i = 0; i < strlen(target_callsign); i++) oppId = (oppId * 131u) + (uint8_t)target_callsign[i];
  uint64_t seed = make_seed(badgeId, oppId, 12345);
  rng_seed(&battleRng, seed);

  strcpy(youPilot.name, "Jordan"); youPilot.gunnery = 2; youPilot.piloting = 5;
  strcpy(enemyPilot.name, "Enemy"); enemyPilot.gunnery = 3; enemyPilot.piloting = 5;
  yourMech = make_loki();
  enemyMech = make_bushwacker();
  if (strstr(target_callsign, "HEX") != NULL) {
    enemyMech = make_loki();
    strcpy(enemyPilot.name, "HEXW0LF");
  } else if (strstr(target_callsign, "FOO") != NULL) {
    enemyMech = make_bushwacker();
    strcpy(enemyPilot.name, "FOOFER");
  }
  battleRange = RANGE_LONG;
  battleTurn = 1;
  battleRangeIndex = 1;
  battleWeaponIndex = 3;
}

void drawBattleRange() {
  clearScreen();
  header("BATTLE MODE");
  char buf[64];
  snprintf(buf, sizeof(buf), "Turn %d  Range: %s", battleTurn, range_name(battleRange));
  printLine(8, 34, buf, ST77XX_YELLOW, 1);
  snprintf(buf, sizeof(buf), "YOU H:%d CT:%d HD:%d", yourMech.heat, yourMech.internal[LOC_CT], yourMech.internal[LOC_HEAD]);
  printLine(8, 52, buf, ST77XX_CYAN, 1);
  snprintf(buf, sizeof(buf), "EN  H:%d CT:%d HD:%d", enemyMech.heat, enemyMech.internal[LOC_CT], enemyMech.internal[LOC_HEAD]);
  printLine(8, 68, buf, ST77XX_RED, 1);

  const char* items[] = {"Close Range", "Hold Range", "Open Range"};
  tft.setTextSize(2);
  for (int i = 0; i < 3; i++) {
    tft.setCursor(18, 104 + i * 30);
    if (i == battleRangeIndex) tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
    else tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print(i == battleRangeIndex ? "> " : "  ");
    tft.print(items[i]);
  }
  footer("A select move  B flee/back");
}
void drawBattleWeapon() {
  clearScreen();
  header("SELECT WEAPON");
  char buf[64];
  snprintf(buf, sizeof(buf), "Range: %s", range_name(battleRange));
  printLine(8, 34, buf, ST77XX_YELLOW, 1);
  tft.setTextSize(1);
  for (int i = 0; i < 3; i++) {
    Weapon* w = &yourMech.hardpoints[i];
    int y = 60 + i * 32;
    if (i == battleWeaponIndex) tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
    else tft.setTextColor(weapon_in_range(w, battleRange) ? ST77XX_WHITE : ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(12, y);
    tft.print(i == battleWeaponIndex ? "> " : "  ");
    tft.print(w->name);
    tft.setCursor(34, y + 14);
    snprintf(buf, sizeof(buf), "DMG %d HEAT %d %s-%s", w->damage, w->heat, range_name(w->minRange), range_name(w->maxRange));
    tft.print(buf);
  }
  tft.setTextSize(2);
  tft.setCursor(12, 168);
  if (battleWeaponIndex == 3) tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
  else tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.print(battleWeaponIndex == 3 ? "> ALPHA STRIKE" : "  ALPHA STRIKE");
  tft.setCursor(12, 198);
  if (battleWeaponIndex == 4) tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
  else tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print(battleWeaponIndex == 4 ? "> HOLD FIRE" : "  HOLD FIRE");
  footer("A fire  B back");
}

PlayerChoice enemy_ai_choice(const Mech* enemy, RangeBand range) {
  PlayerChoice c;
  c.rangeAction = ACT_HOLD;
  for (int i = 0; i < 3; i++) c.fire[i] = 0;
  if (enemy->shutdown) return c;
  if (enemy->heat >= 9) c.rangeAction = ACT_OPEN;
  else if (range == RANGE_LONG) c.rangeAction = ACT_CLOSE;
  else c.rangeAction = ACT_HOLD;
  for (int i = 0; i < 3; i++) {
    const Weapon* w = &enemy->hardpoints[i];
    int inRange = weapon_in_range(w, range);
    int tooHot = (enemy->heat >= 8 && w->heat >= 6);
    if (inRange && !tooHot) c.fire[i] = 1;
  }
  if (!c.fire[0] && !c.fire[1] && !c.fire[2]) {
    for (int i = 0; i < 3; i++) {
      const Weapon* w = &enemy->hardpoints[i];
      if (weapon_in_range(w, range)) { c.fire[i] = 1; break; }
    }
  }
  return c;
}

void resolveOneAttack(const Pilot* attPilot, Mech* attMech, Mech* defMech, const PlayerChoice* choice, const char* attName) {
  int rMod = range_mod(battleRange);
  int hMod = heat_to_hit_mod(attMech->heat);
  int defEffSpeed = mech_effective_speed(defMech);
  int targetMoveMod = (defEffSpeed >= 4) ? 2 : ((defEffSpeed >= 3) ? 1 : 0);
  char buf[64];

  for (int i = 0; i < 3; i++) {
    if (!choice->fire[i]) continue;
    Weapon* w = &attMech->hardpoints[i];
    int inRange = weapon_in_range(w, battleRange);
    attMech->heat += w->heat;
    if (!inRange) {
      snprintf(buf, sizeof(buf), "%s %s OUT", attName, w->name);
      addResult(buf);
      continue;
    }
    int tn = 2 + attPilot->gunnery + rMod + hMod + targetMoveMod;
    int roll = roll_2d6(&battleRng);
    if (roll >= tn) {
      Loc loc = roll_hit_location(&battleRng);
      snprintf(buf, sizeof(buf), "%s %s HIT", attName, w->name);
      addResult(buf);
      snprintf(buf, sizeof(buf), "%s %d dmg", loc_name(loc), w->damage);
      addResult(buf);
      apply_damage(&battleRng, defMech, loc, w->damage);
    } else {
      snprintf(buf, sizeof(buf), "%s %s MISS", attName, w->name);
      addResult(buf);
    }
    if (!mech_alive(defMech)) break;
  }
}

void resolveTurn() {
  resultCount = 0;
  char buf[64];
  snprintf(buf, sizeof(buf), "TURN %d RESULTS", battleTurn);
  addResult(buf);

  PlayerChoice cYou = pendingChoice;
  PlayerChoice cEn = enemy_ai_choice(&enemyMech, battleRange);

  int initYou = roll_2d6(&battleRng) + mech_effective_speed(&yourMech);
  int initEnemy = roll_2d6(&battleRng) + mech_effective_speed(&enemyMech);
  snprintf(buf, sizeof(buf), "Init Y:%d E:%d", initYou, initEnemy);
  addResult(buf);

  int desireYou = (cYou.rangeAction == ACT_CLOSE) ? -1 : (cYou.rangeAction == ACT_OPEN) ? +1 : 0;
  int desireEn = (cEn.rangeAction == ACT_CLOSE) ? -1 : (cEn.rangeAction == ACT_OPEN) ? +1 : 0;
  int rollYou = roll_2d6(&battleRng) + mech_effective_speed(&yourMech) + (6 - youPilot.piloting);
  int rollEn = roll_2d6(&battleRng) + mech_effective_speed(&enemyMech) + (6 - enemyPilot.piloting);
  int rearAdv = determine_rear_advantage(rollYou, rollEn, cYou.rangeAction, cEn.rangeAction, &battleRng);

  if (rollYou > rollEn && desireYou != 0) battleRange = shift_range(battleRange, desireYou);
  else if (rollEn > rollYou && desireEn != 0) battleRange = shift_range(battleRange, desireEn);

  snprintf(buf, sizeof(buf), "Range -> %s", range_name(battleRange));
  addResult(buf);
  if (rearAdv == +1) addResult("You get rear arc");
  else if (rearAdv == -1) addResult("Enemy rear arc");

  int youFirst = (initYou >= initEnemy);
  if (youFirst) {
    resolveOneAttack(&youPilot, &yourMech, &enemyMech, &cYou, "YOU");
    if (mech_alive(&enemyMech)) resolveOneAttack(&enemyPilot, &enemyMech, &yourMech, &cEn, "EN");
  } else {
    resolveOneAttack(&enemyPilot, &enemyMech, &yourMech, &cEn, "EN");
    if (mech_alive(&yourMech)) resolveOneAttack(&youPilot, &yourMech, &enemyMech, &cYou, "YOU");
  }

  sink_heat(&yourMech);
  sink_heat(&enemyMech);
  heat_phase(&yourMech, &youPilot, &battleRng);
  heat_phase(&enemyMech, &enemyPilot, &battleRng);

  if (!mech_alive(&yourMech) || !mech_alive(&enemyMech) || battleTurn >= 30) {
    screen = SCREEN_BATTLE_END;
  } else {
    battleTurn++;
    screen = SCREEN_BATTLE_RESULT;
  }
}

void drawBattleResult() {
  clearScreen();
  header("TURN RESULT");
  tft.setTextSize(1);
  for (int i = 0; i < resultCount; i++) {
    printLine(8, 34 + i * 18, resultLines[i], ST77XX_WHITE, 1);
  }
  footer("A next turn  B exit");
}
void drawBattleEnd() {
  clearScreen();
  header("DUEL END");
  if (mech_alive(&yourMech) && !mech_alive(&enemyMech)) printLine(40, 80, "VICTORY!", ST77XX_GREEN, 3);
  else if (!mech_alive(&yourMech) && mech_alive(&enemyMech)) printLine(52, 80, "DEFEAT", ST77XX_RED, 3);
  else printLine(28, 80, "DRAW/TIMEOUT", ST77XX_YELLOW, 3);
  footer("A/B return to menu");
}

void redraw() {
  switch (screen) {
    case SCREEN_MAIN_MENU: drawMainMenu(); break;
    case SCREEN_RADAR: drawRadar(); break;
    case SCREEN_CONTACT: drawContact(); break;
    case SCREEN_MECH_STATUS: drawMechStatus(); break;
    case SCREEN_PILOT_STATUS: drawPilotStatus(); break;
    case SCREEN_OPTIONS: drawOptions(); break;
    case SCREEN_BATTLE_RANGE: drawBattleRange(); break;
    case SCREEN_BATTLE_WEAPON: drawBattleWeapon(); break;
    case SCREEN_BATTLE_RESULT: drawBattleResult(); break;
    case SCREEN_BATTLE_END: drawBattleEnd(); break;
  }
}

void handleEvent(ButtonEvent ev) {
  if (ev == EV_NONE) return;

  switch (screen) {
    case SCREEN_MAIN_MENU:
      if (ev == EV_UP) { menuIndex = (menuIndex + 3) % 4; redraw(); }
      else if (ev == EV_DOWN) { menuIndex = (menuIndex + 1) % 4; redraw(); }
      else if (ev == EV_A) {
        if (menuIndex == 0) { screen = SCREEN_RADAR; radarIndex = 0; }
        else if (menuIndex == 1) screen = SCREEN_MECH_STATUS;
        else if (menuIndex == 2) screen = SCREEN_PILOT_STATUS;
        else if (menuIndex == 3) screen = SCREEN_OPTIONS;
        redraw();
      }
      break;

    case SCREEN_RADAR:
      if (ev == EV_UP) { radarIndex = (radarIndex + CONTACT_COUNT - 1) % CONTACT_COUNT; redraw(); }
      else if (ev == EV_DOWN) { radarIndex = (radarIndex + 1) % CONTACT_COUNT; redraw(); }
      else if (ev == EV_B) { screen = SCREEN_MAIN_MENU; redraw(); }
      else if (ev == EV_A) { selectedContact = radarIndex; contactIndex = 0; screen = SCREEN_CONTACT; redraw(); }
      break;

    case SCREEN_CONTACT:
      if (ev == EV_UP) { contactIndex = (contactIndex + 2) % 3; redraw(); }
      else if (ev == EV_DOWN) { contactIndex = (contactIndex + 1) % 3; redraw(); }
      else if (ev == EV_B) { screen = SCREEN_RADAR; redraw(); }
      else if (ev == EV_A) {
        if (contactIndex == 0) { battleSetup(contacts[selectedContact].callsign); screen = SCREEN_BATTLE_RANGE; redraw(); }
        else if (contactIndex == 1) {
          clearScreen(); header("COMMS"); printLine(12, 70, "Opening channel...", ST77XX_CYAN, 2); footer("B back");
        } else { screen = SCREEN_RADAR; redraw(); }
      }
      break;

    case SCREEN_MECH_STATUS:
    case SCREEN_PILOT_STATUS:
    case SCREEN_OPTIONS:
      if (ev == EV_B || ev == EV_A) { screen = SCREEN_MAIN_MENU; redraw(); }
      break;

    case SCREEN_BATTLE_RANGE:
      if (ev == EV_UP) { battleRangeIndex = (battleRangeIndex + 2) % 3; redraw(); }
      else if (ev == EV_DOWN) { battleRangeIndex = (battleRangeIndex + 1) % 3; redraw(); }
      else if (ev == EV_B) { screen = SCREEN_CONTACT; redraw(); }
      else if (ev == EV_A) {
        pendingChoice.rangeAction = (battleRangeIndex == 0) ? ACT_CLOSE : (battleRangeIndex == 2) ? ACT_OPEN : ACT_HOLD;
        for (int i = 0; i < 3; i++) pendingChoice.fire[i] = 0;
        battleWeaponIndex = 3;
        screen = SCREEN_BATTLE_WEAPON;
        redraw();
      }
      break;

    case SCREEN_BATTLE_WEAPON:
      if (ev == EV_UP) { battleWeaponIndex = (battleWeaponIndex + 4) % 5; redraw(); }
      else if (ev == EV_DOWN) { battleWeaponIndex = (battleWeaponIndex + 1) % 5; redraw(); }
      else if (ev == EV_B) { screen = SCREEN_BATTLE_RANGE; redraw(); }
      else if (ev == EV_A) {
        for (int i = 0; i < 3; i++) pendingChoice.fire[i] = 0;
        if (battleWeaponIndex >= 0 && battleWeaponIndex <= 2) pendingChoice.fire[battleWeaponIndex] = 1;
        else if (battleWeaponIndex == 3) { pendingChoice.fire[0] = pendingChoice.fire[1] = pendingChoice.fire[2] = 1; }
        resolveTurn();
        redraw();
      }
      break;

    case SCREEN_BATTLE_RESULT:
      if (ev == EV_A) { screen = SCREEN_BATTLE_RANGE; redraw(); }
      else if (ev == EV_B) { screen = SCREEN_MAIN_MENU; redraw(); }
      break;

    case SCREEN_BATTLE_END:
      if (ev == EV_A || ev == EV_B) { screen = SCREEN_MAIN_MENU; redraw(); }
      break;
  }
}

// ===================== ARDUINO ENTRY =====================
void badgeSetup() {
  Serial.begin(115200);
  delay(1500);

 pinMode(BTN_UP, INPUT_PULLUP);
 pinMode(BTN_DOWN, INPUT_PULLUP);
 pinMode(BTN_LEFT, INPUT_PULLUP);
 pinMode(BTN_RIGHT, INPUT_PULLUP);
 pinMode(BTN_SELECT, INPUT_PULLUP);
 pinMode(BTN_START, INPUT_PULLUP);
 pinMode(BTN_A, INPUT_PULLUP);
 pinMode(BTN_B, INPUT_PULLUP);

 pinMode(TFT_BL, OUTPUT);
 digitalWrite(TFT_BL, HIGH);

  displaySpi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.init(240, 320);
  tft.setSPISpeed(8000000L);   // start conservative
  tft.setRotation(1);

  tft.fillScreen(ST77XX_BLACK);

tft.setTextColor(ST77XX_GREEN);
tft.setTextSize(2);
tft.setCursor(20, 80);
tft.println("CPEC - MECH FIGHTERS");

tft.setTextColor(ST77XX_CYAN);
tft.setCursor(45, 115);
tft.println("TEXT PORT v0.65");

delay(1200);
}

void badgeLoop() {
  ButtonEvent ev = readButtonEvent();
  handleEvent(ev);
  delay(5);
}
