// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/windows/d3d11_cdm_proxy.h"

#include <d3d11.h>
#include <d3d11_1.h>
#include <initguid.h>

#include "base/bind.h"
#include "media/base/cdm_proxy_context.h"
#include "media/gpu/windows/d3d11_mocks.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Invoke;
using ::testing::Ne;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::WithArgs;
using ::testing::_;

namespace media {

namespace {

// Use this function to create a mock so that they are ref-counted correctly.
template <typename Interface>
Microsoft::WRL::ComPtr<Interface> CreateMock() {
  Interface* mock = new Interface();
  mock->AddRef();
  return mock;
}

// The values doesn't matter as long as this is consistently used thruout the
// test.
const CdmProxy::Protocol kTestProtocol =
    CdmProxy::Protocol::kIntelConvergedSecurityAndManageabilityEngine;
const CdmProxy::Function kTestFunction =
    CdmProxy::Function::kIntelNegotiateCryptoSessionKeyExchange;
const uint32_t kTestFunctionId = 123;
// clang-format off
DEFINE_GUID(CRYPTO_TYPE_GUID,
            0x01020304, 0xffee, 0xefba,
            0x93, 0xaa, 0x47, 0x77, 0x43, 0xb1, 0x22, 0x98);
// clang-format on

}  // namespace

// Class for mocking D3D11CreateDevice() function.
class D3D11CreateDeviceMock {
 public:
  MOCK_METHOD10(Create, D3D11CdmProxy::CreateDeviceCB::RunType);
};

// Class for mocking the callbacks that get passed to the proxy methods.
class CallbackMock {
 public:
  MOCK_METHOD3(InitializeCallback, CdmProxy::InitializeCB::RunType);
  MOCK_METHOD2(ProcessCallback, CdmProxy::ProcessCB::RunType);
  MOCK_METHOD3(CreateMediaCryptoSessionCallback,
               CdmProxy::CreateMediaCryptoSessionCB::RunType);
};

class D3D11CdmProxyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::map<CdmProxy::Function, uint32_t> function_id_map;
    function_id_map[kTestFunction] = kTestFunctionId;

    proxy_ = std::make_unique<D3D11CdmProxy>(CRYPTO_TYPE_GUID, kTestProtocol,
                                             function_id_map);

    device_mock_ = CreateMock<D3D11DeviceMock>();
    video_device_mock_ = CreateMock<D3D11VideoDeviceMock>();
    video_device1_mock_ = CreateMock<D3D11VideoDevice1Mock>();
    crypto_session_mock_ = CreateMock<D3D11CryptoSessionMock>();
    device_context_mock_ = CreateMock<D3D11DeviceContextMock>();
    video_context_mock_ = CreateMock<D3D11VideoContextMock>();
    video_context1_mock_ = CreateMock<D3D11VideoContext1Mock>();

