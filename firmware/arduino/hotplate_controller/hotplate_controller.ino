#include <Arduino.h>
#include <SPI.h>
#include <math.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>   // save settings in flash (ESP32 NVS)

// ========= SCREEN =========
static constexpr int W = 320;
static constexpr int H = 240;
static constexpr uint8_t TFT_ROT = 3; // your working rotation
TFT_eSPI tft;

// ========= PINS =========
static constexpr int PIN_NTC = 35; // ADC input-only
static constexpr int PIN_SSR = 27; // SSR/MOSFET output (relay drive)

// Touch pins (common CYD)
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// If SSR polarity is backwards, flip this:
static constexpr bool SSR_ACTIVE_HIGH = true;

// ========= NTC =========
static constexpr float NTC_R0   = 100000.0f;
static constexpr float NTC_BETA = 3950.0f;
static constexpr float NTC_T0K  = 273.15f + 25.0f;
static constexpr float R_FIXED  = 10000.0f;
static constexpr float VREF     = 3.3f;

// ========= TOUCH CAL =========
// If edge taps are still off, these 4 values are the next thing to recalibrate.
static constexpr int XMIN = 200, XMAX = 3700;
static constexpr int YMIN = 240, YMAX = 3800;

// ========= CONTROL =========
static float setC = 100.0f;
static bool heatOn = false;         // USER "enabled" in Manual mode
static float tempC = 0.0f;          // display/control temp (offset applied)
static float tempSensorC = 0.0f;    // filtered sensor temp (raw, no offset)
static bool sensorOK = false;
static constexpr float MIN_SET = 0.0f;
static constexpr float MAX_SET = 450.0f;
static constexpr float HYST = 2.0f; // kept (not used by burst logic)

// Display SET number (manual set or reflow set/peak)
static float activeSetForDisplay = 100.0f;

// ========= MODE =========
enum Mode { MODE_MANUAL, MODE_REFLOW, MODE_SETTINGS };
static Mode mode = MODE_MANUAL;

// ========= REFLOW =========
static bool reflowRunning = false;  // USER "enabled" in Reflow mode
static uint32_t reflowStartMs = 0;
static float peakC = 240.0f; // default peak
static constexpr float REFLOW_TOTAL_S = 420.0f;
static constexpr float PLOT_STEP_S = 5.0f;

// program time that can PAUSE (this is what we use instead of wall-clock time)
static float    reflowProgS = 0.0f;
static uint32_t reflowLastTickMs = 0;

// allowed lag before pausing (degC)
static constexpr float REFLOW_SYNC_BAND_C = 3.0f;

// ========= UI LAYOUT =========
static constexpr int TAB_H  = 28;     // tab bar height
static constexpr int INFO_Y = 72;     // info line below values
static constexpr int PLOT_Y = 86;     // plot starts below info line

// LEFT column width for buttons
static constexpr int SIDE_W = 110;

// Plot moved right (left column for buttons)
static constexpr int PLOT_X = SIDE_W + 10;
static constexpr int PLOT_W = W - PLOT_X - 10;
static constexpr int PLOT_H = H - PLOT_Y - 6;

struct Rect { int x,y,w,h; };

// Tabs: Manual & Reflow wide, Settings tiny (gear icon)
static constexpr int SET_TAB_W   = 36;
static constexpr int MAIN_TABS_W = W - SET_TAB_W;
static constexpr int MAN_W       = MAIN_TABS_W / 2;
static constexpr int REF_W       = MAIN_TABS_W - MAN_W;

static Rect tabManualR = {0,              0, MAN_W,     TAB_H};
static Rect tabReflowR = {MAN_W,          0, REF_W,     TAB_H};
static Rect tabSetR    = {MAN_W + REF_W,  0, SET_TAB_W, TAB_H};

static Rect btnMinusR, btnHeatR, btnPlusR;

// ========= OUTPUT (SSR/MOSFET) STATE =========
static bool relayOn = false;                // actual GPIO27 drive state
static bool lastRelayOnDrawn = false;

// Small dot indicator for GPIO27 output state (green=ON, red=OFF)
static constexpr int RELAY_DOT_X = W - 14;
static constexpr int RELAY_DOT_Y = 38;
static constexpr int RELAY_DOT_R = 7;

// ========= SENSOR DEBOUNCE =========
static constexpr uint8_t SENSOR_BAD_TRIP     = 4;
static constexpr uint8_t SENSOR_GOOD_RECOVER = 2;
static uint8_t sensorBadCnt  = 0;
static uint8_t sensorGoodCnt = 0;

// ========= STATE FOR NO-FLICKER REDRAW =========
static Mode  lastModeDrawn      = MODE_MANUAL;
static bool  lastHeatOnDrawn    = false;
static bool  lastReflowRunDrawn = false;
static bool  lastSensorDrawn    = true;
static float lastSetDrawn       = -999.0f;
static float lastTempDrawn      = -999.0f;

