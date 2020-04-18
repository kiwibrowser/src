// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_VIEW_MESSAGES_H_
#define CONTENT_COMMON_VIEW_MESSAGES_H_

// IPC messages for page rendering.

#include <stddef.h>
#include <stdint.h>

#include <set>
#include <string>
#include <vector>

#include "base/memory/shared_memory.h"
#include "base/optional.h"
#include "base/process/process.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "cc/input/touch_action.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/quads/shared_bitmap.h"
#include "content/common/content_export.h"
#include "content/common/content_param_traits.h"
#include "content/common/date_time_suggestion.h"
#include "content/common/frame_replication_state.h"
#include "content/common/navigation_gesture.h"
#include "content/common/text_input_state.h"
#include "content/common/view_message_enums.h"
#include "content/common/visual_properties.h"
#include "content/public/common/common_param_traits.h"
#include "content/public/common/menu_item.h"
#include "content/public/common/page_state.h"
#include "content/public/common/page_zoom.h"
#include "content/public/common/referrer.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/three_d_api_types.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_message_macros.h"
#include "media/base/audio_parameters.h"
#include "media/base/channel_layout.h"
#include "media/base/ipc/media_param_traits.h"
#include "media/capture/ipc/capture_param_traits.h"
#include "net/base/network_change_notifier.h"
#include "ppapi/buildflags/buildflags.h"
#include "third_party/blink/public/common/manifest/web_display_mode.h"
#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_type.h"
#include "third_party/blink/public/platform/web_float_point.h"
#include "third_party/blink/public/platform/web_float_rect.h"
#include "third_party/blink/public/platform/web_intrinsic_sizing_info.h"
#include "third_party/blink/public/web/web_device_emulation_params.h"
#include "third_party/blink/public/web/web_media_player_action.h"
#include "third_party/blink/public/web/web_plugin_action.h"
#include "third_party/blink/public/web/web_popup_type.h"
#include "third_party/blink/public/web/web_text_direction.h"
#include "ui/base/ime/text_input_mode.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/geometry/vector2d_f.h"
#include "ui/gfx/ipc/color/gfx_param_traits.h"
#include "ui/gfx/ipc/gfx_param_traits.h"
#include "ui/gfx/ipc/skia/gfx_skia_param_traits.h"

#if defined(OS_MACOSX)
#include "third_party/blink/public/platform/mac/web_scrollbar_theme.h"
#include "third_party/blink/public/platform/web_scrollbar_buttons_placement.h"
#endif

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT

#define IPC_MESSAGE_START ViewMsgStart

IPC_ENUM_TRAITS_MAX_VALUE(blink::WebDeviceEmulationParams::ScreenPosition,
                          blink::WebDeviceEmulationParams::kScreenPositionLast)
IPC_ENUM_TRAITS_MAX_VALUE(blink::WebMediaPlayerAction::Type,
                          blink::WebMediaPlayerAction::Type::kTypeLast)
IPC_ENUM_TRAITS_MAX_VALUE(blink::WebPluginAction::Type,
                          blink::WebPluginAction::Type::kTypeLast)
IPC_ENUM_TRAITS_MAX_VALUE(blink::WebPopupType,
                          blink::WebPopupType::kWebPopupTypeLast)
IPC_ENUM_TRAITS_MIN_MAX_VALUE(blink::WebScreenOrientationType,
                              blink::kWebScreenOrientationUndefined,
                              blink::WebScreenOrientationTypeLast)
IPC_ENUM_TRAITS_MAX_VALUE(blink::WebTextDirection,
                          blink::WebTextDirection::kWebTextDirectionLast)
IPC_ENUM_TRAITS_MAX_VALUE(blink::WebDisplayMode,
                          blink::WebDisplayMode::kWebDisplayModeLast)
IPC_ENUM_TRAITS_MAX_VALUE(content::MenuItem::Type, content::MenuItem::TYPE_LAST)
IPC_ENUM_TRAITS_MAX_VALUE(content::NavigationGesture,
                          content::NavigationGestureLast)
IPC_ENUM_TRAITS_MIN_MAX_VALUE(content::PageZoom,
                              content::PageZoom::PAGE_ZOOM_OUT,
                              content::PageZoom::PAGE_ZOOM_IN)
IPC_ENUM_TRAITS_MAX_VALUE(gfx::FontRenderParams::Hinting,
                          gfx::FontRenderParams::HINTING_MAX)
