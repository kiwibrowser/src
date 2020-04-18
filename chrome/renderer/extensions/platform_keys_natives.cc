// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/platform_keys_natives.h"

#include <memory>
#include <string>
#include <utility>

#include "base/values.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/renderer/script_context.h"
#include "third_party/blink/public/platform/web_crypto_algorithm.h"
#include "third_party/blink/public/platform/web_crypto_algorithm_params.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_crypto_normalize.h"

namespace extensions {

namespace {

bool StringToWebCryptoOperation(const std::string& str,
                                blink::WebCryptoOperation* op) {
  if (str == "GenerateKey") {
    *op = blink::kWebCryptoOperationGenerateKey;
    return true;
  }
  if (str == "ImportKey") {
    *op = blink::kWebCryptoOperationImportKey;
    return true;
  }
  if (str == "Sign") {
    *op = blink::kWebCryptoOperationSign;
    return true;
  }
  if (str == "Verify") {
    *op = blink::kWebCryptoOperationVerify;
    return true;
  }
  return false;
}

std::unique_ptr<base::DictionaryValue> WebCryptoAlgorithmToBaseValue(
    const blink::WebCryptoAlgorithm& algorithm) {
  DCHECK(!algorithm.IsNull());

  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  const blink::WebCryptoAlgorithmInfo* info =
      blink::WebCryptoAlgorithm::LookupAlgorithmInfo(algorithm.Id());
  dict->SetKey("name", base::Value(info->name));

  const blink::WebCryptoAlgorithm* hash = nullptr;

  const blink::WebCryptoRsaHashedKeyGenParams* rsaHashedKeyGen =
      algorithm.RsaHashedKeyGenParams();
  if (rsaHashedKeyGen) {
    dict->SetKey(
        "modulusLength",
        base::Value(static_cast<int>(rsaHashedKeyGen->ModulusLengthBits())));
    const blink::WebVector<unsigned char>& public_exponent =
        rsaHashedKeyGen->PublicExponent();
    dict->SetWithoutPathExpansion(
        "publicExponent",
        base::Value::CreateWithCopiedBuffer(
            reinterpret_cast<const char*>(public_exponent.Data()),
            public_exponent.size()));

    hash = &rsaHashedKeyGen->GetHash();
    DCHECK(!hash->IsNull());
  }

  const blink::WebCryptoRsaHashedImportParams* rsaHashedImport =
      algorithm.RsaHashedImportParams();
  if (rsaHashedImport) {
    hash = &rsaHashedImport->GetHash();
    DCHECK(!hash->IsNull());
  }

  if (hash) {
    const blink::WebCryptoAlgorithmInfo* hash_info =
        blink::WebCryptoAlgorithm::LookupAlgorithmInfo(hash->Id());

    std::unique_ptr<base::DictionaryValue> hash_dict(new base::DictionaryValue);
    hash_dict->SetKey("name", base::Value(hash_info->name));
    dict->SetWithoutPathExpansion("hash", std::move(hash_dict));
  }
  // Otherwise, |algorithm| is missing support here or no parameters were
  // required.
  return dict;
}

}  // namespace

PlatformKeysNatives::PlatformKeysNatives(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {}

void PlatformKeysNatives::AddRoutes() {
  RouteHandlerFunction("NormalizeAlgorithm",
                       base::Bind(&PlatformKeysNatives::NormalizeAlgorithm,
                                  base::Unretained(this)));
}

void PlatformKeysNatives::NormalizeAlgorithm(
    const v8::FunctionCallbackInfo<v8::Value>& call_info) {
  DCHECK_EQ(call_info.Length(), 2);
  DCHECK(call_info[0]->IsObject());
  DCHECK(call_info[1]->IsString());

  blink::WebCryptoOperation operation;
  if (!StringToWebCryptoOperation(
          *v8::String::Utf8Value(call_info.GetIsolate(), call_info[1]),
          &operation)) {
    return;
  }

  blink::WebString error_details;
  int exception_code = 0;

  blink::WebCryptoAlgorithm algorithm = blink::NormalizeCryptoAlgorithm(
      v8::Local<v8::Object>::Cast(call_info[0]), operation, &exception_code,
      &error_details, call_info.GetIsolate());

  std::unique_ptr<base::DictionaryValue> algorithm_dict;
  if (!algorithm.IsNull())
    algorithm_dict = WebCryptoAlgorithmToBaseValue(algorithm);

  if (!algorithm_dict)
    return;

  call_info.GetReturnValue().Set(content::V8ValueConverter::Create()->ToV8Value(
      algorithm_dict.get(), context()->v8_context()));
}

}  // namespace extensions