// ========= PLOT STATE =========
static int lastProgIdx = -1;
static int lastDotX = -1, lastDotY = -1;
static int lastDotIdx = -1;

// ========= MANUAL HISTORY PLOT =========
static float manualHist[PLOT_W];
static bool manualHistInit = false;

// ========= Manual ramp + hold =========
static float    manualRampSetC   = 25.0f;   // internal ramped setpoint
static uint32_t manualRampLastMs = 0;

static constexpr float MANUAL_RAMP_CPS    = 0.6f; // degC per second ramp rate
static constexpr float MANUAL_SYNC_BAND_C = 3.0f; // pause ramp if behind more than this band

// ========= Burst PWM settings (saved) =========
static float    APPROACH_BAND_C = 25.0f;   // start pulsing within this many °C of target
static uint32_t WINDOW_MS       = 2000;    // burst window (ms), good for zero-cross SSR

// Max power limit to prevent runaway with thermal lag (saved)
static float MAX_DUTY = 0.40f;             // 0.10..1.00 (40% default)

// ========= Temp offset (saved) =========
static float TEMP_OFFSET_C = 0.0f;         // board temp correction
static constexpr float OFS_MIN  = -50.0f;
static constexpr float OFS_MAX  =  50.0f;
static constexpr float OFS_STEP =  1.0f;

static Preferences prefs;

static constexpr float    BAND_MIN  = 5.0f;
static constexpr float    BAND_MAX  = 80.0f;
static constexpr float    BAND_STEP = 5.0f;

static constexpr uint32_t WIN_MIN  = 500;
static constexpr uint32_t WIN_MAX  = 5000;
static constexpr uint32_t WIN_STEP = 250;

static constexpr float PWR_MIN  = 0.10f;
static constexpr float PWR_MAX  = 1.00f;
static constexpr float PWR_STEP = 0.05f;   // 5%

static constexpr int SETTINGS_COUNT = 4;

// settings selection (0=Approach, 1=Window, 2=Max power, 3=Temp offset)
static int settingsSel = 0;
static Rect settingsNextR;

// ========= UTIL =========
static float clampf(float v, float lo, float hi){
  if(v < lo) return lo;
  if(v > hi) return hi;
  return v;
}