IPC_ENUM_TRAITS_MAX_VALUE(gfx::FontRenderParams::SubpixelRendering,
                          gfx::FontRenderParams::SUBPIXEL_RENDERING_MAX)
IPC_ENUM_TRAITS_MAX_VALUE(content::ScreenOrientationValues,
                          content::SCREEN_ORIENTATION_VALUES_LAST)
IPC_ENUM_TRAITS_MAX_VALUE(content::TapMultipleTargetsStrategy,
                          content::TAP_MULTIPLE_TARGETS_STRATEGY_MAX)
IPC_ENUM_TRAITS_MAX_VALUE(content::ThreeDAPIType,
                          content::THREE_D_API_TYPE_LAST)
IPC_ENUM_TRAITS_MAX_VALUE(ui::TextInputMode, ui::TEXT_INPUT_MODE_MAX)
IPC_ENUM_TRAITS_MAX_VALUE(ui::TextInputType, ui::TEXT_INPUT_TYPE_MAX)

#if defined(OS_MACOSX)
IPC_ENUM_TRAITS_MAX_VALUE(
    blink::WebScrollbarButtonsPlacement,
    blink::WebScrollbarButtonsPlacement::kWebScrollbarButtonsPlacementLast)

IPC_ENUM_TRAITS_MAX_VALUE(blink::ScrollerStyle, blink::kScrollerStyleOverlay)
#endif

IPC_STRUCT_TRAITS_BEGIN(blink::WebMediaPlayerAction)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(enable)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(blink::WebPluginAction)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(enable)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(blink::WebFloatPoint)
  IPC_STRUCT_TRAITS_MEMBER(x)
  IPC_STRUCT_TRAITS_MEMBER(y)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(blink::WebFloatRect)
  IPC_STRUCT_TRAITS_MEMBER(x)
  IPC_STRUCT_TRAITS_MEMBER(y)
  IPC_STRUCT_TRAITS_MEMBER(width)
  IPC_STRUCT_TRAITS_MEMBER(height)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(blink::WebSize)
  IPC_STRUCT_TRAITS_MEMBER(width)
  IPC_STRUCT_TRAITS_MEMBER(height)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(blink::WebDeviceEmulationParams)
  IPC_STRUCT_TRAITS_MEMBER(screen_position)
  IPC_STRUCT_TRAITS_MEMBER(screen_size)
  IPC_STRUCT_TRAITS_MEMBER(view_position)
  IPC_STRUCT_TRAITS_MEMBER(device_scale_factor)
  IPC_STRUCT_TRAITS_MEMBER(view_size)
  IPC_STRUCT_TRAITS_MEMBER(scale)
  IPC_STRUCT_TRAITS_MEMBER(viewport_offset)
  IPC_STRUCT_TRAITS_MEMBER(viewport_scale)
  IPC_STRUCT_TRAITS_MEMBER(screen_orientation_angle)
  IPC_STRUCT_TRAITS_MEMBER(screen_orientation_type)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::VisualProperties)
  IPC_STRUCT_TRAITS_MEMBER(screen_info)
  IPC_STRUCT_TRAITS_MEMBER(auto_resize_enabled)
  IPC_STRUCT_TRAITS_MEMBER(min_size_for_auto_resize)
  IPC_STRUCT_TRAITS_MEMBER(max_size_for_auto_resize)
  IPC_STRUCT_TRAITS_MEMBER(new_size)
  IPC_STRUCT_TRAITS_MEMBER(compositor_viewport_pixel_size)
  IPC_STRUCT_TRAITS_MEMBER(browser_controls_shrink_blink_size)
  IPC_STRUCT_TRAITS_MEMBER(scroll_focused_node_into_view)
  IPC_STRUCT_TRAITS_MEMBER(top_controls_height)
  IPC_STRUCT_TRAITS_MEMBER(bottom_controls_height)
  IPC_STRUCT_TRAITS_MEMBER(local_surface_id)
  IPC_STRUCT_TRAITS_MEMBER(visible_viewport_size)
  IPC_STRUCT_TRAITS_MEMBER(is_fullscreen_granted)
  IPC_STRUCT_TRAITS_MEMBER(display_mode)
  IPC_STRUCT_TRAITS_MEMBER(capture_sequence_number)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::MenuItem)
  IPC_STRUCT_TRAITS_MEMBER(label)
  IPC_STRUCT_TRAITS_MEMBER(tool_tip)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(action)
  IPC_STRUCT_TRAITS_MEMBER(rtl)
  IPC_STRUCT_TRAITS_MEMBER(has_directional_override)
  IPC_STRUCT_TRAITS_MEMBER(enabled)
  IPC_STRUCT_TRAITS_MEMBER(checked)
  IPC_STRUCT_TRAITS_MEMBER(submenu)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::DateTimeSuggestion)
  IPC_STRUCT_TRAITS_MEMBER(value)
  IPC_STRUCT_TRAITS_MEMBER(localized_value)
  IPC_STRUCT_TRAITS_MEMBER(label)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::RendererPreferences)
  IPC_STRUCT_TRAITS_MEMBER(can_accept_load_drops)
  IPC_STRUCT_TRAITS_MEMBER(should_antialias_text)
  IPC_STRUCT_TRAITS_MEMBER(hinting)
  IPC_STRUCT_TRAITS_MEMBER(use_autohinter)
  IPC_STRUCT_TRAITS_MEMBER(use_bitmaps)
  IPC_STRUCT_TRAITS_MEMBER(subpixel_rendering)
  IPC_STRUCT_TRAITS_MEMBER(use_subpixel_positioning)
  IPC_STRUCT_TRAITS_MEMBER(focus_ring_color)
  IPC_STRUCT_TRAITS_MEMBER(thumb_active_color)
  IPC_STRUCT_TRAITS_MEMBER(thumb_inactive_color)
  IPC_STRUCT_TRAITS_MEMBER(track_color)
  IPC_STRUCT_TRAITS_MEMBER(active_selection_bg_color)
  IPC_STRUCT_TRAITS_MEMBER(active_selection_fg_color)
  IPC_STRUCT_TRAITS_MEMBER(inactive_selection_bg_color)
  IPC_STRUCT_TRAITS_MEMBER(inactive_selection_fg_color)
  IPC_STRUCT_TRAITS_MEMBER(browser_handles_all_top_level_requests)
  IPC_STRUCT_TRAITS_MEMBER(caret_blink_interval)
  IPC_STRUCT_TRAITS_MEMBER(use_custom_colors)
  IPC_STRUCT_TRAITS_MEMBER(enable_referrers)
  IPC_STRUCT_TRAITS_MEMBER(enable_do_not_track)
  IPC_STRUCT_TRAITS_MEMBER(enable_encrypted_media)
  IPC_STRUCT_TRAITS_MEMBER(webrtc_ip_handling_policy)
  IPC_STRUCT_TRAITS_MEMBER(webrtc_udp_min_port)
  IPC_STRUCT_TRAITS_MEMBER(webrtc_udp_max_port)
  IPC_STRUCT_TRAITS_MEMBER(user_agent_override)
  IPC_STRUCT_TRAITS_MEMBER(accept_languages)
  IPC_STRUCT_TRAITS_MEMBER(tap_multiple_targets_strategy)
  IPC_STRUCT_TRAITS_MEMBER(disable_client_blocked_error_page)
  IPC_STRUCT_TRAITS_MEMBER(plugin_fullscreen_allowed)
  IPC_STRUCT_TRAITS_MEMBER(network_contry_iso)
