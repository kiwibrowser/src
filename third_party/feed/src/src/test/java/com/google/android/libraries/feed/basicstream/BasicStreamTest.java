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

package com.google.android.libraries.feed.basicstream;

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.os.Build.VERSION_CODES;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import com.google.android.libraries.feed.api.actionmanager.ActionManager;
import com.google.android.libraries.feed.api.actionparser.ActionParser;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelProviderFactory;
import com.google.android.libraries.feed.api.stream.ScrollListener;
import com.google.android.libraries.feed.basicstream.internal.StreamRecyclerViewAdapter;
import com.google.android.libraries.feed.basicstream.internal.StreamScrollMonitor;
import com.google.android.libraries.feed.basicstream.internal.drivers.StreamDriver;
import com.google.android.libraries.feed.common.testing.FakeClock;
import com.google.android.libraries.feed.common.time.Clock;
import com.google.android.libraries.feed.host.action.ActionApi;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.DebugBehavior;
import com.google.android.libraries.feed.host.imageloader.ImageLoaderApi;
import com.google.android.libraries.feed.host.stream.CardConfiguration;
import com.google.android.libraries.feed.host.stream.StreamConfiguration;
import com.google.android.libraries.feed.piet.PietManager;
import com.google.android.libraries.feed.piet.host.CustomElementProvider;
import com.google.android.libraries.feed.piet.host.HostBindingProvider;
import com.google.android.libraries.feed.testing.shadows.ShadowRecycledViewPool;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.shadow.api.Shadow;

/** Tests for {@link BasicStream}. */
@RunWith(RobolectricTestRunner.class)
@Config(shadows = {ShadowRecycledViewPool.class})
public class BasicStreamTest {

  private static final int START_PADDING = 1;
  private static final int END_PADDING = 2;
  private static final int TOP_PADDING = 3;
  private static final int BOTTOM_PADDING = 4;

  @Mock private StreamConfiguration streamConfiguration;
  @Mock private ModelProviderFactory modelProviderFactory;
  @Mock private ModelProvider initialModelProvider;
  @Mock private ModelProvider modelProvider;
  @Mock private PietManager pietManager;
  @Mock private StreamDriver streamDriver;
  @Mock private StreamRecyclerViewAdapter adapter;
  @Mock private StreamScrollMonitor streamScrollMonitor;

  private Configuration configuration = new Configuration.Builder().build();
  private BasicStream basicStream;

  @Before
  public void setUp() {
    initMocks(this);

    when(streamConfiguration.getPaddingStart()).thenReturn(START_PADDING);
    when(streamConfiguration.getPaddingEnd()).thenReturn(END_PADDING);
    when(streamConfiguration.getPaddingTop()).thenReturn(TOP_PADDING);
    when(streamConfiguration.getPaddingBottom()).thenReturn(BOTTOM_PADDING);
    
    when(modelProviderFactory.createNew()).thenReturn(initialModelProvider, modelProvider);
    basicStream = createBasicStream();
    basicStream.onCreate(null);
  }

  @Test
  public void testOnSessionStart() {
    basicStream.onSessionStart();

    verify(adapter).setDriver(streamDriver);
  }

  @Test
  public void testOnSessionFinished() {
    basicStream.onSessionFinished();

    verify(initialModelProvider).unregisterObserver(basicStream);
    verify(modelProviderFactory, times(2)).createNew();
    verify(modelProvider).registerObserver(basicStream);
  }

  @Test
  public void testLifecycle_onCreateCalledOnlyOnce() {
    // onCreate is called once in setup
    assertThatRunnable(() -> basicStream.onCreate(null))
        .throwsAnExceptionOfType(IllegalStateException.class);
  }

  @Test
  public void testLifecycle_getViewBeforeOnCreateCrashes() {
    // create BasicStream that has not had onCreate() called.
    basicStream = createBasicStream();
    assertThatRunnable(() -> basicStream.getView())
        .throwsAnExceptionOfType(IllegalStateException.class);
  }

  @Test
  @Config(sdk = VERSION_CODES.JELLY_BEAN)
  public void testPadding_jellyBean() {
    // Padding is setup in constructor.
    View view = basicStream.getView();

    assertThat(view.getPaddingLeft()).isEqualTo(START_PADDING);
    assertThat(view.getPaddingRight()).isEqualTo(END_PADDING);
    assertThat(view.getPaddingTop()).isEqualTo(TOP_PADDING);
    assertThat(view.getPaddingBottom()).isEqualTo(BOTTOM_PADDING);
  }