    proxy_->SetCreateDeviceCallbackForTesting(
        base::BindRepeating(&D3D11CreateDeviceMock::Create,
                            base::Unretained(&create_device_mock_)));
  }
  // Helper method to do Initialize(). Only useful if the test doesn't require
  // access to the mocks later.
  void Initialize(CdmProxy::InitializeCB callback) {
    EXPECT_CALL(create_device_mock_,
                Create(_, D3D_DRIVER_TYPE_HARDWARE, _, _, _, _, _, _, _, _))
        .WillOnce(DoAll(SetArgPointee<7>(device_mock_.Get()),
                        SetArgPointee<9>(device_context_mock_.Get()),
                        Return(S_OK)));

    EXPECT_CALL(*device_mock_.Get(), QueryInterface(IID_ID3D11VideoDevice, _))
        .Times(AtLeast(1))
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(video_device_mock_.Get()), Return(S_OK)));

    EXPECT_CALL(*device_mock_.Get(), QueryInterface(IID_ID3D11VideoDevice1, _))
        .Times(AtLeast(1))
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(video_device1_mock_.Get()), Return(S_OK)));

    EXPECT_CALL(*device_context_mock_.Get(),
                QueryInterface(IID_ID3D11VideoContext, _))
        .Times(AtLeast(1))
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(video_context_mock_.Get()), Return(S_OK)));

    EXPECT_CALL(*device_context_mock_.Get(),
                QueryInterface(IID_ID3D11VideoContext1, _))
        .Times(AtLeast(1))
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(video_context1_mock_.Get()), Return(S_OK)));

    EXPECT_CALL(
        *video_device_mock_.Get(),
        CreateCryptoSession(Pointee(CRYPTO_TYPE_GUID), _,
                            Pointee(D3D11_KEY_EXCHANGE_HW_PROTECTION), _))
        .WillOnce(
            DoAll(SetArgPointee<3>(crypto_session_mock_.Get()), Return(S_OK)));

    EXPECT_CALL(
        *video_device1_mock_.Get(),
        GetCryptoSessionPrivateDataSize(Pointee(CRYPTO_TYPE_GUID), _, _, _, _))
        .WillOnce(DoAll(SetArgPointee<3>(kPrivateInputSize),
                        SetArgPointee<4>(kPrivateOutputSize), Return(S_OK)));

    proxy_->Initialize(nullptr, std::move(callback));
    ::testing::Mock::VerifyAndClearExpectations(device_mock_.Get());
    ::testing::Mock::VerifyAndClearExpectations(video_device_mock_.Get());
    ::testing::Mock::VerifyAndClearExpectations(video_device1_mock_.Get());
    ::testing::Mock::VerifyAndClearExpectations(crypto_session_mock_.Get());
    ::testing::Mock::VerifyAndClearExpectations(device_context_mock_.Get());
    ::testing::Mock::VerifyAndClearExpectations(video_context_mock_.Get());
    ::testing::Mock::VerifyAndClearExpectations(video_context1_mock_.Get());
  }

  std::unique_ptr<D3D11CdmProxy> proxy_;
  D3D11CreateDeviceMock create_device_mock_;
  CallbackMock callback_mock_;

  Microsoft::WRL::ComPtr<D3D11DeviceMock> device_mock_;
  Microsoft::WRL::ComPtr<D3D11VideoDeviceMock> video_device_mock_;
  Microsoft::WRL::ComPtr<D3D11VideoDevice1Mock> video_device1_mock_;
  Microsoft::WRL::ComPtr<D3D11CryptoSessionMock> crypto_session_mock_;
  Microsoft::WRL::ComPtr<D3D11DeviceContextMock> device_context_mock_;
  Microsoft::WRL::ComPtr<D3D11VideoContextMock> video_context_mock_;
  Microsoft::WRL::ComPtr<D3D11VideoContext1Mock> video_context1_mock_;

  // These size values are arbitrary. Used for mocking
  // GetCryptoSessionPrivateDataSize().
  const UINT kPrivateInputSize = 10;
  const UINT kPrivateOutputSize = 40;
};

// Verifies that if device creation fails, then the call fails.
TEST_F(D3D11CdmProxyTest, FailedToCreateDevice) {
  EXPECT_CALL(create_device_mock_, Create(_, _, _, _, _, _, _, _, _, _))
      .WillOnce(Return(E_FAIL));
  EXPECT_CALL(callback_mock_,
              InitializeCallback(CdmProxy::Status::kFail, _, _));
  proxy_->Initialize(nullptr,
                     base::BindOnce(&CallbackMock::InitializeCallback,
                                    base::Unretained(&callback_mock_)));
}

// Initialize() success case.
TEST_F(D3D11CdmProxyTest, Initialize) {
  EXPECT_CALL(callback_mock_, InitializeCallback(CdmProxy::Status::kOk, _, _));
  ASSERT_NO_FATAL_FAILURE(Initialize(base::BindOnce(
      &CallbackMock::InitializeCallback, base::Unretained(&callback_mock_))));
}

// Verifies that Process() won't work if not initialized.
TEST_F(D3D11CdmProxyTest, ProcessUninitialized) {
  // The size nor value here matter, so making non empty non zero vector.
  const std::vector<uint8_t> kAnyInput(16, 0xFF);
  // Output size is also arbitrary, just has to match with the mock.
  const uint32_t kExpectedOutputDataSize = 20;
  EXPECT_CALL(callback_mock_, ProcessCallback(CdmProxy::Status::kFail, _));
  proxy_->Process(kTestFunction, 0, kAnyInput, kExpectedOutputDataSize,
                  base::BindOnce(&CallbackMock::ProcessCallback,
                                 base::Unretained(&callback_mock_)));
}