static bool inRect(const Rect& r, int x, int y){
  return (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
}

static int median3(int a, int b, int c){
  if(a > b){ int t = a; a = b; b = t; }
  if(b > c){ int t = b; b = c; c = t; }
  if(a > b){ int t = a; a = b; b = t; }
  return b;
}

static void ssrWrite(bool on){
  if(SSR_ACTIVE_HIGH) digitalWrite(PIN_SSR, on ? HIGH : LOW);
  else                digitalWrite(PIN_SSR, on ? LOW : HIGH);
}

// load/save settings (ESP32 NVS)
static void loadSettings(){
  prefs.begin("hotplate", true);
  APPROACH_BAND_C = prefs.getFloat("band", APPROACH_BAND_C);
  WINDOW_MS       = prefs.getUInt("win",  WINDOW_MS);
  MAX_DUTY        = prefs.getFloat("pwr", MAX_DUTY);
  TEMP_OFFSET_C   = prefs.getFloat("ofs", TEMP_OFFSET_C);
  prefs.end();

  APPROACH_BAND_C = clampf(APPROACH_BAND_C, BAND_MIN, BAND_MAX);
  if(WINDOW_MS < WIN_MIN) WINDOW_MS = WIN_MIN;
  if(WINDOW_MS > WIN_MAX) WINDOW_MS = WIN_MAX;
  MAX_DUTY = clampf(MAX_DUTY, PWR_MIN, PWR_MAX);
  TEMP_OFFSET_C = clampf(TEMP_OFFSET_C, OFS_MIN, OFS_MAX);
}

static void saveSettings(){
  prefs.begin("hotplate", false);
  prefs.putFloat("band", APPROACH_BAND_C);
  prefs.putUInt("win", WINDOW_MS);
  prefs.putFloat("pwr", MAX_DUTY);
  prefs.putFloat("ofs", TEMP_OFFSET_C);
  prefs.end();
}

// shared heater function (Manual + Reflow)
static bool computeRelayBurst(bool requestHeat, bool sensorOK, float tempC, float activeSet, uint32_t nowMs){
  if(!requestHeat || !sensorOK) return false;

  float err = activeSet - tempC; // + = needs heat
  if(err <= 0.0f) return false;  // at/above target -> OFF

  float duty = err / APPROACH_BAND_C;
  duty = clampf(duty, 0.0f, MAX_DUTY);

  if(WINDOW_MS == 0) return false;
  uint32_t phase = nowMs % WINDOW_MS;
  return phase < (uint32_t)(duty * (float)WINDOW_MS);
}

// Safer thermistor read
static bool readThermistor(float &outC){
  int raw = analogRead(PIN_NTC);
  if(raw <= 0 || raw >= 4095) return false;

  float v = (raw / 4095.0f) * VREF;
  v = clampf(v, 0.001f, VREF - 0.001f);

  float rNTC = R_FIXED * (v / (VREF - v));
  float invT = (1.0f / NTC_T0K) + (1.0f / NTC_BETA) * logf(rNTC / NTC_R0);
  float tK = 1.0f / invT;

  outC = tK - 273.15f;
  if(outC < -50.0f || outC > 450.0f){
    outC = 0.0f;
    return false;
  }
  return true;
}

// ROT=3 mapping with 3-sample median filter for better touch stability
static bool getTouchXY(int &x, int &y){
  // Fast exit when IRQ says no touch; still falls back to touched() if needed
  if(!touchscreen.tirqTouched() && !touchscreen.touched()) return false;
  if(!touchscreen.touched()) return false;

  TS_Point p1 = touchscreen.getPoint();

  delay(4);
  if(!touchscreen.touched()) return false;
  TS_Point p2 = touchscreen.getPoint();

  delay(4);
  if(!touchscreen.touched()) return false;
  TS_Point p3 = touchscreen.getPoint();

  int rawX = median3(p1.y, p2.y, p3.y);
  int rawY = median3(p1.x, p2.x, p3.x);

  x = map(rawX, YMIN, YMAX, W - 1, 0);
  y = map(rawY, XMIN, XMAX, 0, H - 1);

  x = (int)clampf((float)x, 0, W - 1);
  y = (int)clampf((float)y, 0, H - 1);
  return true;
}

// ========= REFLOW CURVE =========
static float reflowSetpoint(float tsec){
  float preheatC = peakC * 0.625f;
  float soakC    = peakC * 0.75f;

  preheatC = clampf(preheatC, 110.0f, peakC - 70.0f);
  soakC    = clampf(soakC, preheatC + 10.0f, peakC - 20.0f);

  if(tsec < 90.0f)  return 25.0f + (preheatC - 25.0f) * (tsec / 90.0f);
  if(tsec < 180.0f) return preheatC + (soakC - preheatC) * ((tsec - 90.0f) / 90.0f);
  if(tsec < 240.0f) return soakC + (peakC - soakC) * ((tsec - 180.0f) / 60.0f);
  if(tsec < 270.0f) return peakC;
  if(tsec < 420.0f) return peakC + (50.0f - peakC) * ((tsec - 270.0f) / 150.0f);
  return 25.0f;
}

static const char* reflowStage(float tsec){
  if(tsec < 90)  return "Preheat";
  if(tsec < 180) return "Soak";
  if(tsec < 240) return "Ramp";
  if(tsec < 270) return "Peak";
  if(tsec < 420) return "Cool";
  return "Done";
}

// ========= DRAWING =========
static void layoutButtons(){
  int marginX = 10;
  int x = marginX;
  int wFull = SIDE_W - 2*marginX;
  int gapX = 8;
  int gapY = 12;

  int yTop = PLOT_Y;
  int availH = (H - 6) - PLOT_Y;

  int rowH = (int)clampf((float)(availH / 3), 40.0f, 60.0f);
  int yRow = yTop + (availH - rowH);

  int heatH = yRow - yTop - gapY;
  if(heatH < 40) heatH = 40;

  btnHeatR = { x, yTop, wFull, heatH };

  int wEach = (wFull - gapX) / 2;
  btnMinusR = { x, yRow, wEach, rowH };
  btnPlusR  = { x + wEach + gapX, yRow, wEach, rowH };
}

static void drawGearIcon(int cx, int cy, int r, uint16_t col){
  tft.drawCircle(cx, cy, r, col);
  tft.drawCircle(cx, cy, r - 3, col);
  for(int i=0;i<8;i++){
    float a = i * (3.14159265f / 4.0f);
    int x0 = cx + (int)((r - 1) * cosf(a));
    int y0 = cy + (int)((r - 1) * sinf(a));
    int x1 = cx + (int)((r + 2) * cosf(a));
    int y1 = cy + (int)((r + 2) * sinf(a));
    tft.drawLine(x0, y0, x1, y1, col);
  }
}

static void drawTabs(){
  uint16_t mFill = (mode == MODE_MANUAL)   ? TFT_DARKGREY : TFT_BLACK;
  uint16_t rFill = (mode == MODE_REFLOW)   ? TFT_DARKGREY : TFT_BLACK;
  uint16_t sFill = (mode == MODE_SETTINGS) ? TFT_DARKGREY : TFT_BLACK;

  tft.fillRect(tabManualR.x, tabManualR.y, tabManualR.w, tabManualR.h, mFill);
  tft.fillRect(tabReflowR.x, tabReflowR.y, tabReflowR.w, tabReflowR.h, rFill);
  tft.fillRect(tabSetR.x,    tabSetR.y,    tabSetR.w,    tabSetR.h,    sFill);

  tft.drawFastHLine(0, TAB_H-1, W, TFT_WHITE);
  tft.drawFastVLine(tabReflowR.x, 0, TAB_H, TFT_WHITE);
  tft.drawFastVLine(tabSetR.x,    0, TAB_H, TFT_WHITE);

  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);

  tft.setTextColor(TFT_WHITE, mFill);
  tft.drawString("MANUAL", tabManualR.x + tabManualR.w/2, TAB_H/2);

  tft.setTextColor(TFT_WHITE, rFill);
  tft.drawString("REFLOW", tabReflowR.x + tabReflowR.w/2, TAB_H/2);

  int gx = tabSetR.x + tabSetR.w/2;
  int gy = TAB_H/2;
  drawGearIcon(gx, gy, 7, TFT_WHITE);
}

