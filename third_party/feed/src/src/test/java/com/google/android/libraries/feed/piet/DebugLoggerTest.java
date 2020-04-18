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

package com.google.android.libraries.feed.piet;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.TextView;
import com.google.android.libraries.feed.piet.DebugLogger.MessageType;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link DebugLogger}. */
@RunWith(RobolectricTestRunner.class)
public class DebugLoggerTest {
  private static final String ERROR_TEXT_1 = "Interdimensional rift formation.";
  private static final String ERROR_TEXT_2 = "Exotic particle containment breach.";
  private static final String WARNING_TEXT = "Noncompliant meson entanglement.";

  @Mock private FrameContext frameContext;

  private Context context;

  private DebugLogger debugLogger;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    debugLogger = new DebugLogger();
  }

  @Test
  public void testGetReportView_singleError() {
    debugLogger.recordMessage(MessageType.ERROR, ERROR_TEXT_1);

    LinearLayout reportView = (LinearLayout) debugLogger.getReportView(MessageType.ERROR, context);

    assertThat(reportView.getOrientation()).isEqualTo(LinearLayout.VERTICAL);
    assertThat(reportView.getLayoutParams().width).isEqualTo(LayoutParams.MATCH_PARENT);
    assertThat(reportView.getLayoutParams().height).isEqualTo(LayoutParams.WRAP_CONTENT);

    assertThat(reportView.getChildCount()).isEqualTo(2);

    // Check the divider
    assertThat(reportView.getChildAt(0).getLayoutParams().width)
        .isEqualTo(LayoutParams.MATCH_PARENT);
    assertThat(reportView.getChildAt(0).getLayoutParams().height)
        .isEqualTo((int) ViewUtils.dpToPx(DebugLogger.ERROR_DIVIDER_WIDTH_DP, context));

    // Check the error box
    assertThat(reportView.getChildAt(1)).isInstanceOf(TextView.class);
    TextView textErrorView = (TextView) reportView.getChildAt(1);
    assertThat(textErrorView.getText()).isEqualTo(ERROR_TEXT_1);

    // Check that padding has been set (but don't check specific values)
    assertThat(textErrorView.getPaddingBottom()).isNotEqualTo(0);
    assertThat(textErrorView.getPaddingTop()).isNotEqualTo(0);
    assertThat(textErrorView.getPaddingStart()).isNotEqualTo(0);
    assertThat(textErrorView.getPaddingEnd()).isNotEqualTo(0);
  }

  @Test
  public void testGetReportView_multipleErrors() {
    debugLogger.recordMessage(MessageType.ERROR, ERROR_TEXT_1);
    debugLogger.recordMessage(MessageType.ERROR, ERROR_TEXT_2);

    LinearLayout reportView = (LinearLayout) debugLogger.getReportView(MessageType.ERROR, context);

    assertThat(reportView.getChildCount()).isEqualTo(3);

    assertThat(((TextView) reportView.getChildAt(1)).getText()).isEqualTo(ERROR_TEXT_1);
    assertThat(((TextView) reportView.getChildAt(2)).getText()).isEqualTo(ERROR_TEXT_2);
  }

  @Test
  public void testGetReportView_zeroErrors() {
    LinearLayout errorView = (LinearLayout) debugLogger.getReportView(MessageType.ERROR, context);

    assertThat(errorView).isNull();
  }

  @Test
  public void testGetMessageTypes() {
    assertThat(debugLogger.getMessages(MessageType.ERROR)).isEmpty();
    assertThat(debugLogger.getMessages(MessageType.WARNING)).isEmpty();

    debugLogger.recordMessage(MessageType.WARNING, WARNING_TEXT);
    assertThat(debugLogger.getMessages(MessageType.ERROR)).isEmpty();
    assertThat(debugLogger.getMessages(MessageType.WARNING)).containsExactly(WARNING_TEXT);

    debugLogger.recordMessage(MessageType.ERROR, ERROR_TEXT_1);
    assertThat(debugLogger.getMessages(MessageType.ERROR)).containsExactly(ERROR_TEXT_1);
    assertThat(debugLogger.getMessages(MessageType.WARNING)).containsExactly(WARNING_TEXT);
  }

  @Test
  public void testGetReportView_byType() {
    debugLogger.recordMessage(MessageType.ERROR, ERROR_TEXT_1);
    debugLogger.recordMessage(MessageType.WARNING, WARNING_TEXT);

    LinearLayout errorView = (LinearLayout) debugLogger.getReportView(MessageType.ERROR, context);

    assertThat(errorView.getChildCount()).isEqualTo(2);
    assertThat(((TextView) errorView.getChildAt(1)).getText()).isEqualTo(ERROR_TEXT_1);

    LinearLayout warningView =
        (LinearLayout) debugLogger.getReportView(MessageType.WARNING, context);

    assertThat(warningView.getChildCount()).isEqualTo(2);
    assertThat(((TextView) warningView.getChildAt(1)).getText()).isEqualTo(WARNING_TEXT);
  }
}
