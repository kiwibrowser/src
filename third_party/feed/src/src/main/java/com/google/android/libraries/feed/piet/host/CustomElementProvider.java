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

package com.google.android.libraries.feed.piet.host;

import android.view.View;
import com.google.search.now.ui.piet.ElementsProto.CustomElementData;

/**
 * Provides custom elements from the host. Host must handle filtering for supported extensions and
 * returning a view.
 */
public interface CustomElementProvider {

  /** Requests that the host create a view based on an extension on CustomElementData. */
  View createCustomElement(CustomElementData customElementData);

  /** Notify the host that Piet is done with and will no longer use this custom element View. */
  void releaseCustomView(View customElementView);
}
