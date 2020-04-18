// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This has to be included first.
// See http://code.google.com/p/googletest/issues/detail?id=371
#include "testing/gtest/include/gtest/gtest.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <memory>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "media/base/test_data_util.h"
#include "media/filters/jpeg_parser.h"
#include "media/gpu/buildflags.h"
#include "media/gpu/test/video_accelerator_unittest_helpers.h"
#include "media/gpu/vaapi/vaapi_jpeg_encode_accelerator.h"
#include "media/video/jpeg_encode_accelerator.h"
#include "third_party/libyuv/include/libyuv.h"
#include "ui/gfx/codec/jpeg_codec.h"

#if BUILDFLAG(USE_VAAPI)
#include "media/gpu/vaapi/vaapi_wrapper.h"
#endif

namespace media {
namespace {

// Default test image file.
const base::FilePath::CharType kDefaultYuvFilename[] =
    FILE_PATH_LITERAL("bali_640x360_P420.yuv:640x360");
// Whether to save encode results to files. Output files will be saved
// in the same directory with unittest. File name is like input file but
// changing the extension to "jpg".
bool g_save_to_file = false;

const double kMeanDiffThreshold = 7.0;
const int kJpegDefaultQuality = 90;

// Environment to create test data for all test cases.
class JpegEncodeAcceleratorTestEnvironment;
JpegEncodeAcceleratorTestEnvironment* g_env;

struct TestImageFile {
  TestImageFile(const base::FilePath::StringType& filename,
                gfx::Size visible_size)
      : filename(filename), visible_size(visible_size) {}

  base::FilePath::StringType filename;

  // The input content of |filename|.
  std::string data_str;

  gfx::Size visible_size;
  size_t output_size;
};

enum class ClientState {
  CREATED,
  INITIALIZED,
  ENCODE_PASS,
  ERROR,
};

class JpegEncodeAcceleratorTestEnvironment : public ::testing::Environment {
 public:
  JpegEncodeAcceleratorTestEnvironment(
      const base::FilePath::CharType* yuv_filenames,
      const base::FilePath log_path,
      const int repeat)
      : repeat_(repeat), log_path_(log_path) {
    user_yuv_files_ = yuv_filenames ? yuv_filenames : kDefaultYuvFilename;
  }
  void SetUp() override;
  void TearDown() override;

  void LogToFile(const std::string& key, const std::string& value);

  // Read image from |filename| to |image_data|.
  void ReadTestYuvImage(base::FilePath& filename, TestImageFile* image_data);

  // Returns a file path for a file in what name specified or media/test/data
  // directory.  If the original file path is existed, returns it first.
  base::FilePath GetOriginalOrTestDataFilePath(const std::string& name);

  // Parsed data from command line.
  std::vector<std::unique_ptr<TestImageFile>> image_data_user_;

  // Parsed data of |test_1280x720_yuv_file_|.
  std::unique_ptr<TestImageFile> image_data_1280x720_white_;
  // Parsed data of |test_640x368_yuv_file_|.
  std::unique_ptr<TestImageFile> image_data_640x368_black_;
  // Parsed data of |test_640x360_yuv_file_|.
  std::unique_ptr<TestImageFile> image_data_640x360_black_;

  // Number of times SimpleEncodeTest should repeat for an image.
  const size_t repeat_;

 private:
  // Create black or white test image with |width| and |height| size.
  void CreateTestYuvImage(int width,
                          int height,
                          bool is_black,
                          base::FilePath* filename);

  const base::FilePath::CharType* user_yuv_files_;
  const base::FilePath log_path_;
  std::unique_ptr<base::File> log_file_;

