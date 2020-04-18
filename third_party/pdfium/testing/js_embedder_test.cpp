// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/js_embedder_test.h"

#include "third_party/base/ptr_util.h"

JSEmbedderTest::JSEmbedderTest()
    : m_pArrayBufferAllocator(
          pdfium::MakeUnique<CFX_V8ArrayBufferAllocator>()) {}

JSEmbedderTest::~JSEmbedderTest() {}

void JSEmbedderTest::SetUp() {
  v8::Isolate::CreateParams params;
  params.array_buffer_allocator = m_pArrayBufferAllocator.get();
  m_pIsolate = v8::Isolate::New(params);

  EmbedderTest::SetExternalIsolate(m_pIsolate);
  EmbedderTest::SetUp();

  v8::Isolate::Scope isolate_scope(m_pIsolate);
  v8::HandleScope handle_scope(m_pIsolate);
  FXJS_PerIsolateData::SetUp(m_pIsolate);
  m_Engine = pdfium::MakeUnique<CFXJS_Engine>(m_pIsolate);
  m_Engine->InitializeEngine();
}

void JSEmbedderTest::TearDown() {
  m_Engine->ReleaseEngine();
  m_Engine.reset();
  EmbedderTest::TearDown();
  m_pIsolate->Dispose();
  m_pIsolate = nullptr;
}

v8::Isolate* JSEmbedderTest::isolate() {
  return m_pIsolate;
}

v8::Local<v8::Context> JSEmbedderTest::GetV8Context() {
  return m_Engine->GetV8Context();
}
