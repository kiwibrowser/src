// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_FONT_PUBLIC_CPP_FONT_SERVICE_THREAD_H_
#define COMPONENTS_SERVICES_FONT_PUBLIC_CPP_FONT_SERVICE_THREAD_H_

#include <stdint.h>

#include <set>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "components/services/font/public/interfaces/font_service.mojom.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "third_party/skia/include/ports/SkFontConfigInterface.h"

namespace font_service {
namespace internal {

class MappedFontFile;

// The thread which services font requests.
//
// The SkFontConfigInterface is a global singleton which can be accessed from
// multiple threads. However, mojo pipes are bound to a single thread. Because
// of this mismatch, we create a thread which owns the mojo pipe, sends and
// receives messages. The multiple threads which call through FontLoader class
// do blocking message calls to this thread.
class FontServiceThread : public base::Thread,
                          public base::RefCountedThreadSafe<FontServiceThread> {
 public:
  explicit FontServiceThread(mojom::FontServicePtr font_service);

  // These methods are proxies which run on your thread, post a blocking task
  // to the FontServiceThread, and wait on an event signaled from the callback.
  bool MatchFamilyName(const char family_name[],
                       SkFontStyle requested_style,
                       SkFontConfigInterface::FontIdentity* out_font_identity,
                       SkString* out_family_name,
                       SkFontStyle* out_style);
  scoped_refptr<MappedFontFile> OpenStream(
      const SkFontConfigInterface::FontIdentity& identity);

 private:
  friend class base::RefCountedThreadSafe<FontServiceThread>;
  ~FontServiceThread() override;

  // Methods which run on the FontServiceThread. The public MatchFamilyName
  // calls this method, this method calls the mojo interface, and sets up the
  // callback to OnMatchFamilyNameComplete.
  void MatchFamilyNameImpl(
      base::WaitableEvent* done_event,
      const char family_name[],
      SkFontStyle requested_style,
      bool* out_valid,
      SkFontConfigInterface::FontIdentity* out_font_identity,
      SkString* out_family_name,
      SkFontStyle* out_style);

  // Called on the FontServiceThread in response to receiving a message from
  // our MatchFamily mojo IPC. This writes the data returned by mojo, and then
  // signals |done_event| to wake up the other thread.
  void OnMatchFamilyNameComplete(
      base::WaitableEvent* done_event,
      bool* out_valid,
      SkFontConfigInterface::FontIdentity* out_font_identity,
      SkString* out_family_name,
      SkFontStyle* out_style,
      mojom::FontIdentityPtr font_identity,
      const std::string& family_name,
      mojom::TypefaceStylePtr style);

  // Implementation of OpenStream; same threading restrictions as MatchFamily.
  void OpenStreamImpl(base::WaitableEvent* done_event,
                      base::File* output_file,
                      const uint32_t id_number);
  void OnOpenStreamComplete(base::WaitableEvent* done_event,
                            base::File* output_file,
                            base::File file);

  // Connection to |font_service_| has gone away. Called on the background
  // thread.
  void OnFontServiceConnectionError();

  // base::Thread
  void Init() override;
  void CleanUp() override;

  // This member is used to safely pass data from one thread to another. It is
  // set in the constructor and is consumed in Init().
  mojo::InterfacePtrInfo<mojom::FontService> font_service_info_;

  // This member is set in Init(). It takes |font_service_info_|, which is
  // non-thread bound, and binds it to the newly created thread.
  mojo::InterfacePtr<mojom::FontService> font_service_;

  // All WaitableEvents supplied to OpenStreamImpl() are added here while
  // waiting on the response from the |font_service_| (FontService::OpenStream()
  // was called, but the callback has not been processed yet). If
  // |font_service_| gets an error during this time all events in
  // |pending_waitable_events_| are signaled. This is necessary as when the
  // pipe is closed the callbacks are never received.
  std::set<base::WaitableEvent*> pending_waitable_events_;

  base::WeakPtrFactory<FontServiceThread> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FontServiceThread);
};

}  // namespace internal
}  // namespace font_service

#endif  // COMPONENTS_SERVICES_FONT_PUBLIC_CPP_FONT_SERVICE_THREAD_H_
