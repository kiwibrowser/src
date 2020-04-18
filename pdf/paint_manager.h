// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_PAINT_MANAGER_H_
#define PDF_PAINT_MANAGER_H_

#include <stdint.h>

#include <vector>

#include "pdf/paint_aggregator.h"
#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/utility/completion_callback_factory.h"

namespace pp {
class Graphics2D;
class Instance;
class Point;
class Rect;
};

// Custom PaintManager for the PDF plugin.  This is branched from the Pepper
// version.  The difference is that this supports progressive rendering of dirty
// rects, where multiple calls to the rendering engine are needed.  It also
// supports having higher-priority rects flushing right away, i.e. the
// scrollbars.
//
// The client's OnPaint
class PaintManager {
 public:
  // Like PaintAggregator's version, but allows the plugin to tell us whether
  // it should be flushed to the screen immediately or when the rest of the
  // plugin viewport is ready.
  struct ReadyRect {
    ReadyRect();
    ReadyRect(const pp::Rect& r, const pp::ImageData& i, bool f);
    ReadyRect(const ReadyRect& that);

    operator PaintAggregator::ReadyRect() const {
      PaintAggregator::ReadyRect rv;
      rv.offset = offset;
      rv.rect = rect;
      rv.image_data = image_data;
      return rv;
    }

    pp::Point offset;
    pp::Rect rect;
    pp::ImageData image_data;
    bool flush_now;
  };
  class Client {
   public:
    // Paints the given invalid area of the plugin to the given graphics
    // device. Returns true if anything was painted.
    //
    // You are given the list of rects to paint in |paint_rects|.  You can
    // combine painting into less rectangles if it's more efficient.  When a
    // rect is painted, information about that paint should be inserted into
    // |ready|.  Otherwise if a paint needs more work, add the rect to
    // |pending|.  If |pending| is not empty, your OnPaint function will get
    // called again.  Once OnPaint is called and it returns no pending rects,
    // all the previously ready rects will be flushed on screen.  The exception
    // is for ready rects that have |flush_now| set to true.  These will be
    // flushed right away.
    //
    // Do not call Flush() on the graphics device, this will be done
    // automatically if you return true from this function since the
    // PaintManager needs to handle the callback.
    //
    // Calling Invalidate/Scroll is not allowed while inside an OnPaint
    virtual void OnPaint(const std::vector<pp::Rect>& paint_rects,
                         std::vector<ReadyRect>* ready,
                         std::vector<pp::Rect>* pending) = 0;

   protected:
    // You shouldn't be doing deleting through this interface.
    virtual ~Client() {}
  };

  // The instance is the plugin instance using this paint manager to do its
  // painting. Painting will automatically go to this instance and you don't
  // have to manually bind any device context (this is all handled by the
  // paint manager).
  //
  // The Client is a non-owning pointer and must remain valid (normally the
  // object implementing the Client interface will own the paint manager).
  //
  // The is_always_opaque flag will be passed to the device contexts that this
  // class creates. Set this to true if your plugin always draws an opaque
  // image to the device. This is used as a hint to the browser that it does
  // not need to do alpha blending, which speeds up painting. If you generate
  // non-opqaue pixels or aren't sure, set this to false for more general
  // blending.
  //
  // If you set is_always_opaque, your alpha channel should always be set to
  // 0xFF or there may be painting artifacts. Being opaque will allow the
  // browser to do a memcpy rather than a blend to paint the plugin, and this
  // means your alpha values will get set on the page backing store. If these
  // values are incorrect, it could mess up future blending. If you aren't
  // sure, it is always correct to specify that it it not opaque.
  //
  // You will need to call SetSize before this class will do anything. Normally
  // you do this from the ViewChanged method of your plugin instance.
  PaintManager(pp::Instance* instance, Client* client, bool is_always_opaque);

  ~PaintManager();

