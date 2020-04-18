// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "media/base/cdm_config.h"
#include "media/base/mock_filters.h"
#include "media/base/test_helpers.h"
#include "media/mojo/buildflags.h"
#include "media/mojo/clients/mojo_decryptor.h"
#include "media/mojo/clients/mojo_demuxer_stream_impl.h"
#include "media/mojo/common/media_type_converters.h"
#include "media/mojo/interfaces/constants.mojom.h"
#include "media/mojo/interfaces/content_decryption_module.mojom.h"
#include "media/mojo/interfaces/decryptor.mojom.h"
#include "media/mojo/interfaces/interface_factory.mojom.h"
#include "media/mojo/interfaces/media_service.mojom.h"
#include "media/mojo/interfaces/renderer.mojom.h"
#include "media/mojo/services/media_interface_provider.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
#include "media/cdm/cdm_paths.h"  // nogncheck
#include "media/mojo/interfaces/cdm_proxy.mojom.h"
#endif

namespace media {

namespace {

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::NiceMock;
using testing::SaveArg;
using testing::StrictMock;
using testing::WithArg;

MATCHER_P(MatchesResult, success, "") {
  return arg->success == success;
}

#if BUILDFLAG(ENABLE_MOJO_CDM) && !defined(OS_ANDROID)
const char kClearKeyKeySystem[] = "org.w3.clearkey";
const char kInvalidKeySystem[] = "invalid.key.system";
#endif

const char kSecurityOrigin[] = "https://foo.com";

// Returns a trivial encrypted DecoderBuffer.
scoped_refptr<DecoderBuffer> CreateEncryptedBuffer() {
  scoped_refptr<DecoderBuffer> encrypted_buffer(new DecoderBuffer(100));
  encrypted_buffer->set_decrypt_config(
      DecryptConfig::CreateCencConfig("dummy_key_id", "0123456789ABCDEF", {}));
  return encrypted_buffer;
}

class MockCdmProxyClient : public mojom::CdmProxyClient {
 public:
  MockCdmProxyClient() = default;
  ~MockCdmProxyClient() override = default;

  // mojom::CdmProxyClient implementation.
  MOCK_METHOD0(NotifyHardwareReset, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockCdmProxyClient);
};

class MockRendererClient : public mojom::RendererClient {
 public:
  MockRendererClient() = default;
  ~MockRendererClient() override = default;

  // mojom::RendererClient implementation.
  MOCK_METHOD3(OnTimeUpdate,
               void(base::TimeDelta time,
                    base::TimeDelta max_time,
                    base::TimeTicks capture_time));
  MOCK_METHOD1(OnBufferingStateChange, void(BufferingState state));
  MOCK_METHOD0(OnEnded, void());
  MOCK_METHOD0(OnError, void());
  MOCK_METHOD1(OnVideoOpacityChange, void(bool opaque));
  MOCK_METHOD1(OnAudioConfigChange, void(const AudioDecoderConfig&));
  MOCK_METHOD1(OnVideoConfigChange, void(const VideoDecoderConfig&));
  MOCK_METHOD1(OnVideoNaturalSizeChange, void(const gfx::Size& size));
  MOCK_METHOD1(OnStatisticsUpdate,
               void(const media::PipelineStatistics& stats));
  MOCK_METHOD0(OnWaitingForDecryptionKey, void());
  MOCK_METHOD1(OnDurationChange, void(base::TimeDelta duration));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockRendererClient);
};

ACTION_P(QuitLoop, run_loop) {
  base::PostTask(FROM_HERE, run_loop->QuitClosure());
}

// Tests MediaService built into a standalone mojo service binary (see
// ServiceMain() in main.cc) where MediaService uses TestMojoMediaClient.
// TestMojoMediaClient supports CDM creation using DefaultCdmFactory (only
// supports Clear Key key system), and Renderer creation using
// DefaultRendererFactory that always create media::RendererImpl.
class MediaServiceTest : public service_manager::test::ServiceTest {
 public:
  MediaServiceTest()
      : ServiceTest("media_service_unittests"),
        cdm_proxy_client_binding_(&cdm_proxy_client_),
        renderer_client_binding_(&renderer_client_),
        video_stream_(DemuxerStream::VIDEO) {}
  ~MediaServiceTest() override = default;

