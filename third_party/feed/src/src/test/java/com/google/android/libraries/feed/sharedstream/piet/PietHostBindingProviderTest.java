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

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.piet.host.HostBindingProvider;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.HostBindingData;
import com.google.search.now.ui.piet.TextProto.ParameterizedText;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests for {@link PietHostBindingProvider}. */
@RunWith(RobolectricTestRunner.class)
public class PietHostBindingProviderTest {

  @Mock private HostBindingProvider hostHostBindingProvider;

  private static final ParameterizedText TEXT_PAYLOAD =
      ParameterizedText.newBuilder().setText("foo").build();

  private static final BindingValue BINDING_WITH_HOST_DATA =
      BindingValue.newBuilder()
          .setHostBindingData(HostBindingData.newBuilder())
          .setParameterizedText(TEXT_PAYLOAD)
          .build();
  private static final BindingValue BINDING_WITHOUT_HOST_DATA =
      BindingValue.newBuilder().setParameterizedText(TEXT_PAYLOAD).build();

  private PietHostBindingProvider hostBindingProvider;
  private PietHostBindingProvider delegatingHostBindingProvider;

  @Before
  public void setUp() {
    initMocks(this);

    hostBindingProvider = new PietHostBindingProvider(/* hostBindingProvider */ null);
    delegatingHostBindingProvider = new PietHostBindingProvider(hostHostBindingProvider);
  }

  @Test
  public void testGetCustomElementDataBindingForValue() {
    assertThat(hostBindingProvider.getCustomElementDataBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(BINDING_WITHOUT_HOST_DATA);
  }

  @Test
  public void testGetCustomElementDataBindingForValue_delegating() {
    BindingValue hostBinding = BindingValue.newBuilder().setBindingId("custom-element").build();
    when(hostHostBindingProvider.getCustomElementDataBindingForValue(BINDING_WITH_HOST_DATA))
        .thenReturn(hostBinding);

    assertThat(
            delegatingHostBindingProvider.getCustomElementDataBindingForValue(
                BINDING_WITH_HOST_DATA))
        .isEqualTo(hostBinding);
  }

  @Test
  public void testGetParameterizedTextBindingForValue() {
    assertThat(hostBindingProvider.getParameterizedTextBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(BINDING_WITHOUT_HOST_DATA);
  }

  @Test
  public void testGetParameterizedTextBindingForValue_delegating() {
    BindingValue hostBinding = BindingValue.newBuilder().setBindingId("parameterized-text").build();
    when(hostHostBindingProvider.getParameterizedTextBindingForValue(BINDING_WITH_HOST_DATA))
        .thenReturn(hostBinding);

    assertThat(
            delegatingHostBindingProvider.getParameterizedTextBindingForValue(
                BINDING_WITH_HOST_DATA))
        .isEqualTo(hostBinding);
  }

  @Test
  public void testGetChunkedTextBindingForValue() {
    assertThat(hostBindingProvider.getChunkedTextBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(BINDING_WITHOUT_HOST_DATA);
  }

  @Test
  public void testGetChunkedTextBindingForValue_delegating() {
    BindingValue hostBinding = BindingValue.newBuilder().setBindingId("chunked-text").build();
    when(hostHostBindingProvider.getChunkedTextBindingForValue(BINDING_WITH_HOST_DATA))
        .thenReturn(hostBinding);

    assertThat(delegatingHostBindingProvider.getChunkedTextBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(hostBinding);
  }

  @Test
  public void testGetImageBindingForValue() {
    assertThat(hostBindingProvider.getChunkedTextBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(BINDING_WITHOUT_HOST_DATA);
  }

  @Test
  public void testGetImageBindingForValue_delegating() {
    BindingValue hostBinding = BindingValue.newBuilder().setBindingId("image").build();
    when(hostHostBindingProvider.getImageBindingForValue(BINDING_WITH_HOST_DATA))
        .thenReturn(hostBinding);

    assertThat(delegatingHostBindingProvider.getImageBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(hostBinding);
  }

  @Test
  public void testGetActionsBindingForValue() {
    assertThat(hostBindingProvider.getActionsBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(BINDING_WITHOUT_HOST_DATA);
  }

  @Test
  public void testGetActionsBindingForValue_delegating() {
    BindingValue hostBinding = BindingValue.newBuilder().setBindingId("actions").build();
    when(hostHostBindingProvider.getActionsBindingForValue(BINDING_WITH_HOST_DATA))
        .thenReturn(hostBinding);

    assertThat(delegatingHostBindingProvider.getActionsBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(hostBinding);
  }

  @Test
  public void testGetGridCellWidthBindingForValue() {
    assertThat(hostBindingProvider.getGridCellWidthBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(BINDING_WITHOUT_HOST_DATA);
  }

  @Test
  public void testGetGridCellWidthBindingForValue_delegating() {
    BindingValue hostBinding = BindingValue.newBuilder().setBindingId("gridcell").build();
    when(hostHostBindingProvider.getGridCellWidthBindingForValue(BINDING_WITH_HOST_DATA))
        .thenReturn(hostBinding);

    assertThat(
            delegatingHostBindingProvider.getGridCellWidthBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(hostBinding);
  }

  @Test
  public void testGetElementListBindingForValue() {
    assertThat(hostBindingProvider.getElementListBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(BINDING_WITHOUT_HOST_DATA);
  }

  @Test
  public void testGetElementListBindingForValue_delegating() {
    BindingValue hostBinding = BindingValue.newBuilder().setBindingId("element-list").build();
    when(hostHostBindingProvider.getElementListBindingForValue(BINDING_WITH_HOST_DATA))
        .thenReturn(hostBinding);

    assertThat(delegatingHostBindingProvider.getElementListBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(hostBinding);
  }

  @Test
  public void testGetVedBindingForValue() {
    assertThat(hostBindingProvider.getElementListBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(BINDING_WITHOUT_HOST_DATA);
  }

  @Test
  public void testGetVedBindingForValue_delegating() {
    BindingValue hostBinding = BindingValue.newBuilder().setBindingId("ved").build();
    when(hostHostBindingProvider.getVedBindingForValue(BINDING_WITH_HOST_DATA))
        .thenReturn(hostBinding);

    assertThat(delegatingHostBindingProvider.getVedBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(hostBinding);
  }

  @Test
  public void testGetTemplateBindingForValue() {
    assertThat(hostBindingProvider.getTemplateBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(BINDING_WITHOUT_HOST_DATA);
  }

  @Test
  public void testGetTemplateBindingForValue_delegating() {
    BindingValue hostBinding = BindingValue.newBuilder().setBindingId("ved").build();
    when(hostHostBindingProvider.getTemplateBindingForValue(BINDING_WITH_HOST_DATA))
        .thenReturn(hostBinding);

    assertThat(delegatingHostBindingProvider.getTemplateBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(hostBinding);
  }

  @Test
  public void testGetStyleBindingForValue() {
    assertThat(hostBindingProvider.getTemplateBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(BINDING_WITHOUT_HOST_DATA);
  }

  @Test
  public void testGetStyleBindingForValue_delegating() {
    BindingValue hostBinding = BindingValue.newBuilder().setBindingId("ved").build();
    when(hostHostBindingProvider.getStyleBindingForValue(BINDING_WITH_HOST_DATA))
        .thenReturn(hostBinding);

    assertThat(delegatingHostBindingProvider.getStyleBindingForValue(BINDING_WITH_HOST_DATA))
        .isEqualTo(hostBinding);
  }
}
