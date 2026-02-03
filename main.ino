// Include CYD configuration FIRST, before any other includes
#include "CYD_Config.h"

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>
#include <TJpg_Decoder.h>

TFT_eSPI tft = TFT_eSPI();

// Touchscreen pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// SD card
#define SD_CS 5

// UI
const int FOOTER_HEIGHT = 40;
const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;

// Slideshow settings
const int slideDelay = 30000;
const int fadeSteps = 20;
const int fadeDelay = 15;

// Time variables
int currentHour = 12;
int currentMinute = 0;
int currentSecond = 0;
unsigned long lastSecondUpdate = 0;

// State control
enum ProgramState {
  TIME_SETTING,
  SLIDESHOW
};

ProgramState currentState = TIME_SETTING;

// Image storage
String imageFiles[50];
int imageFileCount = 0;
int currentImageIndex = 0;

// Footer update tracking
int lastDisplayedSecond = -1;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void updateTime() {
  unsigned long currentMillis = millis();
  
  // Update time every 1000ms exactly
  if (currentMillis - lastSecondUpdate >= 1000) {
    lastSecondUpdate = currentMillis;
    
    currentSecond++;
    if (currentSecond >= 60) {
      currentSecond = 0;
      currentMinute++;
      if (currentMinute >= 60) {
        currentMinute = 0;
        currentHour++;
        if (currentHour >= 24) {
          currentHour = 0;
        }
      }
    }
  }
}

void drawFooter() {
  // Only redraw footer if second has changed
  if (lastDisplayedSecond != currentSecond) {
    lastDisplayedSecond = currentSecond;
    
    tft.fillRect(0, SCREEN_HEIGHT - FOOTER_HEIGHT, SCREEN_WIDTH, FOOTER_HEIGHT, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setTextSize(3);
    
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
    
    int textWidth = strlen(timeStr) * 18;
    int xPos = (SCREEN_WIDTH - textWidth) / 2;
    
    tft.setCursor(xPos, SCREEN_HEIGHT - FOOTER_HEIGHT + 8);
    tft.print(timeStr);
  }
}

// Non-blocking fade out - updates time while fading
void fadeOut() {
  for (int brightness = 255; brightness >= 0; brightness -= (255 / fadeSteps)) {
    analogWrite(21, brightness);
    
    // Update time during fade
    unsigned long fadeStart = millis();
    while (millis() - fadeStart < fadeDelay) {
      updateTime();
      delay(1); // Small delay to prevent CPU hogging
    }
  }
  analogWrite(21, 0);
}

// Non-blocking fade in - updates time while fading
void fadeIn() {
  for (int brightness = 0; brightness <= 255; brightness += (255 / fadeSteps)) {
    analogWrite(21, brightness);
    
    // Update time during fade
    unsigned long fadeStart = millis();
    while (millis() - fadeStart < fadeDelay) {
      updateTime();
      delay(1); // Small delay to prevent CPU hogging
    }
  }
  analogWrite(21, 255);
}

void drawTimeSettingScreen() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(90, 10);
  tft.print("SET TIME");
  
  tft.setTextSize(4);
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
  tft.setCursor(40, 50);
  tft.print(timeStr);
  
  // Hour buttons
  tft.fillRect(20, 110, 60, 40, TFT_GREEN);
  tft.fillRect(20, 160, 60, 40, TFT_RED);
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(40, 120);
  tft.print("+");
  tft.setCursor(40, 170);
  tft.print("-");
  
  // Minute buttons
  tft.fillRect(130, 110, 60, 40, TFT_GREEN);
  tft.fillRect(130, 160, 60, 40, TFT_RED);
  tft.setCursor(150, 120);
  tft.print("+");
  tft.setCursor(150, 170);
  tft.print("-");
  
  // Second buttons
  tft.fillRect(240, 110, 60, 40, TFT_GREEN);
  tft.fillRect(240, 160, 60, 40, TFT_RED);
  tft.setCursor(260, 120);
  tft.print("+");
  tft.setCursor(260, 170);
  tft.print("-");
  
  // Labels
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(35, 100);
  tft.print("Hour");
  tft.setCursor(140, 100);
  tft.print("Min");
  tft.setCursor(255, 100);
  tft.print("Sec");
  
  // Done button
  tft.fillRect(110, 210, 100, 25, TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(135, 215);
  tft.print("DONE");
}

void handleTimeSettingTouch(int x, int y) {
  // Hour +
  if (x >= 20 && x <= 80 && y >= 110 && y <= 150) {
    currentHour = (currentHour + 1) % 24;
    drawTimeSettingScreen();
    delay(150);
  }
  // Hour -
  else if (x >= 20 && x <= 80 && y >= 160 && y <= 200) {
    currentHour = (currentHour - 1 + 24) % 24;
    drawTimeSettingScreen();
    delay(150);
  }
  // Minute +
  else if (x >= 130 && x <= 190 && y >= 110 && y <= 150) {
    currentMinute = (currentMinute + 1) % 60;
    drawTimeSettingScreen();
    delay(150);
  }
  // Minute -
  else if (x >= 130 && x <= 190 && y >= 160 && y <= 200) {
    currentMinute = (currentMinute - 1 + 60) % 60;
    drawTimeSettingScreen();
    delay(150);
  }
  // Second +
  else if (x >= 240 && x <= 300 && y >= 110 && y <= 150) {
    currentSecond = (currentSecond + 1) % 60;
    drawTimeSettingScreen();
    delay(150);
  }
  // Second -
  else if (x >= 240 && x <= 300 && y >= 160 && y <= 200) {
    currentSecond = (currentSecond - 1 + 60) % 60;
    drawTimeSettingScreen();
    delay(150);
  }
  // Done button - CRITICAL: Start slideshow mode
  else if (x >= 110 && x <= 210 && y >= 210 && y <= 235) {
    Serial.println("\n=== TIME SET - STARTING SLIDESHOW MODE ===");
    
    // Show transition message
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(30, 80);
    tft.println("Initializing...");
    tft.setTextSize(1);
    tft.setCursor(30, 110);
    tft.println("Disabling touchscreen");
    tft.setCursor(30, 125);
    tft.println("Starting SD card");
    delay(1000);
    
    // DISABLE TOUCHSCREEN FOREVER
    touchscreenSPI.end();
    Serial.println("✗ Touchscreen disabled");
    
    // INITIALIZE SD CARD
    Serial.println("Initializing SD card...");
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    digitalWrite(XPT2046_CS, HIGH);
    
    if (!SD.begin(SD_CS)) {
      Serial.println("✗ SD Card FAILED!");
      tft.fillScreen(TFT_RED);
      tft.setTextColor(TFT_WHITE, TFT_RED);
      tft.setTextSize(2);
      tft.setCursor(10, 80);
      tft.println("SD FAILED!");
      tft.setTextSize(1);
      tft.setCursor(10, 110);
      tft.println("Insert SD card");
      tft.setCursor(10, 125);
      tft.println("and reset device");
      while(1) delay(1000);
    }
    
    Serial.println("✓ SD Card initialized");
    
    // Scan for images
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(30, 100);
    tft.println("Scanning images...");
    
    scanSDCard();
    
    // Initialize JPEG decoder
    TJpgDec.setJpgScale(1);
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(tft_output);
    Serial.println("✓ JPEG decoder ready");
    
    // Show ready message
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(30, 80);
    tft.println("Ready!");
    tft.setTextSize(1);
    tft.setCursor(30, 110);
    tft.print("Images found: ");
    tft.println(imageFileCount);
    delay(2000);
    
    // Switch to slideshow mode
    currentState = SLIDESHOW;
    lastSecondUpdate = millis();
    
    tft.fillScreen(TFT_BLACK);
    Serial.println("✓ Slideshow started!\n");
    
    delay(100);
  }
}

void displayImage(String filename) {
  String fullPath = "/" + filename;
  tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - FOOTER_HEIGHT, TFT_BLACK);
  TJpgDec.drawSdJpg(0, 0, fullPath);
}

