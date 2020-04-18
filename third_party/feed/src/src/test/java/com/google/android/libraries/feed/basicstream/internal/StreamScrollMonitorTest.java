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

package com.google.android.libraries.feed.basicstream.internal;

import static com.google.android.libraries.feed.basicstream.internal.StreamScrollMonitor.convertRecyclerViewScrollStateToListenerState;
import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.support.v7.widget.RecyclerView;
import com.google.android.libraries.feed.api.stream.ScrollListener;
import com.google.android.libraries.feed.api.stream.ScrollListener.ScrollState;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests for {@link StreamScrollMonitor}. */
@RunWith(RobolectricTestRunner.class)
public class StreamScrollMonitorTest {

  @Mock private ScrollListener scrollListener1;
  @Mock private ScrollListener scrollListener2;

  private StreamScrollMonitor streamScrollMonitor;
  private RecyclerView recyclerView;

  @Before
  public void setUp() {
    initMocks(this);

    Context context = Robolectric.setupActivity(Activity.class);
    recyclerView = new RecyclerView(context);

    streamScrollMonitor = new StreamScrollMonitor(recyclerView);
    streamScrollMonitor.addScrollListener(scrollListener1);
  }

  @Test
  public void testScrollStateOutputs() {
    streamScrollMonitor.onScrollStateChanged(recyclerView, RecyclerView.SCROLL_STATE_IDLE);
    verify(scrollListener1).onScrollStateChanged(ScrollState.IDLE);

    streamScrollMonitor.onScrollStateChanged(recyclerView, RecyclerView.SCROLL_STATE_DRAGGING);
    verify(scrollListener1).onScrollStateChanged(ScrollState.DRAGGING);

    streamScrollMonitor.onScrollStateChanged(recyclerView, RecyclerView.SCROLL_STATE_SETTLING);
    verify(scrollListener1).onScrollStateChanged(ScrollState.SETTLING);

    assertThatRunnable(() -> streamScrollMonitor.onScrollStateChanged(recyclerView, -42))
        .throwsAnExceptionOfType(RuntimeException.class);
  }

  @Test
  public void testOnScrollStateChanged() {
    streamScrollMonitor.onScrollStateChanged(recyclerView, RecyclerView.SCROLL_STATE_IDLE);

    verify(scrollListener1).onScrollStateChanged(ScrollState.IDLE);
  }

  @Test
  public void testOnScrollStateChanged_multipleListeners() {
    streamScrollMonitor.addScrollListener(scrollListener2);

    streamScrollMonitor.onScrollStateChanged(recyclerView, RecyclerView.SCROLL_STATE_IDLE);

    verify(scrollListener1).onScrollStateChanged(ScrollState.IDLE);
    verify(scrollListener2).onScrollStateChanged(ScrollState.IDLE);
  }

  @Test
  public void testOnScrollStateChanged_removedListener() {
    streamScrollMonitor.addScrollListener(scrollListener2);
    streamScrollMonitor.removeScrollListener(scrollListener1);

    streamScrollMonitor.onScrollStateChanged(recyclerView, RecyclerView.SCROLL_STATE_IDLE);

    verify(scrollListener1, never()).onScrollStateChanged(anyInt());
    verify(scrollListener2).onScrollStateChanged(ScrollState.IDLE);
  }

  @Test
  public void testOnScrolled() {
    streamScrollMonitor.onScrolled(recyclerView, 1, 2);

    verify(scrollListener1).onScrolled(1, 2);
  }

  @Test
  public void testOnScrolled_multipleListeners() {
    streamScrollMonitor.addScrollListener(scrollListener2);
    streamScrollMonitor.onScrolled(recyclerView, 1, 2);

    verify(scrollListener1).onScrolled(1, 2);
    verify(scrollListener2).onScrolled(1, 2);
  }

  @Test
  public void testOnScrolled_removedListener() {
    streamScrollMonitor.addScrollListener(scrollListener2);
    streamScrollMonitor.removeScrollListener(scrollListener1);

    streamScrollMonitor.onScrolled(recyclerView, 1, 2);

    verify(scrollListener1, never()).onScrolled(anyInt(), anyInt());
    verify(scrollListener2).onScrolled(1, 2);
  }
}