  void SetUp() override {
    ServiceTest::SetUp();

    service_manager::mojom::InterfaceProviderPtr host_interfaces;
    auto provider = std::make_unique<MediaInterfaceProvider>(
        mojo::MakeRequest(&host_interfaces));

    connector()->BindInterface(mojom::kMediaServiceName, &media_service_);
    media_service_.set_connection_error_handler(
        base::BindRepeating(&MediaServiceTest::MediaServiceConnectionClosed,
                            base::Unretained(this)));
    media_service_->CreateInterfaceFactory(
        mojo::MakeRequest(&interface_factory_), std::move(host_interfaces));
  }

  MOCK_METHOD3(OnCdmInitialized,
               void(mojom::CdmPromiseResultPtr result,
                    int cdm_id,
                    mojom::DecryptorPtr decryptor));
  MOCK_METHOD0(OnCdmConnectionError, void());

  // Returns the CDM ID associated with the CDM.
  int InitializeCdm(const std::string& key_system, bool expected_result) {
    base::RunLoop run_loop;
    interface_factory_->CreateCdm(key_system, mojo::MakeRequest(&cdm_));
    cdm_.set_connection_error_handler(base::BindRepeating(
        &MediaServiceTest::OnCdmConnectionError, base::Unretained(this)));

    int cdm_id = CdmContext::kInvalidCdmId;

    // The last parameter mojom::DecryptorPtr is move-only and not supported by
    // DoAll. Hence use WithArg to only extract the "int cdm_id" out and then
    // call DoAll.
    EXPECT_CALL(*this, OnCdmInitialized(MatchesResult(expected_result), _, _))
        .WillOnce(WithArg<1>(DoAll(SaveArg<0>(&cdm_id), QuitLoop(&run_loop))));
    cdm_->Initialize(key_system, url::Origin::Create(GURL(kSecurityOrigin)),
                     CdmConfig(),
                     base::BindOnce(&MediaServiceTest::OnCdmInitialized,
                                    base::Unretained(this)));
    run_loop.Run();
    return cdm_id;
  }

  MOCK_METHOD4(OnCdmProxyInitialized,
               void(CdmProxy::Status status,
                    CdmProxy::Protocol protocol,
                    uint32_t crypto_session_id,
                    int cdm_id));

  // Returns the CDM ID associated with the CdmProxy.
  int InitializeCdmProxy(const std::string& cdm_guid) {
    base::RunLoop run_loop;
    interface_factory_->CreateCdmProxy(cdm_guid,
                                       mojo::MakeRequest(&cdm_proxy_));

    mojom::CdmProxyClientAssociatedPtrInfo client_ptr_info;
    cdm_proxy_client_binding_.Bind(mojo::MakeRequest(&client_ptr_info));
    int cdm_id = CdmContext::kInvalidCdmId;

    EXPECT_CALL(*this, OnCdmProxyInitialized(CdmProxy::Status::kOk, _, _, _))
        .WillOnce(DoAll(SaveArg<3>(&cdm_id), QuitLoop(&run_loop)));
    cdm_proxy_->Initialize(
        std::move(client_ptr_info),
        base::BindOnce(&MediaServiceTest::OnCdmProxyInitialized,
                       base::Unretained(this)));
    run_loop.Run();
    return cdm_id;
  }

  MOCK_METHOD2(OnDecrypted,
               void(Decryptor::Status, scoped_refptr<DecoderBuffer>));

  void CreateDecryptor(int cdm_id, bool expected_result) {
    base::RunLoop run_loop;
    mojom::DecryptorPtr decryptor_ptr;
    interface_factory_->CreateDecryptor(cdm_id,
                                        mojo::MakeRequest(&decryptor_ptr));
    MojoDecryptor mojo_decryptor(std::move(decryptor_ptr));

    // In the success case, there's no decryption key to decrypt the buffer so
    // we would expect no-key.
    auto expected_status =
        expected_result ? Decryptor::kNoKey : Decryptor::kError;

    EXPECT_CALL(*this, OnDecrypted(expected_status, _))
        .WillOnce(QuitLoop(&run_loop));
    mojo_decryptor.Decrypt(Decryptor::kVideo, CreateEncryptedBuffer(),
                           base::BindRepeating(&MediaServiceTest::OnDecrypted,
                                               base::Unretained(this)));
    run_loop.Run();
  }

