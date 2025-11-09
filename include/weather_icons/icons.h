// Auto-generated icon map
#pragma once
#include <Arduino.h>

#include "Cloud.h"
#include "CloudDrizzle.h"
#include "CloudLightning.h"
#include "CloudRain.h"
#include "CloudSnow.h"
#include "Droplet.h"
#include "Moon.h"
#include "Sun.h"

typedef struct {
    const unsigned char *data;
    int width;
    int height;
} IconEntry;

static const struct {
    const char *code;
    IconEntry icon;
} ApiIconsToIcons[] = {
    {"01d", {sun_bits, sun_width, sun_height}},
    {"01n", {moon_bits, moon_width, moon_height}},
    {"02d", {cloud_bits, cloud_width, cloud_height}},
    {"02n", {cloud_bits, cloud_width, cloud_height}},
    {"03d", {cloud_bits, cloud_width, cloud_height}},
    {"03n", {cloud_bits, cloud_width, cloud_height}},
    {"04d", {cloud_bits, cloud_width, cloud_height}},
    {"04n", {cloud_bits, cloud_width, cloud_height}},
    {"09d", {cloudrain_bits, cloudrain_width, cloudrain_height}},
    {"09n", {cloudrain_bits, cloudrain_width, cloudrain_height}},
    {"10d", {clouddrizzle_bits, clouddrizzle_width, clouddrizzle_height}},
    {"10n", {clouddrizzle_bits, clouddrizzle_width, clouddrizzle_height}},
    {"11d", {cloudlightning_bits, cloudlightning_width, cloudlightning_height}},
    {"11n", {cloudlightning_bits, cloudlightning_width, cloudlightning_height}},
    {"13d", {cloudsnow_bits, cloudsnow_width, cloudsnow_height}},
    {"13n", {cloudsnow_bits, cloudsnow_width, cloudsnow_height}},
    {"50d", {droplet_bits, droplet_width, droplet_height}},
    {"50n", {droplet_bits, droplet_width, droplet_height}},
};

inline const IconEntry* getIconByCode(const char *code) {
    for (size_t i = 0; i < sizeof(ApiIconsToIcons) / sizeof(ApiIconsToIcons[0]); i++) {
        if (strcmp(ApiIconsToIcons[i].code, code) == 0)
            return &ApiIconsToIcons[i].icon;
    }
    return nullptr;
}

inline const IconEntry* getIconByCode(const String &code) {
    return getIconByCode(code.c_str());
}
