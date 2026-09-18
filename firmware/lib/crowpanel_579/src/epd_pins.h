// ================================================================================
// HEADER - CrowPanel 5.79" E-Paper pin map
// ================================================================================
// Bit-banged SPI pin assignments, confirmed against the ELECROW factory
// example (5.79_wifi_http_openweather) and independently against a customer
// teardown photo on the Amazon listing.
// ================================================================================
// PROJECT: dpx_elecrow_eink
// ================================================================================
//
// File: epd_pins.h
// Purpose: Centralize the panel's GPIO map so board revisions or custom
//          carrier boards only require editing this one file.
// Dependencies: Arduino core (digitalWrite/pinMode)
//
// ================================================================================

#pragma once

#ifndef EPD_PIN_SCK
#define EPD_PIN_SCK 12
#endif
#ifndef EPD_PIN_MOSI
#define EPD_PIN_MOSI 11
#endif
#ifndef EPD_PIN_RES
#define EPD_PIN_RES 47
#endif
#ifndef EPD_PIN_DC
#define EPD_PIN_DC 46
#endif
#ifndef EPD_PIN_CS
#define EPD_PIN_CS 45
#endif
#ifndef EPD_PIN_BUSY
#define EPD_PIN_BUSY 48
#endif

// Panel power gate. Pulling this high powers the e-paper's charge pump /
// analog rail; some Elecrow demos hardcode GPIO7 for this. Set to -1 if your
// carrier board ties it permanently high in hardware.
#ifndef EPD_PIN_POWER_EN
#define EPD_PIN_POWER_EN 7
#endif
