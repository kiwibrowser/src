// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Helper class used to find the RenderViewObservers for a given RenderView.
//
// Example usage:
//
//   class MyRVO : public RenderViewObserver,
//                 public RenderViewObserverTracker<MyRVO> {
//     ...
//   };
//
//   MyRVO::MyRVO(RenderView* render_view)
//       : RenderViewObserver(render_view),
//         RenderViewObserverTracker<MyRVO>(render_view) {
//     ...
//   }
//
//  void SomeFunction(RenderView* rv) {
//    MyRVO* my_rvo = new MyRVO(rv);
//    MyRVO* my_rvo_tracked = MyRVO::Get(rv);
//    // my_rvo == my_rvo_tracked
//  }

#ifndef CONTENT_PUBLIC_RENDERER_RENDER_VIEW_OBSERVER_TRACKER_H_
#define CONTENT_PUBLIC_RENDERER_RENDER_VIEW_OBSERVER_TRACKER_H_

#include <map>

#include "base/lazy_instance.h"
#include "base/macros.h"

namespace content {

class RenderView;

template <class T>
class RenderViewObserverTracker {
 public:
  static T* Get(const RenderView* render_view) {
    return static_cast<T*>(render_view_map_.Get()[render_view]);
  }

  explicit RenderViewObserverTracker(const RenderView* render_view)
      : render_view_(render_view) {
    render_view_map_.Get()[render_view] = this;
  }
  ~RenderViewObserverTracker() {
    render_view_map_.Get().erase(render_view_);
  }

 private:
  const RenderView* render_view_;

  static typename base::LazyInstance<
      std::map<const RenderView*, RenderViewObserverTracker<T>*>>::
      DestructorAtExit render_view_map_;

  DISALLOW_COPY_AND_ASSIGN(RenderViewObserverTracker<T>);
};

template <class T>
typename base::LazyInstance<
    std::map<const RenderView*, RenderViewObserverTracker<T>*>>::
    DestructorAtExit RenderViewObserverTracker<T>::render_view_map_ =
        LAZY_INSTANCE_INITIALIZER;

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_RENDER_VIEW_OBSERVER_TRACKER_H_
