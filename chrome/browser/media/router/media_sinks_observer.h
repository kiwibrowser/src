// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MEDIA_SINKS_OBSERVER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MEDIA_SINKS_OBSERVER_H_

#include <vector>

#include "base/macros.h"
#include "chrome/common/media_router/media_sink.h"
#include "chrome/common/media_router/media_source.h"
#include "url/origin.h"

namespace media_router {

class MediaRouter;

// Base class for observing when the collection of sinks compatible with
// a MediaSource has been updated.
// A MediaSinksObserver implementation can be registered to MediaRouter to
// receive results. It can then interpret / process the results accordingly.
// More documentation can be found at
// docs.google.com/document/d/1RDXdzi2y7lRuL08HAe-qlSJG2DMz2iH3gBzMs0IRR78
class MediaSinksObserver {
 public:
  // Constructs an observer from |origin| that will observe for sinks compatible
  // with |source|.
  MediaSinksObserver(MediaRouter* router,
                     const MediaSource& source,
                     const url::Origin& origin);
  virtual ~MediaSinksObserver();

  // Registers with MediaRouter to start observing. Must be called before the
  // observer will start receiving updates. Returns |true| if the observer is
  // initialized. This method is no-op if the observer is already initialized.
  bool Init();

  // This function is invoked when the list of sinks compatible with |source_|
  // has been updated. The result also contains the list of valid origins.
  // If |origins| is empty or contains |origin_|, then |OnSinksReceived(sinks)|
  // will be invoked with |sinks|. Otherwise, it will be invoked with an empty
  // list.
  // Marked virtual for tests.
  virtual void OnSinksUpdated(const std::vector<MediaSink>& sinks,
                              const std::vector<url::Origin>& origins);

  const MediaSource& source() const { return source_; }

 protected:
  // This function is invoked from |OnSinksUpdated(sinks, origins)|.
  // Implementations may not perform operations that modify the Media Router's
  // observer list. In particular, invoking this observer's destructor within
  // OnSinksReceived will result in undefined behavior.
  virtual void OnSinksReceived(const std::vector<MediaSink>& sinks) = 0;

 private:
  const MediaSource source_;
  const url::Origin origin_;
  MediaRouter* const router_;
  bool initialized_;

#if DCHECK_IS_ON()
  bool in_on_sinks_updated_ = false;
#endif

  DISALLOW_COPY_AND_ASSIGN(MediaSinksObserver);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MEDIA_SINKS_OBSERVER_H_
