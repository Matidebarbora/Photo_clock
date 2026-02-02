// Include configuration FIRST, before any other includes
#include "CYD_Config.h"

#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <TJpg_Decoder.h>

TFT_eSPI tft = TFT_eSPI();

// SD card pins for ESP32-2432S028
#define SD_CS 5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

// Slideshow settings
const int slideDelay = 3000;  // 3 seconds per image
const int fadeSteps = 20;     // Number of steps in fade transition (higher = smoother but slower)
const int fadeDelay = 30;     // Delay between fade steps in ms

File root;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// Fade out to black
void fadeOut() {
  for (int brightness = 255; brightness >= 0; brightness -= (255 / fadeSteps)) {
    // Adjust backlight brightness
    int pwmValue = max(0, brightness);
    ledcWrite(0, pwmValue);
    delay(fadeDelay);
  }
  ledcWrite(0, 0);  // Ensure it's fully off
}

// Fade in from black
void fadeIn() {
  for (int brightness = 0; brightness <= 255; brightness += (255 / fadeSteps)) {
    // Adjust backlight brightness
    int pwmValue = min(255, brightness);
    ledcWrite(0, pwmValue);
    delay(fadeDelay);
  }
  ledcWrite(0, 255);  // Ensure it's fully on
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Setup PWM for backlight control (compatible with ESP32 core 3.x)
  pinMode(TFT_BL, OUTPUT);
  ledcAttach(TFT_BL, 5000, 8);  // Pin, frequency (5kHz), resolution (8-bit)
  ledcWrite(0, 255);  // Start at full brightness
  
  Serial.println("\n\n=== ESP32-2432S028 Photo Display ===");
  Serial.println("Starting setup...");
  
  // Initialize display
  Serial.println("Step 1: Initializing TFT display...");
  tft.init();
  Serial.println("  - TFT init complete");
  
  tft.setRotation(1);  // Landscape
  Serial.println("  - Rotation set to 1 (landscape)");
  
  // Color test
  Serial.println("  - Running color test...");
  tft.fillScreen(TFT_RED);
  delay(500);
  tft.fillScreen(TFT_GREEN);
  delay(500);
  tft.fillScreen(TFT_BLUE);
  delay(500);
  tft.fillScreen(TFT_BLACK);
  Serial.println("  - Color test complete!");
  
  // Display text
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 60);
  tft.println("Display OK!");
  tft.setCursor(10, 90);
  tft.println("Check Serial");
  Serial.println("  - Text displayed on screen");
  
  // Initialize JPEG decoder
  Serial.println("\nStep 2: Initializing JPEG decoder...");
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  Serial.println("  - JPEG decoder ready");
  
  // Initialize SD card
  Serial.println("\nStep 3: Initializing SD card...");
  tft.setCursor(10, 120);
  tft.println("Init SD...");
  
  if (!SD.begin(SD_CS)) {
    Serial.println("  - ERROR: SD Card initialization failed!");
    tft.fillScreen(TFT_RED);
    tft.setCursor(10, 100);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.println("SD FAILED!");
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println("  - SD Card initialized successfully!");
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.print("  - SD Card Size: ");
  Serial.print(cardSize);
  Serial.println(" MB");
  
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 60);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("SD Card OK!");
  tft.setCursor(10, 90);
  tft.print("Size: ");
  tft.print(cardSize);
  tft.println(" MB");
  
  // List files
  Serial.println("\nStep 4: Listing files on SD card...");
  root = SD.open("/");
  if (!root) {
    Serial.println("  - ERROR: Failed to open root directory");
  } else {
    Serial.println("  - Files found:");
    int fileCount = 0;
    int jpegCount = 0;
    
    while (true) {
      File entry = root.openNextFile();
      if (!entry) break;
      
      if (!entry.isDirectory()) {
        fileCount++;
        String filename = String(entry.name());
        Serial.print("    ");
        Serial.print(fileCount);
        Serial.print(". ");
        Serial.print(filename);
        Serial.print(" (");
        Serial.print(entry.size());
        Serial.println(" bytes)");
        
        String filenameLower = filename;
        filenameLower.toLowerCase();
        if (filenameLower.indexOf(".jpg") >= 0 || filenameLower.indexOf(".jpeg") >= 0) {
          jpegCount++;
        }
      }
      entry.close();
    }
    root.close();
    
    Serial.print("  - Total files: ");
    Serial.println(fileCount);
    Serial.print("  - JPEG files: ");
    Serial.println(jpegCount);
    
    tft.setCursor(10, 120);
    tft.print("JPEGs: ");
    tft.println(jpegCount);
  }
  
  delay(3000);
  Serial.println("\n=== Setup Complete ===");
  Serial.println("Starting slideshow...\n");
  
  // Fade out the setup screen
  fadeOut();
  tft.fillScreen(TFT_BLACK);
  fadeIn();
}

void displayImage(const char* filename) {
  Serial.print("Loading image: ");
  Serial.println(filename);
  
  // Create full path with leading slash
  String fullPath = "/";
  fullPath += filename;
  
  // Draw the image using full path
  uint16_t result = TJpgDec.drawSdJpg(0, 0, fullPath);
  
  if (result == 0) {
    Serial.println("  - Image displayed successfully");
  } else {
    Serial.print("  - ERROR: Failed to display image. Error code: ");
    Serial.println(result);
  }
}

void loop() {
  Serial.println("\n--- Starting new slideshow cycle ---");
  
  root = SD.open("/");
  
  if (!root) {
    Serial.println("ERROR: Failed to open root directory");
    tft.fillScreen(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.println("Error reading SD");
    delay(5000);
    return;
  }
  
  int imageNumber = 0;
  
  while (true) {
    File entry = root.openNextFile();
    
    if (!entry) {
      Serial.println("--- End of files, restarting slideshow ---");
      root.close();
      break;
    }
    
    if (!entry.isDirectory()) {
      String filename = entry.name();
      String filenameLower = filename;
      filenameLower.toLowerCase();
      
      if (filenameLower.indexOf(".jpg") >= 0 || filenameLower.indexOf(".jpeg") >= 0) {
        imageNumber++;
        Serial.print("\nImage #");
        Serial.print(imageNumber);
        Serial.print(": ");
        
        // Fade out current image
        fadeOut();
        
        // Load new image while screen is dark
        tft.fillScreen(TFT_BLACK);
        displayImage(entry.name());
        
        // Fade in new image
        fadeIn();
        
        Serial.print("Displaying for ");
        Serial.print(slideDelay / 1000);
        Serial.println(" seconds...");
        delay(slideDelay);
      }
    }
    
    entry.close();
  }
  
  if (imageNumber == 0) {
    Serial.println("\nWARNING: No JPEG files found on SD card!");
    tft.fillScreen(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setTextColor(TFT_BLACK, TFT_YELLOW);
    tft.setCursor(20, 100);
    tft.println("No JPEGs found!");
    tft.setCursor(20, 130);
    tft.println("Add .jpg files");
    tft.setCursor(20, 160);
    tft.println("to SD card root");
    delay(5000);
  }
}