// Verifies that using a crypto session that is not reported will fail.
TEST_F(D3D11CdmProxyTest, ProcessInvalidCryptoSessionID) {
  uint32_t crypto_session_id = 0;
  EXPECT_CALL(callback_mock_, InitializeCallback(CdmProxy::Status::kOk, _, _))
      .WillOnce(SaveArg<2>(&crypto_session_id));
  ASSERT_NO_FATAL_FAILURE(Initialize(base::BindOnce(
      &CallbackMock::InitializeCallback, base::Unretained(&callback_mock_))));
  ::testing::Mock::VerifyAndClearExpectations(&callback_mock_);

  // The size nor value here matter, so making non empty non zero vector.
  const std::vector<uint8_t> kAnyInput(16, 0xFF);
  // Output size is also arbitrary, just has to match with the mock.
  const uint32_t kExpectedOutputDataSize = 20;
  EXPECT_CALL(callback_mock_, ProcessCallback(CdmProxy::Status::kFail, _));

  // Use a crypto session ID that hasn't been reported.
  proxy_->Process(kTestFunction, crypto_session_id + 1, kAnyInput,
                  kExpectedOutputDataSize,
                  base::BindOnce(&CallbackMock::ProcessCallback,
                                 base::Unretained(&callback_mock_)));
}

// Matcher for checking whether the structure passed to
// NegotiateCryptoSessionKeyExchange has the expected values.
MATCHER_P2(MatchesKeyExchangeStructure, expected, input_struct_size, "") {
  D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA* actual =
      static_cast<D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA*>(arg);
  if (expected->HWProtectionFunctionID != actual->HWProtectionFunctionID) {
    *result_listener << "function IDs mismatch. Expected "
                     << expected->HWProtectionFunctionID << " actual "
                     << actual->HWProtectionFunctionID;
    return false;
  }
  D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA* expected_input_data =
      expected->pInputData;
  D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA* actual_input_data =
      actual->pInputData;
  if (memcmp(expected_input_data, actual_input_data, input_struct_size) != 0) {
    *result_listener
        << "D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA don't match.";
    return false;
  }
  D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA* expected_output_data =
      expected->pOutputData;
  D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA* actual_output_data =
      actual->pOutputData;
  // Don't check that pbOutput field. It's filled by the callee.
  if (expected_output_data->PrivateDataSize !=
      actual_output_data->PrivateDataSize) {
    *result_listener << "D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA::"
                        "PrivateDataSize don't match. Expected "
                     << expected_output_data->PrivateDataSize << " actual "
                     << actual_output_data->PrivateDataSize;
    return false;
  }
  if (expected_output_data->HWProtectionDataSize !=
      actual_output_data->HWProtectionDataSize) {
    *result_listener << "D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA::"
                        "HWProtectionDataSize don't match. Expected "
                     << expected_output_data->HWProtectionDataSize << " actual "
                     << actual_output_data->HWProtectionDataSize;
    return false;
  }
  if (expected_output_data->TransportTime !=
      actual_output_data->TransportTime) {
    *result_listener << "D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA::"
                        "TransportTime don't match. Expected "
                     << expected_output_data->TransportTime << " actual "
                     << actual_output_data->TransportTime;
    return false;
  }
  if (expected_output_data->ExecutionTime !=
      actual_output_data->ExecutionTime) {
    *result_listener << "D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA::"
                        "ExecutionTime don't match. Expected "
                     << expected_output_data->ExecutionTime << " actual "
                     << actual_output_data->ExecutionTime;
    return false;
  }
  if (expected_output_data->MaxHWProtectionDataSize !=
      actual_output_data->MaxHWProtectionDataSize) {
    *result_listener << "D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA::"
                        "MaxHWProtectionDataSize don't match. Expected "
                     << expected_output_data->MaxHWProtectionDataSize
                     << " actual "
                     << actual_output_data->MaxHWProtectionDataSize;
    return false;
  }
  return true;
}