  // Programatically generated YUV files.
  base::FilePath test_1280x720_yuv_file_;
  base::FilePath test_640x368_yuv_file_;
  base::FilePath test_640x360_yuv_file_;
};

void JpegEncodeAcceleratorTestEnvironment::SetUp() {
  if (!log_path_.empty()) {
    log_file_.reset(new base::File(
        log_path_, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE));
    LOG_ASSERT(log_file_->IsValid());
  }

  CreateTestYuvImage(1280, 720, false, &test_1280x720_yuv_file_);
  CreateTestYuvImage(640, 368, true, &test_640x368_yuv_file_);
  CreateTestYuvImage(640, 360, true, &test_640x360_yuv_file_);

  image_data_1280x720_white_.reset(
      new TestImageFile(test_1280x720_yuv_file_.value(), gfx::Size(1280, 720)));
  ASSERT_NO_FATAL_FAILURE(ReadTestYuvImage(test_1280x720_yuv_file_,
                                           image_data_1280x720_white_.get()));

  image_data_640x368_black_.reset(
      new TestImageFile(test_640x368_yuv_file_.value(), gfx::Size(640, 368)));
  ASSERT_NO_FATAL_FAILURE(ReadTestYuvImage(test_640x368_yuv_file_,
                                           image_data_640x368_black_.get()));

  image_data_640x360_black_.reset(
      new TestImageFile(test_640x360_yuv_file_.value(), gfx::Size(640, 360)));
  ASSERT_NO_FATAL_FAILURE(ReadTestYuvImage(test_640x360_yuv_file_,
                                           image_data_640x360_black_.get()));

  // |user_yuv_files_| may include many files and use ';' as delimiter.
  std::vector<base::FilePath::StringType> files =
      base::SplitString(user_yuv_files_, base::FilePath::StringType(1, ';'),
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  for (const auto& file : files) {
    std::vector<base::FilePath::StringType> filename_and_size =
        base::SplitString(file, base::FilePath::StringType(1, ':'),
                          base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    ASSERT_EQ(2u, filename_and_size.size());
    base::FilePath::StringType filename(filename_and_size[0]);

    std::vector<base::FilePath::StringType> image_resolution =
        base::SplitString(filename_and_size[1],
                          base::FilePath::StringType(1, 'x'),
                          base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    ASSERT_EQ(2u, image_resolution.size());
    int width = 0, height = 0;
    ASSERT_TRUE(base::StringToInt(image_resolution[0], &width));
    ASSERT_TRUE(base::StringToInt(image_resolution[1], &height));

    gfx::Size image_size(width, height);
    ASSERT_TRUE(!image_size.IsEmpty());

    base::FilePath input_file = GetOriginalOrTestDataFilePath(filename);
    auto image_data = std::make_unique<TestImageFile>(filename, image_size);
    ASSERT_NO_FATAL_FAILURE(ReadTestYuvImage(input_file, image_data.get()));
    image_data_user_.push_back(std::move(image_data));
  }
}

void JpegEncodeAcceleratorTestEnvironment::TearDown() {
  log_file_.reset();

  base::DeleteFile(test_1280x720_yuv_file_, false);
  base::DeleteFile(test_640x368_yuv_file_, false);
  base::DeleteFile(test_640x360_yuv_file_, false);
}

void JpegEncodeAcceleratorTestEnvironment::LogToFile(const std::string& key,
                                                     const std::string& value) {
  std::string s = base::StringPrintf("%s: %s\n", key.c_str(), value.c_str());
  LOG(INFO) << s;
  if (log_file_) {
    log_file_->WriteAtCurrentPos(s.data(), static_cast<int>(s.length()));
  }
}

void JpegEncodeAcceleratorTestEnvironment::CreateTestYuvImage(
    int width,
    int height,
    bool is_black,
    base::FilePath* filename) {
  std::vector<uint8_t> buffer(width * height * 3 / 2);

  size_t size = width * height;
  // Fill in Y values.
  memset(buffer.data(), is_black ? 0 : 255, size);
  // FIll in U and V values.
  memset(buffer.data() + size, 128, size / 2);
  LOG_ASSERT(base::CreateTemporaryFile(filename));
  EXPECT_TRUE(base::AppendToFile(
      *filename, reinterpret_cast<char*>(buffer.data()), buffer.size()));
}

void JpegEncodeAcceleratorTestEnvironment::ReadTestYuvImage(
    base::FilePath& input_file,
    TestImageFile* image_data) {
  ASSERT_TRUE(base::ReadFileToString(input_file, &image_data->data_str));

  // This is just a placeholder. We will compute the real output size when we
  // have encoder instance.
  image_data->output_size =
      VideoFrame::AllocationSize(PIXEL_FORMAT_I420, image_data->visible_size);
}

base::FilePath
JpegEncodeAcceleratorTestEnvironment::GetOriginalOrTestDataFilePath(
    const std::string& name) {
  base::FilePath original_file_path = base::FilePath(name);
  base::FilePath return_file_path = GetTestDataFilePath(name);

  if (PathExists(original_file_path))
    return_file_path = original_file_path;

  VLOG(3) << "Use file path " << return_file_path.value();
  return return_file_path;
}

class JpegClient : public JpegEncodeAccelerator::Client {
 public:
  JpegClient(const std::vector<TestImageFile*>& test_image_files,
             ClientStateNotification<ClientState>* note);
  ~JpegClient() override;
  void CreateJpegEncoder();
  void DestroyJpegEncoder();
  void StartEncode(int32_t bitstream_buffer_id);

  // JpegEncodeAccelerator::Client implementation.
  void VideoFrameReady(int32_t buffer_id, size_t encoded_picture_size) override;
  void NotifyError(int32_t buffer_id,
                   JpegEncodeAccelerator::Status status) override;

 private:
  void PrepareMemory(int32_t bitstream_buffer_id);
  void SetState(ClientState new_state);
  void SaveToFile(TestImageFile* image_file, size_t hw_size, size_t sw_size);
  bool CompareHardwareAndSoftwareResults(int width,
                                         int height,
                                         size_t hw_encoded_size,
                                         size_t sw_encoded_size);

  // Calculate mean absolute difference of hardware and software encode results
  // for verifying the similarity.
  double GetMeanAbsoluteDifference(uint8_t* hw_yuv_result,
                                   uint8_t* sw_yuv_result,
                                   size_t yuv_size);

  // Generate software encode result and populate it into |sw_out_shm_|.
  bool GetSoftwareEncodeResult(int width,
                               int height,
                               size_t* sw_encoded_size,
                               base::TimeDelta* sw_encode_time);

  // JpegClient doesn't own |test_image_files_|.
  const std::vector<TestImageFile*>& test_image_files_;

  // A map that stores HW encoding start timestamp for each output buffer id.
  std::map<int, base::TimeTicks> buffer_id_to_start_time_;

  std::unique_ptr<JpegEncodeAccelerator> encoder_;
  ClientState state_;

  // Used to notify another thread about the state. JpegClient does not own
  // this.
  ClientStateNotification<ClientState>* note_;

  // Output buffer prepared for JpegEncodeAccelerator.
  std::unique_ptr<BitstreamBuffer> encoded_buffer_;

  // Mapped memory of input file.
  std::unique_ptr<base::SharedMemory> in_shm_;
  // Mapped memory of output buffer from hardware encoder.
  std::unique_ptr<base::SharedMemory> hw_out_shm_;
  // Mapped memory of output buffer from software encoder.
  std::unique_ptr<base::SharedMemory> sw_out_shm_;

  DISALLOW_COPY_AND_ASSIGN(JpegClient);
};

JpegClient::JpegClient(const std::vector<TestImageFile*>& test_image_files,
                       ClientStateNotification<ClientState>* note)
    : test_image_files_(test_image_files),
      state_(ClientState::CREATED),
      note_(note) {}

JpegClient::~JpegClient() {}

void JpegClient::CreateJpegEncoder() {
  encoder_ = nullptr;

#if BUILDFLAG(USE_VAAPI)
  encoder_ = std::make_unique<VaapiJpegEncodeAccelerator>(
      base::ThreadTaskRunnerHandle::Get());
#endif

  if (!encoder_) {
    LOG(ERROR) << "Failed to create JpegEncodeAccelerator.";
    SetState(ClientState::ERROR);
    return;
  }

  JpegEncodeAccelerator::Status status = encoder_->Initialize(this);
  if (status != JpegEncodeAccelerator::ENCODE_OK) {
    LOG(ERROR) << "JpegEncodeAccelerator::Initialize() failed: " << status;
    SetState(ClientState::ERROR);
    return;
  }
  SetState(ClientState::INITIALIZED);
}

void JpegClient::DestroyJpegEncoder() {
  encoder_.reset();
}

void JpegClient::VideoFrameReady(int32_t buffer_id, size_t hw_encoded_size) {
  base::TimeTicks hw_encode_end = base::TimeTicks::Now();
  base::TimeDelta elapsed_hw =
      hw_encode_end - buffer_id_to_start_time_[buffer_id];

  TestImageFile* test_image = test_image_files_[buffer_id];
  size_t sw_encoded_size = 0;
  base::TimeDelta elapsed_sw;
  LOG_ASSERT(GetSoftwareEncodeResult(test_image->visible_size.width(),
                                     test_image->visible_size.height(),
                                     &sw_encoded_size, &elapsed_sw));

  g_env->LogToFile("hw_encode_time",
                   base::Int64ToString(elapsed_hw.InMicroseconds()));
  g_env->LogToFile("sw_encode_time",
                   base::Int64ToString(elapsed_sw.InMicroseconds()));

  if (g_save_to_file) {
    SaveToFile(test_image, hw_encoded_size, sw_encoded_size);
  }

  if (!CompareHardwareAndSoftwareResults(test_image->visible_size.width(),
                                         test_image->visible_size.height(),
                                         hw_encoded_size, sw_encoded_size)) {
    SetState(ClientState::ERROR);
  } else {
    SetState(ClientState::ENCODE_PASS);
  }

  encoded_buffer_.reset(nullptr);
}

bool JpegClient::GetSoftwareEncodeResult(int width,
                                         int height,
                                         size_t* sw_encoded_size,
                                         base::TimeDelta* sw_encode_time) {
  base::TimeTicks sw_encode_start = base::TimeTicks::Now();
  int y_stride = width;
  int u_stride = width / 2;
  int v_stride = u_stride;
  uint8_t* yuv_src = static_cast<uint8_t*>(in_shm_->memory());
  const int kBytesPerPixel = 4;
  std::vector<uint8_t> rgba_buffer(width * height * kBytesPerPixel);
  std::vector<uint8_t> encoded;
  libyuv::I420ToABGR(yuv_src, y_stride, yuv_src + y_stride * height, u_stride,
                     yuv_src + y_stride * height + u_stride * height / 2,
                     v_stride, rgba_buffer.data(), width * kBytesPerPixel,
                     width, height);

  SkImageInfo info = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType,
                                       kOpaque_SkAlphaType);
  SkPixmap src(info, &rgba_buffer[0], width * kBytesPerPixel);
  if (!gfx::JPEGCodec::Encode(src, kJpegDefaultQuality, &encoded)) {
    return false;
  }

  memcpy(sw_out_shm_->memory(), encoded.data(), encoded.size());
  *sw_encoded_size = encoded.size();
  *sw_encode_time = base::TimeTicks::Now() - sw_encode_start;
  return true;
}

bool JpegClient::CompareHardwareAndSoftwareResults(int width,
                                                   int height,
                                                   size_t hw_encoded_size,
                                                   size_t sw_encoded_size) {
  size_t yuv_size = width * height * 3 / 2;
  uint8_t* hw_yuv_result = new uint8_t[yuv_size];
  int y_stride = width;
  int u_stride = width / 2;
  int v_stride = u_stride;
  if (libyuv::ConvertToI420(
          static_cast<const uint8*>(hw_out_shm_->memory()), hw_encoded_size,
          hw_yuv_result, y_stride, hw_yuv_result + y_stride * height, u_stride,
          hw_yuv_result + y_stride * height + u_stride * height / 2, v_stride,
          0, 0, width, height, width, height, libyuv::kRotate0,
          libyuv::FOURCC_MJPG)) {
    LOG(ERROR) << "Convert HW encoded result to YUV failed";
  }

  uint8_t* sw_yuv_result = new uint8_t[yuv_size];
  if (libyuv::ConvertToI420(
          static_cast<const uint8*>(sw_out_shm_->memory()), sw_encoded_size,
          sw_yuv_result, y_stride, sw_yuv_result + y_stride * height, u_stride,
          sw_yuv_result + y_stride * height + u_stride * height / 2, v_stride,
          0, 0, width, height, width, height, libyuv::kRotate0,
          libyuv::FOURCC_MJPG)) {
    LOG(ERROR) << "Convert SW encoded result to YUV failed";
  }

  double difference =
      GetMeanAbsoluteDifference(hw_yuv_result, sw_yuv_result, yuv_size);
  delete[] hw_yuv_result;
  delete[] sw_yuv_result;

  if (difference > kMeanDiffThreshold) {
    LOG(ERROR) << "HW and SW encode results are not similar enough. diff = "
               << difference;
    return false;
  } else {
    return true;
  }
}

double JpegClient::GetMeanAbsoluteDifference(uint8_t* hw_yuv_result,
                                             uint8_t* sw_yuv_result,
                                             size_t yuv_size) {
  double total_difference = 0;
  for (size_t i = 0; i < yuv_size; i++)
    total_difference += std::abs(hw_yuv_result[i] - sw_yuv_result[i]);
  return total_difference / yuv_size;
}

void JpegClient::NotifyError(int32_t buffer_id,
                             JpegEncodeAccelerator::Status status) {
  LOG(ERROR) << "Notifying of error " << status << " for output buffer id "
             << buffer_id;
  SetState(ClientState::ERROR);
  encoded_buffer_.reset(nullptr);
}

void JpegClient::PrepareMemory(int32_t bitstream_buffer_id) {
  TestImageFile* image_file = test_image_files_[bitstream_buffer_id];

  size_t input_size = image_file->data_str.size();
  if (!in_shm_.get() || input_size > in_shm_->mapped_size()) {
    in_shm_.reset(new base::SharedMemory);
    LOG_ASSERT(in_shm_->CreateAndMapAnonymous(input_size));
  }
  memcpy(in_shm_->memory(), image_file->data_str.data(), input_size);

  if (!hw_out_shm_.get() ||
      image_file->output_size > hw_out_shm_->mapped_size()) {
    hw_out_shm_.reset(new base::SharedMemory);
    LOG_ASSERT(hw_out_shm_->CreateAndMapAnonymous(image_file->output_size));
  }
  memset(hw_out_shm_->memory(), 0, image_file->output_size);

  if (!sw_out_shm_.get() ||
      image_file->output_size > sw_out_shm_->mapped_size()) {
    sw_out_shm_.reset(new base::SharedMemory);
    LOG_ASSERT(sw_out_shm_->CreateAndMapAnonymous(image_file->output_size));
  }
  memset(sw_out_shm_->memory(), 0, image_file->output_size);
}

void JpegClient::SetState(ClientState new_state) {
  DVLOG(2) << "Changing state "
           << static_cast<std::underlying_type<ClientState>::type>(state_)
           << "->"
           << static_cast<std::underlying_type<ClientState>::type>(new_state);
  note_->Notify(new_state);
  state_ = new_state;
}

void JpegClient::SaveToFile(TestImageFile* image_file,
                            size_t hw_size,
                            size_t sw_size) {
  DCHECK_NE(nullptr, image_file);

  base::FilePath in_filename(image_file->filename);
  base::FilePath out_filename = in_filename.ReplaceExtension(".jpg");
  ASSERT_EQ(
      static_cast<int>(hw_size),
      base::WriteFile(out_filename, static_cast<char*>(hw_out_shm_->memory()),
                      hw_size));

  ASSERT_EQ(
      static_cast<int>(sw_size),
      base::WriteFile(out_filename.InsertBeforeExtension("_sw"),
                      static_cast<char*>(sw_out_shm_->memory()), sw_size));
}

void JpegClient::StartEncode(int32_t bitstream_buffer_id) {
  DCHECK_LT(static_cast<size_t>(bitstream_buffer_id), test_image_files_.size());
  TestImageFile* image_file = test_image_files_[bitstream_buffer_id];

  image_file->output_size =
      encoder_->GetMaxCodedBufferSize(image_file->visible_size);
  PrepareMemory(bitstream_buffer_id);

  base::SharedMemoryHandle dup_handle;
  dup_handle = base::SharedMemory::DuplicateHandle(hw_out_shm_->handle());
  encoded_buffer_ = std::make_unique<BitstreamBuffer>(
      bitstream_buffer_id, dup_handle, image_file->output_size);
  scoped_refptr<VideoFrame> input_frame_ = VideoFrame::WrapExternalSharedMemory(
      PIXEL_FORMAT_I420, image_file->visible_size,
      gfx::Rect(image_file->visible_size), image_file->visible_size,
      static_cast<uint8_t*>(in_shm_->memory()), image_file->data_str.size(),
      in_shm_->handle(), 0, base::TimeDelta());

  LOG_ASSERT(input_frame_.get());

  buffer_id_to_start_time_[bitstream_buffer_id] = base::TimeTicks::Now();
  encoder_->Encode(input_frame_, kJpegDefaultQuality, nullptr,
                   *encoded_buffer_);
}

class JpegEncodeAcceleratorTest : public ::testing::Test {
 protected:
  JpegEncodeAcceleratorTest() {}

  void TestEncode(size_t num_concurrent_encoders);

  // This is needed to allow the usage of methods in post_task.h in
  // JpegEncodeAccelerator implementations.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // The elements of |test_image_files_| are owned by
  // JpegEncodeAcceleratorTestEnvironment.
  std::vector<TestImageFile*> test_image_files_;
  std::vector<ClientState> expected_status_;

 protected:
  DISALLOW_COPY_AND_ASSIGN(JpegEncodeAcceleratorTest);
};

void JpegEncodeAcceleratorTest::TestEncode(size_t num_concurrent_encoders) {
  LOG_ASSERT(test_image_files_.size() >= expected_status_.size());
  base::Thread encoder_thread("EncoderThread");
  ASSERT_TRUE(encoder_thread.Start());

  std::vector<std::unique_ptr<ClientStateNotification<ClientState>>> notes;
  std::vector<std::unique_ptr<JpegClient>> clients;

  for (size_t i = 0; i < num_concurrent_encoders; i++) {
    notes.push_back(std::make_unique<ClientStateNotification<ClientState>>());
    clients.push_back(
        std::make_unique<JpegClient>(test_image_files_, notes.back().get()));
    encoder_thread.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&JpegClient::CreateJpegEncoder,
                                  base::Unretained(clients.back().get())));
    ASSERT_EQ(notes[i]->Wait(), ClientState::INITIALIZED);
  }

  for (size_t index = 0; index < test_image_files_.size(); index++) {
    for (size_t i = 0; i < num_concurrent_encoders; i++) {
      encoder_thread.task_runner()->PostTask(
          FROM_HERE, base::BindOnce(&JpegClient::StartEncode,
                                    base::Unretained(clients[i].get()), index));
    }
    if (index < expected_status_.size()) {
      for (size_t i = 0; i < num_concurrent_encoders; i++) {
        ASSERT_EQ(notes[i]->Wait(), expected_status_[index]);
      }
    }
  }

  for (size_t i = 0; i < num_concurrent_encoders; i++) {
    encoder_thread.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&JpegClient::DestroyJpegEncoder,
                                  base::Unretained(clients[i].get())));
  }
  encoder_thread.Stop();
}

