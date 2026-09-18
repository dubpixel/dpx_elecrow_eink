#include "epd_panel.h"
#include "epd_ll.h"

void CrowPanel579::begin(epd_rotation_t rotate)
{
  epd_ll_gpio_init();
  epd_ll_init();
  epd_paint_new(_framebuffer, rotate, EPD_WHITE);
  epd_paint_clear(EPD_WHITE);
  epd_ll_clear_ram();
  epd_ll_refresh_full();
  // Sync the partial-diff reference bank to match what's now actually on
  // screen (blank white). Vendor's EPD_Clear_R26A6H() does this same thing
  // once at startup; missing it is why an earlier version of this driver
  // produced faint/washed-out partial-refresh text -- the reference bank
  // was stuck at epd_ll_clear_ram()'s 0x00 (black) forever, so partial
  // diffs against real black text pixels looked like "no change".
  epd_ll_sync_reference_ram(_framebuffer);
  _partial_count = 0;
  _last_was_partial = false;
}

void CrowPanel579::fullRefresh()
{
  epd_ll_write_frame(_framebuffer);
  epd_ll_refresh_full();
  // Deliberately NOT re-syncing the reference RAM here -- see the comment
  // on epd_ll_sync_reference_ram() and CrowPanel579::begin(). Confirmed on
  // real hardware: syncing after a content-bearing refresh makes the next
  // partial refresh treat that content as "unchanged" and it visibly drops
  // out (only the genuinely new pixels stayed dark). Leaving the reference
  // bank at begin()'s one-time white baseline means every partial refresh
  // instead re-treats the *whole current framebuffer* as "new vs white",
  // which correctly redraws everything drawn so far -- less efficient than
  // a true incremental diff, but not something we can safely claim to do
  // better without datasheet-level LUT documentation we don't have.
  _partial_count = 0;
  _last_was_partial = false;
}

void CrowPanel579::partialRefresh()
{
  epd_ll_write_frame(_framebuffer);

  if (_partial_count >= EPD_PANEL_PARTIAL_REFRESH_LIMIT)
  {
    // Ghosting mitigation the vendor examples skip entirely: force a full
    // refresh periodically instead of partial-refreshing forever.
    Serial.println(F("[epd] partial refresh limit reached, forcing full refresh to clear ghosting"));
    epd_ll_refresh_full();
    _partial_count = 0;
  }
  else
  {
    epd_ll_refresh_partial();
    _partial_count++;
  }
  // No reference-RAM sync here either -- same reasoning as fullRefresh().
  //
  // Known limitation this leaves in place: because the reference bank
  // stays at begin()'s all-white baseline forever, content you *remove*
  // from the framebuffer (draw white over previously-dark pixels) won't
  // visually clear on a partial refresh -- the diff sees "white vs white,
  // no change" and leaves the physically-dark ink alone. It only clears on
  // the next full refresh. Not exercised by this template's demo (which
  // only ever adds content), so treat it as a documented gap, not a fix.
  _last_was_partial = true;
}

void CrowPanel579::sleep()
{
  epd_ll_deep_sleep();
}