static void drawButton(const Rect& r, const char* text, uint16_t fill, uint16_t border){
  tft.fillRoundRect(r.x, r.y, r.w, r.h, 10, fill);
  tft.drawRoundRect(r.x, r.y, r.w, r.h, 10, border);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, fill);
  tft.drawString(text, r.x + r.w/2, r.y + r.h/2);
}

static void drawBottomBar(){
  tft.fillRect(0, PLOT_Y, SIDE_W, H - PLOT_Y, TFT_BLACK);
  tft.drawFastVLine(SIDE_W, PLOT_Y, H - PLOT_Y, TFT_DARKGREY);

  if(mode == MODE_MANUAL){
    drawButton(btnHeatR,  "HEAT", heatOn ? TFT_GREEN : TFT_DARKGREY, TFT_WHITE);
    drawButton(btnMinusR, "-",    TFT_DARKGREY, TFT_WHITE);
    drawButton(btnPlusR,  "+",    TFT_DARKGREY, TFT_WHITE);
  } else if(mode == MODE_REFLOW) {
    drawButton(btnHeatR,  "HEAT", reflowRunning ? TFT_RED : TFT_DARKGREY, TFT_WHITE);
    drawButton(btnMinusR, "-",    TFT_DARKGREY, TFT_WHITE);
    drawButton(btnPlusR,  "+",    TFT_DARKGREY, TFT_WHITE);
  } else {
    drawButton(btnHeatR,  "SAVE", TFT_BLUE, TFT_WHITE);
    drawButton(btnMinusR, "-",    TFT_DARKGREY, TFT_WHITE);
    drawButton(btnPlusR,  "+",    TFT_DARKGREY, TFT_WHITE);
  }
}

static void drawRelayDot(bool on){
  int cx = RELAY_DOT_X;
  int cy = RELAY_DOT_Y;
  int r  = RELAY_DOT_R;

  tft.fillRect(cx - r - 2, cy - r - 2, (r + 2) * 2, (r + 2) * 2, TFT_BLACK);
  tft.fillCircle(cx, cy, r, on ? TFT_GREEN : TFT_RED);
  tft.drawCircle(cx, cy, r, TFT_WHITE);
}

