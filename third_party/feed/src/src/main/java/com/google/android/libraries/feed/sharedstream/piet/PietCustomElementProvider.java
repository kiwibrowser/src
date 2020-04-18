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

import android.content.Context;
import android.view.View;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.piet.host.CustomElementProvider;
import com.google.search.now.ui.piet.ElementsProto.CustomElementData;

/**
 * A Stream implementation of a {@link CustomElementProvider} which handles Stream custom elements
 * and can delegate to a host custom element adapter if needed.
 */
public class PietCustomElementProvider implements CustomElementProvider {

  private static final String TAG = "PietCustomElementPro";

  private final Context context;
  /*@Nullable*/ private final CustomElementProvider hostCustomElementProvider;

  public PietCustomElementProvider(
      Context context, /*@Nullable*/ CustomElementProvider hostCustomElementProvider) {
    this.context = context;
    this.hostCustomElementProvider = hostCustomElementProvider;
  }

  @Override
  public View createCustomElement(CustomElementData customElementData) {
    // We don't currently implement any custom elements yet.  Delegate to host if there is one.
    if (hostCustomElementProvider != null) {
      return hostCustomElementProvider.createCustomElement(customElementData);
    }

    // Just return an empty view if there is not host.
    Logger.w(TAG, "Received request for unknown custom element");
    return new View(context);
  }

  @Override
  public void releaseCustomView(View customElementView) {
    if (hostCustomElementProvider != null) {
      hostCustomElementProvider.releaseCustomView(customElementView);
      return;
    }

    Logger.w(TAG, "Received release for unknown custom element");
  }
}
