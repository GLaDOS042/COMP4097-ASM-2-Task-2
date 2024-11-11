#include <M5StickCPlus2.h>
#include "M5_EzData.h"
#include <Wire.h>

// EzData settings
const char* ssid = "R-Present";
const char* password = "Presentrrs735";
const char* token = "ea29477c1f774cc8bd2426420230ef23";
const char* channel = "soil_moisture";

// Soil moisture threshold
const int MOISTURE_THRESHOLD = 2000;

// Calibration value constants
const int MOISTURE_MIN = 2900;  // Voltage value when probe just touches water surface
const int MOISTURE_MAX = 1630;  // Voltage value when probe is fully submerged

// Global variables
bool isReading = false;
bool isWifiConnected = false;
bool isSensorConnected = false;
unsigned long lastReadTime = 0;
unsigned long lastWifiCheckTime = 0;
const unsigned long READ_INTERVAL = 5000;
const unsigned long WIFI_CHECK_INTERVAL = 3000;  // WiFi reconnection interval

// UI constants
const int STATUS_BAR_HEIGHT = 20;
const int MAIN_TEXT_START = STATUS_BAR_HEIGHT + 5;
const int INDICATOR_SIZE = 12;
const int INDICATOR_MARGIN = 4;

// Sensor pin definition
const int ANALOG_PIN = 36;  // Analog read pin

void drawStatusBar() {
    // Draw status bar background
    M5.Lcd.fillRect(0, 0, M5.Lcd.width(), STATUS_BAR_HEIGHT, DARKGREY);
    
    // WiFi status
    int y_center = STATUS_BAR_HEIGHT / 2 - INDICATOR_SIZE / 2;
    
    // WiFi indicator
    M5.Lcd.fillRect(5, y_center, INDICATOR_SIZE, INDICATOR_SIZE, 
                    isWifiConnected ? GREEN : RED);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(5 + INDICATOR_SIZE + INDICATOR_MARGIN, 5);
    M5.Lcd.print("WiFi");
    
    // Sensor status
    int sensor_x = M5.Lcd.width() - 45 - INDICATOR_SIZE;
    M5.Lcd.fillRect(sensor_x, y_center, INDICATOR_SIZE, INDICATOR_SIZE, 
                    isSensorConnected ? GREEN : RED);
    M5.Lcd.setCursor(sensor_x + INDICATOR_SIZE + INDICATOR_MARGIN, 5);
    M5.Lcd.print("SENS");
}

// Check and attempt to reconnect WiFi
void checkWiFiConnection() {
    if (!isWifiConnected && (millis() - lastWifiCheckTime >= WIFI_CHECK_INTERVAL)) {
        Serial.println("Attempting to reconnect WiFi...");
        isWifiConnected = setupWifi(ssid, password);
        lastWifiCheckTime = millis();
    }
}

// Calculate vertical center position
int calculateVerticalCenter(int contentHeight) {
    int availableHeight = M5.Lcd.height() - STATUS_BAR_HEIGHT;
    int startY = STATUS_BAR_HEIGHT + (availableHeight - contentHeight) / 2;
    return startY;
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    
    // Set ADC resolution
    analogSetWidth(16);
    analogSetAttenuation(ADC_11db);  // Set attenuation to support larger input range
    
    M5.Lcd.setRotation(0);
    M5.Lcd.fillScreen(BLACK);
    
    // Initialize sensor pin
    pinMode(ANALOG_PIN, INPUT);
    
    // Connect to WiFi
    isWifiConnected = setupWifi(ssid, password);
    
    // Check if sensor is working properly
    int initialReading = analogRead(ANALOG_PIN);
    isSensorConnected = (initialReading > 0);
    
    // Initial display
    drawStatusBar();
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    
    // Calculate vertical center position (assuming text height is about 16 pixels)
    int startY = calculateVerticalCenter(16);
    M5.Lcd.setCursor(5, startY);
    M5.Lcd.println("Press A to start");
}

