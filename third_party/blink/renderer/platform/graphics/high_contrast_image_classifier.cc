// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/high_contrast_image_classifier.h"

#include "base/rand_util.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/graphics/highcontrast/highcontrast_classifier.h"
#include "third_party/skia/include/utils/SkNullCanvas.h"

namespace {

bool IsColorGray(const SkColor& color) {
  return abs(static_cast<int>(SkColorGetR(color)) -
             static_cast<int>(SkColorGetG(color))) +
             abs(static_cast<int>(SkColorGetG(color)) -
                 static_cast<int>(SkColorGetB(color))) <=
         8;
}

void RGBAdd(const SkColor& new_pixel, SkColor4f* sink) {
  sink->fR += SkColorGetR(new_pixel);
  sink->fG += SkColorGetG(new_pixel);
  sink->fB += SkColorGetB(new_pixel);
}

void RGBDivide(SkColor4f* sink, int divisor) {
  sink->fR /= divisor;
  sink->fG /= divisor;
  sink->fB /= divisor;
}

float ColorDifference(const SkColor& color1, const SkColor4f& color2) {
  return sqrt((pow(static_cast<float>(SkColorGetR(color1)) - color2.fR, 2.0) +
               pow(static_cast<float>(SkColorGetG(color1)) - color2.fG, 2.0) +
               pow(static_cast<float>(SkColorGetB(color1)) - color2.fB, 2.0)) /
              3.0);
}

const int kPixelsToSample = 1000;
const int kBlocksCount1D = 10;
const int kMinImageSizeForClassification1D = 24;

// Decision tree lower and upper thresholds for grayscale and color images.
const float kLowColorCountThreshold[2] = {0.8125, 0.015137};
const float kHighColorCountThreshold[2] = {1, 0.025635};

}  // namespace

