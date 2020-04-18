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

package com.google.android.libraries.feed.host.stream;

import android.graphics.drawable.Drawable;

/** Class which is able to provide host configuration for default card look and feel. */
// TODO: Look into allowing server configuration of this.
public interface CardConfiguration {

  int getDefaultCornerRadius();

  Drawable getCardBackground();

  /** Returns the amount of padding (in px) at the end of a card in the Stream */
  float getCardBottomPadding();
}