#if defined(OS_LINUX)
  IPC_STRUCT_TRAITS_MEMBER(system_font_family_name)
#endif
#if defined(OS_WIN)
  IPC_STRUCT_TRAITS_MEMBER(caption_font_family_name)
  IPC_STRUCT_TRAITS_MEMBER(caption_font_height)
  IPC_STRUCT_TRAITS_MEMBER(small_caption_font_family_name)
  IPC_STRUCT_TRAITS_MEMBER(small_caption_font_height)
  IPC_STRUCT_TRAITS_MEMBER(menu_font_family_name)
  IPC_STRUCT_TRAITS_MEMBER(menu_font_height)
  IPC_STRUCT_TRAITS_MEMBER(status_font_family_name)
  IPC_STRUCT_TRAITS_MEMBER(status_font_height)
  IPC_STRUCT_TRAITS_MEMBER(message_font_family_name)
  IPC_STRUCT_TRAITS_MEMBER(message_font_height)
  IPC_STRUCT_TRAITS_MEMBER(vertical_scroll_bar_width_in_dips)
  IPC_STRUCT_TRAITS_MEMBER(horizontal_scroll_bar_height_in_dips)
  IPC_STRUCT_TRAITS_MEMBER(arrow_bitmap_height_vertical_scroll_bar_in_dips)
  IPC_STRUCT_TRAITS_MEMBER(arrow_bitmap_width_horizontal_scroll_bar_in_dips)
