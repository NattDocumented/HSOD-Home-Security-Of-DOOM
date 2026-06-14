#include <Arduino.h>
#include <Keypad.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <ld2410.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <driver/i2s.h>

// --------------------- //
// ---- DEFINE PINS ---- //
// --------------------- //

// OLED Display
#define PIN_SDA       A4
#define PIN_SCL       A5

// DFPlayerMini
#define PIN_DF_TX     D0 // (ESP32 TX - DFPlayerMini RX)
#define PIN_DF_RX     D1 // (ESP32 RX - DFPLayerMini TX)

// HLK-LD2410C
#define PIN_LD_TX     A7 // (ESP32 TX - RX)
#define PIN_LD_RX     A6 // (ESP32 RX - TX)
#define PIN_LD_OUT    D11

// ICS-43434
#define PIN_I2S_WS    D10
#define PIN_I2S_SD    D9
#define PIN_I2S_SCK   D13

// LED
#define PIN_LED_RED   A0
#define PIN_LED_GREEN A1

// ---------------- //
// ---- Keypad ---- //
// ---------------- //

const byte KP_ROWS = 4;
const byte KP_COLS = 3;

// LAYOUT
char keys[KP_ROWS][KP_COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'C', '0', 'E'}   // C - Cancel, E - Enter
};

byte rowPins[KP_ROWS] = {5, 6, 7, 8}; // R0 - R3
byte colPins[KP_COLS] = {2, 3, 4}; // C0 - C2

// ---------------------- //
// ---- VOICE UNLOCK ---- //
// ---------------------- //

const char* WIFI_SSID = "YOUR_WIFI_SSID"; // Your WiFi's SSID
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD"; // Your WiFi Password here
const char* WIT_TOKEN = "YOUR_WIT_AI_TOKEN"; // Your Wit.ai's token here

#define SAMPLE_RATE     16000
#define RECORD_MS       3000
#define RECORD_BYTES    (SAMPLE_RATE * (RECORD_MS/1000) * 2)
#define VOICE_THRESHOLD 1500

bool voiceUnlockEnabled = false;
unsigned long lastVoiceCheck = 0;

// ------------------ //
// ---- SETTINGS ---- //
// ------------------ //

const String PIN = "159753";
const int MAX_PIN_ATTEMPTS = 3;
const int countdown_before_alarm = 10;

// -------------------------- //
// ---- MP3 FILES CONFIG ---- //
// -------------------------- //

#define MP3_BEEP_WARNING  1 // short beep during countdown
#define MP3_ALARM_FULL    2 // The DOOM Music!!!
#define MP3_ARMED         3 // Armed SFX
#define MP3_DISARMED      4 // Disarmed SFX
#define MP3_WRONG_PIN     5 // Incorrect
#define MP3_LOCKED_OUT    6 // Locked
#define MP3_WELCOME       7 // startup
#define MP3_DOOM_SPEECH   8 // "So you are the chosen one."
#define MP3_WELCOME_HOME  9 // "Welcome home master."

// -------------------- //
// ---- COMPONENTS ---- //
// -------------------- //

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
DFRobotDFPlayerMini dfplayer;
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KP_ROWS, KP_COLS);
ld2410 radar;

//UART PORT SETUP
HardwareSerial serialDF(1); //UART1 for DFPlayerMini
HardwareSerial serialLD(2); //UART2 for HLK-LD2410C

// ---------------- //
// ---- STATES ---- //
// ---------------- //

enum SystemState {
  STATE_DISARMED,
  STATE_ARMED,
  STATE_COUNTDOWN,
  STATE_ALARM,
  STATE_LOCKED_OUT
};

SystemState CurrentState = STATE_DISARMED;

unsigned long stateEnterAt = 0;
unsigned long lastBeepAt = 0;
int PinAttempts = 0;
String EnteredPin = "";
bool HumanPresent = false;
int countdownLeft = countdown_before_alarm;
bool alarmPlaying = false;
unsigned long lastOLEDAt = 0;

