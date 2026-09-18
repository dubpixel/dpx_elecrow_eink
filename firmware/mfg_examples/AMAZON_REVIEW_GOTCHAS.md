# ================================================================================
# DOCS - Amazon listing review gotchas for the ELECROW CrowPanel 5.79" E-Ink Display
# ================================================================================
# Notes distilled from real customer reviews on the Amazon listing (ASIN
# B0FX4PDW6M), captured because they contain hardware/firmware gotchas that
# are NOT covered in Elecrow's own docs/wiki.
# ================================================================================
# PROJECT: dpx_elecrow_eink
# ================================================================================
#
# File: AMAZON_REVIEW_GOTCHAS.md
# Purpose: Preserve field-tested gotchas from Amazon reviews before they scroll
#          off the listing or get buried under newer reviews.
# Dependencies: none
#
# ================================================================================

Source: https://www.amazon.com/dp/B0FX4PDW6M (ASIN B0FX4PDW6M)
Product: ELECROW ESP32 E-Ink Display 5.79" — CrowPanel, 272x792, ESP32-S3-WROOM-1-N8R8
Listing rating at time of capture: 4.4/5 (65 ratings), $49.99

## 1. Dual-controller framebuffer gap (critical — from review "Great e-ink
   project panel, but be aware of the quirks", 5★, Amazon Customer)

- The panel is NOT one controller — it's **two SSD1683 chips side by side**
  (master/slave), because a single SSD1683 can't drive the full 792px width.
- The framebuffer is actually **800px wide but only 792px is visible**, and
  there is a **permanent hidden ~8px gap in the center** (logical x≈396–403).
  Anything centered horizontally will straddle that gap — design layouts
  around it, don't put content dead-center.
- **Coordinates are NOT bounds-checked.** Passing width/height (792/272) as a
  line or rectangle endpoint instead of the last valid pixel (791/271) does
  **not clip** — it underflows and causes an out-of-bounds write that panics
  the ESP32. Always pass the inclusive last pixel, not the dimension.
- The bundled font is a single blocky ASCII-only typeface, only available in
  sizes 12, 16, 24, 48 — other sizes silently draw nothing. Vendor demos avoid
  this by baking their nicer-looking labels into background bitmaps instead of
  using the font renderer.
  - Reviewer's workaround: rasterize a monospace TTF with Python
    (Pillow `ImageFont.truetype`) at the needed sizes, pack each glyph into
    horizontal MSB-first bytes in a C header matching the vendor's font-table
    layout, then a ~30-line device-side function walks the bytes and calls the
    pixel-plot primitive. Monospace only needs one fixed advance width per
    size (no per-glyph metrics table). Validate the header by rebuilding a
    glyph-sheet PNG before flashing.
- **Refresh behavior:** a full update takes ~1.9s and wears the panel each
  time — redraw about once a minute, not once a second. **Don't mix full /
  partial / fast update modes within one session** — it leaves the panel
  blank even though the busy handshake looks fine; pick one mode and stick
  with it. Always put the panel to sleep after drawing.
- **PlatformIO/toolchain pin:** stay on **Arduino core 2.x
  (espressif32 @ 6.3.2)** — core 3.x breaks the WebServer/BLE APIs the
  Elecrow examples use.
  - Requires `memory_type = qio_opi` and the huge_app partition table, or
    PSRAM/space won't work.
  - The build banner will incorrectly say "No PSRAM" — ignore it,
    `psramFound()` correctly reports 8MB at runtime.
- **Which vendor examples to trust:** `5.79_GPIO` is known-good for text;
  `5.79_wifi_http_openweather` is the best full reference (WiFi + HTTP + JSON
  + layout). The factory source in the vendor download has the correct init
  order.
- Elecrow's own image-conversion tool is **Windows-only** — use
  `image2cpp` with "Horizontal - 1 bit per pixel" instead for image assets.

## 2. Home Assistant / ESPHome integration (from German review by "Nikolaus", 5★)

- Works well with Home Assistant, **but only via ESPHome with a custom
  driver** — the dual-SSD1683 master/slave setup is NOT supported by
  ESPHome's stock e-paper config out of the box.
- Expect a real setup/learning curve wiring up the custom driver + YAML,
  even for someone reasonably comfortable with ESPHome.
- E-paper's slow refresh is a non-issue for slowly-changing dashboard data
  (door sensors, fuel prices, appliance status) — don't expect animations or
  fast menu switching, the tech isn't meant for that.
- **Battery operation is not practical**: the e-paper panel itself draws ~0
  power at rest, but the ESP32 + WiFi + periodic sensor polling draws enough
  that the reviewer tried and abandoned battery power — recommends a
  permanent power source instead.
- Ships with only a minimal built-in bezel/case, not a finished enclosure —
  USB port placement (bottom, on this reviewer's unit) leaves the cable
  hanging; a proper 3D-printed case is hard to find/DIY well.

## 3. Physical fragility (from review "Useful E-Paper Display but Be Aware,
   It's Fragile", 4★, recordmaven)

- **The side rotary/multi-control switch handle is fragile** — it's attached
  via a thin piece of plastic that can snap off if dropped or bumped.
- Recommends mounting in an enclosure or at least a stand rather than leaving
  the board loose on a desk.
- Otherwise: good GPIO breakout via standard DuPont connector, battery
  connector, two buttons + nav switch, SD card support, deep-sleep capable
  for genuine long battery life *if* the app is written to sleep between
  updates (unlike the ESPHome reviewer's polling use case above).
- Documentation and example code are "relatively good" — enough to get
  started with prior ESP32 experience, works with Arduino/VSCode/ESP-IDF.

## 4. General positive notes (from 5★ reviews — Kris S, Carson Muehlstedt, FB)

- Comes as a full ready-to-use dev platform (ESP32-S3 + display + GPIO +
  battery support + USB), not just a bare panel — full schematics provided,
  ships with demo firmware and code examples, plug-and-play with USB-C cable
  included.
- Acrylic case/backing feels sturdy; M2.5 mounting holes on the back for
  attaching to enclosures/mounts.
- Confirms deep-sleep + WiFi-poll-then-sleep pattern gives genuinely good
  battery life for dashboard-style projects (weather stations, etc.).

## Bottom line for this project

- Do NOT center content across the full 792px width without accounting for
  the hidden ~8px gap at x≈396–403.
- Always pass the *last valid pixel* (dimension − 1), never the raw
  width/height, to any line/rect drawing call — there is no bounds-checking
  and it will panic the ESP32.
- Pin the Arduino core to **espressif32 @ 6.3.2** (core 2.x), not 3.x.
- Use `5.79_GPIO.ino` and `5.79_wifi_http_openweather.ino` from the vendor
  repo as the trusted reference examples (see mfg_examples/ in this folder).
- Plan for a physical enclosure early — the side switch is a known weak
  point.
- If integrating with Home Assistant/ESPHome, budget time for a custom
  dual-SSD1683 driver — it won't work with a stock ESPHome e-paper config.