#endif
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::TextInputState)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(mode)
  IPC_STRUCT_TRAITS_MEMBER(flags)
  IPC_STRUCT_TRAITS_MEMBER(value)
  IPC_STRUCT_TRAITS_MEMBER(selection_start)
  IPC_STRUCT_TRAITS_MEMBER(selection_end)
  IPC_STRUCT_TRAITS_MEMBER(composition_start)
  IPC_STRUCT_TRAITS_MEMBER(composition_end)
  IPC_STRUCT_TRAITS_MEMBER(can_compose_inline)
  IPC_STRUCT_TRAITS_MEMBER(show_ime_if_needed)
  IPC_STRUCT_TRAITS_MEMBER(reply_to_request)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_BEGIN(ViewHostMsg_DateTimeDialogValue_Params)
  IPC_STRUCT_MEMBER(ui::TextInputType, dialog_type)
  IPC_STRUCT_MEMBER(double, dialog_value)
  IPC_STRUCT_MEMBER(double, minimum)
  IPC_STRUCT_MEMBER(double, maximum)
  IPC_STRUCT_MEMBER(double, step)
  IPC_STRUCT_MEMBER(std::vector<content::DateTimeSuggestion>, suggestions)
IPC_STRUCT_END()

IPC_STRUCT_BEGIN(ViewHostMsg_SelectionBounds_Params)
  IPC_STRUCT_MEMBER(gfx::Rect, anchor_rect)
  IPC_STRUCT_MEMBER(blink::WebTextDirection, anchor_dir)
  IPC_STRUCT_MEMBER(gfx::Rect, focus_rect)
  IPC_STRUCT_MEMBER(blink::WebTextDirection, focus_dir)
  IPC_STRUCT_MEMBER(bool, is_anchor_first)
IPC_STRUCT_END()

// Messages sent from the browser to the renderer.

#if defined(OS_ANDROID)
// Tells the renderer to cancel an opened date/time dialog.
IPC_MESSAGE_ROUTED0(ViewMsg_CancelDateTimeDialog)

// Replaces a date time input field.
IPC_MESSAGE_ROUTED1(ViewMsg_ReplaceDateTime,
                    double /* dialog_value */)

#endif

// Tells the render side that a ViewHostMsg_LockMouse message has been
// processed. |succeeded| indicates whether the mouse has been successfully
// locked or not.
IPC_MESSAGE_ROUTED1(ViewMsg_LockMouse_ACK,
                    bool /* succeeded */)
// Tells the render side that the mouse has been unlocked.
IPC_MESSAGE_ROUTED0(ViewMsg_MouseLockLost)

// Sends updated preferences to the renderer.
IPC_MESSAGE_ROUTED1(ViewMsg_SetRendererPrefs,
                    content::RendererPreferences)

// This passes a set of webkit preferences down to the renderer.
IPC_MESSAGE_ROUTED1(ViewMsg_UpdateWebPreferences,
                    content::WebPreferences)

// Tells the render view to close.
// Expects a Close_ACK message when finished.
IPC_MESSAGE_ROUTED0(ViewMsg_Close)

// Tells the renderer to update visual properties.  A
// ViewHostMsg_ResizeOrRepaint_ACK  message is generated in response provided
// new_size is not empty and not equal to the view's current size.  The
// generated ViewHostMsg_ResizeOrRepaint_ACK
// message will have the IS_RESIZE_ACK flag set. It also receives the resizer
// rect so that we don't have to fetch it every time WebKit asks for it.
IPC_MESSAGE_ROUTED1(ViewMsg_SynchronizeVisualProperties,
                    content::VisualProperties /* params */)

// Enables device emulation. See WebDeviceEmulationParams for description.
IPC_MESSAGE_ROUTED1(ViewMsg_EnableDeviceEmulation,
                    blink::WebDeviceEmulationParams /* params */)

// Disables device emulation, enabled previously by EnableDeviceEmulation.
IPC_MESSAGE_ROUTED0(ViewMsg_DisableDeviceEmulation)

// Sent to inform the view that it was hidden.  This allows it to reduce its
// resource utilization.
IPC_MESSAGE_ROUTED0(ViewMsg_WasHidden)

// Tells the render view that it is no longer hidden (see WasHidden), and the
// render view is expected to respond with a full repaint if needs_repainting
// is true. If needs_repainting is false, then this message does not trigger a
// message in response.
IPC_MESSAGE_ROUTED2(ViewMsg_WasShown,
                    bool /* needs_repainting */,
                    ui::LatencyInfo /* latency_info */)

