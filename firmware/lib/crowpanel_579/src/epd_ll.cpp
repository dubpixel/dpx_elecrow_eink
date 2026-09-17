#include "epd_ll.h"

// ---- bit-banged SPI -----------------------------------------------------
// Vendor drives this over plain digitalWrite() rather than the hardware SPI
// peripheral. Slower, but it's what the factory firmware validated against
// the panel's timing, so it's kept as-is rather than "optimized" onto
// hardware SPI without a real panel to test against.

static inline void epd_wr_bus(uint8_t dat)
{
  digitalWrite(EPD_PIN_CS, LOW);
  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(EPD_PIN_SCK, LOW);
    digitalWrite(EPD_PIN_MOSI, (dat & 0x80) ? HIGH : LOW);
    digitalWrite(EPD_PIN_SCK, HIGH);
    dat <<= 1;
  }
  digitalWrite(EPD_PIN_CS, HIGH);
}

static inline void epd_wr_reg(uint8_t reg)
{
  digitalWrite(EPD_PIN_DC, LOW);
  epd_wr_bus(reg);
  digitalWrite(EPD_PIN_DC, HIGH);
}

static inline void epd_wr_data8(uint8_t dat)
{
  digitalWrite(EPD_PIN_DC, HIGH);
  epd_wr_bus(dat);
  digitalWrite(EPD_PIN_DC, HIGH);
}

void epd_ll_gpio_init(void)
{
  pinMode(EPD_PIN_SCK, OUTPUT);
  pinMode(EPD_PIN_MOSI, OUTPUT);
  pinMode(EPD_PIN_RES, OUTPUT);
  pinMode(EPD_PIN_DC, OUTPUT);
  pinMode(EPD_PIN_CS, OUTPUT);
  pinMode(EPD_PIN_BUSY, INPUT);

#if EPD_PIN_POWER_EN >= 0
  pinMode(EPD_PIN_POWER_EN, OUTPUT);
  digitalWrite(EPD_PIN_POWER_EN, HIGH);
#endif
}

void epd_ll_busy_wait(void)
{
  // Vendor's raw while(1) is fine on real hardware (BUSY is guaranteed to
  // drop), but a wedged panel would hang the MCU forever with no way to
  // recover from a watchdog. Cap it so a bad panel/cable fails loud instead
  // of silently hanging the whole device.
  uint32_t start = millis();
  while (digitalRead(EPD_PIN_BUSY) != LOW)
  {
    if (millis() - start > 10000)
    {
      Serial.println(F("[epd] WARNING: BUSY did not clear within 10s, continuing anyway"));
      return;
    }
    delay(1);
  }
}

void epd_ll_hw_reset(void)
{
  delay(10);
  digitalWrite(EPD_PIN_RES, LOW);
  delay(10);
  digitalWrite(EPD_PIN_RES, HIGH);
  delay(10);
  epd_ll_busy_wait();
}

void epd_ll_refresh_full(void)
{
  epd_wr_reg(0x22);
  epd_wr_data8(0xF7);
  epd_wr_reg(0x20);
  epd_ll_busy_wait();
}

void epd_ll_refresh_partial(void)
{
  epd_wr_reg(0x22);
  epd_wr_data8(0xDC);
  epd_wr_reg(0x20);
  epd_ll_busy_wait();
}

void epd_ll_refresh_fast(void)
{
  epd_wr_reg(0x22);
  epd_wr_data8(0xC7);
  epd_wr_reg(0x20);
  epd_ll_busy_wait();
}

void epd_ll_deep_sleep(void)
{
  epd_wr_reg(0x10);
  epd_wr_data8(0x01);
  delay(5);
}

void epd_ll_init(void)
{
  epd_ll_hw_reset();
  epd_ll_busy_wait();

  epd_wr_reg(0x12); // SWRESET
  epd_ll_busy_wait();

  epd_wr_reg(0x18); // read built-in temperature sensor
  epd_wr_data8(0x80);

  epd_wr_reg(0x22); // load temperature value
  epd_wr_data8(0xB1);
  epd_wr_reg(0x20);
  epd_ll_busy_wait();

  epd_wr_reg(0x1A); // write to temperature register
  epd_wr_data8(0x64);
  epd_wr_data8(0x00);

  epd_wr_reg(0x22); // load temperature value
  epd_wr_data8(0x91);
  epd_wr_reg(0x20);
  epd_ll_busy_wait();

  epd_wr_reg(0x3C);
  epd_wr_data8(0x3);
  epd_ll_busy_wait();
}