// ---- FORWARD DECLARATIONS ---- //
void showOLED(String l1, String l2, String l3, String l4);
String maskPIN(String pin);
void checkPIN();
void configureRadar();
void initVoice();
bool checkVoice();
void disarmByVoice();
uint8_t* recordAudio(size_t &outSize);
bool ensureWiFiConnected();

// --------------- //
// ---- SETUP ---- //
// --------------- //

void setup() {
  Serial.begin(115200);

  // LEDs
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, LOW);

  // OLED Display
  Wire.begin(PIN_SDA, PIN_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
  }
  showOLED("Booting...", "", "", "");

  //DFPlayerMini
  serialDF.begin(9600, SERIAL_8N1,  PIN_DF_RX, PIN_DF_TX);
  if (!dfplayer.begin(serialDF)) {
    Serial.println("DFPlayer init failed");
    while(true);
  } else {
    dfplayer.volume(25); //NOTE: Volume is between 0 - 30
    dfplayer.play(MP3_WELCOME);
    Serial.println("DFplayer ready");
  }

  //HLK-LD2410C
  serialLD.begin(256000, SERIAL_8N1, PIN_LD_RX, PIN_LD_TX);
  if (!radar.begin(serialLD)) {
    Serial.println("HLK-LD2410C init failed");
  } else {
    configureRadar();
  }
  
  if (radar.isConnected()) {
    Serial.println("LD2410 is connected!");
    Serial.print("Firmware version: ");
    Serial.print(radar.firmware_major_version);
    Serial.print(".");
    Serial.println(radar.firmware_minor_version);
  }

  //DONE
  showOLED("HSOD: HOME SECURITY OF DOOM", "", "Enter PIN + ENT", "to arm");
  initVoice();
  keypad.setHoldTime(850);
}

// ------------------- //
// ---- MAIN LOOP ---- //
// ------------------- //

void loop() {
  radar.read();
  char key = keypad.getKey();

  switch (CurrentState) {
    case STATE_DISARMED: handleDISARMED(key); break;
    case STATE_ARMED: handleARMED(key); break;
    case STATE_ALARM: handleAlarm(key); break;
    case STATE_COUNTDOWN: handleCountdown(key); break;
    case STATE_LOCKED_OUT: handleLockedOut(key); break;
  }

}

// ------------------------ //
// ---- STATE HANDLERS ---- //
// ------------------------ //

void handleDISARMED(char key) {
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, LOW);

  if (key == 'E') {
    if (EnteredPin == PIN) {
      EnteredPin = "";
      CurrentState = STATE_ARMED;
      lastBeepAt = 0;
      countdownLeft = countdown_before_alarm;
      stateEnterAt = millis();
      dfplayer.play(MP3_ARMED);
      showOLED("ARMED", "", "(ㆆ_ㆆ)", "");
    } else {
      EnteredPin = "";
      dfplayer.play(MP3_WRONG_PIN);
      showOLED("DISARMED", "WRONG PIM", "Cannot Arm", "");
      delay(1200);
      showOLED("DISARMED", "", "Enter PIN + ENT", "to arm");
    }
    return;
  }

  if (key == 'C') {
    EnteredPin = "";
    showOLED("DISARMED", "", "Enter PIN + ENT", "to arm");
  } else if (key) {
    EnteredPin += key;
    showOLED("DISARMED", "PIN:", maskPIN(EnteredPin), "");
  }
}

void handleARMED(char key) {
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, HIGH);

  // Update every 2s
  if (millis() - lastOLEDAt > 2000) {
    showOLED("ARMED", "", "(ㆆ_ㆆ)", "");
    lastOLEDAt = millis();
  }

  bool outPin = digitalRead(PIN_LD_OUT);
  bool uartDet = radar.isConnected() && radar.presenceDetected();

  if (outPin || uartDet) {
    Serial.println("Human Detected - Activating background Wi-Fi...");

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }

    EnteredPin = "";
    CurrentState = STATE_COUNTDOWN;
    countdownLeft = countdown_before_alarm;
    lastBeepAt = 0;
    lastOLEDAt = 0;
    stateEnterAt = millis();
    showOLED("ENTER CODE", String(countdown_before_alarm) + "s", "PIN: ", "");
    return;
  }
}