// Tells the renderer to focus the first (last if reverse is true) focusable
// node.
IPC_MESSAGE_ROUTED1(ViewMsg_SetInitialFocus,
                    bool /* reverse */)

// Sent to inform the renderer to invoke a context menu.
// The parameter specifies the location in the render view's coordinates.
IPC_MESSAGE_ROUTED2(ViewMsg_ShowContextMenu,
                    ui::MenuSourceType,
                    gfx::Point /* location where menu should be shown */)

// Tells the renderer to perform the given action on the media player
// located at the given point.
IPC_MESSAGE_ROUTED2(ViewMsg_MediaPlayerActionAt,
                    gfx::Point, /* location */
                    blink::WebMediaPlayerAction)

// Tells the renderer to perform the given action on the plugin located at
// the given point.
IPC_MESSAGE_ROUTED2(ViewMsg_PluginActionAt,
                    gfx::Point, /* location */
                    blink::WebPluginAction)

// Sets the page scale for the current main frame to the given page scale.
IPC_MESSAGE_ROUTED1(ViewMsg_SetPageScale, float /* page_scale_factor */)

// Tell the renderer to add a property to the WebUI binding object.  This
// only works if we allowed WebUI bindings.
IPC_MESSAGE_ROUTED2(ViewMsg_SetWebUIProperty,
                    std::string /* property_name */,
                    std::string /* property_value_json */)

// Used to notify the render-view that we have received a target URL. Used
// to prevent target URLs spamming the browser.
IPC_MESSAGE_ROUTED0(ViewMsg_UpdateTargetURL_ACK)

// Provides the results of directory enumeration.
IPC_MESSAGE_ROUTED2(ViewMsg_EnumerateDirectoryResponse,
                    int /* request_id */,
                    std::vector<base::FilePath> /* files_in_directory */)

// Instructs the renderer to close the current page, including running the
// onunload event handler.
//
// Expects a ClosePage_ACK message when finished.
IPC_MESSAGE_ROUTED0(ViewMsg_ClosePage)

// Notification that a move or resize renderer's containing window has
// started.
IPC_MESSAGE_ROUTED0(ViewMsg_MoveOrResizeStarted)

IPC_MESSAGE_ROUTED2(ViewMsg_UpdateScreenRects,
                    gfx::Rect /* view_screen_rect */,
                    gfx::Rect /* window_screen_rect */)

// Reply to ViewHostMsg_RequestMove, ViewHostMsg_ShowWidget, and
// FrameHostMsg_ShowCreatedWindow, to inform the renderer that the browser has
// processed the move.  The browser may have ignored the move, but it finished
// processing.  This is used because the renderer keeps a temporary cache of the
// widget position while these asynchronous operations are in progress.
IPC_MESSAGE_ROUTED0(ViewMsg_Move_ACK)

// Used to instruct the RenderView to send back updates to the preferred size.
IPC_MESSAGE_ROUTED0(ViewMsg_EnablePreferredSizeChangedMode)

// Changes the text direction of the currently selected input field (if any).
IPC_MESSAGE_ROUTED1(ViewMsg_SetTextDirection,
                    blink::WebTextDirection /* direction */)

// Make the RenderView background transparent or opaque.
IPC_MESSAGE_ROUTED1(ViewMsg_SetBackgroundOpaque, bool /* opaque */)

// Used to tell the renderer not to add scrollbars with height and
// width below a threshold.
IPC_MESSAGE_ROUTED1(ViewMsg_DisableScrollbarsForSmallWindows,
                    gfx::Size /* disable_scrollbar_size_limit */)

// Activate/deactivate the RenderView (i.e., set its controls' tint
// accordingly, etc.).
IPC_MESSAGE_ROUTED1(ViewMsg_SetActive,
                    bool /* active */)

// Response message to ViewHostMsg_CreateWorker.
// Sent when the worker has started.
IPC_MESSAGE_ROUTED0(ViewMsg_WorkerCreated)

// Sent when the worker failed to load the worker script.
// In normal cases, this message is sent after ViewMsg_WorkerCreated is sent.
// But if the shared worker of the same URL already exists and it has failed
// to load the script, when the renderer send ViewHostMsg_CreateWorker before
// the shared worker is killed only ViewMsg_WorkerScriptLoadFailed is sent.
IPC_MESSAGE_ROUTED0(ViewMsg_WorkerScriptLoadFailed)

