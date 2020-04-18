// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/idltest/idltest_api.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/values.h"

namespace {

std::unique_ptr<base::Value> CopyBinaryValueToIntegerList(
    const base::Value::BlobStorage& input) {
  base::Value output(base::Value::Type::LIST);
  auto& list = output.GetList();
  list.reserve(input.size());
  for (int c : input)
    list.emplace_back(c);
  return base::Value::ToUniquePtrValue(std::move(output));
}

}  // namespace

ExtensionFunction::ResponseAction IdltestSendArrayBufferFunction::Run() {
  EXTENSION_FUNCTION_VALIDATE(args_ && !args_->GetList().empty());
  const auto& value = args_->GetList()[0];
  EXTENSION_FUNCTION_VALIDATE(value.is_blob());
  return RespondNow(OneArgument(CopyBinaryValueToIntegerList(value.GetBlob())));
}

ExtensionFunction::ResponseAction IdltestSendArrayBufferViewFunction::Run() {
  EXTENSION_FUNCTION_VALIDATE(args_ && !args_->GetList().empty());
  const auto& value = args_->GetList()[0];
  EXTENSION_FUNCTION_VALIDATE(value.is_blob());
  return RespondNow(OneArgument(CopyBinaryValueToIntegerList(value.GetBlob())));
}

ExtensionFunction::ResponseAction IdltestGetArrayBufferFunction::Run() {
  static constexpr char kHello[] = "hello world";
  return RespondNow(
      OneArgument(base::Value::CreateWithCopiedBuffer(kHello, strlen(kHello))));
}
