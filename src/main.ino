#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h> 
#include <Fonts/Org_01.h>
#include <Fonts/TomThumb.h>

/* --- KONFIGURASI WIFI --- */
const char* ssid = "Testing Information TTH";
const char* password = "tthxsmktelkom_2";

/* --- KONFIGURASI PANEL --- */
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1
#define SCROLL_GAP 20
#define LEFT_OFFSET 10

/* --- VARIABEL GLOBAL --- */
MatrixPanel_I2S_DMA *dma_display = nullptr;
WebServer server(80);
DNSServer dnsServer;
Preferences preferences; 

struct LineData {
  String text;      
  int base_y;       
  int current_x;    
  int pixel_width;  
  uint16_t color;   
  String hexColor;  
  bool need_scroll;
};

LineData lines[3];

int brightness = 60;
int font_choice = 0; 
int scroll_speed = 60; 
const GFXfont* currentFont = &Org_01;

unsigned long previousMillis = 0;

/* --- HTML WEBPAGE --- */
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Matrix Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; text-align: center; background-color: #1a1a1a; color: #fff; margin:0; padding:20px; }
    h2 { color: #00d2ff; }
    .card { background: #2d2d2d; max-width: 400px; margin: 0 auto; padding: 20px; border-radius: 12px; }
    .form-group { margin-bottom: 15px; text-align: left; }
    label { display: block; font-size: 12px; color: #aaa; margin-bottom: 5px; }
    input[type=text] { width: 100%; padding: 10px; box-sizing: border-box; border-radius: 5px; border: 1px solid #444; background: #333; color: white; }
    input[type=color] { width: 100%; height: 40px; border: none; cursor: pointer; background: none; }
    select, input[type=range] { width: 100%; padding: 10px; background: #333; color: white; border: 1px solid #444; border-radius: 5px; }
    .btn { background-color: #00d2ff; color: #000; font-weight: bold; padding: 15px; border: none; cursor: pointer; width: 100%; border-radius: 5px; margin-top: 10px; }
  </style>
</head><body>
  <h2>Matrix Control</h2>
  <div class="card">
    <form action="/save" method="POST">
      <div class="form-group"><label>1. CUSTOMER</label><input type="text" name="t0" value="%T0%"><input type="color" name="c0" value="%C0%"></div>
      <div class="form-group"><label>2. DEVICE</label><input type="text" name="t1" value="%T1%"><input type="color" name="c1" value="%C1%"></div>
      <div class="form-group"><label>3. STATUS</label><input type="text" name="t2" value="%T2%"><input type="color" name="c2" value="%C2%"></div>
      <hr style="border-color:#444;">
      <div class="form-group"><label>BRIGHTNESS: <span id="bval">%BR%</span></label><input type="range" name="br" min="5" max="255" value="%BR%" oninput="document.getElementById('bval').innerText=this.value"></div>
      <div class="form-group"><label>SPEED (Lower=Faster): <span id="sval">%SP%</span> ms</label><input type="range" name="sp" min="10" max="200" value="%SP%" oninput="document.getElementById('sval').innerText=this.value"></div>
      <div class="form-group"><label>FONT</label>
        <select name="ft">
          <option value="0" %F0%>Org_01 (Bold)</option>
          <option value="1" %F1%>TomThumb (Tiny)</option>
        </select>
      </div>
      <input type="submit" value="UPDATE DISPLAY" class="btn">
    </form>
  </div>
</body></html>
)rawliteral";

uint16_t hexToRGB565(const String &hex) {
  if (hex.length() < 7) return 0;
  uint8_t r = (uint8_t) strtol(hex.substring(1,3).c_str(), NULL, 16);
  uint8_t g = (uint8_t) strtol(hex.substring(3,5).c_str(), NULL, 16);
  uint8_t b = (uint8_t) strtol(hex.substring(5,7).c_str(), NULL, 16);

  uint16_t r5 = (uint16_t)((r * 31 + 127) / 255);
  uint16_t g6 = (uint16_t)((g * 63 + 127) / 255);
  uint16_t b5 = (uint16_t)((b * 31 + 127) / 255);

  return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

String rgb565ToHex(uint16_t color) {
  uint8_t r8 = (uint8_t)(((color >> 11) & 0x1F) * 255 / 31);
  uint8_t g8 = (uint8_t)(((color >> 5) & 0x3F) * 255 / 63);
  uint8_t b8 = (uint8_t)((color & 0x1F) * 255 / 31);
  char hex[8];
  sprintf(hex, "#%02X%02X%02X", r8, g8, b8);
  return String(hex);
}

void setLinePositions() {
  if(font_choice == 0) { 
    currentFont = &Org_01;
    lines[0].base_y = 12; 
    lines[1].base_y = 20;
    lines[2].base_y = 28;
  } else {
    currentFont = &TomThumb;
    lines[0].base_y = 12; 
    lines[1].base_y = 20; 
    lines[2].base_y = 28;
  }
}

void updateTextWidth(int index) {
  int16_t x1, y1;
  uint16_t w, h;
  dma_display->setFont(currentFont);
  dma_display->getTextBounds(lines[index].text, 0, 0, &x1, &y1, &w, &h);
  lines[index].pixel_width = w;
  
  if (w > (PANEL_RES_X - LEFT_OFFSET)) {
    lines[index].need_scroll = true;
  } else {
    lines[index].need_scroll = false;
    lines[index].current_x = LEFT_OFFSET;
  }
}

String getPage() {
  String page = index_html;
  page.replace("%T0%", lines[0].text); page.replace("%C0%", lines[0].hexColor);
  page.replace("%T1%", lines[1].text); page.replace("%C1%", lines[1].hexColor);
  page.replace("%T2%", lines[2].text); page.replace("%C2%", lines[2].hexColor);
  page.replace("%BR%", String(brightness)); page.replace("%SP%", String(scroll_speed));
  if(font_choice == 0) { page.replace("%F0%", "selected"); page.replace("%F1%", ""); }
  else { page.replace("%F0%", ""); page.replace("%F1%", "selected"); }
  return page;
}

void loadData() {
  preferences.begin("matrix_data", false);
  brightness = preferences.getInt("br", 60);
  scroll_speed = preferences.getInt("sp", 60);
  font_choice = preferences.getInt("ft", 1); 



  String h0 = preferences.getString("h0", "");
  uint32_t c0 = preferences.getUInt("c0", 0xF800);
  if (h0.length() >= 7) {
    lines[0].hexColor = h0;
    lines[0].color = hexToRGB565(h0);
  } else {
    lines[0].color = (uint16_t)c0;
    lines[0].hexColor = rgb565ToHex(lines[0].color);
  }

  String h1 = preferences.getString("h1", "");
  uint32_t c1 = preferences.getUInt("c1", 0x07E0);
  if (h1.length() >= 7) {
    lines[1].hexColor = h1;
    lines[1].color = hexToRGB565(h1);
  } else {
    lines[1].color = (uint16_t)c1;
    lines[1].hexColor = rgb565ToHex(lines[1].color);
  }

  String h2 = preferences.getString("h2", "");
  uint32_t c2 = preferences.getUInt("c2", 0x001F);
  if (h2.length() >= 7) {
    lines[2].hexColor = h2;
    lines[2].color = hexToRGB565(h2);
  } else {
    lines[2].color = (uint16_t)c2;
    lines[2].hexColor = rgb565ToHex(lines[2].color);
  }

  lines[0].text = preferences.getString("t0", "PENDEK");
  lines[1].text = preferences.getString("t1", "TEKS INI PANJANG SEKALI");
  lines[2].text = preferences.getString("t2", "NORMAL");

  preferences.end();
}

void saveData() {
  preferences.begin("matrix_data", false);
  preferences.putInt("br", brightness); preferences.putInt("sp", scroll_speed); preferences.putInt("ft", font_choice);
  preferences.putString("t0", lines[0].text); preferences.putUInt("c0", lines[0].color); preferences.putString("h0", lines[0].hexColor);
  preferences.putString("t1", lines[1].text); preferences.putUInt("c1", lines[1].color); preferences.putString("h1", lines[1].hexColor);
  preferences.putString("t2", lines[2].text); preferences.putUInt("c2", lines[2].color); preferences.putString("h2", lines[2].hexColor);
  preferences.end();
}

void handleRoot() { server.send(200, "text/html", getPage()); }

void handleSave() {
  if (server.hasArg("t0")) lines[0].text = server.arg("t0");
  if (server.hasArg("c0")) {
    lines[0].hexColor = server.arg("c0");
    lines[0].color = hexToRGB565(lines[0].hexColor);
  }
  if (server.hasArg("t1")) lines[1].text = server.arg("t1");
  if (server.hasArg("c1")) {
    lines[1].hexColor = server.arg("c1");
    lines[1].color = hexToRGB565(lines[1].hexColor);
  }
  if (server.hasArg("t2")) lines[2].text = server.arg("t2");
  if (server.hasArg("c2")) {
    lines[2].hexColor = server.arg("c2");
    lines[2].color = hexToRGB565(lines[2].hexColor);
  }
  if (server.hasArg("br")) brightness = server.arg("br").toInt();
  if (server.hasArg("sp")) scroll_speed = server.arg("sp").toInt();
  if (server.hasArg("ft")) font_choice = server.arg("ft").toInt();

  setLinePositions(); 
  dma_display->setBrightness8(brightness);
  
  for(int i=0; i<3; i++) {
    lines[i].text.toUpperCase();
    lines[i].current_x = LEFT_OFFSET; 
    updateTextWidth(i);
  }
  saveData();
  server.send(200, "text/html", "<meta http-equiv='refresh' content='0;url=/' /> Saving...");
}

void setup() {
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  mxconfig.double_buff = true;
  mxconfig.clkphase = false; 
  
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  
  loadData(); 
  setLinePositions();
  
  dma_display->setBrightness8(brightness);
  dma_display->setFont(currentFont);
  dma_display->setTextWrap(false);
  
  for(int i=0; i<3; i++) updateTextWidth(i);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound([]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= scroll_speed) {
    previousMillis = currentMillis;

    dma_display->fillScreen(0); 

    for (int i = 0; i < 3; i++) {
      dma_display->setFont(currentFont);
      dma_display->setTextColor(lines[i].color);

      if (lines[i].need_scroll) {
        dma_display->setCursor(lines[i].current_x, lines[i].base_y);
        dma_display->print(lines[i].text);

        int totalBlockWidth = lines[i].pixel_width + SCROLL_GAP;
        if ((lines[i].current_x + totalBlockWidth) < PANEL_RES_X) {
          dma_display->setCursor(lines[i].current_x + totalBlockWidth, lines[i].base_y);
          dma_display->print(lines[i].text);
        }

        lines[i].current_x--;
        if (lines[i].current_x < -(totalBlockWidth)) {
          lines[i].current_x += totalBlockWidth;
        }
      } else {
        dma_display->setCursor(LEFT_OFFSET, lines[i].base_y);
        dma_display->print(lines[i].text);
      }
    }
    dma_display->flipDMABuffer(); 
  }
}