// Sent when the worker has connected.
// This message is sent only if the worker successfully loaded the script.
// |used_features| is the set of features that the worker has used. The values
// must be from blink::UseCounter::Feature enum.
IPC_MESSAGE_ROUTED1(ViewMsg_WorkerConnected,
                    std::set<uint32_t> /* used_features */)

// Sent when the worker is destroyed.
IPC_MESSAGE_ROUTED0(ViewMsg_WorkerDestroyed)

// Sent when the worker calls API that should be recored in UseCounter.
// |feature| must be one of the values from blink::UseCounter::Feature
// enum.
IPC_MESSAGE_ROUTED1(ViewMsg_CountFeatureOnSharedWorker,
                    uint32_t /* feature */)

// Sent by the browser to synchronize with the next compositor frame. Used only
// for tests.
IPC_MESSAGE_ROUTED1(ViewMsg_WaitForNextFrameForTests, int /* routing_id */)

#if BUILDFLAG(ENABLE_PLUGINS)
// Reply to ViewHostMsg_OpenChannelToPpapiBroker
// Tells the renderer that the channel to the broker has been created.
IPC_MESSAGE_ROUTED2(ViewMsg_PpapiBrokerChannelCreated,
                    base::ProcessId /* broker_pid */,
                    IPC::ChannelHandle /* handle */)

// Reply to ViewHostMsg_RequestPpapiBrokerPermission.
// Tells the renderer whether permission to access to PPAPI broker was granted
// or not.
IPC_MESSAGE_ROUTED1(ViewMsg_PpapiBrokerPermissionResult,
                    bool /* result */)
#endif

// If the ViewHostMsg_ShowDisambiguationPopup resulted in the user tapping
// inside the popup, instruct the renderer to generate a synthetic tap at that
// offset.
IPC_MESSAGE_ROUTED3(ViewMsg_ResolveTapDisambiguation,
                    base::TimeTicks /* timestamp */,
                    gfx::Point /* tap_viewport_offset */,
                    bool /* is_long_press */)

IPC_MESSAGE_ROUTED0(ViewMsg_SelectWordAroundCaret)

// Sent by the browser to ask the renderer to redraw. Robust to events that can
// happen in renderer (abortion of the commit or draw, loss of output surface
// etc.).
IPC_MESSAGE_ROUTED1(ViewMsg_ForceRedraw,
                    ui::LatencyInfo /* latency_info */)

// Sets the viewport intersection and compositor raster area on the widget for
// an out-of-process iframe.
IPC_MESSAGE_ROUTED2(ViewMsg_SetViewportIntersection,
                    gfx::Rect /* viewport_intersection */,
                    gfx::Rect /* compositor_visible_rect */)

// Sets the inert bit on an out-of-process iframe.
IPC_MESSAGE_ROUTED1(ViewMsg_SetIsInert, bool /* inert */)

// Sets the inherited effective touch action on an out-of-process iframe.
IPC_MESSAGE_ROUTED1(ViewMsg_SetInheritedEffectiveTouchAction, cc::TouchAction)

// Toggles render throttling for an out-of-process iframe.
IPC_MESSAGE_ROUTED2(ViewMsg_UpdateRenderThrottlingStatus,
                    bool /* is_throttled */,
                    bool /* subtree_throttled */)

// -----------------------------------------------------------------------------
// Messages sent from the renderer to the browser.

// These two messages are sent to the parent RenderViewHost to display a widget
// that was created by CreateWidget/CreateFullscreenWidget. |route_id| refers
// to the id that was returned from the corresponding Create message above.
// |initial_rect| is in screen coordinates.
IPC_MESSAGE_ROUTED2(ViewHostMsg_ShowWidget,
                    int /* route_id */,
                    gfx::Rect /* initial_rect */)

// Message to show a full screen widget.
IPC_MESSAGE_ROUTED1(ViewHostMsg_ShowFullscreenWidget,
                    int /* route_id */)

// Sent by the renderer process to request that the browser close the view.
// This corresponds to the window.close() API, and the browser may ignore
// this message.  Otherwise, the browser will generates a ViewMsg_Close
// message to close the view.
IPC_MESSAGE_ROUTED0(ViewHostMsg_Close)

// Send in response to a ViewMsg_UpdateScreenRects so that the renderer can
// throttle these messages.
IPC_MESSAGE_ROUTED0(ViewHostMsg_UpdateScreenRects_ACK)

// Sent by the renderer process to request that the browser move the view.
// This corresponds to the window.resizeTo() and window.moveTo() APIs, and
// the browser may ignore this message.
IPC_MESSAGE_ROUTED1(ViewHostMsg_RequestMove,
                    gfx::Rect /* position */)

