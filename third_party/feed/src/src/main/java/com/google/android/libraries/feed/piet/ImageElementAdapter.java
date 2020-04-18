// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.piet;

import static com.google.android.libraries.feed.common.Validators.checkState;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.piet.AdapterFactory.SingletonKeySupplier;
import com.google.android.libraries.feed.piet.ui.RoundedCornerImageView;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.BindingValue.Visibility;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ImageElement;
import com.google.search.now.ui.piet.ImagesProto.Image;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;

/** An {@link ElementAdapter} for {@code ImageElement} elements. */
class ImageElementAdapter extends ElementAdapter<RoundedCornerImageView, ImageElement> {
  private static final String TAG = "ImageElementAdapter";

  /*@Nullable*/ private LoadImageCallback currentlyLoadingImage = null;

  private ImageElementAdapter(Context context, AdapterParameters parameters) {
    super(context, parameters, createView(context), KeySupplier.SINGLETON_KEY);
  }

  @Override
  ImageElement getModelFromElement(Element baseElement) {
    if (!baseElement.hasImageElement()) {
      throw new IllegalArgumentException(
          String.format("Missing ImageElement; has %s", baseElement.getElementsCase()));
    }
    return baseElement.getImageElement();
  }

  @Override
  void onCreateAdapter(ImageElement model, Element baseElement, FrameContext frameContext) {
    StyleProvider style = getElementStyle();

    if (style.hasWidth() || style.hasHeight()) {
      Context context = getContext();

      if (style.hasWidth()) {
        widthPx = (int) ViewUtils.dpToPx(style.getWidth(), context);
      } else {
        widthPx = DIMENSION_NOT_SET;
      }

      if (style.hasHeight()) {
        heightPx = (int) ViewUtils.dpToPx(style.getHeight(), context);
      } else {
        // Defaults to a square when only the width is defined.
        heightPx = style.hasWidth() ? widthPx : DIMENSION_NOT_SET;
      }
    } else {
      widthPx = DIMENSION_NOT_SET;
      heightPx = DIMENSION_NOT_SET;
    }

    int cornerRadius = 0;
    int cornerBitmask = 0;
    if (style.hasRoundedCorners()) {
      cornerBitmask = style.getRoundedCorners().getBitmask();
      cornerRadius = frameContext.getRoundedCornerRadius(style, getContext());
    }
    getBaseView().setRoundedCorners(cornerBitmask);
    getBaseView().setCornerRadius(cornerRadius);
  }

  @Override
  void onBindModel(ImageElement model, Element baseElement, FrameContext frameContext) {
    Image image;
    switch (model.getContentCase()) {
      case IMAGE:
        image = model.getImage();
        getBaseView().setVisibility(View.VISIBLE);
        break;
      case IMAGE_BINDING:
        BindingValue binding = frameContext.getImageBindingValue(model.getImageBinding());
        setVisibility(binding.getVisibility());
        if (!binding.hasImage()) {
          if (binding.getVisibility() == Visibility.GONE
              || model.getImageBinding().getIsOptional()) {
            setVisibility(Visibility.GONE);
            return;
          } else {
            throw new IllegalArgumentException(
                String.format("Image binding %s had no content", binding.getBindingId()));
          }
        }
        image = binding.getImage();
        break;
      default:
        throw new IllegalStateException(
            String.format(
                "Unsupported or missing content in ImageElement: %s", model.getContentCase()));
    }
    checkState(currentlyLoadingImage == null, "An image loading callback exists; unbind first");
    LoadImageCallback loadImageCallback = new LoadImageCallback(getBaseView());
    currentlyLoadingImage = loadImageCallback;
    getFrameContext().getAssetProvider().getImage(image, loadImageCallback);
  }

  @Override
  void onUnbindModel() {
    if (currentlyLoadingImage != null) {
      currentlyLoadingImage.cancelled = true;
      currentlyLoadingImage = null;
    }
    ImageView imageView = getBaseView();
    if (imageView != null) {
      imageView.setImageDrawable(null);
    }
    widthPx = DIMENSION_NOT_SET;
    heightPx = DIMENSION_NOT_SET;
  }

  @Override
  void onReleaseAdapter() {}

  @Override
  StyleIdsStack getElementStyleIdsStack() {
    return getModel().getStyleReferences();
  }

  private static RoundedCornerImageView createView(Context context) {
    RoundedCornerImageView imageView = new RoundedCornerImageView(context);
    imageView.setCropToPadding(true);
    return imageView;
  }

  private static class LoadImageCallback implements Consumer<Drawable> {
    private final ImageView imageView;
    boolean cancelled = false;

    LoadImageCallback(ImageView imageView) {
      this.imageView = imageView;
    }

    @Override
    public void accept(Drawable drawable) {
      if (!cancelled) {
        imageView.setImageDrawable(drawable);
        imageView.setScaleType(ScaleType.CENTER_CROP);
      }
    }
  }

  static class KeySupplier extends SingletonKeySupplier<ImageElementAdapter, ImageElement> {
    @Override
    public String getAdapterTag() {
      return TAG;
    }

    @Override
    public ImageElementAdapter getAdapter(Context context, AdapterParameters parameters) {
      return new ImageElementAdapter(context, parameters);
    }
  }
}
