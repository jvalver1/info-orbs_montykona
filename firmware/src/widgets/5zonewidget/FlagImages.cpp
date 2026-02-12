#include "FlagImages.h"

// Embedded flag JPG binary data (linked via platformio.ini board_build.embed_files)
extern const uint8_t flag_gb_start[] asm("_binary_images_flags_gb_jpg_start");
extern const uint8_t flag_gb_end[] asm("_binary_images_flags_gb_jpg_end");

extern const uint8_t flag_es_start[] asm("_binary_images_flags_es_jpg_start");
extern const uint8_t flag_es_end[] asm("_binary_images_flags_es_jpg_end");

extern const uint8_t flag_br_start[] asm("_binary_images_flags_br_jpg_start");
extern const uint8_t flag_br_end[] asm("_binary_images_flags_br_jpg_end");

extern const uint8_t flag_us_start[] asm("_binary_images_flags_us_jpg_start");
extern const uint8_t flag_us_end[] asm("_binary_images_flags_us_jpg_end");

extern const uint8_t flag_eg_start[] asm("_binary_images_flags_eg_jpg_start");
extern const uint8_t flag_eg_end[] asm("_binary_images_flags_eg_jpg_end");

const uint8_t *getFlagStart(const String &code) {
    String c = code;
    c.toUpperCase();

    if (c == "GB")
        return flag_gb_start;
    if (c == "ES")
        return flag_es_start;
    if (c == "BR")
        return flag_br_start;
    if (c == "US")
        return flag_us_start;
    if (c == "EG")
        return flag_eg_start;

    return nullptr;
}

uint32_t getFlagSize(const String &code) {
    String c = code;
    c.toUpperCase();

    if (c == "GB")
        return flag_gb_end - flag_gb_start;
    if (c == "ES")
        return flag_es_end - flag_es_start;
    if (c == "BR")
        return flag_br_end - flag_br_start;
    if (c == "US")
        return flag_us_end - flag_us_start;
    if (c == "EG")
        return flag_eg_end - flag_eg_start;

    return 0;
}