// Indicates that the render view has been closed in respose to a
// Close message.
IPC_MESSAGE_CONTROL1(ViewHostMsg_Close_ACK,
                     int /* old_route_id */)

// Indicates that the current page has been closed, after a ClosePage
// message.
IPC_MESSAGE_ROUTED0(ViewHostMsg_ClosePage_ACK)

// Notifies the browser that we want to show a destination url for a potential
// action (e.g. when the user is hovering over a link).
IPC_MESSAGE_ROUTED1(ViewHostMsg_UpdateTargetURL,
                    GURL)

// Sent when the document element is available for the top-level frame.  This
// happens after the page starts loading, but before all resources are
// finished.
IPC_MESSAGE_ROUTED1(ViewHostMsg_DocumentAvailableInMainFrame,
                    bool /* uses_temporary_zoom_level */)

IPC_MESSAGE_ROUTED0(ViewHostMsg_Focus)

IPC_MESSAGE_ROUTED1(ViewHostMsg_SetCursor, content::WebCursor)

// Request a non-decelerating synthetic fling animation to be latched on the
// scroller at the start point, and whose velocity can be changed over time by
// sending multiple AutoscrollFling gestures.  Used for features like
// middle-click autoscroll.
IPC_MESSAGE_ROUTED1(ViewHostMsg_AutoscrollStart, gfx::PointF /* start */)
IPC_MESSAGE_ROUTED1(ViewHostMsg_AutoscrollFling, gfx::Vector2dF /* velocity */)
IPC_MESSAGE_ROUTED0(ViewHostMsg_AutoscrollEnd)

// Get the list of proxies to use for |url|, as a semicolon delimited list
// of "<TYPE> <HOST>:<PORT>" | "DIRECT".
IPC_SYNC_MESSAGE_CONTROL1_2(ViewHostMsg_ResolveProxy,
                            GURL /* url */,
                            bool /* result */,
                            std::string /* proxy list */)

// Tells the browser that a specific Appcache manifest in the current page
// was accessed.
IPC_MESSAGE_ROUTED2(ViewHostMsg_AppCacheAccessed,
                    GURL /* manifest url */,
                    bool /* blocked by policy */)

// Used to go to the session history entry at the given offset (ie, -1 will
// return the "back" item).
IPC_MESSAGE_ROUTED1(ViewHostMsg_GoToEntryAtOffset,
                    int /* offset (from current) of history item to get */)

// Sent from an inactive renderer for the browser to route to the active
// renderer, instructing it to close.
IPC_MESSAGE_ROUTED0(ViewHostMsg_RouteCloseEvent)

// Notifies that the preferred size of the content changed.
IPC_MESSAGE_ROUTED1(ViewHostMsg_DidContentsPreferredSizeChange,
                    gfx::Size /* pref_size */)

// Notifies whether there are JavaScript touch event handlers or not.
IPC_MESSAGE_ROUTED1(ViewHostMsg_HasTouchEventHandlers,
                    bool /* has_handlers */)

#if BUILDFLAG(ENABLE_PLUGINS)
// A renderer sends this to the browser process when it wants to access a PPAPI
// broker. In contrast to FrameHostMsg_OpenChannelToPpapiBroker, this is called
// for every connection.
// The browser will respond with ViewMsg_PpapiBrokerPermissionResult.
IPC_MESSAGE_ROUTED3(ViewHostMsg_RequestPpapiBrokerPermission,
                    int /* routing_id */,
                    GURL /* document_url */,
                    base::FilePath /* plugin_path */)
#endif  // BUILDFLAG(ENABLE_PLUGINS)

// Send the tooltip text for the current mouse position to the browser.
IPC_MESSAGE_ROUTED2(ViewHostMsg_SetTooltipText,
                    base::string16 /* tooltip text string */,
                    blink::WebTextDirection /* text direction hint */)

// Notification that the selection bounds have changed.
IPC_MESSAGE_ROUTED1(ViewHostMsg_SelectionBoundsChanged,
                    ViewHostMsg_SelectionBounds_Params)

// Asks the browser to enumerate a directory.  This is equivalent to running
// the file chooser in directory-enumeration mode and having the user select
// the given directory.  The result is returned in a
// ViewMsg_EnumerateDirectoryResponse message.
IPC_MESSAGE_ROUTED2(ViewHostMsg_EnumerateDirectory,
                    int /* request_id */,
                    base::FilePath /* file_path */)

