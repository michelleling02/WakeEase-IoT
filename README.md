# **WakeEase: Smart Bedroom Automation**

### **Project Overview**
**WakeEase** is a smart IoT-based system designed to enhance wake-up routines, optimize energy usage, and promote better sleep habits. It leverages automation, real-time tracking, and cloud-based analytics to provide users with a comfortable and efficient bedroom environment.

---

### **Features**
1. **Alarm Automation**:
   - Set alarms via the Serial Monitor (`SETALARM hh:mm`).
   - Activates LED and servo motor as part of the alarm routine.
   - Automatically turns off the fan if no response is detected after a timeout (default: 10 seconds).
   - Logs response time when the button is pressed to stop the alarm.

2. **Smart Fan and LED Control**:
   - Automatically controls fan and LED based on presence detection:
     - **Main Area Presence**: Turns on fan and LED.
     - **Bed Area Presence**: Turns on fan and turns off LED.
   - Turns off fan and LED automatically after 2 seconds of inactivity.

3. **Sleep Tracking**:
   - Logs sleep start and end times based on bed presence detection.
   - Publishes sleep durations to the MQTT topic.

4. **Cloud Integration via MQTT**:
   - Publishes real-time data and logs to various MQTT topics:
     - `notification`: Logs events such as LED/Fan state changes and alarm triggers.
     - `led_duration` & `fan_duration`: Tracks ON durations of the devices.
     - `response_time`: Logs how quickly the user responds to alarms.
     - `sleep_duration`: Tracks sleep session durations.
   - Reconnects automatically if MQTT connection is lost.

5. **Energy Efficiency**:
   - Ensures fan and LED are off when no presence is detected.
   - Optimizes energy usage by using sensors and automation.

6. **Real-Time Clock and Time Synchronization**:
   - Synchronizes system time using NTP (Malaysia Time, UTC+8).
   - Provides accurate timestamps for event logging.

---

### **Hardware Components**
- **ESP32 NodeMCU**: Main microcontroller for the system.
- **IR Motion Sensors**:
  - **IR_PIN_1**: Detects presence in the main area.
  - **IR_PIN_2**: Detects presence in the bed area for sleep tracking.
- **LED Lights**: Represents bedroom light.
- **Relay Module**: Controls the fan.
- **Servo Motor**: Represents WakeEase alarm.
- **Push Button**: Allows users to stop the alarm.

---

### **MQTT Topics**
- **`notification`**: Logs all significant events (e.g., LED/Fan state changes, alarm triggers).
- **`led_duration`**: Tracks and logs the duration of LED usage.
- **`fan_duration`**: Tracks and logs the duration of fan usage.
- **`response_time`**: Logs the time taken to respond to alarms.
- **`sleep_duration`**: Publishes sleep session durations.

---

### **Project Files Overview**
- **[WakeEase.ino](https://github.com/michelleling02/WakeEase-IoT/blob/main/WakeEase.ino)**: Arduino sketch for ESP32.
- **[WakeEase_script.py](https://github.com/michelleling02/WakeEase-IoT/blob/main/wakeEase_script.py)**: Python script for MQTT-MongoDB integration.
- **[WakeEase-IoT Connection Setup.pdf](https://github.com/michelleling02/WakeEase-IoT/blob/main/WakeEase-IoT%20Connection%20Setup.pdf)**: Visual guideline for the IoT connection setup.
- **1. [Setting Up Arduino IDE.pdf](https://github.com/michelleling02/WakeEase-IoT/blob/main/%5B1%5D%20Setting%20Up%20Arduino%20IDE.pdf)**: Guide for Arduino IDE configuration.
- **2. [Setting Up a Virtual Machine on Google Cloud Platform.pdf](https://github.com/michelleling02/WakeEase-IoT/blob/main/%5B2%5D%20Setting%20Up%20a%20Virtual%20Machine%20on%20Google%20Cloud%20Platform%20(GCP).pdf)**: VM setup instructions.
- **3. [Setting Up MongoDB and MongoDB Charts.pdf](https://github.com/michelleling02/WakeEase-IoT/blob/main/%5B3%5D%20Setting%20Up%20MongoDB%20and%20MongoDB%20Charts.pdf)**: MongoDB installation and configuration guide.
- **4. [Setting Up MQTT Server and MQTT-MongoDB Connection.pdf](https://github.com/michelleling02/WakeEase-IoT/blob/main/%5B4%5D%20Setting%20Up%20MQTT%20Server%20and%20MQTT-MongoDB%20Connection.pdf)**: Guide for MQTT and MongoDB integration.
- **5. [Setting Up Dashboard in MongoDB Charts.pdf](https://github.com/michelleling02/WakeEase-IoT/blob/main/%5B5%5D%20Setting%20Up%20Dashboard%20in%20MongoDB%20Charts.pdf)**: Steps to create and visualize dashboards in MongoDB Charts.

Note: Please follow the sequence of these files to ensure the system is configured and set up correctly. Each step builds on the previous one, so skipping or rearranging may result in errors during setup.
