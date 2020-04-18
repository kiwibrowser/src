// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screenshot_testing/screenshot_tester.h"

#include <stddef.h>
#include <stdint.h>

#include "ash/shell.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/login/screenshot_testing/SkDiffPixelsMetric.h"
#include "chrome/browser/chromeos/login/screenshot_testing/SkImageDiffer.h"
#include "chrome/browser/chromeos/login/screenshot_testing/SkPMetric.h"
#include "chromeos/chromeos_switches.h"
#include "content/public/browser/browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "ui/snapshot/snapshot.h"

namespace {

// Sets test mode for screenshot testing, using regular comparison.
const char kTestMode[] = "test";

// Sets update mode for screenshot testing.
const char kUpdateMode[] = "update";

// Sets test mode for screenshot testing, using PerceptualDiff as comparison.
const char kPdiffTestMode[] = "pdiff-test";

}  // namespace

namespace chromeos {

ScreenshotTester::ScreenshotTester()
    : test_mode_(false), pdiff_enabled_(false), weak_factory_(this) {}

ScreenshotTester::~ScreenshotTester() {}

ScreenshotTester::Result::Result() {}

ScreenshotTester::Result::Result(const Result& other) = default;

ScreenshotTester::Result::~Result() {}

bool ScreenshotTester::TryInitialize() {
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(switches::kEnableScreenshotTestingWithMode))
    return false;

  std::string mode = command_line.GetSwitchValueASCII(
      switches::kEnableScreenshotTestingWithMode);
  CHECK(mode == kUpdateMode || mode == kTestMode || mode == kPdiffTestMode)
      << "Invalid mode for screenshot testing: " << mode;

  CHECK(command_line.HasSwitch(chromeos::switches::kGoldenScreenshotsDir))
      << "No directory with golden screenshots specified, use "
         "--golden-screenshots-dir";

  golden_screenshots_dir_ =
      command_line.GetSwitchValuePath(switches::kGoldenScreenshotsDir);

  if (mode == kTestMode || mode == kPdiffTestMode) {
    test_mode_ = true;
    generate_artifacts_ = command_line.HasSwitch(switches::kArtifactsDir);
    if (generate_artifacts_) {
      artifacts_dir_ = command_line.GetSwitchValuePath(switches::kArtifactsDir);
    }
  }
  if (mode == kPdiffTestMode) {
    pdiff_enabled_ = true;
  }
  return true;
}

std::string ScreenshotTester::GetImageFileName(
    const std::string& file_name_prefix,
    ImageCategories category) {
  std::string file_name = file_name_prefix + "_";
  switch (category) {
    case kGoldenScreenshot: {
      file_name += "golden_screenshot";
      break;
    }
    case kFailedScreenshot: {
      file_name += "failed_screenshot";
      break;
    }
    case kDifferenceImage: {
      file_name += "difference";
      break;
    }
  }
  return file_name + ".png";
}

base::FilePath ScreenshotTester::GetImageFilePath(
    const std::string& file_name_prefix,
    ImageCategories category) {
  std::string file_name = GetImageFileName(file_name_prefix, category);
  base::FilePath file_path;
  if (category == kGoldenScreenshot) {
    file_path = golden_screenshots_dir_.AppendASCII(file_name);
  } else {
    file_path = artifacts_dir_.AppendASCII(file_name);
  }
  return file_path;
}