  MOCK_METHOD1(OnRendererInitialized, void(bool));

  void InitializeRenderer(const VideoDecoderConfig& video_config,
                          bool expected_result) {
    base::RunLoop run_loop;
    interface_factory_->CreateRenderer(
        media::mojom::HostedRendererType::kDefault, std::string(),
        mojo::MakeRequest(&renderer_));

    video_stream_.set_video_decoder_config(video_config);

    mojom::DemuxerStreamPtrInfo video_stream_proxy_info;
    mojo_video_stream_.reset(new MojoDemuxerStreamImpl(
        &video_stream_, MakeRequest(&video_stream_proxy_info)));

    mojom::RendererClientAssociatedPtrInfo client_ptr_info;
    renderer_client_binding_.Bind(mojo::MakeRequest(&client_ptr_info));

    std::vector<mojom::DemuxerStreamPtrInfo> streams;
    streams.push_back(std::move(video_stream_proxy_info));

    EXPECT_CALL(*this, OnRendererInitialized(expected_result))
        .WillOnce(QuitLoop(&run_loop));
    renderer_->Initialize(
        std::move(client_ptr_info), std::move(streams), base::nullopt,
        base::nullopt,
        base::BindOnce(&MediaServiceTest::OnRendererInitialized,
                       base::Unretained(this)));
    run_loop.Run();
  }

  MOCK_METHOD0(MediaServiceConnectionClosed, void());

 protected:
  mojom::MediaServicePtr media_service_;
  mojom::InterfaceFactoryPtr interface_factory_;
  mojom::ContentDecryptionModulePtr cdm_;
  mojom::CdmProxyPtr cdm_proxy_;
  mojom::RendererPtr renderer_;

  NiceMock<MockCdmProxyClient> cdm_proxy_client_;
  mojo::AssociatedBinding<mojom::CdmProxyClient> cdm_proxy_client_binding_;

  NiceMock<MockRendererClient> renderer_client_;
  mojo::AssociatedBinding<mojom::RendererClient> renderer_client_binding_;

  StrictMock<MockDemuxerStream> video_stream_;
  std::unique_ptr<MojoDemuxerStreamImpl> mojo_video_stream_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaServiceTest);
};

}  // namespace

// Note: base::RunLoop::RunUntilIdle() does not work well in these tests because
// even when the loop is idle, we may still have pending events in the pipe.
// - If you have an InterfacePtr hosted by the service in the service process,
//   you can use InterfacePtr::FlushForTesting(). Note that this doesn't drain
//   the task runner in the test process and doesn't cover all negative cases.
// - If you expect a callback on an InterfacePtr call or connection error, use
//   base::RunLoop::Run() and QuitLoop().

// TODO(crbug.com/829233): Enable these tests on Android.
#if BUILDFLAG(ENABLE_MOJO_CDM) && !defined(OS_ANDROID)
TEST_F(MediaServiceTest, InitializeCdm_Success) {
  InitializeCdm(kClearKeyKeySystem, true);
}

TEST_F(MediaServiceTest, InitializeCdm_InvalidKeySystem) {
  InitializeCdm(kInvalidKeySystem, false);
}

TEST_F(MediaServiceTest, Decryptor_WithCdm) {
  int cdm_id = InitializeCdm(kClearKeyKeySystem, true);
  CreateDecryptor(cdm_id, true);
}
#endif  // BUILDFLAG(ENABLE_MOJO_CDM) && !defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_MOJO_RENDERER)
TEST_F(MediaServiceTest, InitializeRenderer) {
  InitializeRenderer(TestVideoConfig::Normal(), true);
}
#endif  // BUILDFLAG(ENABLE_MOJO_RENDERER)

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
TEST_F(MediaServiceTest, CdmProxy) {
  InitializeCdmProxy(kClearKeyCdmGuid);
}

TEST_F(MediaServiceTest, Decryptor_WithCdmProxy) {
  int cdm_id = InitializeCdmProxy(kClearKeyCdmGuid);
  CreateDecryptor(cdm_id, true);
}

TEST_F(MediaServiceTest, Decryptor_WrongCdmId) {
  int cdm_id = InitializeCdmProxy(kClearKeyCdmGuid);
  CreateDecryptor(cdm_id + 1, false);
}