void scanSDCard() {
  Serial.println("Scanning SD card...");
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root");
    return;
  }
  
  imageFileCount = 0;
  while (imageFileCount < 50) {
    File entry = root.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory()) {
      String filename = String(entry.name());
      String filenameLower = filename;
      filenameLower.toLowerCase();
      
      if (filenameLower.endsWith(".jpg") || filenameLower.endsWith(".jpeg")) {
        imageFiles[imageFileCount] = filename;
        Serial.print("  ");
        Serial.print(imageFileCount + 1);
        Serial.print(". ");
        Serial.println(filename);
        imageFileCount++;
      }
    }
    entry.close();
  }
  root.close();
  
  Serial.print("Found ");
  Serial.print(imageFileCount);
  Serial.println(" JPEG files");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Photo Display with Touch Setup ===");
  Serial.println("Phase 1: Time Setting Mode");
  
  // Setup pins
  pinMode(21, OUTPUT);
  analogWrite(21, 255);
  
  pinMode(XPT2046_CS, OUTPUT);
  digitalWrite(XPT2046_CS, HIGH);
  
  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  Serial.println("✓ Display initialized");
  
  // Initialize touchscreen for time setting
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);
  Serial.println("✓ Touchscreen initialized");
  
  // Show welcome and instructions
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(50, 60);
  tft.println("Welcome!");
  tft.setTextSize(1);
  tft.setCursor(20, 100);
  tft.println("Set the time, then press DONE");
  tft.setCursor(20, 115);
  tft.println("to start the photo slideshow");
  
  delay(3000);
  
  // Show time setting screen
  drawTimeSettingScreen();
  
  Serial.println("\n>> Set the time and press DONE");
  Serial.println(">> Touch will be disabled after DONE");
  Serial.println(">> Slideshow will start automatically\n");
  
  lastSecondUpdate = millis();
}

void loop() {
  if (currentState == TIME_SETTING) {
    // TIME SETTING MODE - Touch is active
    if (touchscreen.tirqTouched() && touchscreen.touched()) {
      TS_Point p = touchscreen.getPoint();
      int x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
      int y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
      
      handleTimeSettingTouch(x, y);
    }
  } 
  else if (currentState == SLIDESHOW) {
    // SLIDESHOW MODE - Touch is disabled, SD card active
    
    // Update time continuously
    updateTime();
    
    // Only redraw footer when second changes
    drawFooter();
    
    if (imageFileCount > 0) {
      static unsigned long lastImageChange = 0;
      static bool firstRun = true;
      
      if (firstRun || millis() - lastImageChange >= slideDelay) {
        firstRun = false;
        lastImageChange = millis();
        
        Serial.print("Image ");
        Serial.print(currentImageIndex + 1);
        Serial.print("/");
        Serial.print(imageFileCount);
        Serial.print(": ");
        Serial.println(imageFiles[currentImageIndex]);
        
        // Fades now update time internally
        fadeOut();
        displayImage(imageFiles[currentImageIndex]);
        drawFooter();
        fadeIn();
        
        currentImageIndex = (currentImageIndex + 1) % imageFileCount;
      }
    } else {
      // No images found
      static bool errorShown = false;
      if (!errorShown) {
        tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - FOOTER_HEIGHT, TFT_YELLOW);
        tft.setTextSize(2);
        tft.setTextColor(TFT_BLACK, TFT_YELLOW);
        tft.setCursor(20, 80);
        tft.println("No JPEGs!");
        errorShown = true;
      }
    }
  }
}