void ScreenshotTester::Run(const std::string& test_name) {
  PNGFile current_screenshot = TakeScreenshot();
  base::FilePath golden_screenshot_path =
      GetImageFilePath(test_name, kGoldenScreenshot);
  PNGFile golden_screenshot = LoadGoldenScreenshot(golden_screenshot_path);
  if (test_mode_) {
    CHECK(golden_screenshot.get())
        << "A golden screenshot is required for screenshot testing";
    VLOG(0) << "Loaded golden screenshot";
    Result result = CompareScreenshots(golden_screenshot, current_screenshot);
    VLOG(0) << "Compared";
    LogComparisonResults(result);
    if (!result.screenshots_match && generate_artifacts_) {
      // Saving diff imag
      if (!pdiff_enabled_) {
        base::FilePath difference_image_path =
            GetImageFilePath(test_name, kDifferenceImage);
        CHECK(SaveImage(difference_image_path, result.diff_image));
      }

      // Saving failed screenshot
      base::FilePath failed_screenshot_path =
          GetImageFilePath(test_name, kFailedScreenshot);
      CHECK(SaveImage(failed_screenshot_path, current_screenshot));
    }
    ASSERT_TRUE(result.screenshots_match);
  } else {
    bool golden_screenshot_needs_update;
    if (golden_screenshot.get()) {
      // There is a golden screenshot, so we need to check it first.
      Result result = CompareScreenshots(golden_screenshot, current_screenshot);
      golden_screenshot_needs_update = (!result.screenshots_match);
    } else {
      // There is no golden screenshot for this test at all.
      golden_screenshot_needs_update = true;
    }
    if (golden_screenshot_needs_update) {
      bool golden_screenshot_saved =
          SaveImage(golden_screenshot_path, current_screenshot);
      CHECK(golden_screenshot_saved);
    } else {
      VLOG(0) << "Golden screenshot does not differ from the current one, no "
                 "need to update";
    }
  }
}

void ScreenshotTester::IgnoreArea(const SkIRect& area) {
  ignored_areas_.push_back(area);
}

void ScreenshotTester::EraseIgnoredAreas(SkBitmap& bitmap) {
  for (std::vector<SkIRect>::iterator it = ignored_areas_.begin();
       it != ignored_areas_.end(); ++it) {
    bitmap.eraseArea((*it), SK_ColorWHITE);
  }
}

bool ScreenshotTester::SaveImage(const base::FilePath& image_path,
                                 PNGFile png_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!png_data.get()) {
    LOG(ERROR) << "There is no png data";
    return false;
  }
  if (!base::CreateDirectory(image_path.DirName())) {
    LOG(ERROR) << "Can't create directory" << image_path.DirName().value();
    return false;
  }
  if (static_cast<size_t>(base::WriteFile(
          image_path, reinterpret_cast<const char*>(png_data->front()),
          png_data->size())) != png_data->size()) {
    LOG(ERROR) << "Can't save screenshot " << image_path.BaseName().value()
               << ".";
    return false;
  }
  VLOG(0) << "Screenshot " << image_path.BaseName().value() << " saved to "
          << image_path.DirName().value() << ".";
  return true;
}

void ScreenshotTester::ReturnScreenshot(base::RunLoop* run_loop,
                                        PNGFile* screenshot,
                                        PNGFile png_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  *screenshot = png_data;
  run_loop->Quit();
}

ScreenshotTester::PNGFile ScreenshotTester::TakeScreenshot() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  aura::Window* primary_window = ash::Shell::GetPrimaryRootWindow();
  gfx::Rect rect = primary_window->bounds();
  PNGFile screenshot;
  base::RunLoop run_loop;
  ui::GrabWindowSnapshotAsyncPNG(
      primary_window, rect,
      base::Bind(&ScreenshotTester::ReturnScreenshot,
                 weak_factory_.GetWeakPtr(), base::Unretained(&run_loop),
                 &screenshot));
  run_loop.Run();
  return screenshot;
}

ScreenshotTester::PNGFile ScreenshotTester::LoadGoldenScreenshot(
    base::FilePath image_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!base::PathExists(image_path)) {
    LOG(WARNING) << "Can't find a golden screenshot for this test";
    return 0;
  }

  int64_t golden_screenshot_size;
  base::GetFileSize(image_path, &golden_screenshot_size);

  if (golden_screenshot_size == -1) {
    CHECK(false) << "Can't get golden screenshot size";
  }
  scoped_refptr<base::RefCountedBytes> png_data = new base::RefCountedBytes;
  png_data->data().resize(golden_screenshot_size);
  base::ReadFile(image_path, reinterpret_cast<char*>(&(png_data->data()[0])),
                 golden_screenshot_size);

  return png_data;
}

