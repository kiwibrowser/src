// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "base/optional.h"
#include "base/path_service.h"
#include "pdf/document_loader.h"
#include "pdf/pdfium/pdfium_engine.h"
#include "pdf/url_loader_wrapper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::InSequence;
using testing::_;

namespace chrome_pdf {

namespace {

class TestDocumentLoader : public DocumentLoader {
 public:
  explicit TestDocumentLoader(Client* client) : client_(client) {
    base::FilePath pdf_path;
    CHECK(base::PathService::Get(base::DIR_SOURCE_ROOT, &pdf_path));
    pdf_path = pdf_path.Append(FILE_PATH_LITERAL("pdf"))
                   .Append(FILE_PATH_LITERAL("test"))
                   .Append(FILE_PATH_LITERAL("data"))
                   .Append(FILE_PATH_LITERAL("hello_world2.pdf"));
    CHECK(base::ReadFileToString(pdf_path, &pdf_data_));
  }
  ~TestDocumentLoader() override = default;

  // DocumentLoader:
  bool Init(std::unique_ptr<URLLoaderWrapper> loader,
            const std::string& url) override {
    NOTREACHED();
    return false;
  }

  bool GetBlock(uint32_t position, uint32_t size, void* buf) const override {
    if (!IsDataAvailable(position, size))
      return false;

    memcpy(buf, pdf_data_.data() + position, size);
    return true;
  }

  bool IsDataAvailable(uint32_t position, uint32_t size) const override {
    return position < pdf_data_.size() && size <= pdf_data_.size() &&
           position + size <= pdf_data_.size();
  }

  void RequestData(uint32_t position, uint32_t size) override {
    client_->OnDocumentComplete();
  }

  bool IsDocumentComplete() const override { return true; }

  uint32_t GetDocumentSize() const override { return pdf_data_.size(); }

  uint32_t BytesReceived() const override { return pdf_data_.size(); }

 private:
  Client* const client_;
  std::string pdf_data_;
};

std::unique_ptr<DocumentLoader> CreateTestDocumentLoader(
    DocumentLoader::Client* client) {
  return std::make_unique<TestDocumentLoader>(client);
}

class TestClient : public PDFEngine::Client {
 public:
  TestClient() = default;
  ~TestClient() override = default;

  // PDFEngine::Client:
  MOCK_METHOD2(NotifyNumberOfFindResultsChanged, void(int, bool));
  MOCK_METHOD1(NotifySelectedFindResultChanged, void((int)));

  bool Confirm(const std::string& message) override { return false; }

  std::string Prompt(const std::string& question,
                     const std::string& default_answer) override {
    return std::string();
  }

  std::string GetURL() override { return std::string(); }

  pp::URLLoader CreateURLLoader() override { return pp::URLLoader(); }

  std::vector<SearchStringResult> SearchString(const base::char16* string,
                                               const base::char16* term,
                                               bool case_sensitive) override {
    EXPECT_TRUE(case_sensitive);
    base::string16 haystack = base::string16(string);
    base::string16 needle = base::string16(term);

    std::vector<SearchStringResult> results;

    size_t pos = 0;
    while (1) {
      pos = haystack.find(needle, pos);
      if (pos == base::string16::npos)
        break;

      SearchStringResult result;
      result.length = needle.size();
      result.start_index = pos;
      results.push_back(result);
      pos += needle.size();
    }
    return results;
  }

  pp::Instance* GetPluginInstance() override { return nullptr; }

  bool IsPrintPreview() override { return false; }

  uint32_t GetBackgroundColor() override { return 0; }

  float GetToolbarHeightInScreenCoords() override { return 0; }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestClient);
};

}  // namespace

class FindTextTest : public testing::Test {
 public:
  FindTextTest() = default;
  ~FindTextTest() override = default;

 protected:
  void SetUp() override {
    InitializePDFium();
    PDFiumEngine::SetCreateDocumentLoaderFunctionForTesting(
        &CreateTestDocumentLoader);
  }
  void TearDown() override {
    PDFiumEngine::SetCreateDocumentLoaderFunctionForTesting(nullptr);
    FPDF_DestroyLibrary();
  }

  void InitializePDFium() {
    FPDF_LIBRARY_CONFIG config;
    config.version = 2;
    config.m_pUserFontPaths = nullptr;
    config.m_pIsolate = nullptr;
    config.m_v8EmbedderSlot = 0;
    FPDF_InitLibraryWithConfig(&config);
  }

  DISALLOW_COPY_AND_ASSIGN(FindTextTest);
};

TEST_F(FindTextTest, FindText) {
  pp::URLLoader dummy_loader;
  TestClient client;
  PDFiumEngine engine(&client, true);
  ASSERT_TRUE(engine.New("https://chromium.org/dummy.pdf", ""));
  ASSERT_TRUE(engine.HandleDocumentLoad(dummy_loader));

  {
    InSequence sequence;

    EXPECT_CALL(client, NotifyNumberOfFindResultsChanged(1, false));
    EXPECT_CALL(client, NotifySelectedFindResultChanged(0));
    for (int i = 1; i < 10; ++i)
      EXPECT_CALL(client, NotifyNumberOfFindResultsChanged(i + 1, false));
    EXPECT_CALL(client, NotifyNumberOfFindResultsChanged(10, true));
  }

  engine.StartFind("o", /*case_sensitive=*/true);
}

}  // namespace chrome_pdf
