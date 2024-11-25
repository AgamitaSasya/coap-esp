#include <WiFi.h>
#include <coap-simple.h>
#include <ArduinoJson.h>

// Konfigurasi WiFi
const char* ssid = "Alif";
const char* password = "54541212";

// Konfigurasi Server CoAP
IPAddress serverIP(192, 168, 1, 5);  // Sesuaikan dengan IP server
const int serverPort = 5683;

// Inisialisasi UDP dan CoAP
WiFiUDP udp;
Coap coap(udp);

// Node ID
const int nodeId = 1;

// Fungsi untuk membuat data dummy sensor
void generateSensorData(char* buffer) {
    StaticJsonDocument<512> doc;
    
    doc["node_id"] = nodeId;
    doc["delay"] = random(30, 100);              
    doc["payload_size"] = random(128, 512);      
    doc["throughput"] = random(400, 600);        
    doc["current"] = random(200, 300) / 100.0;   
    doc["voltage"] = random(22000, 24000) / 100.0; 
    doc["power"] = random(18000, 22000) / 100.0;   
    doc["power_consumption"] = random(14000, 16000) / 100.0;
    doc["temperature"] = random(2400, 2800) / 100.0;
    doc["humidity"] = random(5500, 6500) / 100.0;   

    JsonArray soilData = doc.createNestedArray("soil_moisture_data");
    
    JsonObject soil1 = soilData.createNestedObject();
    soil1["sensor_order"] = 1;
    soil1["moisture"] = random(2800, 3200) / 100.0;
    
    JsonObject soil2 = soilData.createNestedObject();
    soil2["sensor_order"] = 2;
    soil2["moisture"] = random(3800, 4200) / 100.0;

    serializeJson(doc, buffer, 512);
}

// Callback untuk response dari server
void responseHandler(CoapPacket &packet, IPAddress ip, int port) {
    char p[packet.payloadlen + 1];
    memcpy(p, packet.payload, packet.payloadlen);
    p[packet.payloadlen] = '\0';
    
    Serial.print("Response Code: ");
    Serial.println(packet.code);
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
    coap.response(responseHandler);
    
    randomSeed(analogRead(0)); // Inisialisasi random number generator
    delay(1000);
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Connection lost! Reconnecting...");
        WiFi.begin(ssid, password);
        delay(5000);
        return;
    }

    char jsonBuffer[512];
    generateSensorData(jsonBuffer);
    
    Serial.println("\nSending sensor data (PUT):");
    Serial.println(jsonBuffer);
    
    // Method 1: Menggunakan fungsi put() langsung
    uint16_t msgid = coap.put(
        serverIP,
        serverPort,
        "/sensor",
        jsonBuffer
    );

    /* Method 2: Menggunakan send() dengan parameter lengkap
    uint16_t msgid = coap.send(
        serverIP,
        serverPort,
        "/sensor",
        COAP_CON,
        COAP_PUT,
        NULL,
        0,
        (uint8_t*)jsonBuffer,
        strlen(jsonBuffer),
        COAP_APPLICATION_JSON
    );
    */
    
    if(msgid == 0) {
        Serial.println("Error sending data");
    } else {
        Serial.print("Message ID: ");
        Serial.println(msgid);
    }
    
    // Loop untuk menerima response
    for(int i = 0; i < 3; i++) {
        coap.loop();
        delay(500);
    }
    
    delay(5000); // Tunggu 5 detik sebelum pengiriman berikutnya
}