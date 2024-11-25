#include <WiFi.h>
#include <coap-simple.h>
#include <ArduinoJson.h>

// Konfigurasi WiFi
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Konfigurasi Server CoAP
IPAddress serverIP(192, 168, 1, 100);  // Sesuaikan dengan IP server
const int serverPort = 5683;

// Inisialisasi UDP dan CoAP
WiFiUDP udp;
Coap coap(udp);

// Node ID
const int nodeId = 1;

// Fungsi untuk membuat data dummy sensor
void generateSensorData(char* buffer) {
    // Buat objek JSON dengan kapasitas yang cukup
    StaticJsonDocument<512> doc;
    
    // Tambahkan data sensor
    doc["node_id"] = nodeId;
    doc["delay"] = random(30, 100);              // 30-100ms
    doc["payload_size"] = random(128, 512);      // 128-512 bytes
    doc["throughput"] = random(400, 600);        // 400-600 kbps
    doc["current"] = random(200, 300) / 100.0;   // 2.00-3.00A
    doc["voltage"] = random(22000, 24000) / 100.0; // 220-240V
    doc["power"] = random(18000, 22000) / 100.0;   // 180-220W
    doc["power_consumption"] = random(14000, 16000) / 100.0; // 140-160W
    doc["temperature"] = random(2400, 2800) / 100.0; // 24-28°C
    doc["humidity"] = random(5500, 6500) / 100.0;    // 55-65%

    // Buat array untuk soil moisture
    JsonArray soilData = doc.createNestedArray("soil_moisture_data");
    
    // Tambahkan 2 sensor soil moisture
    JsonObject soil1 = soilData.createNestedObject();
    soil1["sensor_order"] = 1;
    soil1["moisture"] = random(2800, 3200) / 100.0;  // 28-32%
    
    JsonObject soil2 = soilData.createNestedObject();
    soil2["sensor_order"] = 2;
    soil2["moisture"] = random(3800, 4200) / 100.0;  // 38-42%

    // Serialize JSON ke buffer
    serializeJson(doc, buffer, 512);
}

// Callback untuk response dari server
void responseHandler(CoapPacket &packet, IPAddress ip, int port) {
    char p[packet.payloadlen + 1];
    memcpy(p, packet.payload, packet.payloadlen);
    p[packet.payloadlen] = '\0';
    
    Serial.print("Response: ");
    Serial.println(p);
}

void setup() {
    Serial.begin(115200);
    
    // Koneksi ke WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Start CoAP
    coap.start();
    // Set callback untuk response
    coap.response(responseHandler);
    
    // Tunggu sebentar untuk stabilisasi
    delay(1000);
}

void loop() {
    char jsonBuffer[512];
    generateSensorData(jsonBuffer);
    
    Serial.println("\nSending sensor data:");
    Serial.println(jsonBuffer);
    
    // Kirim data menggunakan CoAP POST
    uint16_t msgid = coap.send(
        serverIP,
        serverPort,
        "/sensor",
        COAP_CON,     // Confirmable message untuk piggybacked response
        COAP_POST,    // Gunakan POST method
        NULL,         // Tidak ada token
        0,           // Token length 0
        (uint8_t*)jsonBuffer,
        strlen(jsonBuffer),
        COAP_APPLICATION_JSON
    );
    
    // Loop untuk menerima response
    for(int i = 0; i < 2; i++) {
        coap.loop();
        delay(500);
    }
    
    // Tunggu 5 detik sebelum pengiriman berikutnya
    delay(5000);
}