// Verifies that Process() works.
TEST_F(D3D11CdmProxyTest, Process) {
  uint32_t crypto_session_id = 0;
  EXPECT_CALL(callback_mock_,
              InitializeCallback(CdmProxy::Status::kOk, kTestProtocol, _))
      .WillOnce(SaveArg<2>(&crypto_session_id));
  ASSERT_NO_FATAL_FAILURE(Initialize(base::BindOnce(
      &CallbackMock::InitializeCallback, base::Unretained(&callback_mock_))));
  ::testing::Mock::VerifyAndClearExpectations(&callback_mock_);

  // The size nor value here matter, so making non empty non zero vector.
  const std::vector<uint8_t> kAnyInput(16, 0xFF);
  // Output size is also arbitrary, just has to match with the mock.
  const uint32_t kExpectedOutputDataSize = 20;

  const uint32_t input_structure_size =
      sizeof(D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA) - 4 +
      kAnyInput.size();
  const uint32_t output_structure_size =
      sizeof(D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA) - 4 +
      kExpectedOutputDataSize;
  std::unique_ptr<uint8_t[]> input_data_raw(new uint8_t[input_structure_size]);
  std::unique_ptr<uint8_t[]> output_data_raw(
      new uint8_t[output_structure_size]);

  D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA* input_data =
      reinterpret_cast<D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA*>(
          input_data_raw.get());
  D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA* output_data =
      reinterpret_cast<D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA*>(
          output_data_raw.get());

  D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA expected_key_exchange_data = {};
  expected_key_exchange_data.HWProtectionFunctionID = kTestFunctionId;
  expected_key_exchange_data.pInputData = input_data;
  expected_key_exchange_data.pOutputData = output_data;
  input_data->PrivateDataSize = kPrivateInputSize;
  input_data->HWProtectionDataSize = 0;
  memcpy(input_data->pbInput, kAnyInput.data(), kAnyInput.size());

  output_data->PrivateDataSize = kPrivateOutputSize;
  output_data->HWProtectionDataSize = 0;
  output_data->TransportTime = 0;
  output_data->ExecutionTime = 0;
  output_data->MaxHWProtectionDataSize = kExpectedOutputDataSize;

  // The value does not matter, so making non zero vector.
  std::vector<uint8_t> test_output_data(kExpectedOutputDataSize, 0xAA);
  EXPECT_CALL(callback_mock_, ProcessCallback(CdmProxy::Status::kOk, _));

  auto set_test_output_data = [&test_output_data](void* output) {
    D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA* kex_struct =
        static_cast<D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA*>(output);
    memcpy(kex_struct->pOutputData->pbOutput, test_output_data.data(),
           test_output_data.size());
  };

  EXPECT_CALL(*video_context_mock_.Get(),
              NegotiateCryptoSessionKeyExchange(
                  _, sizeof(expected_key_exchange_data),
                  MatchesKeyExchangeStructure(&expected_key_exchange_data,
                                              input_structure_size)))
      .WillOnce(DoAll(WithArgs<2>(Invoke(set_test_output_data)), Return(S_OK)));

  proxy_->Process(kTestFunction, crypto_session_id, kAnyInput,
                  kExpectedOutputDataSize,
                  base::BindOnce(&CallbackMock::ProcessCallback,
                                 base::Unretained(&callback_mock_)));
}

TEST_F(D3D11CdmProxyTest, CreateMediaCryptoSessionUninitialized) {
  // The size nor value here matter, so making non empty non zero vector.
  const std::vector<uint8_t> kAnyInput(16, 0xFF);
  EXPECT_CALL(callback_mock_,
              CreateMediaCryptoSessionCallback(CdmProxy::Status::kFail, _, _));
  proxy_->CreateMediaCryptoSession(
      kAnyInput, base::BindOnce(&CallbackMock::CreateMediaCryptoSessionCallback,
                                base::Unretained(&callback_mock_)));
}

// Tests the case where no extra data is specified. This is a success case.
TEST_F(D3D11CdmProxyTest, CreateMediaCryptoSessionNoExtraData) {
  uint32_t crypto_session_id_from_initialize = 0;
  EXPECT_CALL(callback_mock_,
              InitializeCallback(CdmProxy::Status::kOk, kTestProtocol, _))
      .WillOnce(SaveArg<2>(&crypto_session_id_from_initialize));
  ASSERT_NO_FATAL_FAILURE(Initialize(base::BindOnce(
      &CallbackMock::InitializeCallback, base::Unretained(&callback_mock_))));
  ::testing::Mock::VerifyAndClearExpectations(&callback_mock_);

  // Expect a new crypto session.
  EXPECT_CALL(callback_mock_, CreateMediaCryptoSessionCallback(
                                  CdmProxy::Status::kOk,
                                  Ne(crypto_session_id_from_initialize), _));
  auto media_crypto_session_mock = CreateMock<D3D11CryptoSessionMock>();
  EXPECT_CALL(*video_device_mock_.Get(),
              CreateCryptoSession(Pointee(CRYPTO_TYPE_GUID), _,
                                  Pointee(CRYPTO_TYPE_GUID), _))
      .WillOnce(DoAll(SetArgPointee<3>(media_crypto_session_mock.Get()),
                      Return(S_OK)));

  EXPECT_CALL(*video_context1_mock_.Get(), GetDataForNewHardwareKey(_, _, _, _))
      .Times(0);

  EXPECT_CALL(*video_context1_mock_.Get(),
              CheckCryptoSessionStatus(media_crypto_session_mock.Get(), _))
      .WillOnce(DoAll(SetArgPointee<1>(D3D11_CRYPTO_SESSION_STATUS_OK),
                      Return(S_OK)));
  proxy_->CreateMediaCryptoSession(
      std::vector<uint8_t>(),
      base::BindOnce(&CallbackMock::CreateMediaCryptoSessionCallback,
                     base::Unretained(&callback_mock_)));
}

