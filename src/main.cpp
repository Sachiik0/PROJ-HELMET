#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BILL_ACCEPTOR_PIN 2
#define BILL_INHIBIT_PIN 4
#define COIN_ACCEPTOR_PIN 3
#define COIN_INHIBIT_PIN 5
#define SOLENOID_PIN 8
#define BUZZER 7 
#define REED_SWITCH 6
#define BOUNCE_DURATION 50 // Define debounce duration in milliseconds

volatile int credit = 0;
volatile unsigned long lastBillDebounceTime = 0;
volatile unsigned long lastCoinDebounceTime = 0;

volatile int coinPulseCount = 0;  // To count the pulses from the coin acceptor

// Initialize the LCD with the I2C address 0x27 and 20 columns x 4 rows
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Function declarations
void billCounter();
void coinCounter();
void handleBillInput();
void handleCoinInput();
void disableAcceptors();
void enableAcceptors();
void openAndCloseSolenoid();

void setup() {
  Serial.begin(9600);
  
  pinMode(BILL_ACCEPTOR_PIN, INPUT_PULLUP);
  pinMode(COIN_ACCEPTOR_PIN, INPUT_PULLUP);
  pinMode(BILL_INHIBIT_PIN, OUTPUT);
  pinMode(COIN_INHIBIT_PIN, OUTPUT);
  pinMode(SOLENOID_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(REED_SWITCH, INPUT_PULLUP); // Assuming the reed switch is active low

  // Initially set inhibit pins to HIGH (disabled state)
  digitalWrite(BILL_INHIBIT_PIN, HIGH);
  digitalWrite(COIN_INHIBIT_PIN, HIGH);
  digitalWrite(SOLENOID_PIN, LOW); // Ensure solenoid is initially closed
  digitalWrite(BUZZER, LOW); // Ensure buzzer is initially off

  // Initialize the LCD with the number of columns and rows
  lcd.begin(20, 4); // 20 columns and 4 rows
  lcd.backlight(); // Turn on the LCD backlight
  lcd.setCursor(0, 0);
  lcd.print("Insert Coin");

  attachInterrupt(digitalPinToInterrupt(BILL_ACCEPTOR_PIN), billCounter, CHANGE);
  attachInterrupt(digitalPinToInterrupt(COIN_ACCEPTOR_PIN), coinCounter, FALLING);
}

void openAndCloseSolenoid() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Door Open.");
  Serial.println("Door Open.");

  digitalWrite(SOLENOID_PIN, HIGH); // Open solenoid

  // Continuously check if the reed switch indicates the door is closed
  while (true) {
    if (digitalRead(REED_SWITCH) == LOW) { // Assuming LOW means door is closed
      // Stop the buzzer and update the display
      digitalWrite(BUZZER, LOW);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Door Closed");
      Serial.println("Door Closed.");
      digitalWrite(SOLENOID_PIN, LOW); // Close solenoid
      lcd.setCursor(0, 1);
      lcd.print("Door Secured");
      delay(2000); // Delay to ensure the door is secured properly
      break;
    } else {
      // Door is open, sound the buzzer
      digitalWrite(BUZZER, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("Door Open - Buzz");
      delay(100); // Small delay to avoid rapid looping
    }
  }
}

void loop() {
  // Check if the coin pulses have been counted and process them
  if (coinPulseCount > 0) {
    handleCoinInput();
  }

  // Check if credit has reached 50
  if (credit >= 50) {
    openAndCloseSolenoid();
    credit = 0;  // Reset credit after opening the solenoid
  }
  
  // Enable bill and coin acceptors if credit is less than 50
  if (credit < 50) {
    enableAcceptors();
  }

  // Display the current credit value on the LCD at the end of the loop
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Insert Credit");
  lcd.setCursor(0, 1);
  lcd.print("Credit: ");
  lcd.print(credit);

  delay(1000); // Delay to prevent rapid updates (adjust as needed)
}

// ISR for bill acceptor pin
void billCounter() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastBillDebounceTime > BOUNCE_DURATION) {
    if (!digitalRead(BILL_ACCEPTOR_PIN)) {
      handleBillInput();
    }
    lastBillDebounceTime = currentTime; // Update last debounce time
  }
}

// ISR for coin acceptor pin
void coinCounter() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastCoinDebounceTime > BOUNCE_DURATION) {
    coinPulseCount++;  // Increment pulse count for every pulse
    lastCoinDebounceTime = currentTime; // Update last debounce time
  }
}

void handleBillInput() {
  // Check if adding bill would exceed credit limit
  if (credit + 10 <= 50) {
    credit += 10; // Increase credit by 10 for bills
    Serial.print("Credit: ");
    Serial.println(credit);
  } else {
    // Disable bill and coin acceptors
    disableAcceptors();
  }
}

void handleCoinInput() {
  // Add the value of the coin based on pulse count
  if (credit + coinPulseCount <= 50) {
    credit += coinPulseCount;  // Increase credit by the number of pulses
    Serial.print("Credit: ");
    Serial.println(credit);
    
    // Display the updated credit on the LCD
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Credit: ");
    lcd.print(credit);
  } else {
    // Disable bill and coin acceptors
    disableAcceptors();
    
    Serial.println("Credit limit reached, no more coins accepted.");
  }
  coinPulseCount = 0;  // Reset pulse count after processing
}

void disableAcceptors() {
  digitalWrite(BILL_INHIBIT_PIN, LOW);
  digitalWrite(COIN_INHIBIT_PIN, LOW);
}

void enableAcceptors() {
  digitalWrite(BILL_INHIBIT_PIN, HIGH);
  digitalWrite(COIN_INHIBIT_PIN, HIGH);
}
