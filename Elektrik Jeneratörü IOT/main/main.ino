#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// WiFi credentials
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Pin definitions
const int LED_PIN = 2;    // Onboard LED (D4)
const int RELAY_PIN = 5;  // Relay (D1)

// Web server
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP8266 Control</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.0rem;}
    button {padding: 16px 40px; font-size: 22px; margin: 10px;}
  </style>
</head>
<body>
  <h2>ESP8266 LED & Relay Control</h2>
  <p>LED State: <span id="ledState">%LEDSTATE%</span></p>
  <button onclick="toggle('led')">Toggle LED</button>
  <p>Relay State: <span id="relayState">%RELAYSTATE%</span></p>
  <button onclick="toggle('relay')">Toggle Relay</button>
<script>
function toggle(device) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/toggle?device=" + device, true);
  xhr.send();
  setTimeout(function(){ location.reload(); }, 500);
}
</script>
</body>
</html>
)rawliteral";

String ledState = "OFF";
String relayState = "OFF";

String processor(const String& var) {
  if (var == "LEDSTATE") return ledState;
  if (var == "RELAYSTATE") return relayState;
  return String();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);    // LED OFF (active LOW)
  digitalWrite(RELAY_PIN, LOW);   // Relay OFF

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("device")) {
      String device = request->getParam("device")->value();
      if (device == "led") {
        int state = digitalRead(LED_PIN);
        digitalWrite(LED_PIN, !state);
        ledState = (digitalRead(LED_PIN) == LOW) ? "ON" : "OFF";
      } else if (device == "relay") {
        int state = digitalRead(RELAY_PIN);
        digitalWrite(RELAY_PIN, !state);
        relayState = (digitalRead(RELAY_PIN) == HIGH) ? "ON" : "OFF";
      }
    }
    request->send(200, "text/plain", "OK");
  });

  server.begin();
}

void loop() {
  // Nothing needed here
}