// |arg| is void*. This casts the pointer to uint8_t* and checks whether they
// match.
MATCHER_P(CastedToUint8Are, expected, "") {
  const uint8_t* actual = static_cast<const uint8_t*>(arg);
  for (size_t i = 0; i < expected.size(); ++i) {
    if (actual[i] != expected[i]) {
      *result_listener << "Mismatch at element " << i;
      return false;
    }
  }
  return true;
}

// Verifies that extra data is used when creating a media crypto session.
TEST_F(D3D11CdmProxyTest, CreateMediaCryptoSessionWithExtraData) {
  uint32_t crypto_session_id_from_initialize = 0;
  EXPECT_CALL(callback_mock_,
              InitializeCallback(CdmProxy::Status::kOk, kTestProtocol, _))
      .WillOnce(SaveArg<2>(&crypto_session_id_from_initialize));
  ASSERT_NO_FATAL_FAILURE(Initialize(base::BindOnce(
      &CallbackMock::InitializeCallback, base::Unretained(&callback_mock_))));
  ::testing::Mock::VerifyAndClearExpectations(&callback_mock_);

  // Expect a new crypto session.
  EXPECT_CALL(callback_mock_, CreateMediaCryptoSessionCallback(
                                  CdmProxy::Status::kOk,
                                  Ne(crypto_session_id_from_initialize), _));

  auto media_crypto_session_mock = CreateMock<D3D11CryptoSessionMock>();
  EXPECT_CALL(*video_device_mock_.Get(),
              CreateCryptoSession(Pointee(CRYPTO_TYPE_GUID), _,
                                  Pointee(CRYPTO_TYPE_GUID), _))
      .WillOnce(DoAll(SetArgPointee<3>(media_crypto_session_mock.Get()),
                      Return(S_OK)));
  // The size nor value here matter, so making non empty non zero vector.
  const std::vector<uint8_t> kAnyInput(16, 0xFF);
  const uint64_t kAnyOutputData = 23298u;
  EXPECT_CALL(*video_context1_mock_.Get(),
              GetDataForNewHardwareKey(media_crypto_session_mock.Get(),
                                       kAnyInput.size(),
                                       CastedToUint8Are(kAnyInput), _))
      .WillOnce(DoAll(SetArgPointee<3>(kAnyOutputData), Return(S_OK)));

  EXPECT_CALL(*video_context1_mock_.Get(),
              CheckCryptoSessionStatus(media_crypto_session_mock.Get(), _))
      .WillOnce(DoAll(SetArgPointee<1>(D3D11_CRYPTO_SESSION_STATUS_OK),
                      Return(S_OK)));
  proxy_->CreateMediaCryptoSession(
      kAnyInput, base::BindOnce(&CallbackMock::CreateMediaCryptoSessionCallback,
                                base::Unretained(&callback_mock_)));
}

// Verify that GetCdmContext() is implemented and does not return null.
TEST_F(D3D11CdmProxyTest, GetCdmContext) {
  base::WeakPtr<CdmContext> context = proxy_->GetCdmContext();
  ASSERT_TRUE(context);
}

TEST_F(D3D11CdmProxyTest, GetCdmProxyContext) {
  base::WeakPtr<CdmContext> context = proxy_->GetCdmContext();
  ASSERT_TRUE(context);
  ASSERT_TRUE(context->GetCdmProxyContext());
}

TEST_F(D3D11CdmProxyTest, GetD3D11DecryptContextNoKey) {
  base::WeakPtr<CdmContext> context = proxy_->GetCdmContext();
  ASSERT_TRUE(context);
  CdmProxyContext* proxy_context = context->GetCdmProxyContext();
  // The key ID doesn't matter.
  auto decrypt_context = proxy_context->GetD3D11DecryptContext("");
  EXPECT_FALSE(decrypt_context);
}