namespace blink {

HighContrastImageClassifier::HighContrastImageClassifier()
    : use_testing_random_generator_(false), testing_random_generator_seed_(0) {}

int HighContrastImageClassifier::GetRandomInt(const int min, const int max) {
  if (use_testing_random_generator_) {
    testing_random_generator_seed_ *= 7;
    testing_random_generator_seed_ += 15485863;
    testing_random_generator_seed_ %= 256205689;
    return min + testing_random_generator_seed_ % (max - min);
  }

  return base::RandInt(min, max - 1);
}

bool HighContrastImageClassifier::ShouldApplyHighContrastFilterToImage(
    Image& image) {
  HighContrastClassification result = image.GetHighContrastClassification();
  if (result != HighContrastClassification::kNotClassified)
    return result == HighContrastClassification::kApplyHighContrastFilter;

  if (image.width() < kMinImageSizeForClassification1D ||
      image.height() < kMinImageSizeForClassification1D) {
    result = HighContrastClassification::kApplyHighContrastFilter;
  } else {
    std::vector<float> features;
    if (!ComputeImageFeatures(image, &features))
      result = HighContrastClassification::kDoNotApplyHighContrastFilter;
    else
      result = ClassifyImage(features);
  }

  image.SetHighContrastClassification(result);
  return result == HighContrastClassification::kApplyHighContrastFilter;
}

// This function computes a single feature vector based on a sample set of image
// pixels. Please refer to |GetSamples| function for description of the sampling
// method, and |GetFeatures| function for description of the features.
bool HighContrastImageClassifier::ComputeImageFeatures(
    Image& image,
    std::vector<float>* features) {
  SkBitmap bitmap;
  if (!GetBitmap(image, &bitmap))
    return false;

  if (use_testing_random_generator_)
    testing_random_generator_seed_ = 0;

  std::vector<SkColor> sampled_pixels;
  float transparency_ratio;
  float background_ratio;
  GetSamples(bitmap, &sampled_pixels, &transparency_ratio, &background_ratio);

  GetFeatures(sampled_pixels, transparency_ratio, background_ratio, features);
  return true;
}

bool HighContrastImageClassifier::GetBitmap(Image& image, SkBitmap* bitmap) {
  if (!image.IsBitmapImage() || !image.width() || !image.height())
    return false;

  bitmap->allocPixels(
      SkImageInfo::MakeN32(image.width(), image.height(), kPremul_SkAlphaType));
  SkCanvas canvas(*bitmap);
  canvas.clear(SK_ColorTRANSPARENT);
  canvas.drawImageRect(image.PaintImageForCurrentFrame().GetSkImage(),
                       SkRect::MakeIWH(image.width(), image.height()), nullptr);
  return true;
}

// Extracts sample pixels from the image, focusing on the foreground and
// ignoring static background. The image is separated into uniformly distributed
// blocks through its width and height, each block is sampled, and checked to
// see if it seem to be background or foreground (See IsBlockBackground for
// details). Samples from foreground blocks are collected, and if they are less
// than 50% of requested samples, the foreground blocks are resampled to get
// more points.
void HighContrastImageClassifier::GetSamples(
    const SkBitmap& bitmap,
    std::vector<SkColor>* sampled_pixels,
    float* transparency_ratio,
    float* background_ratio) {
  int pixels_per_block = kPixelsToSample / (kBlocksCount1D * kBlocksCount1D);

  int transparent_pixels = 0;
  int opaque_pixels = 0;
  int blocks_count = 0;

  std::vector<int> horizontal_grid(kBlocksCount1D + 1);
  std::vector<int> vertical_grid(kBlocksCount1D + 1);
  for (int block = 0; block <= kBlocksCount1D; block++) {
    horizontal_grid[block] = static_cast<int>(
        round(block * bitmap.width() / static_cast<float>(kBlocksCount1D)));
    vertical_grid[block] = static_cast<int>(
        round(block * bitmap.height() / static_cast<float>(kBlocksCount1D)));
  }

  sampled_pixels->clear();
  std::vector<IntRect> foreground_blocks;

  for (int y = 0; y < kBlocksCount1D; y++) {
    for (int x = 0; x < kBlocksCount1D; x++) {
      IntRect block(horizontal_grid[x], vertical_grid[y],
                    horizontal_grid[x + 1] - horizontal_grid[x],
                    vertical_grid[y + 1] - vertical_grid[y]);

      std::vector<SkColor> block_samples;
      int block_transparent_pixels;
      GetBlockSamples(bitmap, block, pixels_per_block, &block_samples,
                      &block_transparent_pixels);
      opaque_pixels += static_cast<int>(block_samples.size());
      transparent_pixels += block_transparent_pixels;

      if (!IsBlockBackground(block_samples, block_transparent_pixels)) {
        sampled_pixels->insert(sampled_pixels->end(), block_samples.begin(),
                               block_samples.end());
        foreground_blocks.push_back(block);
      }
      blocks_count++;
    }
  }

  *transparency_ratio = static_cast<float>(transparent_pixels) /
                        (transparent_pixels + opaque_pixels);
  *background_ratio =
      1.0 - static_cast<float>(foreground_blocks.size()) / blocks_count;

  // If samples are too few, resample foreground blocks.
  if (sampled_pixels->size() < kPixelsToSample / 2 &&
      foreground_blocks.size()) {
    pixels_per_block = static_cast<int>(
        ceil((kPixelsToSample - static_cast<float>(sampled_pixels->size())) /
             static_cast<float>(foreground_blocks.size())));
    for (const IntRect& block : foreground_blocks) {
      std::vector<SkColor> block_samples;
      int unused;
      GetBlockSamples(bitmap, block, pixels_per_block, &block_samples, &unused);
      sampled_pixels->insert(sampled_pixels->end(), block_samples.begin(),
                             block_samples.end());
    }
  }
}

// Selects random samples from a block of the image. Returns the opaque sampled
// pixels, and the number of transparent sampled pixels.
void HighContrastImageClassifier::GetBlockSamples(
    const SkBitmap& bitmap,
    const IntRect& block,
    const int required_samples_count,
    std::vector<SkColor>* sampled_pixels,
    int* transparent_pixels_count) {
  *transparent_pixels_count = 0;

  int x1 = block.X();
  int y1 = block.Y();
  int x2 = block.MaxX();
  int y2 = block.MaxY();
  DCHECK(x1 < bitmap.width());
  DCHECK(y1 < bitmap.height());
  DCHECK(x2 <= bitmap.width());
  DCHECK(y2 <= bitmap.height());

  sampled_pixels->clear();
  for (int i = 0; i < required_samples_count; i++) {
    int x = GetRandomInt(x1, x2);
    int y = GetRandomInt(y1, y2);
    SkColor new_sample = bitmap.getColor(x, y);
    if (SkColorGetA(new_sample) < 128)
      (*transparent_pixels_count)++;
    else
      sampled_pixels->push_back(new_sample);
  }
}

// Given sampled pixels and transparent pixels count of a block of the image,
// decides if that block is background or not. If more than 80% of the block is
// transparent, or the divergence of colors in the block is less than 5%, the
// block is considered background.
bool HighContrastImageClassifier::IsBlockBackground(
    const std::vector<SkColor>& sampled_pixels,
    const int transparent_pixels) {
  if (static_cast<int>(sampled_pixels.size()) <= transparent_pixels / 4)
    return true;

  SkColor4f average;
  average.fR = 0;
  average.fG = 0;
  average.fB = 0;

  for (const SkColor& sample : sampled_pixels)
    RGBAdd(sample, &average);

  RGBDivide(&average, sampled_pixels.size());

  float sum = 0;
  for (const auto& sample : sampled_pixels)
    sum += ColorDifference(sample, average);
  sum /= 128;  // Normalize to 0..1 range.
  float divergence = sum / sampled_pixels.size();
  return divergence < 0.05;
}

// This function computes a single feature vector from a sample set of image
// pixels. The current features are:
// 0: 1 if color, 0 if grayscale.
// 1: Ratio of the number of bucketed colors used in the image to all
//    possiblities. Color buckets are represented with 4 bits per color channel.
// 2: Ratio of transparent area to the whole image.
// 3: Ratio of the background area to the whole image.
void HighContrastImageClassifier::GetFeatures(
    const std::vector<SkColor>& sampled_pixels,
    const float transparency_ratio,
    const float background_ratio,
    std::vector<float>* features) {
  int samples_count = static_cast<int>(sampled_pixels.size());

  // Is image grayscale.
  int color_pixels = 0;
  for (const SkColor& sample : sampled_pixels) {
    if (!IsColorGray(sample))
      color_pixels++;
  }
  ColorMode color_mode = (color_pixels > samples_count / 100)
                             ? ColorMode::kColor
                             : ColorMode::kGrayscale;

  features->resize(4);

  // Feature 0: Is Colorful?
  (*features)[0] = color_mode == ColorMode::kColor;

  // Feature 1: Color Buckets Ratio.
  (*features)[1] = ComputeColorBucketsRatio(sampled_pixels, color_mode);

  // Feature 2: Transparency Ratio
  (*features)[2] = transparency_ratio;

  // Feature 3: Background Ratio.
  (*features)[3] = background_ratio;
}

float HighContrastImageClassifier::ComputeColorBucketsRatio(
    const std::vector<SkColor>& sampled_pixels,
    const ColorMode color_mode) {
  std::set<unsigned> buckets;
  // If image is in color, use 4 bits per color channel, otherwise 4 bits for
  // illumination.
  if (color_mode == ColorMode::kColor) {
    for (const SkColor& sample : sampled_pixels) {
      unsigned bucket = ((SkColorGetR(sample) >> 4) << 8) +
                        ((SkColorGetG(sample) >> 4) << 4) +
                        ((SkColorGetB(sample) >> 4));
      buckets.insert(bucket);
    }
  } else {
    for (const SkColor& sample : sampled_pixels) {
      unsigned illumination =
          (SkColorGetR(sample) * 5 + SkColorGetG(sample) * 3 +
           SkColorGetB(sample) * 2) /
          10;
      buckets.insert(illumination / 16);
    }
  }

  // Using 4 bit per channel representation of each color bucket, there would be
  // 2^4 buckets for grayscale images and 2^12 for color images.
  const float max_buckets[] = {16, 4096};
  return static_cast<float>(buckets.size()) /
         max_buckets[color_mode == ColorMode::kColor];
}

HighContrastClassification
HighContrastImageClassifier::ClassifyImageUsingDecisionTree(
    const std::vector<float>& features) {
  DCHECK_EQ(features.size(), 4u);

  int is_color = features[0] > 0;
  float color_count_ratio = features[1];
  float low_color_count_threshold = kLowColorCountThreshold[is_color];
  float high_color_count_threshold = kHighColorCountThreshold[is_color];

  // Very few colors means it's not a photo, apply the filter.
  if (color_count_ratio < low_color_count_threshold)
    return HighContrastClassification::kApplyHighContrastFilter;

  // Too many colors means it's probably photorealistic, do not apply it.
  if (color_count_ratio > high_color_count_threshold)
    return HighContrastClassification::kDoNotApplyHighContrastFilter;

  // In-between, decision tree cannot give a precise result.
  return HighContrastClassification::kNotClassified;
}

HighContrastClassification HighContrastImageClassifier::ClassifyImage(
    const std::vector<float>& features) {
  DCHECK_EQ(features.size(), 4u);

  HighContrastClassification result = ClassifyImageUsingDecisionTree(features);

  // If decision tree cannot decide, we use a neural network to decide whether
  // to filter or not based on all the features.
  if (result == HighContrastClassification::kNotClassified) {
    highcontrast_tfnative_model::FixedAllocations nn_temp;
    float nn_out;
    highcontrast_tfnative_model::Inference(&features[0], &nn_out, &nn_temp);
    result = nn_out > 0
                 ? HighContrastClassification::kApplyHighContrastFilter
                 : HighContrastClassification::kDoNotApplyHighContrastFilter;
  }

  return result;
}

}  // namespace blink