TEST_F(JpegEncodeAcceleratorTest, SimpleEncode) {
  for (size_t i = 0; i < g_env->repeat_; i++) {
    for (auto& image : g_env->image_data_user_) {
      test_image_files_.push_back(image.get());
      expected_status_.push_back(ClientState::ENCODE_PASS);
    }
  }
  TestEncode(1);
}

TEST_F(JpegEncodeAcceleratorTest, MultipleEncoders) {
  for (auto& image : g_env->image_data_user_) {
    test_image_files_.push_back(image.get());
    expected_status_.push_back(ClientState::ENCODE_PASS);
  }
  TestEncode(3);
}

TEST_F(JpegEncodeAcceleratorTest, ResolutionChange) {
  test_image_files_.push_back(g_env->image_data_640x368_black_.get());
  test_image_files_.push_back(g_env->image_data_1280x720_white_.get());
  test_image_files_.push_back(g_env->image_data_640x368_black_.get());
  for (size_t i = 0; i < test_image_files_.size(); i++)
    expected_status_.push_back(ClientState::ENCODE_PASS);
  TestEncode(1);
}

TEST_F(JpegEncodeAcceleratorTest, CodedSizeAlignment) {
  test_image_files_.push_back(g_env->image_data_640x360_black_.get());
  expected_status_.push_back(ClientState::ENCODE_PASS);
  TestEncode(1);
}

}  // namespace
}  // namespace media

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  base::CommandLine::Init(argc, argv);
  base::ShadowingAtExitManager at_exit_manager;

  // Needed to enable DVLOG through --vmodule.
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  LOG_ASSERT(logging::InitLogging(settings));

  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  DCHECK(cmd_line);

  const base::FilePath::CharType* yuv_filenames = nullptr;
  base::FilePath log_path;
  size_t repeat = 1;
  base::CommandLine::SwitchMap switches = cmd_line->GetSwitches();
  for (base::CommandLine::SwitchMap::const_iterator it = switches.begin();
       it != switches.end(); ++it) {
    // yuv_filenames can include one or many files and use ';' as delimiter.
    // For each file, it should follow the format "[filename]:[width]x[height]".
    // For example, "lake.yuv:4160x3120".
    if (it->first == "yuv_filenames") {
      yuv_filenames = it->second.c_str();
      continue;
    }
    if (it->first == "output_log") {
      log_path = base::FilePath(
          base::FilePath::StringType(it->second.begin(), it->second.end()));
      continue;
    }
    if (it->first == "repeat") {
      if (!base::StringToSizeT(it->second, &repeat)) {
        LOG(INFO) << "Can't parse parameter |repeat|: " << it->second;
        repeat = 1;
      }
      continue;
    }
    if (it->first == "save_to_file") {
      media::g_save_to_file = true;
      continue;
    }
    if (it->first == "v" || it->first == "vmodule")
      continue;
    if (it->first == "h" || it->first == "help")
      continue;
    LOG(ERROR) << "Unexpected switch: " << it->first << ":" << it->second;
    return -EINVAL;
  }
#if BUILDFLAG(USE_VAAPI)
  media::VaapiWrapper::PreSandboxInitialization();
#endif

  media::g_env = reinterpret_cast<media::JpegEncodeAcceleratorTestEnvironment*>(
      testing::AddGlobalTestEnvironment(
          new media::JpegEncodeAcceleratorTestEnvironment(yuv_filenames,
                                                          log_path, repeat)));

  return RUN_ALL_TESTS();
}
