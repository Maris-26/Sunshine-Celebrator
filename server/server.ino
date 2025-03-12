#include <Arduino.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino_APDS9960.h>  // Light sensor library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>  // OLED display library

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID        "35b88443-045d-4687-8fc4-a79c946baab8"
#define CHARACTERISTIC_UUID "c5cc25dd-2467-4829-87bc-55ccf7b0b622"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;

// APDS9960 light sensor
APDS9960 apds(Wire, -1);  // I2C interface, no interrupt

// Signal processing parameters
#define FILTER_SIZE 5  // Moving average window size
#define ALPHA 0.3      // EWMA coefficient (0.0 - 1.0)

// Variables for filtering
int lightBuffer[FILTER_SIZE] = {0};
int bufferIndex = 0;
int raw_light = 0;
float filtered_light = 0;
float last_filtered_light = 0;
float rate = 0;  // Rate of change

// BLE connection callback
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("Client connected");
        deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
        Serial.println("Client disconnected, restarting BLE advertising...");
        deviceConnected = false;
        delay(100);
        pServer->getAdvertising()->start();
    }
};

// Moving Average Filter
float movingAverage(int newValue) {
    lightBuffer[bufferIndex] = newValue;
    bufferIndex = (bufferIndex + 1) % FILTER_SIZE;  
    int sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sum += lightBuffer[i];
    }
    return sum / (float)FILTER_SIZE;
}

// Exponential Weighted Moving Average (EWMA)
float exponentialMovingAverage(float newValue, float prevFiltered) {
    return (ALPHA * newValue) + ((1 - ALPHA) * prevFiltered);
}

// Function to update OLED display
void updateOLED() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    
    display.setCursor(0, 0);
    display.print("Raw: ");
    display.println(raw_light);
    
    display.setCursor(0, 20);
    display.print("Smoothed: ");
    display.println(filtered_light);
    
    display.setCursor(0, 40);
    display.print("Rate: ");
    display.println(rate);
    
    display.display();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE Server");

    // Initialize OLED
    Wire.begin(9, 8);  // Ensure correct SDA & SCL pins
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  
        Serial.println("OLED initialization failed");
        while (1);
    }
    Serial.println("OLED initialized");
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("BLE Server Ready");
    display.display();
    delay(2000);

    // Initialize light sensor
    if (!apds.begin()) {
        Serial.println("Sensor initialization failed");
        while (1);
    }
    Serial.println("Sensor initialized");

    // Initialize BLE
    BLEDevice::init("ESP32C3_SERVER");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE Server broadcasting");
}

void loop() {
    if (apds.colorAvailable()) {
        int r, g, b, c;
        if (apds.readColor(r, g, b, c)) {
            raw_light = c;  // Read light intensity

            // Signal Processing
            float ma_light = movingAverage(raw_light);  
            filtered_light = exponentialMovingAverage(ma_light, filtered_light);
            rate = filtered_light - last_filtered_light;
            last_filtered_light = filtered_light;

            Serial.print("Raw: ");
            Serial.print(raw_light);
            Serial.print(" | Smoothed: ");
            Serial.print(filtered_light);
            Serial.print(" | Rate: ");
            Serial.println(rate);

            // Update OLED display
            updateOLED();

            // BLE Transmission
            if (deviceConnected) {
                char data[20];
                snprintf(data, sizeof(data), "%.1f,%.1f", filtered_light, rate);
                pCharacteristic->setValue(data);
                pCharacteristic->notify();
                Serial.println("Sent: " + String(data));
            }
        }
    }
    delay(500);  // Sample every 500ms
}
