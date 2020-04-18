// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/xfa_js_embedder_test.h"

#include <string>

#include "fpdfsdk/cpdfsdk_helpers.h"
#include "fpdfsdk/fpdfxfa/cpdfxfa_context.h"
#include "fxjs/cfxjse_engine.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/base/ptr_util.h"
#include "xfa/fxfa/cxfa_ffapp.h"

XFAJSEmbedderTest::XFAJSEmbedderTest()
    : array_buffer_allocator_(
          pdfium::MakeUnique<CFX_V8ArrayBufferAllocator>()) {}

XFAJSEmbedderTest::~XFAJSEmbedderTest() {}

void XFAJSEmbedderTest::SetUp() {
  v8::Isolate::CreateParams params;
  params.array_buffer_allocator = array_buffer_allocator_.get();
  isolate_ = v8::Isolate::New(params);
  ASSERT_TRUE(isolate_);

  EmbedderTest::SetExternalIsolate(isolate_);
  EmbedderTest::SetUp();

  CXFA_FFApp::SkipFontLoadForTesting(true);
}

void XFAJSEmbedderTest::TearDown() {
  CXFA_FFApp::SkipFontLoadForTesting(false);

  value_.reset();
  script_context_ = nullptr;

  EmbedderTest::TearDown();

  isolate_->Dispose();
  isolate_ = nullptr;
}

CXFA_Document* XFAJSEmbedderTest::GetXFADocument() {
  auto* pDoc = CPDFDocumentFromFPDFDocument(document());
  if (!pDoc)
    return nullptr;

  auto* pContext = static_cast<CPDFXFA_Context*>(pDoc->GetExtension());
  if (!pContext)
    return nullptr;

  return pContext->GetXFADoc()->GetXFADoc();
}

bool XFAJSEmbedderTest::OpenDocumentWithOptions(
    const std::string& filename,
    const char* password,
    LinearizeOption linearize_option,
    JavaScriptOption javascript_option) {
  // JS required for XFA.
  ASSERT(javascript_option == JavaScriptOption::kEnableJavaScript);
  if (!EmbedderTest::OpenDocumentWithOptions(
          filename, password, linearize_option, javascript_option)) {
    return false;
  }
  script_context_ = GetXFADocument()->GetScriptContext();
  return true;
}

bool XFAJSEmbedderTest::Execute(const ByteStringView& input) {
  if (ExecuteHelper(input))
    return true;

  CFXJSE_Value msg(GetIsolate());
  value_->GetObjectPropertyByIdx(1, &msg);
  fprintf(stderr, "FormCalc: %.*s\n", static_cast<int>(input.GetLength()),
          input.unterminated_c_str());
  // If the parsing of the input fails, then v8 will not run, so there will be
  // no value here to print.
  if (msg.IsString() && !msg.ToWideString().IsEmpty())
    fprintf(stderr, "JS ERROR: %ls\n", msg.ToWideString().c_str());
  return false;
}

bool XFAJSEmbedderTest::ExecuteSilenceFailure(const ByteStringView& input) {
  return ExecuteHelper(input);
}

bool XFAJSEmbedderTest::ExecuteHelper(const ByteStringView& input) {
  value_ = pdfium::MakeUnique<CFXJSE_Value>(GetIsolate());
  return script_context_->RunScript(CXFA_Script::Type::Formcalc,
                                    WideString::FromUTF8(input).AsStringView(),
                                    value_.get(), GetXFADocument()->GetRoot());
}