TEST_F(MediaServiceTest, DeferredDestruction_CdmProxy) {
  InitializeCdmProxy(kClearKeyCdmGuid);

  // Disconnecting InterfaceFactory should not terminate the MediaService since
  // there is still a CdmProxy hosted.
  interface_factory_.reset();
  cdm_proxy_.FlushForTesting();

  // Disconnecting CdmProxy will now terminate the MediaService.
  base::RunLoop run_loop;
  EXPECT_CALL(*this, MediaServiceConnectionClosed())
      .WillOnce(QuitLoop(&run_loop));
  cdm_proxy_.reset();
  run_loop.Run();
}
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

TEST_F(MediaServiceTest, Decryptor_WithoutCdmOrCdmProxy) {
  // Creating decryptor without creating CDM or CdmProxy.
  CreateDecryptor(1, false);
}

TEST_F(MediaServiceTest, Lifetime_DestroyMediaService) {
  // Disconnecting |media_service_| doesn't terminate MediaService
  // since |interface_factory_| is still alive. This is ensured here since
  // MediaServiceConnectionClosed() is not called.
  EXPECT_CALL(*this, MediaServiceConnectionClosed()).Times(0);
  media_service_.reset();
  interface_factory_.FlushForTesting();
}

TEST_F(MediaServiceTest, Lifetime_DestroyInterfaceFactory) {
  // Disconnecting InterfaceFactory will now terminate the MediaService since
  // there's no media components hosted.
  base::RunLoop run_loop;
  EXPECT_CALL(*this, MediaServiceConnectionClosed())
      .WillOnce(QuitLoop(&run_loop));
  interface_factory_.reset();
  run_loop.Run();
}

#if (BUILDFLAG(ENABLE_MOJO_CDM) && !defined(OS_ANDROID)) || \
    BUILDFLAG(ENABLE_MOJO_RENDERER)
// MediaService stays alive as long as there are InterfaceFactory impls, which
// are then deferred destroyed until no media components (e.g. CDM or Renderer)
// are hosted.
TEST_F(MediaServiceTest, Lifetime) {
#if BUILDFLAG(ENABLE_MOJO_CDM) && !defined(OS_ANDROID)
  InitializeCdm(kClearKeyKeySystem, true);
#endif

#if BUILDFLAG(ENABLE_MOJO_RENDERER)
  InitializeRenderer(TestVideoConfig::Normal(), true);
#endif

  // Disconnecting CDM and Renderer services doesn't terminate MediaService
  // since |interface_factory_| is still alive.
  cdm_.reset();
  renderer_.reset();
  interface_factory_.FlushForTesting();

  // Disconnecting InterfaceFactory will now terminate the MediaService.
  base::RunLoop run_loop;
  EXPECT_CALL(*this, MediaServiceConnectionClosed())
      .WillOnce(QuitLoop(&run_loop));
  interface_factory_.reset();
  run_loop.Run();
}

TEST_F(MediaServiceTest, DeferredDestruction) {
#if BUILDFLAG(ENABLE_MOJO_CDM) && !defined(OS_ANDROID)
  InitializeCdm(kClearKeyKeySystem, true);
#endif

#if BUILDFLAG(ENABLE_MOJO_RENDERER)
  InitializeRenderer(TestVideoConfig::Normal(), true);
#endif

  ASSERT_TRUE(cdm_ || renderer_);

  // Disconnecting InterfaceFactory should not terminate the MediaService since
  // there are still media components (CDM or Renderer) hosted.
  interface_factory_.reset();
  if (cdm_)
    cdm_.FlushForTesting();
  else if (renderer_)
    renderer_.FlushForTesting();
  else
    NOTREACHED();

  // Disconnecting CDM and Renderer will now terminate the MediaService.
  base::RunLoop run_loop;
  EXPECT_CALL(*this, MediaServiceConnectionClosed())
      .WillOnce(QuitLoop(&run_loop));
  cdm_.reset();
  renderer_.reset();
  run_loop.Run();
}
#endif  // (BUILDFLAG(ENABLE_MOJO_CDM) && !defined(OS_ANDROID)) ||
        //  BUILDFLAG(ENABLE_MOJO_RENDERER)

}  // namespace media