void handleAlarm(char key) {
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, HIGH);

  if (!alarmPlaying) {
    dfplayer.loop(MP3_ALARM_FULL);
    alarmPlaying = true;
    showOLED("!! INTRUDER ALERT !!", "Hold C for Voice", "PIN:", maskPIN(EnteredPin));
  }

  //Keypad
  if (key == 'C') {
    unsigned long startCheck = millis();
    bool isLongPress = false;

    while (millis() - startCheck < 900) {
      keypad.getKey(); 
      if (keypad.getState() == HOLD) {
        isLongPress = true;
        break;
      }
      if (keypad.getState() == RELEASED) {
        break;
      }
      delay(10);
    }

    if (isLongPress) {
      if (checkVoice()) {
        disarmByVoice();
        return;
      }
    } else {
      EnteredPin = "";
      showOLED("!! INTRUDER ALERT !!", "PIN Cleared", "PIN:", "");
    }
  } else if (key == 'E') {
    checkPIN();
  } else if (key) {
    EnteredPin += key;
    showOLED("!! INTRUDER ALERT !!", "Hold C for Voice", "PIN:", maskPIN(EnteredPin));
  }
}

void handleCountdown(char key) {
  unsigned long now = millis();
  countdownLeft = countdown_before_alarm - (int)((now - stateEnterAt) / 1000);

  //Update OLED
  static int lastShown = -1;
  if (countdownLeft != lastShown) {
    lastShown = countdownLeft;
    showOLED("ENTER CODE", String(countdownLeft) + "s remaining", "Hold C for Voice", "PIN: " + maskPIN(EnteredPin));
  }

  //Beep
  unsigned long beepEvery = (countdownLeft <= 3) ? 500 : 2000;
  if (now - lastBeepAt > beepEvery) {
    dfplayer.play(MP3_BEEP_WARNING);
    lastBeepAt = now;
  }

  //Keypad
  if (key == 'C') {
    unsigned long startCheck = millis();
    bool isLongPress = false;

    while (millis() - startCheck < 900) {
      keypad.getKey();
      if (keypad.getState() == HOLD) {
        isLongPress = true;
        break;
      }
      if (keypad.getState() == RELEASED) {
        break;
      }
      delay(10);
    }

    if (isLongPress) {
      if (checkVoice()) {
        disarmByVoice();
        return;
      }
    } else {
      EnteredPin = "";
      showOLED("ENTER CODE", String(countdownLeft) + "s remaining", "PIN Cleared", "");
      delay(600);
    }
  } else if (key == 'E') {
    checkPIN();
    return;
  } else if (key) {
    EnteredPin += key;
  }

  // Timer ran out yay end of function
  if (countdownLeft <= 0) {
    CurrentState = STATE_ALARM;
    alarmPlaying = false;
    stateEnterAt = millis();
  }
}

void handleLockedOut(char key) {
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, HIGH);
  
  unsigned long elapsed = millis( )- stateEnterAt;
  unsigned long remaining = 300000UL - elapsed;
  int minsLeft = remaining / 60000;
  int secsLeft = (remaining % 60000) / 1000;

  if (millis() - lastOLEDAt > 1000) {
    showOLED("LOCKED OUT", "Too many attempts", "Unlocks in: ", String(minsLeft) + "m " + String(secsLeft) + "s");
    lastOLEDAt = millis();
  }

  // back after 5 min
  if (elapsed > 300000UL) {
    PinAttempts = 0;
    CurrentState = STATE_ARMED;
    countdownLeft = countdown_before_alarm;
    lastBeepAt = 0;
    lastOLEDAt = 0;
    stateEnterAt = millis();
  }
}