static void drawValues(){
  tft.fillRect(10, 30, W - 44, 40, TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(String("TEMP: ") + String(tempC, 1) + " C", 10, 30);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(String("SET : ") + String(activeSetForDisplay, 0) + " C", 10, 50);

  bool heatEnabledShown = (mode == MODE_MANUAL) ? heatOn : reflowRunning;

  tft.setTextDatum(TR_DATUM);
  int rightX = W - 34;

  tft.setTextColor(heatEnabledShown ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.drawString(heatEnabledShown ? "HEAT : ON " : "HEAT : OFF", rightX, 30);

  tft.setTextColor(sensorOK ? TFT_CYAN : TFT_ORANGE, TFT_BLACK);
  tft.drawString(sensorOK ? "SENSOR: OK" : "SENSOR: BAD", rightX, 50);
}

// ---- Plot mapping (reflow) ----
static int mapTimeToX(float tsec){
  float x = (tsec / REFLOW_TOTAL_S) * (float)(PLOT_W - 1);
  return PLOT_X + (int)clampf(x, 0, (float)(PLOT_W - 1));
}

static int plotYmax(){
  return (int)clampf(peakC + 10.0f, 200.0f, 450.0f);
}

static int mapTempToY(float tc){
  float tMin = 25.0f;
  float tMax = (float)plotYmax();
  float norm = (tc - tMin) / (tMax - tMin);
  norm = clampf(norm, 0.0f, 1.0f);
  return PLOT_Y + (PLOT_H - 1) - (int)(norm * (float)(PLOT_H - 1));
}

static void drawPlotFrame(){
  tft.fillRect(PLOT_X, PLOT_Y, PLOT_W, PLOT_H, TFT_BLACK);
  tft.drawRect(PLOT_X, PLOT_Y, PLOT_W, PLOT_H, TFT_DARKGREY);
}

static void drawPlotInfo(float tsec){
  tft.fillRect(10, INFO_Y, W-20, 12, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if(reflowRunning){
    tft.drawString(String(reflowStage(tsec)) + " t=" + String((int)tsec) + "s", 10, INFO_Y);
  } else {
    tft.drawString(String("Peak=") + String((int)peakC) + "C", 10, INFO_Y);
  }
}

static void drawReflowBlueCurveOnce(){
  drawPlotFrame();
  int lastX = mapTimeToX(0);
  int lastY = mapTempToY(reflowSetpoint(0));
  for(float tt = PLOT_STEP_S; tt <= REFLOW_TOTAL_S; tt += PLOT_STEP_S){
    int x = mapTimeToX(tt);
    int y = mapTempToY(reflowSetpoint(tt));
    tft.drawLine(lastX, lastY, x, y, TFT_BLUE);
    lastX = x;
    lastY = y;
  }
  lastProgIdx = 0;
  lastDotX = lastDotY = -1;
  lastDotIdx = -1;
  drawPlotInfo(0.0f);
}

static void redrawSegmentForIdx(int idx){
  int points = (int)(REFLOW_TOTAL_S / PLOT_STEP_S) + 1;
  if(idx < 0 || idx >= points-1) return;
  float t0 = idx * PLOT_STEP_S;
  float t1 = (idx + 1) * PLOT_STEP_S;
  int x0 = mapTimeToX(t0);
  int y0 = mapTempToY(reflowSetpoint(t0));
  int x1 = mapTimeToX(t1);
  int y1 = mapTempToY(reflowSetpoint(t1));
  tft.drawLine(x0, y0, x1, y1, TFT_BLUE);
  if(idx < lastProgIdx) tft.drawLine(x0, y0, x1, y1, TFT_RED);
}

static void updateDot(int idxNow){
  if(lastDotX >= 0 && lastDotY >= 0){
    tft.fillCircle(lastDotX, lastDotY, 4, TFT_BLACK);
    redrawSegmentForIdx(lastDotIdx - 1);
    redrawSegmentForIdx(lastDotIdx);
    redrawSegmentForIdx(lastDotIdx + 1);
  }
  float t = idxNow * PLOT_STEP_S;
  int cx = mapTimeToX(t);
  int cy = mapTempToY(reflowSetpoint(t));
  tft.fillCircle(cx, cy, 3, TFT_WHITE);
  lastDotX = cx;
  lastDotY = cy;
  lastDotIdx = idxNow;
}

static void drawReflowProgressIncrement(float tsec){
  int points = (int)(REFLOW_TOTAL_S / PLOT_STEP_S) + 1;
  int idxNow = (int)floorf(clampf(tsec / PLOT_STEP_S, 0.0f, (float)(points-1)));
  if(idxNow > lastProgIdx){
    for(int i = lastProgIdx; i < idxNow; i++){
      float t0 = i * PLOT_STEP_S;
      float t1 = (i+1) * PLOT_STEP_S;
      int x0 = mapTimeToX(t0);
      int y0 = mapTempToY(reflowSetpoint(t0));
      int x1 = mapTimeToX(t1);
      int y1 = mapTempToY(reflowSetpoint(t1));
      tft.drawLine(x0, y0, x1, y1, TFT_RED);
    }
    lastProgIdx = idxNow;
  }
  updateDot(idxNow);
  drawPlotInfo(tsec);
}

// ========= MANUAL PLOT =========
static int mapManualTempToY(float tc){
  float tMin = 25.0f;
  float tMax = 450.0f;
  float norm = (tc - tMin) / (tMax - tMin);
  norm = clampf(norm, 0.0f, 1.0f);
  return PLOT_Y + (PLOT_H - 1) - (int)(norm * (float)(PLOT_H - 1));
}

static void manualPlotInitNow(){
  float v = tempC;
  for(int i=0;i<PLOT_W;i++) manualHist[i] = v;
  manualHistInit = true;
}

static void manualPlotPush(float v){
  for(int i=0;i<PLOT_W-1;i++) manualHist[i] = manualHist[i+1];
  manualHist[PLOT_W-1] = v;
}

static void drawManualPlot(){
  drawPlotFrame();

  int ySet = mapManualTempToY(setC);
  tft.drawFastHLine(PLOT_X+1, ySet, PLOT_W-2, TFT_DARKGREY);

  int lastX = PLOT_X;
  int lastY = mapManualTempToY(manualHist[0]);
  for(int i=1;i<PLOT_W;i++){
    int x = PLOT_X + i;
    int y = mapManualTempToY(manualHist[i]);
    tft.drawLine(lastX, lastY, x, y, TFT_YELLOW);
    lastX = x;
    lastY = y;
  }
}

// ========= SETTINGS PANEL =========
static void drawSettingRow(int idx, int y, const char* label, const String& value){
  int rowH   = 20;
  int xLabel = PLOT_X + 10;
  int xValue = PLOT_X + 118;

  Rect r = { PLOT_X + 6, y, PLOT_W - 12, rowH };
  uint16_t bg = (settingsSel == idx) ? TFT_DARKGREY : TFT_BLACK;

  tft.fillRect(r.x, r.y, r.w, r.h, bg);

  int ty = r.y + (rowH - 8) / 2;
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, bg);
  tft.drawString(label, xLabel, ty);
  tft.drawString(value, xValue, ty);
}

static void drawSettingsPanel(){
  drawPlotFrame();
  tft.setTextSize(1);

  int y0  = PLOT_Y + 8;
  int gap = 4;

  drawSettingRow(0, y0 + 0*(20 + gap), "Approach:", String((int)APPROACH_BAND_C) + " C");
  drawSettingRow(1, y0 + 1*(20 + gap), "Burst win:", String(WINDOW_MS) + " ms");
  drawSettingRow(2, y0 + 2*(20 + gap), "Max pwr:",   String((int)(MAX_DUTY * 100.0f)) + " %");

  String ofs = String((TEMP_OFFSET_C >= 0) ? "+" : "") + String((int)TEMP_OFFSET_C) + " C";
  drawSettingRow(3, y0 + 3*(20 + gap), "Temp ofs:",  ofs);

  settingsNextR = { PLOT_X + 8, PLOT_Y + PLOT_H - 34, PLOT_W - 16, 28 };
  drawButton(settingsNextR, "NEXT", TFT_BLUE, TFT_WHITE);
}

static void redrawScreen(){
  tft.fillScreen(TFT_BLACK);
  layoutButtons();
  drawTabs();
  drawBottomBar();
  tft.fillRect(10, INFO_Y, W-20, 12, TFT_BLACK);

  if(mode == MODE_REFLOW){
    drawReflowBlueCurveOnce();
  } else if(mode == MODE_MANUAL) {
    if(!manualHistInit) manualPlotInitNow();
    drawManualPlot();
  } else {
    drawSettingsPanel();
  }

  drawValues();
  drawRelayDot(relayOn);
  lastRelayOnDrawn = relayOn;

  lastModeDrawn      = mode;
  lastHeatOnDrawn    = heatOn;
  lastReflowRunDrawn = reflowRunning;
  lastSensorDrawn    = sensorOK;
  lastSetDrawn       = activeSetForDisplay;
  lastTempDrawn      = tempC;
}

// ========= CLICK HANDLER =========
static void handleClick(int x, int y){
  // Tabs: ONLY the real tab rectangles
  if(inRect(tabManualR, x, y) || inRect(tabReflowR, x, y) || inRect(tabSetR, x, y)){
    Mode newMode = mode;

    if(inRect(tabManualR, x, y))      newMode = MODE_MANUAL;
    else if(inRect(tabReflowR, x, y)) newMode = MODE_REFLOW;
    else if(inRect(tabSetR, x, y))    newMode = MODE_SETTINGS;

    if(newMode != mode){
      mode = newMode;

      if(mode == MODE_SETTINGS){
        heatOn = false;
        reflowRunning = false;
        reflowLastTickMs = 0;
        manualRampLastMs = 0;
      }

      if(mode == MODE_MANUAL){
        reflowRunning = false;
        reflowLastTickMs = 0;
        manualPlotInitNow();

        if(heatOn){
          manualRampSetC = tempC;
          manualRampLastMs = millis();
        } else {
          manualRampLastMs = 0;
        }
      }

      redrawScreen();
    }
    return;
  }

  // Settings: one big NEXT button, no fragile row-tap selection
  if(mode == MODE_SETTINGS){
    if(inRect(settingsNextR, x, y)){
      settingsSel = (settingsSel + 1) % SETTINGS_COUNT;
      drawSettingsPanel();
      return;
    }
  }

  if(inRect(btnMinusR, x, y)){
    if(mode == MODE_MANUAL){
      setC = clampf(setC - 5.0f, MIN_SET, MAX_SET);

      if(heatOn && manualRampSetC > setC) manualRampSetC = setC;

      activeSetForDisplay = setC;
      drawValues();
      drawManualPlot();
    } else if(mode == MODE_REFLOW) {
      peakC = clampf(peakC - 5.0f, 200.0f, 450.0f);
      if(!reflowRunning) activeSetForDisplay = peakC;
      drawReflowBlueCurveOnce();
      drawBottomBar();
      drawValues();
    } else {
      if(settingsSel == 0){
        APPROACH_BAND_C = clampf(APPROACH_BAND_C - BAND_STEP, BAND_MIN, BAND_MAX);
      } else if(settingsSel == 1) {
        if(WINDOW_MS > WIN_MIN + WIN_STEP) WINDOW_MS -= WIN_STEP;
        else WINDOW_MS = WIN_MIN;
      } else if(settingsSel == 2) {
        MAX_DUTY = clampf(MAX_DUTY - PWR_STEP, PWR_MIN, PWR_MAX);
      } else {
        TEMP_OFFSET_C = clampf(TEMP_OFFSET_C - OFS_STEP, OFS_MIN, OFS_MAX);
      }
      drawSettingsPanel();
      drawValues();
    }
    return;
  }

  if(inRect(btnPlusR, x, y)){
    if(mode == MODE_MANUAL){
      setC = clampf(setC + 5.0f, MIN_SET, MAX_SET);
      activeSetForDisplay = setC;
      drawValues();
      drawManualPlot();
    } else if(mode == MODE_REFLOW) {
      peakC = clampf(peakC + 5.0f, 200.0f, 450.0f);
      if(!reflowRunning) activeSetForDisplay = peakC;
      drawReflowBlueCurveOnce();
      drawBottomBar();
      drawValues();
    } else {
      if(settingsSel == 0){
        APPROACH_BAND_C = clampf(APPROACH_BAND_C + BAND_STEP, BAND_MIN, BAND_MAX);
      } else if(settingsSel == 1) {
        if(WINDOW_MS + WIN_STEP < WIN_MAX) WINDOW_MS += WIN_STEP;
        else WINDOW_MS = WIN_MAX;
      } else if(settingsSel == 2) {
        MAX_DUTY = clampf(MAX_DUTY + PWR_STEP, PWR_MIN, PWR_MAX);
      } else {
        TEMP_OFFSET_C = clampf(TEMP_OFFSET_C + OFS_STEP, OFS_MIN, OFS_MAX);
      }
      drawSettingsPanel();
      drawValues();
    }
    return;
  }

  if(inRect(btnHeatR, x, y)){
    if(mode == MODE_MANUAL){
      heatOn = !heatOn;

      if(heatOn){
        manualRampSetC = tempC;
        manualRampLastMs = millis();
      } else {
        manualRampLastMs = 0;
      }

      drawBottomBar();
      drawValues();
    } else if(mode == MODE_REFLOW) {
      if(!reflowRunning){
        reflowRunning = true;
        reflowStartMs = millis();
        reflowProgS = 0.0f;
        reflowLastTickMs = reflowStartMs;
        drawReflowBlueCurveOnce();
      } else {
        reflowRunning = false;
        reflowLastTickMs = 0;
        drawPlotInfo(0.0f);
      }
      drawBottomBar();
      drawValues();
    } else {
      saveSettings();
      drawSettingsPanel();
      drawValues();
    }
    return;
  }
}

// ========= SETUP / LOOP =========
void setup(){
  Serial.begin(115200);

  pinMode(PIN_SSR, OUTPUT);
  relayOn = false;
  ssrWrite(false);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_NTC, ADC_11db);

  tft.begin();
  tft.setRotation(TFT_ROT);

  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchSPI);
  touchscreen.setRotation(0);

  loadSettings();

  manualPlotInitNow();
  redrawScreen();
}

