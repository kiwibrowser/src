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

import com.google.search.now.ui.piet.BindingRefsProto.ActionsBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ChunkedTextBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.CustomBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ElementListBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.GridCellWidthBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ImageBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.ParameterizedTextBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.StyleBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.TemplateBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.VedBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.HostBindingData;

/**
 * Interface which allows a host to provide bindings to Piet directly. This specifically allows
 * changing bindings based on on-device information.
 *
 * <p>Methods are called by Piet during the binding process when their associated *BindingRef is
 * found and the server has specified a {@link BindingValue} which includes a {@link
 * HostBindingData} set.
 *
 * <p>Methods returns should include the associated binding filled in with no {@link
 * HostBindingData}. If a {@link HostBindingData} is set on the returned {@link BindingValue}
 * instances then it will be ignored.
 *
 * <p>This class provides a default implementation which just removes {@link HostBindingData} from
 * any server specified {@link HostBindingData}.
 *
 * <p>See [INTERNAL LINK].
 */
public class HostBindingProvider {

  /**
   * Called by Piet during the binding process for a {@link CustomBindingRef} if the server has
   * specified a BindingValue which includes {@link HostBindingData} set.
   */
  public BindingValue getCustomElementDataBindingForValue(BindingValue bindingValue) {
    return clearHostBindingData(bindingValue);
  }

  /**
   * Called by Piet during the binding process for a {@link ParameterizedTextBindingRef} if the
   * server has specified a BindingValue which includes {@link HostBindingData} set.
   */
  public BindingValue getParameterizedTextBindingForValue(BindingValue bindingValue) {
    return clearHostBindingData(bindingValue);
  }

  /**
   * Called by Piet during the binding process for a {@link ChunkedTextBindingRef} if the server has
   * specified a BindingValue which includes {@link HostBindingData} set.
   */
  public BindingValue getChunkedTextBindingForValue(BindingValue bindingValue) {
    return clearHostBindingData(bindingValue);
  }

  /**
   * Called by Piet during the binding process for a {@link ImageBindingRef} if the server has
   * specified a BindingValue which includes {@link HostBindingData} set.
   */
  public BindingValue getImageBindingForValue(BindingValue bindingValue) {
    return clearHostBindingData(bindingValue);
  }

  /**
   * Called by Piet during the binding process for a {@link ActionsBindingRef} if the server has
   * specified a BindingValue which includes {@link HostBindingData} set.
   */
  public BindingValue getActionsBindingForValue(BindingValue bindingValue) {
    return clearHostBindingData(bindingValue);
  }

  /**
   * Called by Piet during the binding process for a {@link GridCellWidthBindingRef} if the server
   * has specified a BindingValue which includes {@link HostBindingData} set.
   */
  public BindingValue getGridCellWidthBindingForValue(BindingValue bindingValue) {
    return clearHostBindingData(bindingValue);
  }

  /**
   * Called by Piet during the binding process for a {@link ElementListBindingRef} if the server has
   * specified a BindingValue which includes {@link HostBindingData} set.
   */
  public BindingValue getElementListBindingForValue(BindingValue bindingValue) {
    return clearHostBindingData(bindingValue);
  }

  /**
   * Called by Piet during the binding process for a {@link VedBindingRef} if the server has
   * specified a BindingValue which includes {@link HostBindingData} set.
   */
  public BindingValue getVedBindingForValue(BindingValue bindingValue) {
    return clearHostBindingData(bindingValue);
  }

  /**
   * Called by Piet during the binding process for a {@link TemplateBindingRef} if the server has
   * specified a BindingValue which includes {@link HostBindingData} set.
   */
  public BindingValue getTemplateBindingForValue(BindingValue bindingValue) {
    return clearHostBindingData(bindingValue);
  }

  /**
   * Called by Piet during the binding process for a {@link StyleBindingRef} if the server has
   * specified a BindingValue which includes {@link HostBindingData} set.
   */
  public BindingValue getStyleBindingForValue(BindingValue bindingValue) {
    return clearHostBindingData(bindingValue);
  }

  private BindingValue clearHostBindingData(BindingValue bindingValue) {
    return bindingValue.toBuilder().clearHostBindingData().build();
  }
}
