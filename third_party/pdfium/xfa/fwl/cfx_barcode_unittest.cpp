// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xfa/fwl/cfx_barcode.h"

#include <memory>
#include <string>
#include <utility>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "core/fxge/cfx_renderdevice.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/test_support.h"
#include "third_party/base/ptr_util.h"

class BarcodeTest : public testing::Test {
 public:
  void SetUp() override {
    BC_Library_Init();

    auto device = pdfium::MakeUnique<CFX_DefaultRenderDevice>();
    auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
    if (bitmap->Create(640, 480, FXDIB_Rgb32))
      bitmap_ = bitmap;
    ASSERT_TRUE(bitmap_);
    ASSERT_TRUE(device->Attach(bitmap_, false, nullptr, false));
    device_ = std::move(device);
  }

  void TearDown() override {
    bitmap_.Reset();
    device_.reset();
    barcode_.reset();
    BC_Library_Destroy();
  }

  CFX_Barcode* barcode() const { return barcode_.get(); }

  bool Create(BC_TYPE type) {
    barcode_ = CFX_Barcode::Create(type);
    if (!barcode_)
      return false;

    barcode_->SetModuleHeight(300);
    barcode_->SetModuleWidth(420);
    barcode_->SetHeight(298);
    barcode_->SetWidth(418);
    return true;
  }

  bool RenderDevice() {
    return barcode_->RenderDevice(device_.get(), &matrix_);
  }

  std::string BitmapChecksum() {
    return GenerateMD5Base16(bitmap_->GetBuffer(),
                             bitmap_->GetPitch() * bitmap_->GetHeight());
  }

 protected:
  CFX_Matrix matrix_;
  std::unique_ptr<CFX_Barcode> barcode_;
  std::unique_ptr<CFX_RenderDevice> device_;
  RetainPtr<CFX_DIBitmap> bitmap_;
};

TEST_F(BarcodeTest, Code39) {
  EXPECT_TRUE(Create(BC_CODE39));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("cd4cd3f36da38ff58d9f621827018903", BitmapChecksum());
}

TEST_F(BarcodeTest, CodaBar) {
  EXPECT_TRUE(Create(BC_CODABAR));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("481189dc4f86eddb8c42343c9b8ef1dd", BitmapChecksum());
}

TEST_F(BarcodeTest, Code128) {
  EXPECT_TRUE(Create(BC_CODE128));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("11b21c178a9fd866d8be196c2103b263", BitmapChecksum());
}

TEST_F(BarcodeTest, Code128_B) {
  EXPECT_TRUE(Create(BC_CODE128_B));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("11b21c178a9fd866d8be196c2103b263", BitmapChecksum());
}

TEST_F(BarcodeTest, Code128_C) {
  EXPECT_TRUE(Create(BC_CODE128_C));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("6284ec8503d5a948c9518108da33cdd3", BitmapChecksum());
}

TEST_F(BarcodeTest, Ean8) {
  EXPECT_TRUE(Create(BC_EAN8));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("22d85bcb02d48f48813f02a1cc9cfe8c", BitmapChecksum());
}

TEST_F(BarcodeTest, UPCA) {
  EXPECT_TRUE(Create(BC_UPCA));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("cce41fc30852744c44b3353059b568b4", BitmapChecksum());
}

TEST_F(BarcodeTest, Ean13) {
  EXPECT_TRUE(Create(BC_EAN13));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("187091ec1fd1830fc4d41d40a923d4fb", BitmapChecksum());
}

TEST_F(BarcodeTest, Pdf417) {
  EXPECT_TRUE(Create(BC_PDF417));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("2bdb9b39f20c5763da6a0d7c7b1f6933", BitmapChecksum());
}

TEST_F(BarcodeTest, DataMatrix) {
  EXPECT_TRUE(Create(BC_DATAMATRIX));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("5e5cd9a680b86fcd4ffd53ed36e3c980", BitmapChecksum());
}

TEST_F(BarcodeTest, QrCode) {
  EXPECT_TRUE(Create(BC_QR_CODE));
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  EXPECT_EQ("4751c6e0f67749fabe24f787128decee", BitmapChecksum());
}
