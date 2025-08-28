#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

WebServer server(80);
const char* ap_ssid = "ESP32_Hotspot";
const char* ap_pass = "12345678";


#define EEPROM_SIZE 512

#define SSID_ADDR 0
#define PASS_ADDR 32

void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 WiFi Setup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background-color: #000;
      color: #fff;
      text-align: center;
      margin: 0;
      padding: 20px;
    }
    h2 {
      margin-top: 40px;
      font-size: 24px;
    }
    button {
      padding: 12px 25px;
      background-color: #1e90ff;
      color: white;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      font-size: 16px;
      margin-top: 20px;
    }
    button:hover {
      background-color: #4682b4;
    }
    #wifi-box {
      display: none;
      background-color: #111;
      border: 1px solid #444;
      border-radius: 10px;
      width: 90%;
      max-width: 400px;
      margin: 30px auto;
      padding: 15px;
      box-shadow: 0 0 10px rgba(0,0,0,0.5);
    }
    .wifi-item {
      padding: 12px;
      border-bottom: 1px solid #333;
      cursor: pointer;
      transition: background-color 0.2s ease;
    }
    .wifi-item:hover {
      background-color: #222;
    }
  </style>
</head>
<body>
  <h2>ESP32 WiFi Setup</h2>
  <button onclick="scanWifi()">Check WiFi</button>

  <div id="wifi-box"></div>

  <script>
    function scanWifi() {
      fetch("/scan").then(res => res.json()).then(data => {
        let box = document.getElementById("wifi-box");
        box.style.display = "block";
        box.innerHTML = "";
        data.forEach(wifi => {
          let div = document.createElement("div");
          div.className = "wifi-item";
          div.innerHTML = wifi.ssid + " (" + wifi.rssi + " dBm)";
          div.onclick = function() {
            let pass = prompt("Nhập mật khẩu cho \"" + wifi.ssid + "\":", "");
            if(pass !== null) {
              fetch(`/connect?ssid=${encodeURIComponent(wifi.ssid)}&pass=${encodeURIComponent(pass)}`)
                .then(r => r.text())
                .then(t => alert(t));
            }
          };
          box.appendChild(div);
        });
      });
    }
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", page);
}

void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; ++i) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i));
    json += "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleConnect() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  // Lưu SSID và mật khẩu vào EEPROM
  EEPROM.writeString(SSID_ADDR, ssid);
  EEPROM.writeString(PASS_ADDR, pass);
  EEPROM.commit();

  WiFi.begin(ssid.c_str(), pass.c_str());

  int timeout = 10000; // 10s
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    server.send(200, "text/plain", "Kết nối thành công! IP: " + WiFi.localIP().toString());

    WiFi.mode(WIFI_STA); 
    Serial.println("Đã chuyển sang chế độ Station.");
  } else {
    server.send(200, "text/plain", "Kết nối thất bại. Sai mật khẩu hoặc không tìm thấy mạng.");
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  

  String ssid = EEPROM.readString(SSID_ADDR);
  String pass = EEPROM.readString(PASS_ADDR);

  if (ssid.length() > 0 && pass.length() > 0) {

    WiFi.begin(ssid.c_str(), pass.c_str());
    int timeout = 10000; // 10s
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
      delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Đã kết nối WiFi: " + ssid);
      

      WiFi.mode(WIFI_STA);
      Serial.println("Đã chuyển sang chế độ Station.");
    } else {
      Serial.println("Kết nối WiFi thất bại, sử dụng chế độ AP.");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ap_ssid, ap_pass);
    }
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_pass);
  }

  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/connect", handleConnect);
  server.begin();
}

void loop() {
  server.handleClient();
}