// ----------------------------------- //
// ---- FUNCTIONS / QOL / HELPERS ---- //
// ----------------------------------- //

void showOLED(String l1, String l2, String l3, String l4) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0,0); oled.println(l1);
  oled.setCursor(0, 16); oled.println(l2);
  oled.setCursor(0, 32); oled.println(l3);
  oled.setCursor(0, 48); oled.println(l4);

  oled.display();
}

String maskPIN(String pin) {
  String masked = "";
  for (int i = 0; i < pin.length(); i++) masked += "*";
  return masked;
}

void checkPIN() {
  if (EnteredPin == PIN) {
    // CORRECT
    PinAttempts = 0;
    EnteredPin = "";
    alarmPlaying = false;
    dfplayer.stop();
    dfplayer.play(MP3_DISARMED);
    //dfplayer.play(MP3_WELCOME_HOME);
    CurrentState = STATE_DISARMED;
    stateEnterAt = millis();
    showOLED("DISARMED", "", "Welcome back", "");
  } else {
    // INCORRECT
    PinAttempts++;
    EnteredPin = "";
    dfplayer.play(MP3_WRONG_PIN);

    int left = MAX_PIN_ATTEMPTS - PinAttempts;
    showOLED("WRONG PIN", String(left) + "attempt(s) left", "", "");
    delay(1200);

    if (PinAttempts >= MAX_PIN_ATTEMPTS) {
      dfplayer.play(MP3_LOCKED_OUT);
      CurrentState = STATE_LOCKED_OUT;
      stateEnterAt = millis();
    }
  }
}

void configureRadar() {
  // IDK WHAT I'M DOING IM JUST GUESSING PLEASE CHECK THIS SOON --> Update: Nvm I understand it now...

  if (radar.setMaxValues(4, 3, 5)) {
    Serial.println("Radar range set: 3m moving, 2.25m, stationary");
  } else {
    Serial.println("Radar config failed - fallback to def");
  }

  // Note for reference: values x 0.75 = real world
  // Ex: 4 x 0.75 = 3.0m, 3 x 0.75 = 2.25m... yeah that's it...
}

void initVoice() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");

  unsigned long startWait = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWait < 4000) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    voiceUnlockEnabled = true;
    Serial.println("\nWiFi OK: " + WiFi.localIP().toString());
    showOLED("WiFi Connected", WiFi.localIP().toString(), "Wit.ai ready", "");
    delay(1500);
  } else {
    voiceUnlockEnabled = false;
    Serial.println("\nWiFi failed");
    showOLED("WiFi Offline:", "Will reconnect", "when triggered", "");
    delay(1250);
  }
}

