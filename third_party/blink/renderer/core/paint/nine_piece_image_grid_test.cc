// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/nine_piece_image_grid.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/css/css_gradient_value.h"
#include "third_party/blink/renderer/core/style/nine_piece_image.h"
#include "third_party/blink/renderer/core/style/style_generated_image.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

namespace blink {
namespace {

class NinePieceImageGridTest : public RenderingTest {
 public:
  NinePieceImageGridTest() = default;

  StyleImage* GeneratedImage() {
    cssvalue::CSSGradientValue* gradient =
        cssvalue::CSSLinearGradientValue::Create(
            nullptr, nullptr, nullptr, nullptr, nullptr, cssvalue::kRepeating);
    return StyleGeneratedImage::Create(*gradient);
  }

 private:
  void SetUp() override { RenderingTest::SetUp(); }
};

TEST_F(NinePieceImageGridTest, NinePieceImagePainting_NoDrawables) {
  NinePieceImage nine_piece;
  nine_piece.SetImage(GeneratedImage());

  IntSize image_size(100, 100);
  IntRect border_image_area(0, 0, 100, 100);
  IntRectOutsets border_widths(0, 0, 0, 0);

  NinePieceImageGrid grid = NinePieceImageGrid(
      nine_piece, image_size, border_image_area, border_widths);
  for (NinePiece piece = kMinPiece; piece < kMaxPiece; ++piece) {
    NinePieceImageGrid::NinePieceDrawInfo draw_info =
        grid.GetNinePieceDrawInfo(piece, 1);
    EXPECT_FALSE(draw_info.is_drawable);
  }
}

TEST_F(NinePieceImageGridTest, NinePieceImagePainting_AllDrawable) {
  NinePieceImage nine_piece;
  nine_piece.SetImage(GeneratedImage());
  nine_piece.SetImageSlices(LengthBox(10, 10, 10, 10));
  nine_piece.SetFill(true);

  IntSize image_size(100, 100);
  IntRect border_image_area(0, 0, 100, 100);
  IntRectOutsets border_widths(10, 10, 10, 10);

  NinePieceImageGrid grid = NinePieceImageGrid(
      nine_piece, image_size, border_image_area, border_widths);
  for (NinePiece piece = kMinPiece; piece < kMaxPiece; ++piece) {
    NinePieceImageGrid::NinePieceDrawInfo draw_info =
        grid.GetNinePieceDrawInfo(piece, 1);
    EXPECT_TRUE(draw_info.is_drawable);
  }
}

TEST_F(NinePieceImageGridTest, NinePieceImagePainting_NoFillMiddleNotDrawable) {
  NinePieceImage nine_piece;
  nine_piece.SetImage(GeneratedImage());
  nine_piece.SetImageSlices(LengthBox(10, 10, 10, 10));
  nine_piece.SetFill(false);  // default

  IntSize image_size(100, 100);
  IntRect border_image_area(0, 0, 100, 100);
  IntRectOutsets border_widths(10, 10, 10, 10);

  NinePieceImageGrid grid = NinePieceImageGrid(
      nine_piece, image_size, border_image_area, border_widths);
  for (NinePiece piece = kMinPiece; piece < kMaxPiece; ++piece) {
    NinePieceImageGrid::NinePieceDrawInfo draw_info =
        grid.GetNinePieceDrawInfo(piece, 1);
    if (piece != kMiddlePiece)
      EXPECT_TRUE(draw_info.is_drawable);
    else
      EXPECT_FALSE(draw_info.is_drawable);
  }
}

TEST_F(NinePieceImageGridTest, NinePieceImagePainting_TopLeftDrawable) {
  NinePieceImage nine_piece;
  nine_piece.SetImage(GeneratedImage());
  nine_piece.SetImageSlices(LengthBox(10, 10, 10, 10));

  IntSize image_size(100, 100);
  IntRect border_image_area(0, 0, 100, 100);
  IntRectOutsets border_widths(10, 10, 10, 10);

  const struct {
    IntRectOutsets border_widths;
    bool expected_is_drawable;
  } test_cases[] = {
      {IntRectOutsets(0, 0, 0, 0), false},
      {IntRectOutsets(10, 0, 0, 0), false},
      {IntRectOutsets(0, 0, 0, 10), false},
      {IntRectOutsets(10, 0, 0, 10), true},
  };

  for (const auto& test_case : test_cases) {
    NinePieceImageGrid grid = NinePieceImageGrid(
        nine_piece, image_size, border_image_area, test_case.border_widths);
    for (NinePiece piece = kMinPiece; piece < kMaxPiece; ++piece) {
      NinePieceImageGrid::NinePieceDrawInfo draw_info =
          grid.GetNinePieceDrawInfo(piece, 1);
      if (piece == kTopLeftPiece)
        EXPECT_EQ(draw_info.is_drawable, test_case.expected_is_drawable);
    }
  }
}

TEST_F(NinePieceImageGridTest, NinePieceImagePainting_ScaleDownBorder) {
  NinePieceImage nine_piece;
  nine_piece.SetImage(GeneratedImage());
  nine_piece.SetImageSlices(LengthBox(10, 10, 10, 10));

  IntSize image_size(100, 100);
  IntRect border_image_area(0, 0, 100, 100);
  IntRectOutsets border_widths(10, 10, 10, 10);

  // Set border slices wide enough so that the widths are scaled
  // down and corner pieces cover the entire border image area.
  nine_piece.SetBorderSlices(BorderImageLengthBox(6));

  NinePieceImageGrid grid = NinePieceImageGrid(
      nine_piece, image_size, border_image_area, border_widths);
  for (NinePiece piece = kMinPiece; piece < kMaxPiece; ++piece) {
    NinePieceImageGrid::NinePieceDrawInfo draw_info =
        grid.GetNinePieceDrawInfo(piece, 1);
    if (draw_info.is_corner_piece)
      EXPECT_EQ(draw_info.destination.Size(), FloatSize(50, 50));
    else
      EXPECT_TRUE(draw_info.destination.Size().IsEmpty());
  }
}

TEST_F(NinePieceImageGridTest, NinePieceImagePainting) {
  const struct {
    IntSize image_size;
    IntRect border_image_area;
    IntRectOutsets border_widths;
    bool fill;
    LengthBox image_slices;
    Image::TileRule horizontal_rule;
    Image::TileRule vertical_rule;
    struct {
      bool is_drawable;
      bool is_corner_piece;
      FloatRect destination;
      FloatRect source;
      float tile_scale_horizontal;
      float tile_scale_vertical;
      Image::TileRule horizontal_rule;
      Image::TileRule vertical_rule;
    } pieces[9];
  } test_cases[] = {
      {// Empty border and slices but with fill
       IntSize(100, 100),
       IntRect(0, 0, 100, 100),
       IntRectOutsets(0, 0, 0, 0),
       true,
       LengthBox(Length(0, kFixed), Length(0, kFixed), Length(0, kFixed),
                 Length(0, kFixed)),
       Image::kStretchTile,
       Image::kStretchTile,
       {
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kStretchTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kStretchTile},
           {true, false, FloatRect(0, 0, 100, 100), FloatRect(0, 0, 100, 100),
            1, 1, Image::kStretchTile, Image::kStretchTile},
       }},
      {// Single border and fill
       IntSize(100, 100),
       IntRect(0, 0, 100, 100),
       IntRectOutsets(0, 0, 10, 0),
       true,
       LengthBox(Length(20, kPercent), Length(20, kPercent),
                 Length(20, kPercent), Length(20, kPercent)),
       Image::kStretchTile,
       Image::kStretchTile,
       {
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kStretchTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kStretchTile},
           {true, false, FloatRect(0, 90, 100, 10), FloatRect(20, 80, 60, 20),
            0.5, 0.5, Image::kStretchTile, Image::kStretchTile},
           {true, false, FloatRect(0, 0, 100, 90), FloatRect(20, 20, 60, 60),
            1.666667, 1.5, Image::kStretchTile, Image::kStretchTile},
       }},
      {// All borders, no fill
       IntSize(100, 100),
       IntRect(0, 0, 100, 100),
       IntRectOutsets(10, 10, 10, 10),
       false,
       LengthBox(Length(20, kPercent), Length(20, kPercent),
                 Length(20, kPercent), Length(20, kPercent)),
       Image::kStretchTile,
       Image::kStretchTile,
       {
           {true, true, FloatRect(0, 0, 10, 10), FloatRect(0, 0, 20, 20), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {true, true, FloatRect(0, 90, 10, 10), FloatRect(0, 80, 20, 20), 1,
            1, Image::kStretchTile, Image::kStretchTile},
           {true, false, FloatRect(0, 10, 10, 80), FloatRect(0, 20, 20, 60),
            0.5, 0.5, Image::kStretchTile, Image::kStretchTile},
           {true, true, FloatRect(90, 0, 10, 10), FloatRect(80, 0, 20, 20), 1,
            1, Image::kStretchTile, Image::kStretchTile},
           {true, true, FloatRect(90, 90, 10, 10), FloatRect(80, 80, 20, 20), 1,
            1, Image::kStretchTile, Image::kStretchTile},
           {true, false, FloatRect(90, 10, 10, 80), FloatRect(80, 20, 20, 60),
            0.5, 0.5, Image::kStretchTile, Image::kStretchTile},
           {true, false, FloatRect(10, 0, 80, 10), FloatRect(20, 0, 60, 20),
            0.5, 0.5, Image::kStretchTile, Image::kStretchTile},
           {true, false, FloatRect(10, 90, 80, 10), FloatRect(20, 80, 60, 20),
            0.5, 0.5, Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kStretchTile},
       }},
      {// Single border, no fill
       IntSize(100, 100),
       IntRect(0, 0, 100, 100),
       IntRectOutsets(0, 0, 0, 10),
       false,
       LengthBox(Length(20, kPercent), Length(20, kPercent),
                 Length(20, kPercent), Length(20, kPercent)),
       Image::kStretchTile,
       Image::kRoundTile,
       {
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {true, false, FloatRect(0, 0, 10, 100), FloatRect(0, 20, 20, 60),
            0.5, 0.5, Image::kStretchTile, Image::kRoundTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kRoundTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kRoundTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kRoundTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kRoundTile},
       }},
      {// All borders but no slices, with fill (stretch horizontally, space
       // vertically)
       IntSize(100, 100),
       IntRect(0, 0, 100, 100),
       IntRectOutsets(10, 10, 10, 10),
       true,
       LengthBox(Length(0, kFixed), Length(0, kFixed), Length(0, kFixed),
                 Length(0, kFixed)),
       Image::kStretchTile,
       Image::kSpaceTile,
       {
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kSpaceTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, true, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 1, 1,
            Image::kStretchTile, Image::kStretchTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kSpaceTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kSpaceTile},
           {false, false, FloatRect(0, 0, 0, 0), FloatRect(0, 0, 0, 0), 0, 0,
            Image::kStretchTile, Image::kSpaceTile},
           {true, false, FloatRect(10, 10, 80, 80), FloatRect(0, 0, 100, 100),
            0.800000, 1, Image::kStretchTile, Image::kSpaceTile},
       }},
  };

