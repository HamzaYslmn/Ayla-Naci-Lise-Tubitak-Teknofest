#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT11.h>

// WiFi bilgileri
const char* ssid = "a";
const char* password = "12345667";

// Telegram Bot bilgileri
#define BOTtoken "***"
#define CHAT_ID "***"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Bot sorgu aralığı
int botRequestDelay = 1000;
unsigned long lastTimeBotRan = 0;

const int ledPin = 12;
bool ledState = LOW;

// DHT11 sensör pini
DHT11 dht11(4);

#define PIR_PIN 13           // PIR sensörünün bağlı olduğu GPIO pini
#define BUZZER_PIN 25        // Buzzer'ın bağlı olduğu GPIO pini (örn: 25)
#define GAS_SENSOR_PIN 34    // Gaz sensörünün bağlı olduğu analog pin

bool lastMotionState = LOW;     // Son hareket durumu
bool alarmMode = false;         // Alarm modu başlangıçta kapalı
bool gasAlertSent = false;      // Gaz bildirimi gönderildi mi?

// Yeni mesajları işleme fonksiyonu
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;
    Serial.println(text);

    int atIndex = text.indexOf('@');
    if (atIndex != -1) {
      text = text.substring(0, atIndex);
    }

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/led_toggle to toggle LED\n";
      welcome += "/state to get current LED state\n";
      welcome += "/temperature to get DHT11 readings\n";
      welcome += "/alarm_on to activate alarm mode\n";
      welcome += "/alarm_off to deactivate alarm mode\n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/led_toggle") {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      String msg = ledState ? "LED ON" : "LED OFF";
      bot.sendMessage(chat_id, "LED state toggled: " + msg, "");
    }

    if (text == "/state") {
      String alarmMsg = alarmMode ? "Alarm mode: ON\n" : "Alarm mode: OFF\n";
      if (digitalRead(ledPin))
        bot.sendMessage(chat_id, alarmMsg + "LED is ON", "");
      else
        bot.sendMessage(chat_id, alarmMsg + "LED is OFF", "");
    }

    if (text == "/temperature") {
      int temperature = 0;
      int humidity = 0;
      int result = dht11.readTemperatureHumidity(temperature, humidity);
      if (result == 0) {
        String msg = "Temperature: " + String(temperature) + " °C\n";
        msg += "Humidity: " + String(humidity) + " %";
        bot.sendMessage(chat_id, msg, "");
      } else {
        bot.sendMessage(chat_id, DHT11::getErrorString(result), "");
      }
    }

    if (text == "/alarm_on") {
      alarmMode = true;
      digitalWrite(ledPin, ledState); // LED'i önceki duruma getir
      bot.sendMessage(chat_id, "Alarm modu AKTIF!", "");
    }

    if (text == "/alarm_off") {
      alarmMode = false;
      digitalWrite(ledPin, ledState); // LED'i önceki duruma getir
      bot.sendMessage(chat_id, "Alarm modu PASIF!", "");
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);

  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(GAS_SENSOR_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

  // Sensör ısınması için bekleme (MQ sensörleri için gerekebilir)
  Serial.println("Gaz sensörü ısınması için 10 saniye bekleniyor...");
  delay(10000);
}

void loop() {
  // PIR sensöründen veri oku ve hareket algılaması
  int motionDetected = digitalRead(PIR_PIN);
  if (motionDetected == HIGH && lastMotionState == LOW) {
    if (alarmMode) {
      Serial.println("ALARM! Hareket algılandı!");
      bot.sendMessage(CHAT_ID, "ALARM! Hareket algılandı!", "");
      digitalWrite(ledPin, ledState);
    } else {
      Serial.println("Hareket algılandı! LED yanıyor.");
      digitalWrite(ledPin, HIGH);
    }
    lastMotionState = HIGH;
  }
  else if (motionDetected == LOW && lastMotionState == HIGH) {
    Serial.println("Hareket algılanmadı. LED kapalı.");
    digitalWrite(ledPin, ledState);
    lastMotionState = LOW;
  }

  // Gaz sensöründen veri oku
  int gasValue = analogRead(GAS_SENSOR_PIN);
  Serial.print("Gaz sensör değeri: ");
  Serial.println(gasValue);

  // Gaz eşik değeri kontrolü:
  // Eğer gaz değeri eşik değerden yüksekse buzzer'ı çalıştır,
  // aksi durumda buzzer kapalı kalır.
  int gasThreshold = 500; // Eşik değeri (sensör tipine ve ortam koşullarına göre ayarlanır)
  if (gasValue > gasThreshold) {
    buzzerOn();
    if (!gasAlertSent) {
      bot.sendMessage(CHAT_ID, "Dikkat! Gaz seviyesi yüksek!", "");
      gasAlertSent = true;
    }
  } else {
    // Gaz değeri eşik altındaysa buzzer çalışmamalı
    buzzerOff();
    gasAlertSent = false;
  }

  // Telegram bot mesajlarını kontrol et
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  delay(200); // Döngü arası bekleme
}

void buzzerOn() {
  digitalWrite(BUZZER_PIN, HIGH);
}

void buzzerOff() {
  digitalWrite(BUZZER_PIN, LOW);
}