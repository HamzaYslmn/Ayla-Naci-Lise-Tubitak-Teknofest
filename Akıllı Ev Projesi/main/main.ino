#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP32Servo.h>

// Wi‑Fi credentials
const char* ssid     = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Telegram BOT
#define BOTtoken "XXXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#define CHAT_ID  "XXXXXXXXXX"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

const int ledPin       = 2;
bool      ledState     = LOW;
unsigned long lastTime = 0;
const int  pollDelay   = 1000;

// PIR sensor and alarm
const int pirPin        = 4;
bool      alarmOn      = false;
bool      lastPirState = LOW;

// MQ-9 Gas sensor
const int mq9Pin = 34;
const int mq9Threshold = 1000;
bool gasAlarmOn = false;

// Relay
const int relayPin = 5;
bool relayState = LOW;

// DHT11 sensor
#define DHTPIN  15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float lastTemp = 0;
float lastHum  = 0;

// LDR sensor
const int ldrPin = 35;
int lastLdrValue = -1;
const int ldrThreshold = 2000;
bool ldrAlarmOn = false;

// Servo motor
const int servoPin = 13;
Servo myServo;
int servoAngle = 0;
bool servoAttached = false;

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, ledState);

    pinMode(pirPin, INPUT);
    pinMode(mq9Pin, INPUT);

    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, relayState);

    pinMode(ldrPin, INPUT);

    dht.begin();

    myServo.attach(servoPin);
    servoAttached = true;
    myServo.write(servoAngle);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected, IP: ");
    Serial.println(WiFi.localIP());
}

void sendMenu(const String &chat_id, const String &from) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) lastTemp = t;
    if (!isnan(h)) lastHum = h;

    int ldrValue = analogRead(ldrPin);
    lastLdrValue = ldrValue;

    String menu = "Welcome, " + from + ".\n";
    menu += "-------------------------------\n";
    menu += "| Item        | State         |\n";
    menu += "-------------------------------\n";
    menu += "| LED         | " + String(ledState ? "ON " : "OFF") + "           |\n";
    menu += "| Alarm       | " + String(alarmOn ? "ON " : "OFF") + "           |\n";
    menu += "| Gas Alarm   | " + String(gasAlarmOn ? "ON " : "OFF") + "           |\n";
    menu += "| LDR Alarm   | " + String(ldrAlarmOn ? "ON " : "OFF") + "           |\n";
    menu += "| Relay       | " + String(relayState ? "ON " : "OFF") + "           |\n";
    menu += "| Servo       | " + String(servoAngle) + " deg      |\n";
    menu += "-------------------------------\n";
    menu += "Commands:\n";
    menu += "/toggle_led   /toggle_alarm\n";
    menu += "/toggle_gas   /toggle_relay\n";
    menu += "/toggle_ldr   /servo_0\n";
    menu += "/servo_90     /servo_180\n";
    menu += "\n";
    menu += "DHT11 Sensor:\n";
    menu += "Temperature: ";
    menu += isnan(lastTemp) ? "--" : String(lastTemp, 1) + " C";
    menu += "\nHumidity: ";
    menu += isnan(lastHum) ? "--" : String(lastHum, 1) + " %";
    menu += "\n";
    menu += "LDR Value: ";
    menu += String(ldrValue);
    menu += "\n";
    bot.sendMessage(chat_id, menu, "");
}

void processCommand(const String &cmd, const String &chat_id, const String &from) {
    if (chat_id != CHAT_ID) {
        bot.sendMessage(chat_id, "Unauthorized user", "");
        return;
    }

    if (cmd == "/start" || cmd == "/help" || cmd == "/menu") {
        sendMenu(chat_id, from);
        return;
    }

    if (cmd == "/toggle_led") {
        ledState = !ledState;
        digitalWrite(ledPin, ledState);
        bot.sendMessage(chat_id, ledState ? "LED turned ON" : "LED turned OFF", "");
        return;
    }

    if (cmd == "/toggle_alarm") {
        alarmOn = !alarmOn;
        bot.sendMessage(chat_id, alarmOn ? "Alarm is now ON" : "Alarm is now OFF", "");
        Serial.println(alarmOn ? "Alarm enabled" : "Alarm disabled");
        return;
    }

    if (cmd == "/toggle_gas") {
        gasAlarmOn = !gasAlarmOn;
        bot.sendMessage(chat_id, gasAlarmOn ? "Gas alarm is now ON" : "Gas alarm is now OFF", "");
        Serial.println(gasAlarmOn ? "Gas alarm enabled" : "Gas alarm disabled");
        return;
    }

    if (cmd == "/toggle_ldr") {
        ldrAlarmOn = !ldrAlarmOn;
        bot.sendMessage(chat_id, ldrAlarmOn ? "LDR alarm is now ON" : "LDR alarm is now OFF", "");
        Serial.println(ldrAlarmOn ? "LDR alarm enabled" : "LDR alarm disabled");
        return;
    }

    if (cmd == "/toggle_relay") {
        relayState = !relayState;
        digitalWrite(relayPin, relayState);
        bot.sendMessage(chat_id, relayState ? "Relay turned ON" : "Relay turned OFF", "");
        Serial.println(relayState ? "Relay turned ON" : "Relay turned OFF");
        return;
    }

    // Simplified servo control
    if (cmd.startsWith("/servo ")) {
        int degree = cmd.substring(7).toInt();
        if (degree >= 0 && degree <= 180) {
            if (!servoAttached) {
                myServo.attach(servoPin);
                servoAttached = true;
            }
            servoAngle = degree;
            myServo.write(servoAngle);
            bot.sendMessage(chat_id, "Servo moved to " + String(servoAngle) + " degrees", "");
            Serial.println("Servo moved to " + String(servoAngle) + " degrees");
        } else {
            bot.sendMessage(chat_id, "Invalid servo angle. Use 0-180.", "");
        }
        return;
    }
}

void handleNewMessages() {
    int num = bot.getUpdates(bot.last_message_received + 1);
    for (int i = 0; i < num; i++) {
        String cid  = String(bot.messages[i].chat_id);
        String txt  = bot.messages[i].text;
        String from = bot.messages[i].from_name;
        processCommand(txt, cid, from);
    }
}

void checkPirSensor() {
    bool currentPir = digitalRead(pirPin);
    if (alarmOn && currentPir && !lastPirState) {
        Serial.println("Motion detected! Thief alert!");
        bot.sendMessage(CHAT_ID, "Alert! Motion detected. Possible thief!", "");
    }
    lastPirState = currentPir;
}

void checkGasSensor() {
    int mq9Value = analogRead(mq9Pin);
    if (gasAlarmOn && mq9Value > mq9Threshold) {
        Serial.println("Gas detected! MQ-9 value: " + String(mq9Value));
        bot.sendMessage(CHAT_ID, "Alert! Dangerous gas detected!", "");
        delay(5000);
    }
}

void checkLdrSensor() {
    int ldrValue = analogRead(ldrPin);
    if (ldrAlarmOn && ldrValue < ldrThreshold) {
        Serial.println("LDR alert! Low light detected. LDR value: " + String(ldrValue));
        bot.sendMessage(CHAT_ID, "Alert! Low light detected by LDR!", "");
        delay(5000);
    }
    lastLdrValue = ldrValue;
}

void loop() {
    unsigned long now = millis();
    if (now - lastTime >= pollDelay) {
        handleNewMessages();
        lastTime = now;
    }

    checkPirSensor();
    checkGasSensor();
    checkLdrSensor();
}