// -- master half RAM addressing --------------------------------------------
static void epd_set_ram_master_window(void)
{
  epd_wr_reg(0x11); // data entry mode: Y decrement, X increment
  epd_wr_data8(0x05);
  epd_wr_reg(0x44); // Ram-X address start/end
  epd_wr_data8(0x00);
  epd_wr_data8(0x31); // 400/8-1
  epd_wr_reg(0x45);   // Ram-Y address start/end
  epd_wr_data8(0x0f);
  epd_wr_data8(0x01);
  epd_wr_data8(0x00);
  epd_wr_data8(0x00);
}

static void epd_set_ram_master_addr(void)
{
  epd_wr_reg(0x4e);
  epd_wr_data8(0x00);
  epd_wr_reg(0x4f);
  epd_wr_data8(0x0f);
  epd_wr_data8(0x01);
}

// -- slave half RAM addressing ----------------------------------------------
static void epd_set_ram_slave_window(void)
{
  epd_wr_reg(0x91);
  epd_wr_data8(0x04);
  epd_wr_reg(0xc4);
  epd_wr_data8(0x31);
  epd_wr_data8(0x00);
  epd_wr_reg(0xc5);
  epd_wr_data8(0x0f);
  epd_wr_data8(0x01);
  epd_wr_data8(0x00);
  epd_wr_data8(0x00);
}

static void epd_set_ram_slave_addr(void)
{
  epd_wr_reg(0xce);
  epd_wr_data8(0x31);
  epd_wr_reg(0xcf);
  epd_wr_data8(0x0f);
  epd_wr_data8(0x01);
}

void epd_ll_clear_ram(void)
{
  epd_set_ram_master_window();
  epd_set_ram_master_addr();
  epd_wr_reg(0x24);
  for (uint16_t i = 0; i < EPD_GATE_BITS; i++)
    for (uint16_t j = 0; j < EPD_SOURCE_BYTES; j++)
      epd_wr_data8(0xFF);

  epd_set_ram_master_addr();
  epd_wr_reg(0x26);
  for (uint16_t i = 0; i < EPD_GATE_BITS; i++)
    for (uint16_t j = 0; j < EPD_SOURCE_BYTES; j++)
      epd_wr_data8(0x00);

  epd_set_ram_slave_window();
  epd_set_ram_slave_addr();
  epd_wr_reg(0xA4);
  for (uint16_t i = 0; i < EPD_GATE_BITS; i++)
    for (uint16_t j = 0; j < EPD_SOURCE_BYTES; j++)
      epd_wr_data8(0xFF);

  epd_set_ram_slave_addr();
  epd_wr_reg(0xA6);
  for (uint16_t i = 0; i < EPD_GATE_BITS; i++)
    for (uint16_t j = 0; j < EPD_SOURCE_BYTES; j++)
      epd_wr_data8(0x00);
}

void epd_ll_write_frame(const uint8_t *image_bw)
{
  uint32_t tempcol = 0, templine = 0;

  epd_set_ram_master_window();
  epd_set_ram_master_addr();
  epd_wr_reg(0x24);
  for (uint32_t i = 0; i < EPD_HALF_PANEL_BYTES; i++)
  {
    epd_wr_data8(*(image_bw + templine * EPD_SOURCE_BYTES * 2 + tempcol));
    if (++templine >= EPD_GATE_BITS)
    {
      tempcol++;
      templine = 0;
    }
  }

  epd_set_ram_slave_window();
  epd_set_ram_slave_addr();
  epd_wr_reg(0xa4);
  for (uint32_t i = 0; i < EPD_HALF_PANEL_BYTES; i++)
  {
    epd_wr_data8(*(image_bw + templine * EPD_SOURCE_BYTES * 2 + tempcol));
    if (++templine >= EPD_GATE_BITS)
    {
      tempcol++;
      templine = 0;
    }
  }
}
