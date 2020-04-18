// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_SCREENSHOT_TESTER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_SCREENSHOT_TESTER_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace base {
class RunLoop;
}

namespace chromeos {

// A class that allows taking, saving and comparing screnshots while
// running tests.
class ScreenshotTester {
 public:
  ScreenshotTester();
  virtual ~ScreenshotTester();

  typedef scoped_refptr<base::RefCountedMemory> PNGFile;

  // Contains the results of comparison
  struct Result {
    Result();
    Result(const Result& other);
    ~Result();

    // Is true if screenshots are considered to be equal, false otherwise.
    bool screenshots_match;

    // How similar are images from 0 to 1.
    double similarity;

    // Image representing difference. Might not be filled during to specifics of
    // some comparison methods.
    PNGFile diff_image;
  };

  // Returns true if the screenshots should be taken and will be taken,
  // false otherwise. Also gets all the information from the command line
  // swithes.
  bool TryInitialize();

  // Does all the work that has been stated through switches:
  // updates golden screenshot or takes a new screenshot and compares it
  // with the golden one. |test_name| is the name of the test from which
  // we run this method.
  void Run(const std::string& test_name);

  // Remembers that area |area| should be ignored during comparison.
  void IgnoreArea(const SkIRect& area);

  // Prints out how similar two images are.
  void LogSimilarity(double similarity, bool screenshots_match);

 private:
  // Categories of images that can be possible generated and used during this
  // test.
  enum ImageCategories {
    kGoldenScreenshot,
    kFailedScreenshot,
    kDifferenceImage
  };

  // When images are same less that |kPrecision| pixels, they are considered to
  // be different.
  const double kPrecision = 0.99;

  // When difference in any of the RGB colors in any of the pixels between two
  // images
  // exceeds |kMaxAllowedColorDifference|, images are considered to be
  // different.
  const int kMaxAllowedColorDifference = 32;

  // Returns full name of a .png file of a |category| type starting with
  // |file_name_prefix| prefix,
  // e.g. MyTest_golden_screenshot.png
  std::string GetImageFileName(const std::string& file_name_prefix,
                               ImageCategories category);

  // Returns full path for an image of this category.
  base::FilePath GetImageFilePath(const std::string& file_name_prefix,
                                  ImageCategories category);

  // Erases areas that should be ignored during comparison from the screenshot.
  void EraseIgnoredAreas(SkBitmap& bitmap);

  // Takes a screenshot and returns it.
  PNGFile TakeScreenshot();

  // Saves |png_data| as a new golden screenshot for test |test_name_|.
  void UpdateGoldenScreenshot(PNGFile png_data);

  // Saves an image |png_data|, assuming it is a .png file.
  // Returns true if image was saved successfully.
  bool SaveImage(const base::FilePath& image_path, PNGFile png_data);

  // Logs results of comparison accordint to information in |result|.
  void LogComparisonResults(const ScreenshotTester::Result& result);

  // Saves |png_data| as a current screenshot.
  void ReturnScreenshot(base::RunLoop* run_loop,
                        PNGFile* screenshot,
                        PNGFile png_data);

  // Loads golden screenshot from the disk, assuming it lies at |image_path|.
  // Fails if there is no such a file.
  PNGFile LoadGoldenScreenshot(base::FilePath image_path);

  // Converts .png file to a bitmap which is ready for comparison (erasing
  // ignored areas included).
  SkBitmap ProcessImageForComparison(const PNGFile& image);

  // Calls comparing two given screenshots with one of the comparators.
  // Returns a Result structure containing comparison results.
  Result CompareScreenshots(const PNGFile& model, const PNGFile& sample);

  // Compares images using PerceptualDiff, and returns a Result structure
  // without a diff image containing comparison results.
  Result CompareScreenshotsPerceptually(SkBitmap model_bitmap,
                                        SkBitmap sample_bitmap);

  // Compares images pixel-by-pixel with some features and returns a Result
  // structure containing comparison results.
  Result CompareScreenshotsRegularly(SkBitmap model_bitmap,
                                     SkBitmap sample_bitmap);

  // Path to the directory for golden screenshots.
  base::FilePath golden_screenshots_dir_;

  // Path to the directory where screenshots that failed comparing
  // and difference between them and golden ones will be stored.
  base::FilePath artifacts_dir_;

  // Is true when we're in test mode:
  // comparing golden screenshots and current ones.
  bool test_mode_;

  // Is true when switches specify that PerceptualDiff should
  // be used to compare images.
  bool pdiff_enabled_;

  // Is true when switches specify that artifacts should be saved somewhere.
  bool generate_artifacts_;

  // Vector which holds areas which the comparison ignores because
  // them being different is not a bug (e.g. time on the clock).
  std::vector<SkIRect> ignored_areas_;

  base::WeakPtrFactory<ScreenshotTester> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ScreenshotTester);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_SCREENSHOT_TESTER_H_
