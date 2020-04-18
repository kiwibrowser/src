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

package com.google.android.libraries.feed.sharedstream.piet;

import android.graphics.drawable.Drawable;
import android.text.format.DateUtils;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.time.Clock;
import com.google.android.libraries.feed.host.imageloader.ImageLoaderApi;
import com.google.android.libraries.feed.host.stream.CardConfiguration;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.search.now.ui.piet.ImagesProto.Image;
import com.google.search.now.ui.piet.ImagesProto.ImageSource;
import java.util.ArrayList;
import java.util.List;

/**
 * A Piet {@link AssetProvider} which uses {@link ImageLoaderApi} to load images and has Stream
 * specific implementations for other methods.
 */
public class PietAssetProvider implements AssetProvider {

  private final ImageLoaderApi imageLoaderApi;
  private final CardConfiguration cardConfiguration;
  private final Clock clock;

  public PietAssetProvider(
      ImageLoaderApi imageLoaderApi, CardConfiguration cardConfiguration, Clock clock) {
    this.imageLoaderApi = imageLoaderApi;
    this.cardConfiguration = cardConfiguration;
    this.clock = clock;
  }

  @Override
  public void getImage(Image image, Consumer<Drawable> consumer) {
    List<String> urls = new ArrayList<>(image.getSourcesList().size());
    for (ImageSource source : image.getSourcesList()) {
      urls.add(source.getUrl());
    }

    imageLoaderApi.loadDrawable(urls, consumer);
  }

  @Override
  public String getRelativeElapsedString(long elapsedTimeMillis) {
    return DateUtils.getRelativeTimeSpanString(
            clock.currentTimeMillis() - elapsedTimeMillis,
            clock.currentTimeMillis(),
            DateUtils.MINUTE_IN_MILLIS,
            DateUtils.FORMAT_ABBREV_RELATIVE)
        .toString();
  }

  @Override
  public int getDefaultCornerRadius() {
    return cardConfiguration.getDefaultCornerRadius();
  }
}
