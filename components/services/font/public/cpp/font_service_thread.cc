// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/font/public/cpp/font_service_thread.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/synchronization/waitable_event.h"
#include "components/services/font/public/cpp/mapped_font_file.h"

namespace font_service {
namespace internal {

namespace {
const char kFontThreadName[] = "Font_Proxy_Thread";
}  // namespace

FontServiceThread::FontServiceThread(mojom::FontServicePtr font_service)
    : base::Thread(kFontThreadName),
      font_service_info_(font_service.PassInterface()),
      weak_factory_(this) {
  Start();
}

bool FontServiceThread::MatchFamilyName(
    const char family_name[],
    SkFontStyle requested_style,
    SkFontConfigInterface::FontIdentity* out_font_identity,
    SkString* out_family_name,
    SkFontStyle* out_style) {
  DCHECK_NE(GetThreadId(), base::PlatformThread::CurrentId());

  bool out_valid = false;
  // This proxies to the other thread, which proxies to mojo. Only on the reply
  // from mojo do we return from this.
  base::WaitableEvent done_event(
      base::WaitableEvent::ResetPolicy::AUTOMATIC,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&FontServiceThread::MatchFamilyNameImpl, this, &done_event,
                 family_name, requested_style, &out_valid, out_font_identity,
                 out_family_name, out_style));
  done_event.Wait();

  return out_valid;
}

scoped_refptr<MappedFontFile> FontServiceThread::OpenStream(
    const SkFontConfigInterface::FontIdentity& identity) {
  DCHECK_NE(GetThreadId(), base::PlatformThread::CurrentId());

  base::File stream_file;
  // This proxies to the other thread, which proxies to mojo. Only on the reply
  // from mojo do we return from this.
  base::WaitableEvent done_event(
      base::WaitableEvent::ResetPolicy::AUTOMATIC,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  task_runner()->PostTask(FROM_HERE,
                          base::Bind(&FontServiceThread::OpenStreamImpl, this,
                                     &done_event, &stream_file, identity.fID));
  done_event.Wait();

  if (!stream_file.IsValid()) {
    // The font-service may have been killed.
    return nullptr;
  }

  // Converts the file to out internal type.
  scoped_refptr<MappedFontFile> mapped_font_file =
      new MappedFontFile(identity.fID);
  if (!mapped_font_file->Initialize(std::move(stream_file)))
    return nullptr;

  return mapped_font_file;
}

FontServiceThread::~FontServiceThread() {
  Stop();
}

void FontServiceThread::MatchFamilyNameImpl(
    base::WaitableEvent* done_event,
    const char family_name[],
    SkFontStyle requested_style,
    bool* out_valid,
    SkFontConfigInterface::FontIdentity* out_font_identity,
    SkString* out_family_name,
    SkFontStyle* out_style) {
  DCHECK_EQ(GetThreadId(), base::PlatformThread::CurrentId());

  if (font_service_.encountered_error()) {
    *out_valid = false;
    done_event->Signal();
    return;
  }

  mojom::TypefaceStylePtr style(mojom::TypefaceStyle::New());
  style->weight = requested_style.weight();
  style->width = requested_style.width();
  style->slant = static_cast<mojom::TypefaceSlant>(requested_style.slant());

  pending_waitable_events_.insert(done_event);
  font_service_->MatchFamilyName(
      family_name, std::move(style),
      base::Bind(&FontServiceThread::OnMatchFamilyNameComplete, this,
                 done_event, out_valid, out_font_identity, out_family_name,
                 out_style));
}

void FontServiceThread::OnMatchFamilyNameComplete(
    base::WaitableEvent* done_event,
    bool* out_valid,
    SkFontConfigInterface::FontIdentity* out_font_identity,
    SkString* out_family_name,
    SkFontStyle* out_style,
    mojom::FontIdentityPtr font_identity,
    const std::string& family_name,
    mojom::TypefaceStylePtr style) {
  DCHECK_EQ(GetThreadId(), base::PlatformThread::CurrentId());
  pending_waitable_events_.erase(done_event);

  *out_valid = !font_identity.is_null();
  if (font_identity) {
    out_font_identity->fID = font_identity->id;
    out_font_identity->fTTCIndex = font_identity->ttc_index;
    out_font_identity->fString = font_identity->str_representation.data();
    // TODO(erg): fStyle isn't set. This is rather odd, however it matches the
    // behaviour of the current Linux IPC version.

    *out_family_name = family_name.data();
    *out_style = SkFontStyle(style->weight, style->width,
                             static_cast<SkFontStyle::Slant>(style->slant));
  }

  done_event->Signal();
}

void FontServiceThread::OpenStreamImpl(base::WaitableEvent* done_event,
                                       base::File* output_file,
                                       const uint32_t id_number) {
  DCHECK_EQ(GetThreadId(), base::PlatformThread::CurrentId());
  if (font_service_.encountered_error()) {
    done_event->Signal();
    return;
  }

  pending_waitable_events_.insert(done_event);
  font_service_->OpenStream(
      id_number, base::Bind(&FontServiceThread::OnOpenStreamComplete, this,
                            done_event, output_file));
}

void FontServiceThread::OnOpenStreamComplete(base::WaitableEvent* done_event,
                                             base::File* output_file,
                                             base::File file) {
  pending_waitable_events_.erase(done_event);
  *output_file = std::move(file);
  done_event->Signal();
}

void FontServiceThread::OnFontServiceConnectionError() {
  std::set<base::WaitableEvent*> events;
  events.swap(pending_waitable_events_);
  for (base::WaitableEvent* event : events)
    event->Signal();
}

void FontServiceThread::Init() {
  font_service_.Bind(std::move(font_service_info_));
  font_service_.set_connection_error_handler(
      base::Bind(&FontServiceThread::OnFontServiceConnectionError,
                 weak_factory_.GetWeakPtr()));
}

void FontServiceThread::CleanUp() {
  font_service_.reset();
}

}  // namespace internal
}  // namespace font_service
