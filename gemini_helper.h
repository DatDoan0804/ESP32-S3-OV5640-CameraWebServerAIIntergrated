#pragma once

#include <Arduino.h>
#include "esp_camera.h"

// Externally define credentials to prevent hardcoding in helper files
extern const char* GEMINI_API_KEY;

/**
 * Encodes the camera frame buffer to Base64 and transmits it to the Gemini API.
 * Requires PSRAM to be enabled in the build environment.
 * 
 * @param fb Pointer to the camera frame buffer
 * @param prompt The instruction for the vision model
 * @return String containing the model's text response or an error code
 */

String askGemini(camera_fb_t *fb, const String& prompt);