void loop() {
    M5.update();
    checkWiFiConnection();  // Periodically check WiFi connection
    
    if (M5.BtnA.wasPressed()) {
        isReading = !isReading;
        M5.Lcd.fillRect(0, MAIN_TEXT_START, M5.Lcd.width(), M5.Lcd.height() - STATUS_BAR_HEIGHT, BLACK);
        if (isReading) {
            int startY = calculateVerticalCenter(16);
            M5.Lcd.setCursor(5, startY);
            M5.Lcd.println("Reading...");
        }
    }
    
    if (M5.BtnB.wasPressed()) {
        isReading = false;
        M5.Lcd.fillRect(0, MAIN_TEXT_START, M5.Lcd.width(), M5.Lcd.height() - STATUS_BAR_HEIGHT, BLACK);
        int startY = calculateVerticalCenter(32);  // Two lines of text
        M5.Lcd.setCursor(5, startY);
        M5.Lcd.println("System reset");
        M5.Lcd.println("Press A to start");
    }
    
    if (isReading && (millis() - lastReadTime >= READ_INTERVAL)) {
        // Read soil moisture value (using 16-bit mode)
        analogSetWidth(16);  // Set to 16-bit resolution
        auto rawAnalog = analogRead(ANALOG_PIN);
        // Read millivolt value directly
        float voltage = analogReadMilliVolts(ANALOG_PIN);
        Serial.printf("Raw analog value: %d (0x%X)\n", rawAnalog, rawAnalog);
        Serial.printf("Voltage: %.2f mV\n", voltage);
        
        // Calculate moisture percentage (based on EarthBase class implementation)
        float moisturePercent;
        if (MOISTURE_MIN > MOISTURE_MAX) {
            if (voltage > MOISTURE_MIN) {
                moisturePercent = 0.0;
            } else if (voltage < MOISTURE_MAX) {
                moisturePercent = 100.0;
            } else {
                moisturePercent = abs(voltage - MOISTURE_MIN) / abs(MOISTURE_MAX - MOISTURE_MIN) * 100.0;
            }
        } else {
            if (voltage < MOISTURE_MIN) {
                moisturePercent = 0.0;
            } else if (voltage > MOISTURE_MAX) {
                moisturePercent = 100.0;
            } else {
                moisturePercent = abs(voltage - MOISTURE_MIN) / abs(MOISTURE_MAX - MOISTURE_MIN) * 100.0;
            }
        }
        
        int moistureAnalog = round(moisturePercent);  // Round to integer
        
        Serial.printf("Raw analog value: %d (0x%X)\n", rawAnalog, rawAnalog);
        Serial.printf("Voltage: %.2f mV\n", voltage);
        Serial.printf("Moisture: %d%%\n", moistureAnalog);
        
        // Check sensor status
        isSensorConnected = (rawAnalog > 0);
        
        if (isSensorConnected) {
            // Check WiFi status
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("WiFi disconnected before sending data");
                isWifiConnected = false;
            } else {
                Serial.println("WiFi connected, attempting to send data");
                // Send to EzData
                bool sendResult = setData(token, channel, moistureAnalog);
                Serial.printf("Send result: %s\n", sendResult ? "Success" : "Failed");
                
                if (sendResult) {
                    isWifiConnected = true;
                    lastWifiCheckTime = millis();
                } else {
                    isWifiConnected = false;
                    Serial.println("Failed to send data to EzData");
                }
            }
            
            // Update display
            drawStatusBar();
            M5.Lcd.fillRect(0, MAIN_TEXT_START, M5.Lcd.width(), M5.Lcd.height() - STATUS_BAR_HEIGHT, BLACK);
            
            // Calculate total height of display content (3 lines of text)
            int contentHeight = 16 * 3;
            int startY = calculateVerticalCenter(contentHeight);
            
            M5.Lcd.setTextSize(2);
            M5.Lcd.setCursor(5, startY);
            M5.Lcd.setTextColor(WHITE);
            M5.Lcd.printf("Moisture:\n%d%%", moistureAnalog);  // Add percentage symbol
            
            // Display moisture status
            if (moistureAnalog < 30) {  // Adjust threshold to percentage
                M5.Lcd.setTextColor(RED);
                M5.Lcd.println("\nNeed Water!");
            } else {
                M5.Lcd.setTextColor(GREEN);
                M5.Lcd.println("\nPlants OK");
            }
        } else {
            drawStatusBar();
            M5.Lcd.fillRect(0, MAIN_TEXT_START, M5.Lcd.width(), M5.Lcd.height() - STATUS_BAR_HEIGHT, BLACK);
            int startY = calculateVerticalCenter(16);
            M5.Lcd.setCursor(5, startY);
            M5.Lcd.println("Sensor Error");
        }
        
        lastReadTime = millis();
    }
    
    delay(100);
}