bool checkVoice() {
  //Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    if (!ensureWiFiConnected()) {
      showOLED("VOICE ERROR", "WiFi Unreachable", "Use Keypad PIN", "");
      delay(2000);
      return false;
    }
  }

  if (!voiceUnlockEnabled || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  showOLED("LISTENING...", "", "Say the phrase", "");

  // Record audio buffer
  size_t audioSize = 0;
  uint8_t* audioBuffer = recordAudio(audioSize);
  if (!audioBuffer || audioSize == 0) {
    showOLED("Voice Error", "Record Failed", "", "");
    if (audioBuffer) {
      free(audioBuffer);
    }
    return false;
  }

  // Noise Gate
  int16_t* samples = (int16_t*)audioBuffer;
  size_t sampleCount = audioSize / 2;
  int32_t peakVolume = 0;

  for (size_t i = 0; i < sampleCount; i++) {
    int32_t absoluteValue = abs(samples[i]);
    if (absoluteValue > peakVolume) {
      peakVolume = absoluteValue;
    }
  }

  Serial.print("Noise Gate: Peak volume detected");
  Serial.println(peakVolume);

  if (peakVolume < VOICE_THRESHOLD) {
    Serial.println("Noise Gate: Audio too quiet. Aborting.");
    free(audioBuffer);
    delay(1500);
    return false;
  }

  // Raw PCM to Wit.ai (no base64, no WAV header required)
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("api.wit.ai", 443)) {
    showOLED("Wit.ai:", "Connection failed", "", "");
    free(audioBuffer);
    return false;
  }

  String header = "POST /speech?v=20230215 HTTP/1.1\r\n"
                  "HOST: api.wit.ai\r\n"
                  "Authorization: Bearer " + String(WIT_TOKEN) + "\r\n"
                  "Content-Type: audio/raw;encoding=signed-integer;bits=16;rate=" + String(SAMPLE_RATE) + ";endian=little\r\n"
                  "Transfer-Encoding: chunked\r\n"
                  "Connection: close\r\n\r\n";
  client.print(header);

  showOLED("PROCESSING...", "Sending audio...", "", "");

  size_t bytesSent = 0;
  const size_t chunkSize = 512; // 512 bytes at a time could try 256 but idk

  while (bytesSent < audioSize) {
    size_t toSend = audioSize - bytesSent;
    if (toSend > chunkSize) {
      toSend = chunkSize;
    }

    // send chunksize in Hex format + actual data after
    client.printf("%X\r\n", toSend);
    client.write(audioBuffer + bytesSent, toSend);
    client.print("\r\n");

    bytesSent += toSend;
    delay(10);
  }

  client.print("0\r\n\r\n");
  free(audioBuffer);
  showOLED("PROCESSING...", "Analyzing...", "", "");

  String finalResult = "";
  while (client.connected() || client.available()) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      int textIndex =line.indexOf("\"text\": \"");
      if (textIndex != -1) {
        int start = textIndex + 9;
        int end = line.indexOf("\"", start);
        finalResult = line.substring(start, end);
      }
    }
  }

  client.stop();

  // Check Parse    
  if (finalResult != "") {
    Serial.println("Wit.ai parsed text: " + finalResult);
    finalResult.toLowerCase();

    if (finalResult.indexOf("open sesame") != -1 || finalResult.indexOf("chicken nuggets") != -1) {
      showOLED("ACCESS GRANTED", finalResult, "", "");
      delay(1500);
      return true;
    } else {
      showOLED("ACCESS DENIED", "Phrase wrong: ", finalResult, "");
      delay(2000);
    }
  } else {
    showOLED("Voice Error", "No speech detected", "", "");
    delay(1500);
  }
  
  return false;
}

void disarmByVoice() {
  PinAttempts = 0;
  EnteredPin = "";
  alarmPlaying = false;
  dfplayer.stop();
  dfplayer.play(MP3_DISARMED);
  CurrentState = STATE_DISARMED;
  stateEnterAt = millis();
  showOLED("DISARMED", "", "Voice Unlocked", "");
  Serial.println("Disarmed via voice");
}

uint8_t* recordAudio(size_t &outSize) {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
  };
  i2s_pin_config_t pins = {
    .bck_io_num = PIN_I2S_SCK,
    .ws_io_num = PIN_I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = PIN_I2S_SD,
  };
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_start(I2S_NUM_0);
  uint8_t* buf = (uint8_t*)malloc(RECORD_BYTES);
  if (!buf) {
    i2s_driver_uninstall(I2S_NUM_0);
    outSize = 0;
    return nullptr;
  }
  size_t bytesRead = 0, total = 0;
  while (total < RECORD_BYTES) {
    i2s_read(I2S_NUM_0, buf + total, RECORD_BYTES - total,  &bytesRead, portMAX_DELAY);
    total += bytesRead;
  }
  i2s_stop(I2S_NUM_0);
  i2s_driver_uninstall(I2S_NUM_0);
  outSize = total;
  return buf;
}

bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.println("Emergency WiFi Reconnect...");
  showOLED("CONNECTING...", "Waking up WiFi..", "Please wait", "");

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 3000) {
    delay(125);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Emergency Reconnect Success");
    voiceUnlockEnabled = true;
    return true;
  }

  Serial.println("Emergency Reconnect Failed");
  return false;
}