// When the renderer needs the browser to transfer focus cross-process on its
// behalf in the focus hierarchy. This may focus an element in the browser ui or
// a cross-process frame, as appropriate.
IPC_MESSAGE_ROUTED1(ViewHostMsg_TakeFocus,
                    bool /* reverse */)

// Required for opening a date/time dialog
IPC_MESSAGE_ROUTED1(ViewHostMsg_OpenDateTimeDialog,
                    ViewHostMsg_DateTimeDialogValue_Params /* value */)

// Required for updating text input state.
IPC_MESSAGE_ROUTED1(ViewHostMsg_TextInputStateChanged,
                    content::TextInputState /* text_input_state */)

// Sent when the renderer changes its page scale factor.
IPC_MESSAGE_ROUTED1(ViewHostMsg_PageScaleFactorChanged,
                    float /* page_scale_factor */)

// Updates the minimum/maximum allowed zoom percent for this tab from the
// default values.  If |remember| is true, then the zoom setting is applied to
// other pages in the site and is saved, otherwise it only applies to this
// tab.
IPC_MESSAGE_ROUTED2(ViewHostMsg_UpdateZoomLimits,
                    int /* minimum_percent */,
                    int /* maximum_percent */)

IPC_MESSAGE_ROUTED2(ViewHostMsg_FrameSwapMessages,
                    uint32_t /* frame_token */,
                    std::vector<IPC::Message> /* messages */)

// Send back a string to be recorded by UserMetrics.
IPC_MESSAGE_CONTROL1(ViewHostMsg_UserMetricsRecordAction,
                     std::string /* action */)

// Notifies the browser of an event occurring in the media pipeline.
IPC_MESSAGE_CONTROL1(ViewHostMsg_MediaLogEvents,
                     std::vector<media::MediaLogEvent> /* events */)

// Requests to lock the mouse. Will result in a ViewMsg_LockMouse_ACK message
// being sent back.
// |privileged| is used by Pepper Flash. If this flag is set to true, we won't
// pop up a bubble to ask for user permission or take mouse lock content into
// account.
IPC_MESSAGE_ROUTED2(ViewHostMsg_LockMouse,
                    bool /* user_gesture */,
                    bool /* privileged */)

// Requests to tell the renderer for the containing frame of the current
// renderer of a change in intrinsic sizing info parameters. This is only
// used for SVG inside of <object>, and not for iframes.
IPC_MESSAGE_ROUTED1(ViewHostMsg_IntrinsicSizingInfoChanged,
                    blink::WebIntrinsicSizingInfo)

// Requests to unlock the mouse. A ViewMsg_MouseLockLost message will be sent
// whenever the mouse is unlocked (which may or may not be caused by
// ViewHostMsg_UnlockMouse).
IPC_MESSAGE_ROUTED0(ViewHostMsg_UnlockMouse)

// Notifies that multiple touch targets may have been pressed, and to show
// the disambiguation popup.
IPC_MESSAGE_ROUTED3(ViewHostMsg_ShowDisambiguationPopup,
                    gfx::Rect, /* Border of touched targets */
                    gfx::Size, /* Size of zoomed image */
                    base::SharedMemoryHandle /* Bitmap pixels */)

// Message sent from renderer to the browser when the element that is focused
// has been touched. A bool is passed in this message which indicates if the
// node is editable.
IPC_MESSAGE_ROUTED1(ViewHostMsg_FocusedNodeTouched,
                    bool /* editable */)

// Sent once a paint happens after the first non empty layout. In other words,
// after the frame widget has painted something.
IPC_MESSAGE_ROUTED0(ViewHostMsg_DidFirstVisuallyNonEmptyPaint)

// Sent in reply to ViewMsg_WaitForNextFrameForTests.
IPC_MESSAGE_ROUTED0(ViewHostMsg_WaitForNextFrameForTests_ACK)

// Acknowledges that a SelectWordAroundCaret completed with the specified
// result and adjustments to the selection offsets.
IPC_MESSAGE_ROUTED3(ViewHostMsg_SelectWordAroundCaretAck,
                    bool /* did_select */,
                    int /* start_adjust */,
                    int /* end_adjust */)

// Adding a new message? Stick to the sort order above: first platform
// independent ViewMsg, then ifdefs for platform specific ViewMsg, then platform
// independent ViewHostMsg, then ifdefs for platform specific ViewHostMsg.

#endif  // CONTENT_COMMON_VIEW_MESSAGES_H_
