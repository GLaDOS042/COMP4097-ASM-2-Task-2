#include <M5_EzData.h>

WiFiClient wifi_client;
HTTPClient http_client;

const char *host = "https://ezdata2.m5stack.com/api/v2/UiFlowData";

int setupWifi(const char *ssid, const char *password) {
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        return 1;
    }
    Serial.println("\nWiFi connection failed");
    return 0;
}

int setData(const char *token, const char *topic, int val) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return 0;
    }

    String url = String(host) + "/addEzData";
    
    if (!http_client.begin(url)) {
        Serial.println("HTTP client setup failed");
        return 0;
    }

    // 準備 JSON 數據
    StaticJsonDocument<200> doc;
    doc["deviceToken"] = token;
    doc["name"] = topic;
    doc["dataType"] = "number";
    doc["permissions"] = "1";
    doc["value"] = val;

    String jsonString;
    serializeJson(doc, jsonString);

    http_client.addHeader("Content-Type", "application/json");
    
    int httpCode = http_client.POST(jsonString);
    String payload = http_client.getString();
    
    Serial.printf("HTTP Response code: %d\n", httpCode);
    Serial.printf("Response payload: %s\n", payload.c_str());

    http_client.end();
    
    return (httpCode == HTTP_CODE_OK);
}

int getData(const char *token, const char *topic, int &result) {
    if (WiFi.status() != WL_CONNECTED) {
        return 0;
    }

    String url = String(host) + "/getEzData?deviceToken=" + token + "&key=" + topic;
    
    if (!http_client.begin(url)) {
        return 0;
    }

    int httpCode = http_client.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http_client.getString();
        StaticJsonDocument<200> doc;
        deserializeJson(doc, payload);
        result = doc["data"]["value"].as<int>();
        http_client.end();
        return 1;
    }
    
    http_client.end();
    return 0;
}

int removeData(const char *token, const char *topic) {
    if (WiFi.status() != WL_CONNECTED) {
        return 0;
    }

    String url = String(host) + "/deleteEzData?deviceToken=" + token + "&dataToken=" + topic;
    
    if (!http_client.begin(url)) {
        return 0;
    }

    int httpCode = http_client.sendRequest("DELETE");
    http_client.end();
    
    return (httpCode == HTTP_CODE_OK);
}
