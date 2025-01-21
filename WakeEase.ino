#include <PubSubClient.h>
#include <WiFi.h>
#include <time.h>
#include <ESP32Servo.h>

// Wi-Fi Credentials
const char* ssid = "yourwifi";  // Replace with your Wi-Fi SSID
const char* password = "yourwifipassword";    // Replace with your Wi-Fi password

// MQTT Credentials
const char* MQTT_SERVER = "yourExternalIP"; // Replace with your VM External IP
const int MQTT_PORT = 1883;               // Non-TLS communication port
WiFiClient espClient;
PubSubClient client(espClient);

/* 
  GPIO pin assignments for IR sensor, 
  button, relay, LED and servo motor
*/
#define IR_PIN 22
#define BUTTON_PIN 23
#define RELAY_PIN 26
#define LED_PIN 27
#define SERVO_PIN 32

// Servo object
Servo Myservo;

/* 
  Variables to store the alarm time 
  prompted by user (hour and minute)
*/
int alarmHour;
int alarmMinute;

/*
  Variables for tracking Fan-LED duration: 
  previous state, start time, and ON duration
*/
bool lastLEDState = false;
unsigned long ledStartTime = 0;
unsigned long ledDuration = 0;

/*
  Variables for handling alarm routine including 
  managing IR activity and calculating response time
*/
bool isAlarmActive = false;
bool isFanOffDueToTimeout = false;
bool isSensorActive = true;
bool alarmTriggeredThisMinute = false;
unsigned long currentTime = 0;
unsigned long buttonPressTime = 0;
unsigned long buttonResponseTime = 0;
const unsigned long alarmTimeout = 10 * 1000;

// Buffers for formatted date-time and MQTT messages
char timeStr[50];
char msg[150];

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  
  client.setServer(MQTT_SERVER, MQTT_PORT);

  // Configure NTP for Malaysia Time (UTC +8)
  configTime(8 * 3600, 0, "pool.ntp.org");
  Serial.println("Time synchronized using NTP");

  // Set pin modes
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  /* 
    Initialize system states: turn OFF fan and LED, 
    attach servo motor, and set it to initial position
  */
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  Myservo.attach(SERVO_PIN);
  Myservo.write(0);

  Serial.println("WakeEase: Smart Bedroom Automation Initialized...");
  setAlarmTime(); // Prompt user to set the alarm time during setup
}

void loop() {
  struct tm timeInfo;

  // Get the current time
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to obtain time");
    delay(1000);
    return;
  }
  
  // Reconnect to MQTT server if disconnected
  if (!client.connected()) {
    reconnect();
  }

  client.loop(); // Handle MQTT messages

  // Read the states of the button, IR sensor, and current LED status
  int buttonState = !digitalRead(BUTTON_PIN);
  int irVal = isSensorActive ? !digitalRead(IR_PIN) : 0;
  bool currentLEDState = digitalRead(LED_PIN);

  // Log LED state transitions
  if (currentLEDState && !lastLEDState && !isAlarmActive) {
    ledStartTime = millis(); // Record LED ON time
    getFormattedTime(timeStr);
    sprintf(msg, "[%s] Fan-LED turned ON", timeStr);
    client.publish("notification", msg);
    Serial.println(msg);
  } else if (!currentLEDState && lastLEDState) {
    getFormattedTime(timeStr);
    sprintf(msg, "[%s] Fan-LED turned OFF", timeStr);
    client.publish("notification", msg);
    Serial.println(msg);
    ledDuration = millis() - ledStartTime; // Calculate LED ON duration
    sprintf(msg, "[%s] Fan-LED ON duration: %.2f seconds.", timeStr, ledDuration / 1000.0);
    client.publish("duration", msg);
    Serial.println(msg);
  }
  lastLEDState = currentLEDState; // Update last LED state

  /*
    Alarm routine: Trigger the alarm at the set time 
    and ensure it activates only once per minute
  */
  if (timeInfo.tm_hour == alarmHour && timeInfo.tm_min == alarmMinute) {
    if (!alarmTriggeredThisMinute) {
      triggerAlarm();
      alarmTriggeredThisMinute = true; // Prevent re-triggering within the same minute
    }
  } else {
    alarmTriggeredThisMinute = false; // Reset the trigger for the next matching time
  }

  /* 
    Manage alarm mode and non-alarm modes
    1. alarm mode: handle active alarm routines 
    2. non-alarm mode: control Fan-LED based on presence detection
  */
  if (isAlarmActive) {
    handleAlarmRoutine(buttonState);
  } 
  // Presence detected: Turn ON Fan-LED
  else if (irVal) {
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
  } 
  // No presence detected: Turn OFF Fan-LED after a delay
  else {
    delay(2000);
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
  }
}

