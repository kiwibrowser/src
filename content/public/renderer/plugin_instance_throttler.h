// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_PLUGIN_INSTANCE_THROTTLER_H_
#define CONTENT_PUBLIC_RENDERER_PLUGIN_INSTANCE_THROTTLER_H_

#include <memory>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/renderer/render_frame.h"

namespace blink {
class WebPlugin;
}

namespace gfx {
class Size;
}

class SkBitmap;

namespace content {

// This class manages the metric collection, throttling, and unthrottling of a
// single peripheral plugin instance. If the Power Saver feature is disabled,
// the plugin instance will never actually be throttled, but still collects
// user interaction metrics.
//
// The process for throttling a plugin is as follows:
// 1) Attempt to find a representative keyframe to display as a placeholder for
//    the plugin.
// 2) a) If a representative keyframe is found, throttle the plugin at that
//       keyframe.
//    b) If a representative keyframe is not found, throttle the plugin after a
//       certain period of time.
//
// The plugin will then be unthrottled by receiving a mouse click from the user.
//
// To choose a representative keyframe, we first wait for a certain number of
// "interesting" frames to be displayed by the plugin. A frame is called
// interesting if it meets some heuristic. After we have seen a certain number
// of interesting frames, we throttle the plugin and use that frame as the
// representative keyframe.
class CONTENT_EXPORT PluginInstanceThrottler {
 public:
  // How the throttled power saver is unthrottled, if ever.
  // These numeric values are used in UMA logs; do not change them.
  enum PowerSaverUnthrottleMethod {
    UNTHROTTLE_METHOD_DO_NOT_RECORD = -1,  // Sentinel value to skip recording.
    UNTHROTTLE_METHOD_NEVER = 0,
    UNTHROTTLE_METHOD_BY_CLICK = 1,
    UNTHROTTLE_METHOD_BY_WHITELIST = 2,
    UNTHROTTLE_METHOD_BY_AUDIO = 3,
    UNTHROTTLE_METHOD_BY_SIZE_CHANGE = 4,
    UNTHROTTLE_METHOD_BY_OMNIBOX_ICON = 5,
    UNTHROTTLE_METHOD_NUM_ITEMS
  };

  class Observer {
   public:
    // Guaranteed to be called before the throttle is engaged.
    virtual void OnKeyframeExtracted(const SkBitmap* bitmap) {}

    virtual void OnThrottleStateChange() {}

    virtual void OnPeripheralStateChange() {}

    // Called when the plugin should be hidden due to a placeholder.
    virtual void OnHiddenForPlaceholder(bool hidden) {}

    virtual void OnThrottlerDestroyed() {}
  };

  static std::unique_ptr<PluginInstanceThrottler> Create(
      RenderFrame::RecordPeripheralDecision record_decision);

  static void RecordUnthrottleMethodMetric(PowerSaverUnthrottleMethod method);

  virtual ~PluginInstanceThrottler() {}

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  virtual bool IsThrottled() const = 0;
  virtual bool IsHiddenForPlaceholder() const = 0;

  // Marks the plugin as essential. Unthrottles the plugin if already throttled.
  virtual void MarkPluginEssential(PowerSaverUnthrottleMethod method) = 0;

  // Called by the placeholder when the plugin should temporarily be hidden.
  virtual void SetHiddenForPlaceholder(bool hidden) = 0;

  virtual blink::WebPlugin* GetWebPlugin() const = 0;

  // Gets the throttler's best estimate of the plugin's visible dimensions.
  virtual const gfx::Size& GetSize() const = 0;

  // Throttler needs to know when the plugin audio is throttled, as this may
  // prevent the plugin from generating new frames.
  virtual void NotifyAudioThrottled() = 0;

 protected:
  PluginInstanceThrottler() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PluginInstanceThrottler);
};
}

#endif  // CONTENT_PUBLIC_RENDERER_PLUGIN_INSTANCE_THROTTLER_H_
