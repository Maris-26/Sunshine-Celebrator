#include <Arduino.h>
// #include <BLEDevice.h>         // Uncomment when using BLE
// #include <BLEUtils.h>
// #include <BLEScan.h>
// #include <BLEAdvertisedDevice.h>

// Stepper motor control pins (X27.168)
#define IN1  0  // D0 -> Coil A1
#define IN2  1  // D1 -> Coil A2
#define IN3  3  // D3 -> Coil B1
#define IN4  2  // D2 -> Coil B2

// Button pin for toggling motor on/off (Updated to GPIO D8)
#define BUTTON_PIN 8  // Button connected to ESP32-C3 D8

// UUIDs for BLE Service and Characteristic (Used for communication with the server)
// static BLEUUID serviceUUID("35b88443-045d-4687-8fc4-a79c946baab8");
// static BLEUUID charUUID("c5cc25dd-2467-4829-87bc-55ccf7bob622");

// Step sequence (Full step mode)
const int step_sequence[4][4] = {
    {1, 0, 0, 0},  // Step 1
    {0, 1, 0, 0},  // Step 2
    {0, 0, 1, 0},  // Step 3
    {0, 0, 0, 1}   // Step 4
};

// Variables for motor state and button control
bool motorRunning = true;  // Motor starts ON
bool lastButtonState = HIGH;  // Button state tracking
int currentState = 1;  // The stepper motor cycles through 4 states

// Simulated BLE values (Replace with real BLE input when available)
float Smoothed = 1200;  // Placeholder light intensity
float Rate = 30;        // Placeholder rate of change

// Function to move stepper motor one step
void stepMotor(int step, int speedMultiplier) {
    digitalWrite(IN1, step_sequence[step][0]);
    digitalWrite(IN2, step_sequence[step][1]);
    digitalWrite(IN3, step_sequence[step][2]);
    digitalWrite(IN4, step_sequence[step][3]);
    delay(5 / speedMultiplier); // Adjust speed based on multiplier
}

// Function to rotate stepper motor based on speed & direction
void rotateMotor(bool clockwise, int speedMultiplier) {
    int delayTime = 5 / speedMultiplier;  // Adjust speed

    Serial.print("🔄 ");
    Serial.print(clockwise ? "Clockwise" : "Counterclockwise");
    Serial.print(" | Speed Multiplier: ");
    Serial.println(speedMultiplier);

    for (int i = 0; i < 512; i++) {  // 512 steps = 1 full rotation
        int stepIndex = clockwise ? (i % 4) : (3 - (i % 4));  // Set direction
        stepMotor(stepIndex, speedMultiplier);
        delay(delayTime);
    }

    Serial.println("⏸️ Pause 1s");
    delay(1000);
}

// Function to update motor state based on BLE data (Commented Out for Now)
/*
void updateMotorState() {
    if (Smoothed > 1000 && abs(Rate) <= 50) {
        currentState = 1;  // Normal CW
    } else if (Smoothed <= 1000 && abs(Rate) <= 50) {
        currentState = 2;  // Normal CCW
    } else if (Smoothed > 1000 && abs(Rate) > 50) {
        currentState = 3;  // Fast CW
    } else if (Smoothed <= 1000 && abs(Rate) > 50) {
        currentState = 4;  // Fast CCW
    }
}
*/

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("✅ ESP32C3 Initialized - Stepper Motor + Button + BLE");

    // Stepper motor pin setup
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    
    // Button setup (internal pull-up)
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    Serial.println("✅ Stepper Motor and Button Ready");

    // Uncomment this when BLE is available
    /*
    BLEDevice::init("ESP32C3_CLIENT");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
    */
}

void loop() {
    // Read button state (Active LOW)
    bool buttonState = digitalRead(BUTTON_PIN);

    if (buttonState == LOW && lastButtonState == HIGH) {
        motorRunning = !motorRunning;  // Toggle motor state
        Serial.print("🎯 Button Pressed - Motor ");
        Serial.println(motorRunning ? "Running" : "Stopped");
        delay(300);  // Debounce delay to avoid multiple detections
    }

    lastButtonState = buttonState;

    if (motorRunning) {
        // updateMotorState(); // Would be used if BLE was working

        Serial.print("🔄 Motor State: ");
        Serial.println(currentState);

        // Cycle through stepper motor states in order (instead of BLE-controlled)
        switch (currentState) {
            case 1:
                Serial.println("➡️ Mode 1: Normal CW");
                rotateMotor(true, 1);
                break;
            case 2:
                Serial.println("⬅️ Mode 2: Normal CCW");
                rotateMotor(false, 1);
                break;
            case 3:
                Serial.println("⚡ Mode 3: Fast CW");
                rotateMotor(true, 2);
                break;
            case 4:
                Serial.println("⚡ Mode 4: Fast CCW");
                rotateMotor(false, 2);
                break;
        }

        // Cycle to the next state after each full rotation
        currentState = (currentState % 4) + 1;
    } else {
        Serial.println("⏸️ Motor Stopped...");
        delay(100);
    }
}
