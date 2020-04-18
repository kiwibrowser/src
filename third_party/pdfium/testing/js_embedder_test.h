// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_JS_EMBEDDER_TEST_H_
#define TESTING_JS_EMBEDDER_TEST_H_

#include <memory>

#include "fxjs/cfxjs_engine.h"
#include "testing/embedder_test.h"

class JSEmbedderTest : public EmbedderTest {
 public:
  JSEmbedderTest();
  ~JSEmbedderTest() override;

  void SetUp() override;
  void TearDown() override;

  v8::Isolate* isolate();
  v8::Local<v8::Context> GetV8Context();
  CFXJS_Engine* engine() { return m_Engine.get(); }

 private:
  std::unique_ptr<CFX_V8ArrayBufferAllocator> m_pArrayBufferAllocator;
  v8::Isolate* m_pIsolate = nullptr;
  std::unique_ptr<CFXJS_Engine> m_Engine;
};

#endif  // TESTING_JS_EMBEDDER_TEST_H_
