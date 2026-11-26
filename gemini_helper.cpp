#include "gemini_helper.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <mbedtls/base64.h>

const char *GEMINI_API_KEY = "";

String askGemini(camera_fb_t *fb, const String &prompt) {
  if (!fb || !fb->buf) {
    return "Error: Invalid frame buffer reference.";
  }

  // Calculate required Base64 buffer size: (len + 2) / 3 * 4 + 1 for null
  // terminator
  size_t base64_size = ((fb->len + 2) / 3) * 4 + 1;

  // CRITICAL: Allocate buffer explicitly in PSRAM. Standard heap will fail.
  char *base64_buf = (char *)heap_caps_malloc(base64_size, MALLOC_CAP_SPIRAM);
  if (!base64_buf) {
    return "Error: PSRAM allocation failed. Ensure PSRAM is enabled.";
  }

  size_t outputLength;
  int mbedtls_err =
      mbedtls_base64_encode((unsigned char *)base64_buf, base64_size,
                            &outputLength, fb->buf, fb->len);

  if (mbedtls_err != 0) {
    free(base64_buf);
    return "Error: mbedtls Base64 encoding failed with code " +
           String(mbedtls_err);
  }

  // Construct JSON manually to prevent ArduinoJson from fragmenting the heap
  String payload;
  payload.reserve(base64_size + prompt.length() + 100);
  payload = "{\"contents\":[{\"parts\":[{\"text\":\"";
  payload += prompt;
  payload += "\"},{\"inline_data\":{\"mime_type\":\"image/jpeg\",\"data\":\"";
  payload += base64_buf;
  payload += "\"}}]}]}";

  // Free the PSRAM buffer immediately to release memory before the HTTP
  // transaction
  free(base64_buf);

  WiFiClientSecure client;
  client.setInsecure(); // Retained for development; implement CA verification
                        // for production

  HTTPClient http;
  String url = String("https://generativelanguage.googleapis.com/v1beta/models/"
                      "gemini-3.5-flash-lite:generateContent?key=") +
               GEMINI_API_KEY;

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);
  String result = "";

  if (httpCode == 200) {
    // 1. Wait for and read the entire payload into memory first
    String responsePayload = http.getString();

    // Print to Serial monitor so you can see exactly what Gemini returns
    Serial.println("Gemini Raw Response: ");
    Serial.println(responsePayload);

    // 2. Increase document size to 4096 to prevent 'NoMemory' errors on long
    // descriptions
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, responsePayload);

    if (!error) {
      const char *text = doc["candidates"][0]["content"]["parts"][0]["text"];
      result = text ? String(text) : "Error: JSON structural deviation.";
    } else {
      result = "Error: Deserialization failed - " + String(error.c_str());
    }
  } else {
    result = "API Error: HTTP " + String(httpCode) + " - " + http.getString();
  }

  http.end();
  return result;
}