void loop(){
  // ---- Touch: click on release, but more forgiving ----
  static bool touchDown = false;
  static uint32_t downMs = 0;
  static int xStart = 0, yStart = 0;
  static int xLast  = 0, yLast  = 0;
  static int maxDx = 0, maxDy = 0;
  static uint32_t lastClickMs = 0;

  int tx, ty;
  bool pressed = getTouchXY(tx, ty);

  if(pressed){
    if(!touchDown){
      touchDown = true;
      downMs = millis();
      xStart = xLast = tx;
      yStart = yLast = ty;
      maxDx = maxDy = 0;
    } else {
      xLast = tx;
      yLast = ty;
      int dx = abs(xLast - xStart);
      int dy = abs(yLast - yStart);
      if(dx > maxDx) maxDx = dx;
      if(dy > maxDy) maxDy = dy;
    }
  } else {
    if(touchDown){
      uint32_t upMs = millis();
      uint32_t held = upMs - downMs;

      if(held >= 15 && maxDx <= 45 && maxDy <= 45){
        if(upMs - lastClickMs >= 80){
          lastClickMs = upMs;
          int cx = (xStart + xLast) / 2;
          int cy = (yStart + yLast) / 2;
          handleClick(cx, cy);
        }
      }
    }
    touchDown = false;
  }

  // ---- Control update (every 200ms) ----
  static uint32_t lastCtrlMs = 0;
  uint32_t now = millis();
  if(now - lastCtrlMs < 200) return;
  lastCtrlMs = now;

  // ---- Sensor read with debounce ----
  float t;
  bool valid = readThermistor(t);

  if(valid){
    sensorBadCnt = 0;
    if(sensorGoodCnt < 255) sensorGoodCnt++;
    if(sensorGoodCnt >= SENSOR_GOOD_RECOVER) sensorOK = true;

    // Filter raw sensor first, then apply offset
    if(sensorOK) tempSensorC = 0.8f * tempSensorC + 0.2f * t;
    else         tempSensorC = t;

    tempC = tempSensorC + TEMP_OFFSET_C;
  } else {
    sensorGoodCnt = 0;
    if(sensorBadCnt < 255) sensorBadCnt++;
    if(sensorBadCnt >= SENSOR_BAD_TRIP) sensorOK = false;
    // keep last tempC
  }

  bool requestHeat = false;
  float activeSet = setC;

  // ========= REFLOW TIMING: PAUSE IF BEHIND =========
  if(mode == MODE_REFLOW && reflowRunning){

    if(reflowLastTickMs == 0) reflowLastTickMs = now;
    float dt = (now - reflowLastTickMs) / 1000.0f;
    reflowLastTickMs = now;
    if(dt < 0) dt = 0;
    if(dt > 1.0f) dt = 1.0f;

    float spNow = reflowSetpoint(reflowProgS);

    if(sensorOK){
      if(reflowProgS < 270.0f){
        if(tempC >= (spNow - REFLOW_SYNC_BAND_C)){
          reflowProgS += dt;
        }
      } else {
        reflowProgS += dt;
      }
    }

    float tsec = reflowProgS;

    if(tsec >= REFLOW_TOTAL_S){
      reflowRunning = false;
      reflowLastTickMs = 0;
      requestHeat = false;
      activeSet = peakC;
      drawPlotInfo(0.0f);
    } else {
      requestHeat = true;
      activeSet = reflowSetpoint(tsec);
    }

  } else {
    // ========= MANUAL: ramp + hold =========
    if(mode == MODE_MANUAL){
      requestHeat = heatOn;

      if(heatOn){
        if(manualRampLastMs == 0) manualRampLastMs = now;
        float dt = (now - manualRampLastMs) / 1000.0f;
        manualRampLastMs = now;
        if(dt < 0) dt = 0;
        if(dt > 1.0f) dt = 1.0f;

        if(setC < manualRampSetC){
          manualRampSetC = setC;
        } else if(setC > manualRampSetC) {
          if(sensorOK){
            if(tempC >= (manualRampSetC - MANUAL_SYNC_BAND_C)){
              manualRampSetC += MANUAL_RAMP_CPS * dt;
              if(manualRampSetC > setC) manualRampSetC = setC;
            }
          }
        }

        activeSet = manualRampSetC;
      } else {
        manualRampLastMs = 0;
        activeSet = setC;
      }

    } else {
      requestHeat = false;
      if(mode == MODE_REFLOW) activeSet = peakC;
      else                    activeSet = setC;
    }
  }

  // ---- Relay logic ----
  relayOn = computeRelayBurst(requestHeat, sensorOK, tempC, activeSet, now);
  ssrWrite(relayOn);

  if(mode == MODE_MANUAL) activeSetForDisplay = setC;
  else                    activeSetForDisplay = activeSet;

  // ---- Minimal redraw ----
  drawValues();

  if(relayOn != lastRelayOnDrawn){
    drawRelayDot(relayOn);
    lastRelayOnDrawn = relayOn;
  }

  if(mode != lastModeDrawn || heatOn != lastHeatOnDrawn || reflowRunning != lastReflowRunDrawn){
    drawTabs();
    drawBottomBar();
    lastModeDrawn      = mode;
    lastHeatOnDrawn    = heatOn;
    lastReflowRunDrawn = reflowRunning;

    if(mode == MODE_SETTINGS){
      drawSettingsPanel();
    }
  }

  // Manual plot update (1 Hz)
  if(mode == MODE_MANUAL){
    static uint32_t lastManPlotMs = 0;
    if(now - lastManPlotMs >= 1000){
      lastManPlotMs = now;
      if(!manualHistInit) manualPlotInitNow();
      manualPlotPush(tempC);
      drawManualPlot();
    }
  }

  // Reflow plot incremental
  if(mode == MODE_REFLOW){
    static uint32_t lastPlotMs = 0;
    if(now - lastPlotMs >= 400){
      lastPlotMs = now;
      if(reflowRunning){
        float tsec = reflowProgS;
        drawReflowProgressIncrement(tsec);
      } else {
        drawPlotInfo(0.0f);
      }
    }
  }
}