  for (auto& test_case : test_cases) {
    NinePieceImage nine_piece;
    nine_piece.SetImage(GeneratedImage());
    nine_piece.SetFill(test_case.fill);
    nine_piece.SetImageSlices(test_case.image_slices);
    nine_piece.SetHorizontalRule(
        (ENinePieceImageRule)test_case.horizontal_rule);
    nine_piece.SetVerticalRule((ENinePieceImageRule)test_case.vertical_rule);

    NinePieceImageGrid grid = NinePieceImageGrid(
        nine_piece, test_case.image_size, test_case.border_image_area,
        test_case.border_widths);
    for (NinePiece piece = kMinPiece; piece < kMaxPiece; ++piece) {
      NinePieceImageGrid::NinePieceDrawInfo draw_info =
          grid.GetNinePieceDrawInfo(piece, 1);
      EXPECT_EQ(test_case.pieces[piece].is_drawable, draw_info.is_drawable);
      if (!test_case.pieces[piece].is_drawable)
        continue;

      EXPECT_EQ(test_case.pieces[piece].destination.X(),
                draw_info.destination.X());
      EXPECT_EQ(test_case.pieces[piece].destination.Y(),
                draw_info.destination.Y());
      EXPECT_EQ(test_case.pieces[piece].destination.Width(),
                draw_info.destination.Width());
      EXPECT_EQ(test_case.pieces[piece].destination.Height(),
                draw_info.destination.Height());
      EXPECT_EQ(test_case.pieces[piece].source.X(), draw_info.source.X());
      EXPECT_EQ(test_case.pieces[piece].source.Y(), draw_info.source.Y());
      EXPECT_EQ(test_case.pieces[piece].source.Width(),
                draw_info.source.Width());
      EXPECT_EQ(test_case.pieces[piece].source.Height(),
                draw_info.source.Height());

      if (test_case.pieces[piece].is_corner_piece)
        continue;

      EXPECT_FLOAT_EQ(test_case.pieces[piece].tile_scale_horizontal,
                      draw_info.tile_scale.Width());
      EXPECT_FLOAT_EQ(test_case.pieces[piece].tile_scale_vertical,
                      draw_info.tile_scale.Height());
      EXPECT_EQ(test_case.pieces[piece].horizontal_rule,
                draw_info.tile_rule.horizontal);
      EXPECT_EQ(test_case.pieces[piece].vertical_rule,
                draw_info.tile_rule.vertical);
    }
  }
}

}  // namespace
}  // namespace blink
