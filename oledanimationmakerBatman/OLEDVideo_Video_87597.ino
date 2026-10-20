/*
 * ESP8266 SSD1306 OLED Video Player: OLEDVideo_Video_87597
 * Plays 1-bit monochrome video frames from flash memory (PROGMEM).
 * 
 * Wiring (NodeMCU V2/V3 example labels in brackets):
 *   OLED GND -> ESP8266 GND
 *   OLED VCC -> ESP8266 3.3V
 *   OLED SDA -> ESP8266 GPIO 4 (D2)
 *   OLED SCL -> ESP8266 GPIO 5 (D1)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "frames.h"

// ==============================================================================
// CONFIGURATION
// ==============================================================================
// Enable TEST_MODE to play only the first 24 frames (1 second of video)
#define TEST_MODE false

// OLED Display Configuration
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Share reset pin or set to -1 if not used
#define SCREEN_ADDRESS 0x3C

// Target Frame Rate Config
#ifndef TARGET_FPS
#define TARGET_FPS     30
#endif
const uint32_t FRAME_PERIOD_US = 1000000 / TARGET_FPS;

// Pin Definitions for I2C on ESP8266
#define I2C_SDA 4  // NodeMCU Pin D2
#define I2C_SCL 5  // NodeMCU Pin D1

// Initialize SSD1306 display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Diagnostic Variables
uint32_t last_fps_report_time = 0;
uint32_t frames_rendered = 0;
float actual_fps = 0.0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Wait for serial port to connect
  }
  
  Serial.println("\n--- ESP8266 OLED Video Player Initialization ---");
  
  #if TEST_MODE
    Serial.println("Mode: TEST MODE ENABLED (playing only first 24 frames)");
  #else
    Serial.print("Mode: NORMAL MODE (playing full ");
    Serial.print(TOTAL_FRAMES);
    Serial.println(" frames)");
  #endif
  
  // Initialize I2C bus with ESP8266 custom pins
  Serial.print("Initializing I2C bus (SDA: ");
  Serial.print(I2C_SDA);
  Serial.print(", SCL: ");
  Serial.print(I2C_SCL);
  Serial.println(")...");
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Configure fast I2C clock (400kHz stable max for ESP8266)
  Serial.println("Setting I2C clock to Fast Mode (400 kHz)...");
  Wire.setClock(400000); 

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("[ERROR] SSD1306 allocation failed. Check wiring and address (0x3C)."));
    while (true) {
      delay(1000);
    }
  }
  
  Serial.println("SSD1306 initialized successfully.");

  display.clearDisplay();
  display.display();
  
  last_fps_report_time = millis();
}

void loop() {
  int frame_limit = TEST_MODE ? TEST_FRAMES : TOTAL_FRAMES;
  
  for (int i = 0; i < frame_limit; i++) {
    uint32_t frame_start_us = micros();
    
    display.clearDisplay();
    display.drawBitmap(0, 0, video_frames[i], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    
    uint32_t render_end_us = micros();
    uint32_t render_time_us = render_end_us - frame_start_us;
    
    frames_rendered++;
    
    uint32_t current_time_ms = millis();
    if (current_time_ms - last_fps_report_time >= 1000) {
      actual_fps = (float)frames_rendered * 1000.0 / (current_time_ms - last_fps_report_time);
      frames_rendered = 0;
      last_fps_report_time = current_time_ms;
      
      Serial.print("Playback Stats -> Frame: ");
      Serial.print(i + 1);
      Serial.print("/");
      Serial.print(frame_limit);
      Serial.print(" | Render Time: ");
      Serial.print((float)render_time_us / 1000.0, 2);
      Serial.print(" ms | Actual FPS: ");
      Serial.print(actual_fps, 1);
      Serial.println(" Hz");
    }
    
    // Crucial on ESP8266: yield() prevents Watchdog Timer (WDT) resets
    yield(); 
    
    uint32_t elapsed_us = micros() - frame_start_us;
    if (elapsed_us < FRAME_PERIOD_US) {
      uint32_t delay_us = FRAME_PERIOD_US - elapsed_us;
      delayMicroseconds(delay_us);
    }
  }
}