  // Returns the size of the graphics context to allocate for a given plugin
  // size. We may allocated a slightly larger buffer than required so that we
  // don't have to resize the context when scrollbars appear/dissapear due to
  // zooming (which can result in flickering).
  static pp::Size GetNewContextSize(const pp::Size& current_context_size,
                                    const pp::Size& plugin_size);

  // You must call this function before using if you use the 0-arg constructor.
  // See the constructor for what these arguments mean.
  void Initialize(pp::Instance* instance,
                  Client* client,
                  bool is_always_opaque);

  // Sets the size of the plugin. If the size is the same as the previous call,
  // this will be a NOP. If the size has changed, a new device will be
  // allocated to the given size and a paint to that device will be scheduled.
  //
  // This is intended to be called from ViewChanged with the size of the
  // plugin. Since it tracks the old size and only allocates when the size
  // changes, you can always call this function without worrying about whether
  // the size changed or ViewChanged is called for another reason (like the
  // position changed).
  void SetSize(const pp::Size& new_size, float new_device_scale);

  // Invalidate the entire plugin.
  void Invalidate();

  // Invalidate the given rect.
  void InvalidateRect(const pp::Rect& rect);

  // The given rect should be scrolled by the given amounts.
  void ScrollRect(const pp::Rect& clip_rect, const pp::Point& amount);

  // Returns the size of the graphics context for the next paint operation.
  // This is the pending size if a resize is pending (the plugin has called
  // SetSize but we haven't actually painted it yet), or the current size of
  // no resize is pending.
  pp::Size GetEffectiveSize() const;
  float GetEffectiveDeviceScale() const;

  // Set the transform for the graphics layer.
  // If |schedule_flush| is true, it ensures a flush will be scheduled for
  // this change. If |schedule_flush| is false, then the change will not take
  // effect until another change causes a flush.
  void SetTransform(float scale,
                    const pp::Point& origin,
                    const pp::Point& translate,
                    bool schedule_flush);
  // Resets any transform for the graphics layer.
  // This does not schedule a flush.
  void ClearTransform();

 private:
  // Disallow copy and assign (these are unimplemented).
  PaintManager(const PaintManager&);
  PaintManager& operator=(const PaintManager&);

  // Makes sure there is a callback that will trigger a paint at a later time.
  // This will be either a Flush callback telling us we're allowed to generate
  // more data, or, if there's no flush callback pending, a manual call back
  // to the message loop via ExecuteOnMainThread.
  void EnsureCallbackPending();

  // Does the client paint and executes a Flush if necessary.
  void DoPaint();

  // Executes a Flush.
  void Flush();

  // Callback for asynchronous completion of Flush.
  void OnFlushComplete(int32_t);

  // Callback for manual scheduling of paints when there is no flush callback
  // pending.
  void OnManualCallbackComplete(int32_t);

  pp::Instance* instance_;

  // Non-owning pointer. See the constructor.
  Client* client_;

  bool is_always_opaque_;

  pp::CompletionCallbackFactory<PaintManager> callback_factory_;

  // This graphics device will be is_null() if no graphics has been manually
  // set yet.
  pp::Graphics2D graphics_;

  PaintAggregator aggregator_;

  // See comment for EnsureCallbackPending for more on how these work.
  bool manual_callback_pending_;
  bool flush_pending_;
  bool flush_requested_;

  // When we get a resize, we don't bind right away (see SetSize). The
  // has_pending_resize_ tells us that we need to do a resize for the next
  // paint operation. When true, the new size is in pending_size_.
  bool has_pending_resize_;
  bool graphics_need_to_be_bound_;
  pp::Size pending_size_;
  pp::Size plugin_size_;
  float pending_device_scale_;
  float device_scale_;

  // True iff we're in the middle of a paint.
  bool in_paint_;

  // True if we haven't painted the plugin viewport yet.
  bool first_paint_;

  // True when the view size just changed and we're waiting for a paint.
  bool view_size_changed_waiting_for_paint_;
};

#endif  // PDF_PAINT_MANAGER_H_
