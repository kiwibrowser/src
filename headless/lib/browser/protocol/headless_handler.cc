// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/protocol/headless_handler.h"

#include "base/base64.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "cc/base/switches.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/switches.h"
#include "content/public/common/content_switches.h"
#include "headless/lib/browser/headless_browser_impl.h"
#include "headless/lib/browser/headless_web_contents_impl.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_util.h"

namespace headless {
namespace protocol {

using HeadlessExperimental::ScreenshotParams;

namespace {

base::LazyInstance<std::set<HeadlessHandler*>>::Leaky g_instances =
    LAZY_INSTANCE_INITIALIZER;

enum class ImageEncoding { kPng, kJpeg };
constexpr int kDefaultScreenshotQuality = 80;

std::string EncodeBitmap(const SkBitmap& bitmap,
                         ImageEncoding encoding,
                         int quality) {
  gfx::Image image = gfx::Image::CreateFrom1xBitmap(bitmap);
  DCHECK(!image.IsEmpty());

  scoped_refptr<base::RefCountedMemory> data;
  if (encoding == ImageEncoding::kPng) {
    data = image.As1xPNGBytes();
  } else if (encoding == ImageEncoding::kJpeg) {
    scoped_refptr<base::RefCountedBytes> bytes(new base::RefCountedBytes());
    if (gfx::JPEG1xEncodedDataFromImage(image, quality, &bytes->data()))
      data = bytes;
  }

  if (!data || !data->front())
    return std::string();

  std::string base_64_data;
  base::Base64Encode(
      base::StringPiece(reinterpret_cast<const char*>(data->front()),
                        data->size()),
      &base_64_data);
  return base_64_data;
}

void OnBeginFrameFinished(
    std::unique_ptr<HeadlessHandler::BeginFrameCallback> callback,
    ImageEncoding encoding,
    int quality,
    bool has_damage,
    std::unique_ptr<SkBitmap> bitmap) {
  if (!bitmap || bitmap->drawsNothing()) {
    callback->sendSuccess(has_damage, Maybe<String>());
    return;
  }
  std::string data = EncodeBitmap(*bitmap, encoding, quality);
  callback->sendSuccess(has_damage, std::move(data));
}

}  // namespace

// static
void HeadlessHandler::OnNeedsBeginFrames(
    HeadlessWebContentsImpl* headless_contents,
    bool needs_begin_frames) {
  if (!g_instances.IsCreated())
    return;
  for (const HeadlessHandler* handler : g_instances.Get()) {
    if (handler->enabled_ && handler->frontend_)
      handler->frontend_->NeedsBeginFramesChanged(needs_begin_frames);
  }
}

HeadlessHandler::HeadlessHandler(base::WeakPtr<HeadlessBrowserImpl> browser,
                                 content::WebContents* web_contents)
    : DomainHandler(HeadlessExperimental::Metainfo::domainName, browser),
      web_contents_(web_contents) {}

HeadlessHandler::~HeadlessHandler() {
  DCHECK(g_instances.Get().find(this) == g_instances.Get().end());
}

void HeadlessHandler::Wire(UberDispatcher* dispatcher) {
  frontend_.reset(new HeadlessExperimental::Frontend(dispatcher->channel()));
  HeadlessExperimental::Dispatcher::wire(dispatcher, this);
}

Response HeadlessHandler::Enable() {
  g_instances.Get().insert(this);
  HeadlessWebContentsImpl* headless_contents =
      HeadlessWebContentsImpl::From(browser().get(), web_contents_);
  enabled_ = true;
  if (headless_contents->needs_external_begin_frames() && frontend_)
    frontend_->NeedsBeginFramesChanged(true);
  return Response::OK();
}

Response HeadlessHandler::Disable() {
  enabled_ = false;
  g_instances.Get().erase(this);
  return Response::OK();
}

void HeadlessHandler::BeginFrame(Maybe<double> in_frame_time_ticks,
                                 Maybe<double> in_interval,
                                 Maybe<bool> in_no_display_updates,
                                 Maybe<ScreenshotParams> screenshot,
                                 std::unique_ptr<BeginFrameCallback> callback) {
  HeadlessWebContentsImpl* headless_contents =
      HeadlessWebContentsImpl::From(browser().get(), web_contents_);
  if (!headless_contents->begin_frame_control_enabled()) {
    callback->sendFailure(Response::Error(
        "Command is only supported if BeginFrameControl is enabled."));
    return;
  }

  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          ::switches::kRunAllCompositorStagesBeforeDraw)) {
    LOG(WARNING) << "BeginFrameControl commands are designed to be used with "
                    "--run-all-compositor-stages-before-draw, see "
                    "https://goo.gl/3zHXhB for more info.";
  }

  base::TimeTicks frame_time_ticks;
  base::TimeDelta interval;
  bool no_display_updates = in_no_display_updates.fromMaybe(false);

  if (in_frame_time_ticks.isJust()) {
    frame_time_ticks = base::TimeTicks() + base::TimeDelta::FromMillisecondsD(
                                               in_frame_time_ticks.fromJust());
  } else {
    frame_time_ticks = base::TimeTicks::Now();
  }

  if (in_interval.isJust()) {
    double interval_double = in_interval.fromJust();
    if (interval_double <= 0) {
      callback->sendFailure(
          Response::InvalidParams("interval has to be greater than 0"));
      return;
    }
    interval = base::TimeDelta::FromMillisecondsD(interval_double);
  } else {
    interval = viz::BeginFrameArgs::DefaultInterval();
  }

  base::TimeTicks deadline = frame_time_ticks + interval;

  bool capture_screenshot = false;
  ImageEncoding encoding;
  int quality;

  if (screenshot.isJust()) {
    capture_screenshot = true;
    const std::string format =
        screenshot.fromJust()->GetFormat(ScreenshotParams::FormatEnum::Png);
    if (format != ScreenshotParams::FormatEnum::Png &&
        format != ScreenshotParams::FormatEnum::Jpeg) {
      callback->sendFailure(
          Response::InvalidParams("Invalid screenshot.format"));
      return;
    }
    encoding = format == ScreenshotParams::FormatEnum::Png
                   ? ImageEncoding::kPng
                   : ImageEncoding::kJpeg;
    quality = screenshot.fromJust()->GetQuality(kDefaultScreenshotQuality);
    if (quality < 0 || quality > 100) {
      callback->sendFailure(Response::InvalidParams(
          "screenshot.quality has to be in range 0..100"));
      return;
    }
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          ::switches::kRunAllCompositorStagesBeforeDraw) &&
      headless_contents->HasPendingFrame()) {
    LOG(WARNING) << "A BeginFrame is already in flight. In "
                    "--run-all-compositor-stages-before-draw mode, only a "
                    "single BeginFrame should be active at the same time.";
  }

  headless_contents->BeginFrame(
      frame_time_ticks, deadline, interval, no_display_updates,
      capture_screenshot,
      base::BindOnce(&OnBeginFrameFinished, std::move(callback), encoding,
                     quality));
}

}  // namespace protocol
}  // namespace headless
