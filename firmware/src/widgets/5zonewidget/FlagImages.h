#ifndef FLAG_IMAGES_H
#define FLAG_IMAGES_H

#include <Arduino.h>

// Get the embedded flag JPG data for a given country code
// Returns nullptr if the flag is not found
const uint8_t *getFlagStart(const String &code);

// Get the size of the embedded flag JPG data for a given country code
// Returns 0 if the flag is not found
uint32_t getFlagSize(const String &code);

#endif // FLAG_IMAGES_H
