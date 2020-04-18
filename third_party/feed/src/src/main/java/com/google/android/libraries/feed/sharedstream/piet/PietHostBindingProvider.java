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

import com.google.android.libraries.feed.piet.host.HostBindingProvider;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;

/**
 * A Stream implementation of a {@link HostBindingProvider} which handles Stream host bindings and
 * can delegate to a host host binding provider if needed.
 */
// TODO: This initially just delegates to hosts. Add more support to this in the future.
public class PietHostBindingProvider extends HostBindingProvider {

  /*@Nullable*/ private final HostBindingProvider hostHostBindingProvider;

  public PietHostBindingProvider(/*@Nullable*/ HostBindingProvider hostHostBindingProvider) {
    this.hostHostBindingProvider = hostHostBindingProvider;
  }

  @Override
  public BindingValue getCustomElementDataBindingForValue(BindingValue bindingValue) {
    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getCustomElementDataBindingForValue(bindingValue);
    }
    return super.getCustomElementDataBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getParameterizedTextBindingForValue(BindingValue bindingValue) {
    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getParameterizedTextBindingForValue(bindingValue);
    }
    return super.getParameterizedTextBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getChunkedTextBindingForValue(BindingValue bindingValue) {
    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getChunkedTextBindingForValue(bindingValue);
    }
    return super.getChunkedTextBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getImageBindingForValue(BindingValue bindingValue) {
    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getImageBindingForValue(bindingValue);
    }
    return super.getImageBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getActionsBindingForValue(BindingValue bindingValue) {
    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getActionsBindingForValue(bindingValue);
    }
    return super.getActionsBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getGridCellWidthBindingForValue(BindingValue bindingValue) {
    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getGridCellWidthBindingForValue(bindingValue);
    }
    return super.getGridCellWidthBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getElementListBindingForValue(BindingValue bindingValue) {
    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getElementListBindingForValue(bindingValue);
    }
    return super.getElementListBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getVedBindingForValue(BindingValue bindingValue) {
    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getVedBindingForValue(bindingValue);
    }
    return super.getVedBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getTemplateBindingForValue(BindingValue bindingValue) {
    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getTemplateBindingForValue(bindingValue);
    }
    return super.getTemplateBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getStyleBindingForValue(BindingValue bindingValue) {
    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getStyleBindingForValue(bindingValue);
    }
    return super.getStyleBindingForValue(bindingValue);
  }
}
