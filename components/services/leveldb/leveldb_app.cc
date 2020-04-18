// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/leveldb/leveldb_app.h"

#include "base/task_scheduler/post_task.h"
#include "components/services/leveldb/leveldb_service_impl.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace leveldb {

LevelDBApp::LevelDBApp()
    : file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {
  registry_.AddInterface<mojom::LevelDBService>(
      base::Bind(&LevelDBApp::Create, base::Unretained(this)));
}

LevelDBApp::~LevelDBApp() {}

void LevelDBApp::OnStart() {}

void LevelDBApp::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void LevelDBApp::Create(leveldb::mojom::LevelDBServiceRequest request) {
  if (!service_)
    service_.reset(new LevelDBServiceImpl(file_task_runner_));
  bindings_.AddBinding(service_.get(), std::move(request));
}

}  // namespace leveldb
