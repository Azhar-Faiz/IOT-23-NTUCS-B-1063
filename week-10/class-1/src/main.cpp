#include <WiFi.h>

// -------- Wi-Fi credentials --------
const char* ssid     = "Wifi";       // Replace with your 2.4 GHz Wi-Fi SSID
const char* password = "shahid@152!";   // Replace with your Wi-Fi password

// -------- Static IP configuration --------
IPAddress local_IP(192, 168, 18, 200);     // ESP32 static IP (must be free)
IPAddress gateway(192, 168, 1, 1);        // Router IP
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);         // Optional
IPAddress secondaryDNS(8, 8, 4, 4);       // Optional

// -------- Web server & LED pin --------
WiFiServer server(80);       // HTTP server on port 80
const int LED_PIN = 2;       // Built-in LED

void setup() {
  Serial.begin(115200);

  // LED setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Set Static IP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Static IP Failed, using DHCP");
  }

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Timeout after 20 seconds
    if (millis() - startTime > 20000) {
      Serial.println("\nFailed to connect Wi-Fi. Restarting...");
      ESP.restart();  // Restart ESP if it cannot connect
    }
  }

  Serial.println("\nConnected!");
  Serial.print("ESP32 Static IP: ");
  Serial.println(WiFi.localIP());

  // Start web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;  // No client, exit loop()

  Serial.println("New Client connected");

  String requestLine = "";
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (c == '\r') continue;
      if (c == '\n') break;
      requestLine += c;
    }
  }
  Serial.println(requestLine);

  // LED control
  if (requestLine.indexOf("GET /LED=ON") != -1) {
    digitalWrite(LED_PIN, HIGH);
  } else if (requestLine.indexOf("GET /LED=OFF") != -1) {
    digitalWrite(LED_PIN, LOW);
  }

  // HTML response
  String htmlPage =
    "<!DOCTYPE html><html>"
    "<head><meta charset='UTF-8'><title>ESP32 LED Control</title></head>"
    "<body>"
    "<h1>ESP32 LED Control</h1>"
    "<p><a href=\"/LED=ON\"><button>LED ON</button></a></p>"
    "<p><a href=\"/LED=OFF\"><button>LED OFF</button></a></p>"
    "</body></html>";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println(htmlPage);

  delay(1);
  client.stop();
  Serial.println("Client disconnected");
}