SkBitmap ScreenshotTester::ProcessImageForComparison(const PNGFile& image) {
  CHECK(image.get());
  SkBitmap current_bitmap;
  gfx::PNGCodec::Decode(image->front(), image->size(), &current_bitmap);
  EraseIgnoredAreas(current_bitmap);
  return current_bitmap;
}

ScreenshotTester::Result ScreenshotTester::CompareScreenshots(
    const PNGFile& model,
    const PNGFile& sample) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  SkBitmap model_bitmap;
  SkBitmap sample_bitmap;

  model_bitmap = ProcessImageForComparison(model);
  sample_bitmap = ProcessImageForComparison(sample);

  if (pdiff_enabled_) {
    return CompareScreenshotsPerceptually(model_bitmap, sample_bitmap);
  } else {
    return CompareScreenshotsRegularly(model_bitmap, sample_bitmap);
  }
}

void ScreenshotTester::LogSimilarity(double similarity,
                                     bool screenshots_match) {
  VLOG(0) << "Screenshots similarity: " << std::setprecision(5)
          << similarity * 100 << "\%";
  if (!pdiff_enabled_ && screenshots_match) {
    if (similarity == 1) {  // 100%
      VLOG(0) << "Screenshots match perfectly";
    } else {
      VLOG(0) << "Screenshots differ slightly, but it is still a match";
    }
  }
}

void ScreenshotTester::LogComparisonResults(
    const ScreenshotTester::Result& result) {
  std::string comparison_type = pdiff_enabled_ ? "PerceptualDiff" : "regular";
  if (result.screenshots_match) {
    VLOG(0) << "Screenshot testing passed using " << comparison_type
            << " comparison";
  } else {
    LOG(ERROR) << "Screenshot testing failed using " << comparison_type
               << " comparison";
    if (!pdiff_enabled_) {
      VLOG(0) << "(HINT): Result may be false negative. Try using "
                 "PerceptualDiff comparison (use pdiff-test mode instead of "
                 "test)";
    }
  }
  LogSimilarity(result.similarity, result.screenshots_match);
}

ScreenshotTester::Result ScreenshotTester::CompareScreenshotsRegularly(
    SkBitmap model_bitmap,
    SkBitmap sample_bitmap) {
  SkDifferentPixelsMetric differ;

  SkImageDiffer::BitmapsToCreate diff_parameters;
  diff_parameters.rgbDiff = true;
  SkImageDiffer::Result result;

  differ.diff(&model_bitmap, &sample_bitmap, diff_parameters, &result);

  Result testing_result;

  testing_result.screenshots_match =
      (result.result >= kPrecision &&
       result.maxRedDiff <= kMaxAllowedColorDifference &&
       result.maxGreenDiff <= kMaxAllowedColorDifference &&
       result.maxBlueDiff <= kMaxAllowedColorDifference);

  testing_result.similarity = result.result;

  scoped_refptr<base::RefCountedBytes> diff_image(new base::RefCountedBytes);
  diff_image->data().resize(result.rgbDiffBitmap.computeByteSize());
  CHECK(gfx::PNGCodec::EncodeBGRASkBitmap(result.rgbDiffBitmap, false,
                                          &diff_image->data()))
      << "Could not encode difference to PNG";
  testing_result.diff_image = diff_image;

  return testing_result;
}

ScreenshotTester::Result ScreenshotTester::CompareScreenshotsPerceptually(
    SkBitmap model_bitmap,
    SkBitmap sample_bitmap) {
  SkPMetric differ;
  SkImageDiffer::BitmapsToCreate diff_parameters;
  SkImageDiffer::Result result;

  differ.diff(&model_bitmap, &sample_bitmap, diff_parameters, &result);

  ScreenshotTester::Result testing_result;
  testing_result.similarity = result.result;
  testing_result.screenshots_match =
      (result.result == SkImageDiffer::RESULT_CORRECT);

  return testing_result;
}

}  // namespace chromeos
