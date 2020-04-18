// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_LAYER_TREE_PIXEL_TEST_H_
#define CC_TEST_LAYER_TREE_PIXEL_TEST_H_

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "cc/test/layer_tree_test.h"
#include "components/viz/common/resources/single_release_callback.h"
#include "ui/gl/gl_implementation.h"

class SkBitmap;

namespace viz {
class CopyOutputRequest;
class CopyOutputResult;
}

namespace cc {
class PixelComparator;
class SolidColorLayer;
class TextureLayer;

class LayerTreePixelTest : public LayerTreeTest {
 public:
  enum PixelTestType {
    PIXEL_TEST_GL,
    PIXEL_TEST_SOFTWARE,
  };

 protected:
  LayerTreePixelTest();
  ~LayerTreePixelTest() override;

  // LayerTreeTest overrides.
  std::unique_ptr<viz::TestLayerTreeFrameSink> CreateLayerTreeFrameSink(
      const viz::RendererSettings& renderer_settings,
      double refresh_rate,
      scoped_refptr<viz::ContextProvider> compositor_context_provider,
      scoped_refptr<viz::RasterContextProvider> worker_context_provider)
      override;
  std::unique_ptr<viz::OutputSurface> CreateDisplayOutputSurfaceOnThread(
      scoped_refptr<viz::ContextProvider> compositor_context_provider) override;

  virtual std::unique_ptr<viz::CopyOutputRequest> CreateCopyOutputRequest();

  void ReadbackResult(std::unique_ptr<viz::CopyOutputResult> result);

  void BeginTest() override;
  void SetupTree() override;
  void AfterTest() override;
  void EndTest() override;
  void InitializeSettings(LayerTreeSettings* settings) override;

  void TryEndTest();

  scoped_refptr<SolidColorLayer> CreateSolidColorLayer(const gfx::Rect& rect,
                                                       SkColor color);
  scoped_refptr<SolidColorLayer> CreateSolidColorLayerWithBorder(
      const gfx::Rect& rect,
      SkColor color,
      int border_width,
      SkColor border_color);

  void RunPixelTest(PixelTestType type,
                    scoped_refptr<Layer> content_root,
                    base::FilePath file_name);

  void RunSingleThreadedPixelTest(PixelTestType test_type,
                                  scoped_refptr<Layer> content_root,
                                  base::FilePath file_name);

  void RunPixelTestWithReadbackTarget(PixelTestType type,
                                      scoped_refptr<Layer> content_root,
                                      Layer* target,
                                      base::FilePath file_name);

  SkBitmap CopyMailboxToBitmap(const gfx::Size& size,
                               const gpu::Mailbox& mailbox,
                               const gpu::SyncToken& sync_token);

  void Finish();

  // Allow tests to enlarge the backing texture for a non-root render pass, to
  // simulate reusing a larger texture from a previous frame for a new
  // render pass. This should be called before the output surface is bound.
  void set_enlarge_texture_amount(const gfx::Size& enlarge_texture_amount) {
    enlarge_texture_amount_ = enlarge_texture_amount;
  }

  // Common CSS colors defined for tests to use.
  static const SkColor kCSSOrange = 0xffffa500;
  static const SkColor kCSSBrown = 0xffa52a2a;
  static const SkColor kCSSGreen = 0xff008000;
  static const SkColor kCSSBlack = 0xff000000;

  gl::DisableNullDrawGLBindings enable_pixel_output_;
  std::unique_ptr<PixelComparator> pixel_comparator_;
  PixelTestType test_type_;
  scoped_refptr<Layer> content_root_;
  Layer* readback_target_;
  base::FilePath ref_file_;
  std::unique_ptr<SkBitmap> result_bitmap_;
  std::vector<scoped_refptr<TextureLayer>> texture_layers_;
  int pending_texture_mailbox_callbacks_;
  gfx::Size enlarge_texture_amount_;
};

}  // namespace cc

#endif  // CC_TEST_LAYER_TREE_PIXEL_TEST_H_
