// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/file_util/public/cpp/sandboxed_rar_analyzer.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/common/safe_browsing/archive_analyzer_results.h"
#include "chrome/services/file_util/public/mojom/constants.mojom.h"
#include "chrome/services/file_util/public/mojom/safe_archive_analyzer.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "services/service_manager/public/cpp/connector.h"

SandboxedRarAnalyzer::SandboxedRarAnalyzer(
    const base::FilePath& rar_file,
    const ResultCallback& callback,
    service_manager::Connector* connector)
    : file_path_(rar_file), callback_(callback), connector_(connector) {
  DCHECK(callback);
  DCHECK(!file_path_.value().empty());
}

void SandboxedRarAnalyzer::Start() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&SandboxedRarAnalyzer::PrepareFileToAnalyze, this));
}

SandboxedRarAnalyzer::~SandboxedRarAnalyzer() = default;

void SandboxedRarAnalyzer::AnalyzeFile() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!analyzer_ptr_);
  DCHECK(!file_path_.value().empty());

  connector_->BindInterface(chrome::mojom::kFileUtilServiceName,
                            mojo::MakeRequest(&analyzer_ptr_));
  analyzer_ptr_.set_connection_error_handler(base::BindOnce(
      &SandboxedRarAnalyzer::AnalyzeFileDone, base::Unretained(this),
      safe_browsing::ArchiveAnalyzerResults()));
  analyzer_ptr_->AnalyzeRarFile(
      file_path_, base::BindOnce(&SandboxedRarAnalyzer::AnalyzeFileDone, this));
}

void SandboxedRarAnalyzer::AnalyzeFileDone(
    const safe_browsing::ArchiveAnalyzerResults& results) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  analyzer_ptr_.reset();
  callback_.Run(results);
}

void SandboxedRarAnalyzer::PrepareFileToAnalyze() {
  base::File file(file_path_, base::File::FLAG_OPEN | base::File::FLAG_READ);

  if (!file.IsValid()) {
    // TODO(vakh): Add UMA metrics here to check how often this happens.
    ReportFileFailure();
    return;
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&SandboxedRarAnalyzer::AnalyzeFile, this));
}

void SandboxedRarAnalyzer::ReportFileFailure() {
  DCHECK(!analyzer_ptr_);

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(callback_, safe_browsing::ArchiveAnalyzerResults()));
}

std::string SandboxedRarAnalyzer::DebugString() const {
  return base::StringPrintf("path: %" PRIsFP "; analyzer_ptr_: %p",
                            file_path_.value().c_str(), analyzer_ptr_.get());
}

std::ostream& operator<<(std::ostream& os,
                         const SandboxedRarAnalyzer& rar_analyzer) {
  os << rar_analyzer.DebugString();
  return os;
}