// Verifies that keys are set and is acccessible with a getter.
TEST_F(D3D11CdmProxyTest, SetKeyAndGetDecryptContext) {
  base::WeakPtr<CdmContext> context = proxy_->GetCdmContext();
  ASSERT_TRUE(context);
  CdmProxyContext* proxy_context = context->GetCdmProxyContext();

  uint32_t crypto_session_id_from_initialize = 0;
  EXPECT_CALL(callback_mock_,
              InitializeCallback(CdmProxy::Status::kOk, kTestProtocol, _))
      .WillOnce(SaveArg<2>(&crypto_session_id_from_initialize));
  ASSERT_NO_FATAL_FAILURE(Initialize(base::BindOnce(
      &CallbackMock::InitializeCallback, base::Unretained(&callback_mock_))));
  ::testing::Mock::VerifyAndClearExpectations(&callback_mock_);

  std::vector<uint8_t> kKeyId = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  };
  std::vector<uint8_t> kKeyBlob = {
      0xab, 0x01, 0x20, 0xd3, 0xee, 0x05, 0x99, 0x87,
      0xff, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x7F,
  };
  proxy_->SetKey(crypto_session_id_from_initialize, kKeyId, kKeyBlob);

  std::string key_id_str(kKeyId.begin(), kKeyId.end());
  auto decrypt_context = proxy_context->GetD3D11DecryptContext(key_id_str);
  ASSERT_TRUE(decrypt_context);

  EXPECT_TRUE(decrypt_context->crypto_session)
      << "Crypto session should not be null.";
  const uint8_t* key_blob =
      reinterpret_cast<const uint8_t*>(decrypt_context->key_blob);
  EXPECT_EQ(kKeyBlob, std::vector<uint8_t>(
                          key_blob, key_blob + decrypt_context->key_blob_size));
  EXPECT_EQ(CRYPTO_TYPE_GUID, decrypt_context->key_info_guid);
}

// Verify that removing a key works.
TEST_F(D3D11CdmProxyTest, RemoveKey) {
  base::WeakPtr<CdmContext> context = proxy_->GetCdmContext();
  ASSERT_TRUE(context);
  CdmProxyContext* proxy_context = context->GetCdmProxyContext();

  uint32_t crypto_session_id_from_initialize = 0;
  EXPECT_CALL(callback_mock_,
              InitializeCallback(CdmProxy::Status::kOk, kTestProtocol, _))
      .WillOnce(SaveArg<2>(&crypto_session_id_from_initialize));
  ASSERT_NO_FATAL_FAILURE(Initialize(base::BindOnce(
      &CallbackMock::InitializeCallback, base::Unretained(&callback_mock_))));
  ::testing::Mock::VerifyAndClearExpectations(&callback_mock_);

  std::vector<uint8_t> kKeyId = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  };
  std::vector<uint8_t> kKeyBlob = {
      0xab, 0x01, 0x20, 0xd3, 0xee, 0x05, 0x99, 0x87,
      0xff, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x7F,
  };
  proxy_->SetKey(crypto_session_id_from_initialize, kKeyId, kKeyBlob);
  proxy_->RemoveKey(crypto_session_id_from_initialize, kKeyId);

  std::string keyblob_str(kKeyId.begin(), kKeyId.end());
  auto decrypt_context = proxy_context->GetD3D11DecryptContext(keyblob_str);
  EXPECT_FALSE(decrypt_context);
}

// Calling SetKey() and RemoveKey() for non-existent crypto session should
// not crash.
TEST_F(D3D11CdmProxyTest, SetRemoveKeyWrongCryptoSessionId) {
  const uint32_t kAnyCryptoSessionId = 0x9238;
  const std::vector<uint8_t> kEmpty;
  proxy_->RemoveKey(kAnyCryptoSessionId, kEmpty);
  proxy_->SetKey(kAnyCryptoSessionId, kEmpty, kEmpty);
}

TEST_F(D3D11CdmProxyTest, ProxyInvalidationInvalidatesCdmContext) {
  base::WeakPtr<CdmContext> context = proxy_->GetCdmContext();
  EXPECT_TRUE(context);
  proxy_.reset();
  EXPECT_FALSE(context);
}

}  // namespace media
