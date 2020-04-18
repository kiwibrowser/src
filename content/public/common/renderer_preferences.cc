// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/renderer_preferences.h"

#include "build/build_config.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/font_render_params.h"

namespace content {

RendererPreferences::RendererPreferences()
    : can_accept_load_drops(true),
      should_antialias_text(true),
      hinting(gfx::FontRenderParams::HINTING_MEDIUM),
      use_autohinter(false),
      use_bitmaps(false),
      subpixel_rendering(gfx::FontRenderParams::SUBPIXEL_RENDERING_NONE),
      use_subpixel_positioning(false),
      focus_ring_color(SkColorSetARGB(255, 229, 151, 0)),
      thumb_active_color(SkColorSetRGB(244, 244, 244)),
      thumb_inactive_color(SkColorSetRGB(234, 234, 234)),
      track_color(SkColorSetRGB(211, 211, 211)),
      active_selection_bg_color(SkColorSetRGB(30, 144, 255)),
      active_selection_fg_color(SK_ColorWHITE),
      inactive_selection_bg_color(SkColorSetRGB(200, 200, 200)),
      inactive_selection_fg_color(SkColorSetRGB(50, 50, 50)),
      browser_handles_all_top_level_requests(false),
      caret_blink_interval(base::TimeDelta::FromMilliseconds(500)),
      use_custom_colors(true),
      enable_referrers(true),
      enable_do_not_track(false),
      enable_encrypted_media(true),
      webrtc_udp_min_port(0),
      webrtc_udp_max_port(0),
      tap_multiple_targets_strategy(TAP_MULTIPLE_TARGETS_STRATEGY_POPUP),
      disable_client_blocked_error_page(false),
      plugin_fullscreen_allowed(true)
#if defined(OS_WIN)
      ,
      caption_font_height(0),
      small_caption_font_height(0),
      menu_font_height(0),
      status_font_height(0),
      message_font_height(0),
      vertical_scroll_bar_width_in_dips(0),
      horizontal_scroll_bar_height_in_dips(0),
      arrow_bitmap_height_vertical_scroll_bar_in_dips(0),
      arrow_bitmap_width_horizontal_scroll_bar_in_dips(0)
#endif
{
}

RendererPreferences::RendererPreferences(const RendererPreferences& other) =
    default;

RendererPreferences::~RendererPreferences() { }

}  // namespace content