  @Test
  @Config(sdk = VERSION_CODES.KITKAT)
  public void testPadding_kitKat() {
    // Padding is setup in constructor.
    View view = basicStream.getView();

    assertThat(view.getPaddingStart()).isEqualTo(START_PADDING);
    assertThat(view.getPaddingEnd()).isEqualTo(END_PADDING);
    assertThat(view.getPaddingTop()).isEqualTo(TOP_PADDING);
    assertThat(view.getPaddingBottom()).isEqualTo(BOTTOM_PADDING);
  }

  @Test
  public void testTrim() {
    ShadowRecycledViewPool viewPool = Shadow.extract(getStreamRecyclerView().getRecycledViewPool());

    // RecyclerView ends up clearing the pool initially when the adapter is set on the RecyclerView.
    // Verify that has happened before anything else
    assertThat(viewPool.getClearCallCount()).isEqualTo(1);

    basicStream.trim();

    verify(pietManager).purgeRecyclerPools();

    // We expect the clear() call to be 2 as RecyclerView ends up clearing the pool initially when
    // the adapter is set on the RecyclerView.  So one call for that and one call for trim() call
    // on stream.
    assertThat(viewPool.getClearCallCount()).isEqualTo(2);
  }

  @Test
  public void testAddScrollListener() {
    ScrollListener scrollListener = mock(ScrollListener.class);
    basicStream.addScrollListener(scrollListener);
    verify(streamScrollMonitor).addScrollListener(scrollListener);
  }

  @Test
  public void testRemoveScrollListener() {
    ScrollListener scrollListener = mock(ScrollListener.class);
    basicStream.removeScrollListener(scrollListener);
    verify(streamScrollMonitor).removeScrollListener(scrollListener);
  }

  private RecyclerView getStreamRecyclerView() {
    return (RecyclerView) basicStream.getView();
  }

  private BasicStream createBasicStream() {
    return new BasicStreamForTest(
        Robolectric.setupActivity(Activity.class),
        streamConfiguration,
        mock(CardConfiguration.class),
        mock(ImageLoaderApi.class),
        mock(ActionParser.class),
        mock(ActionApi.class),
        mock(CustomElementProvider.class),
        DebugBehavior.VERBOSE,
        new ThreadUtils(),
        /* headers= */ new ArrayList<>(),
        new FakeClock(),
        modelProviderFactory,
        new HostBindingProvider(),
        mock(ActionManager.class),
        configuration);
  }

  private class BasicStreamForTest extends BasicStream {

    public BasicStreamForTest(
        Context context,
        StreamConfiguration streamConfiguration,
        CardConfiguration cardConfiguration,
        ImageLoaderApi imageLoaderApi,
        ActionParser actionParser,
        ActionApi actionApi,
        /*@Nullable*/ CustomElementProvider customElementProvider,
        DebugBehavior debugBehavior,
        ThreadUtils threadUtils,
        List<View> headers,
        Clock clock,
        ModelProviderFactory modelProviderFactory,
        /*@Nullable*/ HostBindingProvider hostBindingProvider,
        ActionManager actionManager,
        Configuration configuration) {
      super(
          context,
          streamConfiguration,
          cardConfiguration,
          imageLoaderApi,
          actionParser,
          actionApi,
          customElementProvider,
          debugBehavior,
          threadUtils,
          headers,
          clock,
          modelProviderFactory,
          hostBindingProvider,
          actionManager,
          configuration);
    }

    @Override
    PietManager createPietManager(
        Context context,
        CardConfiguration cardConfiguration,
        ImageLoaderApi imageLoaderApi,
        /*@Nullable*/ CustomElementProvider customElementProvider,
        DebugBehavior debugBehavior,
        Clock clock,
        /*@Nullable*/ HostBindingProvider hostBindingProvider) {
      return pietManager;
    }

    @Override
    StreamDriver createStreamDriver(
        ModelProvider modelProvider, ThreadUtils threadUtils, Configuration configuration) {
      return streamDriver;
    }

    @Override
    StreamRecyclerViewAdapter createRecyclerViewAdapter(
        Context context,
        CardConfiguration cardConfiguration,
        PietManager pietManager,
        ActionParser actionParser,
        ActionApi actionApi,
        ActionManager actionManager) {
      return adapter;
    }

    @Override
    StreamScrollMonitor createStreamScrollMonitor(RecyclerView recyclerView) {
      return streamScrollMonitor;
    }
  }
}
