// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/chromoting_host_context.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "remoting/base/auto_thread.h"
#include "remoting/base/url_request_context_getter.h"

namespace remoting {

namespace {

void DisallowBlockingOperations() {
  base::ThreadRestrictions::SetIOAllowed(false);
  // TODO(crbug.com/793486): Re-enable after the underlying issue is fixed.
  // base::ThreadRestrictions::DisallowBaseSyncPrimitives();
}

}  // namespace

ChromotingHostContext::ChromotingHostContext(
    scoped_refptr<AutoThreadTaskRunner> ui_task_runner,
    scoped_refptr<AutoThreadTaskRunner> audio_task_runner,
    scoped_refptr<AutoThreadTaskRunner> file_task_runner,
    scoped_refptr<AutoThreadTaskRunner> input_task_runner,
    scoped_refptr<AutoThreadTaskRunner> network_task_runner,
    scoped_refptr<AutoThreadTaskRunner> video_capture_task_runner,
    scoped_refptr<AutoThreadTaskRunner> video_encode_task_runner,
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
    ui::SystemInputInjectorFactory* system_input_injector_factory)
    : ui_task_runner_(ui_task_runner),
      audio_task_runner_(audio_task_runner),
      file_task_runner_(file_task_runner),
      input_task_runner_(input_task_runner),
      network_task_runner_(network_task_runner),
      video_capture_task_runner_(video_capture_task_runner),
      video_encode_task_runner_(video_encode_task_runner),
      url_request_context_getter_(url_request_context_getter),
      system_input_injector_factory_(system_input_injector_factory) {}

ChromotingHostContext::~ChromotingHostContext() = default;

std::unique_ptr<ChromotingHostContext> ChromotingHostContext::Copy() {
  return base::WrapUnique(new ChromotingHostContext(
      ui_task_runner_, audio_task_runner_, file_task_runner_,
      input_task_runner_, network_task_runner_, video_capture_task_runner_,
      video_encode_task_runner_, url_request_context_getter_,
      system_input_injector_factory_));
}

scoped_refptr<AutoThreadTaskRunner> ChromotingHostContext::audio_task_runner()
    const {
  return audio_task_runner_;
}

scoped_refptr<AutoThreadTaskRunner> ChromotingHostContext::file_task_runner()
    const {
  return file_task_runner_;
}

scoped_refptr<AutoThreadTaskRunner> ChromotingHostContext::input_task_runner()
    const {
  return input_task_runner_;
}

scoped_refptr<AutoThreadTaskRunner> ChromotingHostContext::network_task_runner()
    const {
  return network_task_runner_;
}

scoped_refptr<AutoThreadTaskRunner> ChromotingHostContext::ui_task_runner()
    const {
  return ui_task_runner_;
}

scoped_refptr<AutoThreadTaskRunner>
ChromotingHostContext::video_capture_task_runner() const {
  return video_capture_task_runner_;
}

scoped_refptr<AutoThreadTaskRunner>
ChromotingHostContext::video_encode_task_runner() const {
  return video_encode_task_runner_;
}

scoped_refptr<net::URLRequestContextGetter>
ChromotingHostContext::url_request_context_getter() const {
  return url_request_context_getter_;
}

ui::SystemInputInjectorFactory*
ChromotingHostContext::system_input_injector_factory() const {
  return system_input_injector_factory_;
}

std::unique_ptr<ChromotingHostContext> ChromotingHostContext::Create(
    scoped_refptr<AutoThreadTaskRunner> ui_task_runner) {
#if defined(OS_WIN)
  // On Windows the AudioCapturer requires COM, so we run a single-threaded
  // apartment, which requires a UI thread.
  scoped_refptr<AutoThreadTaskRunner> audio_task_runner =
      AutoThread::CreateWithLoopAndComInitTypes(
          "ChromotingAudioThread", ui_task_runner, base::MessageLoop::TYPE_UI,
          AutoThread::COM_INIT_STA);
#else   // !defined(OS_WIN)
  scoped_refptr<AutoThreadTaskRunner> audio_task_runner =
      AutoThread::CreateWithType("ChromotingAudioThread", ui_task_runner,
                                 base::MessageLoop::TYPE_IO);
#endif  // !defined(OS_WIN)
  scoped_refptr<AutoThreadTaskRunner> file_task_runner =
      AutoThread::CreateWithType("ChromotingFileThread", ui_task_runner,
                                 base::MessageLoop::TYPE_IO);

  scoped_refptr<AutoThreadTaskRunner> network_task_runner =
      AutoThread::CreateWithType("ChromotingNetworkThread", ui_task_runner,
                                 base::MessageLoop::TYPE_IO);
  network_task_runner->PostTask(FROM_HERE,
                                base::Bind(&DisallowBlockingOperations));

  return base::WrapUnique(new ChromotingHostContext(
      ui_task_runner, audio_task_runner, file_task_runner,
      AutoThread::CreateWithType("ChromotingInputThread", ui_task_runner,
                                 base::MessageLoop::TYPE_IO),
      network_task_runner,
#if defined(OS_MACOSX)
      // Mac requires a UI thread for the capturer.
      AutoThread::CreateWithType("ChromotingCaptureThread", ui_task_runner,
                                 base::MessageLoop::TYPE_UI),
#else
      AutoThread::Create("ChromotingCaptureThread", ui_task_runner),
#endif
      AutoThread::Create("ChromotingEncodeThread", ui_task_runner),
      base::MakeRefCounted<URLRequestContextGetter>(network_task_runner),
      nullptr));
}

#if defined(OS_CHROMEOS)

// static
std::unique_ptr<ChromotingHostContext> ChromotingHostContext::CreateForChromeOS(
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> file_task_runner,
    ui::SystemInputInjectorFactory* system_input_injector_factory) {
  DCHECK(url_request_context_getter.get());

  // AutoThreadTaskRunner is a TaskRunner with the special property that it will
  // continue to process tasks until no references remain, at least. The
  // QuitClosure we usually pass does the simple thing of stopping the
  // underlying TaskRunner. Since we are re-using browser's threads, we cannot
  // stop them explicitly. Therefore, base::DoNothing is passed in as the quit
  // closure.
  scoped_refptr<AutoThreadTaskRunner> io_auto_task_runner =
      new AutoThreadTaskRunner(io_task_runner, base::DoNothing());
  scoped_refptr<AutoThreadTaskRunner> file_auto_task_runner =
      new AutoThreadTaskRunner(file_task_runner, base::DoNothing());
  scoped_refptr<AutoThreadTaskRunner> ui_auto_task_runner =
      new AutoThreadTaskRunner(ui_task_runner, base::DoNothing());

  // Use browser's file thread as the joiner as it is the only browser-thread
  // that allows blocking I/O, which is required by thread joining.
  return base::WrapUnique(new ChromotingHostContext(
      ui_auto_task_runner,
      AutoThread::Create("ChromotingAudioThread", file_auto_task_runner),
      file_auto_task_runner,
      ui_auto_task_runner,  // input_task_runner
      io_auto_task_runner,  // network_task_runner
      ui_auto_task_runner,  // video_capture_task_runner
      AutoThread::Create("ChromotingEncodeThread", file_auto_task_runner),
      url_request_context_getter, system_input_injector_factory));
}
#endif  // defined(OS_CHROMEOS)

}  // namespace remoting
