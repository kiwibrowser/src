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
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import com.google.android.libraries.feed.piet.host.CustomElementProvider;
import com.google.search.now.ui.piet.ElementsProto.CustomElementData;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests for {@link PietCustomElementProvider}. */
@RunWith(RobolectricTestRunner.class)
public class PietCustomElementProviderTest {

  @Mock private CustomElementProvider hostCustomElementProvider;

  private Context context;
  private View hostCreatedView;
  private PietCustomElementProvider customElementProvider;
  private PietCustomElementProvider delegatingCustomElementProvider;

  @Before
  public void setUp() {
    initMocks(this);

    context = Robolectric.setupActivity(Activity.class);
    hostCreatedView = new View(context);
    customElementProvider =
        new PietCustomElementProvider(context, /* customElementProvider */ null);
    delegatingCustomElementProvider =
        new PietCustomElementProvider(context, hostCustomElementProvider);
  }

  @Test
  public void testCreateCustomElement_noHostDelegate() {
    View view = customElementProvider.createCustomElement(CustomElementData.getDefaultInstance());
    assertThat(view).isNotNull();
  }

  @Test
  public void testCreateCustomElement_hostDelegate() {
    CustomElementData customElementData = CustomElementData.getDefaultInstance();
    when(hostCustomElementProvider.createCustomElement(customElementData))
        .thenReturn(hostCreatedView);

    View view = delegatingCustomElementProvider.createCustomElement(customElementData);
    assertThat(view).isEqualTo(hostCreatedView);
  }

  @Test
  public void testReleaseView_noHostDelegate() {
    customElementProvider.releaseCustomView(new View(context));

    // Above call should not throw
  }

  @Test
  public void testReleaseView_hostDelegate() {
    delegatingCustomElementProvider.releaseCustomView(hostCreatedView);

    verify(hostCustomElementProvider).releaseCustomView(hostCreatedView);
  }
}
