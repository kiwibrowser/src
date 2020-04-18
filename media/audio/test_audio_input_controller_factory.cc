// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/test_audio_input_controller_factory.h"

#include <utility>

#include "media/audio/audio_io.h"
#include "media/audio/audio_manager.h"

namespace media {

TestAudioInputController::TestAudioInputController(
    TestAudioInputControllerFactory* factory,
    AudioManager* audio_manager,
    const AudioParameters& audio_parameters,
    EventHandler* event_handler,
    SyncWriter* sync_writer,
    UserInputMonitor* user_input_monitor,
    StreamType type)
    : AudioInputController(audio_manager->GetTaskRunner(),
                           event_handler,
                           sync_writer,
                           user_input_monitor,
                           audio_parameters,
                           type),
      audio_parameters_(audio_parameters),
      factory_(factory),
      event_handler_(event_handler),
      sync_writer_(sync_writer) {}

TestAudioInputController::~TestAudioInputController() {
  // Inform the factory so that it allows creating new instances in future.
  factory_->OnTestAudioInputControllerDestroyed(this);
}

void TestAudioInputController::Record() {
  if (factory_->delegate_)
    factory_->delegate_->TestAudioControllerOpened(this);
}

void TestAudioInputController::Close(base::OnceClosure closed_task) {
  GetTaskRunnerForTesting()->PostTask(FROM_HERE, std::move(closed_task));
  if (factory_->delegate_)
    factory_->delegate_->TestAudioControllerClosed(this);
}

TestAudioInputControllerFactory::TestAudioInputControllerFactory()
    : controller_(NULL),
      delegate_(NULL) {
}

TestAudioInputControllerFactory::~TestAudioInputControllerFactory() {
  DCHECK(!controller_);
}

AudioInputController* TestAudioInputControllerFactory::Create(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    AudioInputController::SyncWriter* sync_writer,
    AudioManager* audio_manager,
    AudioInputController::EventHandler* event_handler,
    AudioParameters params,
    UserInputMonitor* user_input_monitor,
    AudioInputController::StreamType type) {
  DCHECK(!controller_);  // Only one test instance managed at a time.
  controller_ =
      new TestAudioInputController(this, audio_manager, params, event_handler,
                                   sync_writer, user_input_monitor, type);
  return controller_;
}

void TestAudioInputControllerFactory::OnTestAudioInputControllerDestroyed(
    TestAudioInputController* controller) {
  DCHECK_EQ(controller_, controller);
  controller_ = NULL;
}

}  // namespace media