// Prompts user to set the alarm time through serial input
void setAlarmTime() {
  int tempHour = -1;
  int tempMinute = -1;

  // Prompt for hour
  while (tempHour < 0 || tempHour > 23) {
    Serial.println("Enter the hour (0-23):");
    while (Serial.available() == 0) {
      // Wait for user input
    }
    tempHour = Serial.parseInt();
    while (Serial.available() > 0) {
      Serial.read(); // Clear the input buffer
    }
    if (tempHour < 0 || tempHour > 23) {
      Serial.println("Invalid hour. Please enter a value between 0 and 23.");
    }
  }

  // Prompt for minute
  while (tempMinute < 0 || tempMinute > 59) {
    Serial.println("Enter the minute (0-59):");
    while (Serial.available() == 0) {
      // Wait for user input
    }
    tempMinute = Serial.parseInt();
    while (Serial.available() > 0) {
      Serial.read(); // Clear the input buffer
    }
    if (tempMinute < 0 || tempMinute > 59) {
      Serial.println("Invalid minute. Please enter a value between 0 and 59.");
    }
  }

  // Set alarm time
  alarmHour = tempHour;
  alarmMinute = tempMinute;
  sprintf(msg, "Alarm time set to %02d%02dH", alarmHour, alarmMinute);
  client.publish("notification", msg);
  Serial.println(msg);
}

// Trigger the alarm
void triggerAlarm() {
  isAlarmActive = true;
  isFanOffDueToTimeout = false;
  isSensorActive = false; // Disable IR sensor during alarm
  ledStartTime = millis(); // Record alarm start time
  digitalWrite(LED_PIN, HIGH); // Turn ON LED
  getFormattedTime(timeStr);
  sprintf(msg, "[%s] Alarm triggered: Servo and LED ON", timeStr);
  client.publish("notification", msg);
  Serial.println(msg);
}

// Handle alarm routine when active
void handleAlarmRoutine(int buttonState) {
  currentTime = millis();
  digitalWrite(LED_PIN, HIGH);

  // Activate servo motor for alarm effect
  if ((currentTime / 500) % 2 == 0) {
    Myservo.write(90);
  } else {
    Myservo.write(0);
  }

  // Check for timeout to turn OFF fan
  if (currentTime - ledStartTime >= alarmTimeout && !isFanOffDueToTimeout) {
    if (digitalRead(RELAY_PIN) == HIGH) { // Check if the fan is ON
      digitalWrite(RELAY_PIN, LOW);  // Turn OFF fan
      getFormattedTime(timeStr);
      sprintf(msg, "[%s] Button not pressed after snooze time: Fan OFF", timeStr);
      client.publish("notification", msg);
      Serial.println(msg);
    }
  }

  // Stop alarm when button is pressed
  if (buttonState) {
    buttonPressTime = millis();
    buttonResponseTime = buttonPressTime - ledStartTime; // Calculate response time
    getFormattedTime(timeStr);
    sprintf(msg, "[%s] Button pressed after %.2f seconds.", timeStr, buttonResponseTime / 1000.0);
    client.publish("response_time", msg);
    Serial.println(msg);

    stopAlarm();
  }
}

// Stop the alarm
void stopAlarm() {
  isAlarmActive = false;
  isSensorActive = true; // Re-enable IR sensor
  Myservo.write(0); // Reset servo motor
  digitalWrite(RELAY_PIN, HIGH); // Turn ON fan
  getFormattedTime(timeStr);
  sprintf(msg, "[%s] Alarm stopped: Fan ON.", timeStr);
  client.publish("notification", msg);

  // Keep fan ON for 5 seconds before checking IR sensor
  delay(5000);

  // Check IR sensor after the delay
  int irVal = !digitalRead(IR_PIN);
  if (!irVal) {
    digitalWrite(RELAY_PIN, LOW); // Turn OFF fan if no presence detected
  } else {
    sprintf(msg, "[%s] Presence detected: Fan-LED remain ON.", timeStr);
    client.publish("notification", msg);
  }
}

// Reconnect to MQTT server
void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT server");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// Get formatted date and time string
void getFormattedTime(char* buffer) {
  struct tm timeInfo;
  if (getLocalTime(&timeInfo)) {
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
            timeInfo.tm_year + 1900,
            timeInfo.tm_mon + 1,
            timeInfo.tm_mday,
            timeInfo.tm_hour,
            timeInfo.tm_min,
            timeInfo.tm_sec);
  } else {
    strcpy(buffer, "Date/Time unavailable");
  }
}