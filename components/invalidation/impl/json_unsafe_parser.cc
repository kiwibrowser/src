// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/json_unsafe_parser.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/json/json_parser.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"

namespace syncer {

void JsonUnsafeParser::Parse(const std::string& unsafe_json,
                             SuccessCallback success_callback,
                             ErrorCallback error_callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](const std::string& unsafe_json, SuccessCallback success_callback,
             ErrorCallback error_callback) {
            std::string error_msg;
            int error_line, error_column;
            std::unique_ptr<base::Value> value =
                base::JSONReader::ReadAndReturnError(
                    unsafe_json, base::JSON_ALLOW_TRAILING_COMMAS, nullptr,
                    &error_msg, &error_line, &error_column);
            if (value) {
              std::move(success_callback).Run(std::move(value));
            } else {
              std::move(error_callback)
                  .Run(base::StringPrintf("%s (%d:%d)", error_msg.c_str(),
                                          error_line, error_column));
            }
          },
          unsafe_json, std::move(success_callback), std::move(error_callback)));
}

}  // namespace syncer
