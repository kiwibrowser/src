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
import static org.mockito.Matchers.anyInt;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.search.now.ui.piet.TextProto.ParameterizedText;
import com.google.search.now.ui.piet.TextProto.ParameterizedText.Parameter;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link ParameterizedTextEvaluator} */
@RunWith(RobolectricTestRunner.class)
// TODO: Create a test of evaluteHtml
public class ParameterizedTextEvaluatorTest {

  @Mock private FrameContext frameContext;
  @Mock private AssetProvider assetProvider;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    when(frameContext.getAssetProvider()).thenReturn(assetProvider);
  }

  @Test
  public void testEvaluate_noText() {
    ParameterizedText text = ParameterizedText.getDefaultInstance();
    ParameterizedTextEvaluator evaluator = new ParameterizedTextEvaluator();
    assertThat(evaluator.evaluate(frameContext, text)).isEmpty();
  }

  @Test
  public void testEvaluate_noParameters() {
    String content = "content";
    ParameterizedText.Builder text = ParameterizedText.newBuilder();
    text.setText(content);
    ParameterizedTextEvaluator evaluator = new ParameterizedTextEvaluator();
    assertThat(evaluator.evaluate(frameContext, text.build())).isEqualTo(content);
  }

  @Test
  public void testEvaluate_time() {
    String content = "content %s";
    String time = "10 minutes";
    when(assetProvider.getRelativeElapsedString(anyInt())).thenReturn(time);

    ParameterizedText.Builder text = ParameterizedText.newBuilder();
    text.setText(content);
    text.addParameters(Parameter.newBuilder().setTimestampSeconds(10).build());
    ParameterizedTextEvaluator evaluator = new ParameterizedTextEvaluator();

    assertThat(evaluator.evaluate(frameContext, text.build()))
        .isEqualTo(String.format(content, time));
  }

  @Test
  public void testEvaluate_multipleParameters() {
    String content = "content %s - %s";
    String time1 = "10 minutes";
    String time2 = "20 minutes";
    when(assetProvider.getRelativeElapsedString(anyInt())).thenReturn(time1).thenReturn(time2);

    ParameterizedText.Builder text = ParameterizedText.newBuilder();
    text.setText(content);
    text.addParameters(Parameter.newBuilder().setTimestampSeconds(1).build());
    text.addParameters(Parameter.newBuilder().setTimestampSeconds(2).build());
    ParameterizedTextEvaluator evaluator = new ParameterizedTextEvaluator();

    assertThat(evaluator.evaluate(frameContext, text.build()))
        .isEqualTo(String.format(content, time1, time2));
  }

  @Test
  public void testTimeConversion() {
    String content = "%s";
    String time = "10 minutes";
    when(assetProvider.getRelativeElapsedString(1000)).thenReturn(time);

    ParameterizedText.Builder text = ParameterizedText.newBuilder();
    text.setText(content);
    text.addParameters(Parameter.newBuilder().setTimestampSeconds(1).build());
    ParameterizedTextEvaluator evaluator =
        new ParameterizedTextEvaluator() {
          @Override
          long getCurrentTimeMillis() {
            return 2000L;
          }
        };

    assertThat(evaluator.evaluate(frameContext, text.build()))
        .isEqualTo(String.format(content, time));
  }
}
