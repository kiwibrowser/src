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
package com.google.android.libraries.feed.feedactionreader;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyList;
import static org.mockito.Matchers.same;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.actionmanager.ActionReader;
import com.google.android.libraries.feed.api.common.DismissActionWithSemanticProperties;
import com.google.android.libraries.feed.api.common.SemanticPropertiesWithId;
import com.google.android.libraries.feed.api.common.testing.ContentIdGenerators;
import com.google.android.libraries.feed.api.protocoladapter.ProtocolAdapter;
import com.google.android.libraries.feed.api.store.ActionMutation.ActionType;
import com.google.android.libraries.feed.api.store.Store;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.testing.WireProtocolResponseBuilder;
import com.google.android.libraries.feed.common.time.Clock;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.Configuration.ConfigKey;
import com.google.search.now.feed.client.StreamDataProto.StreamAction;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.TimeUnit;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link FeedActionReader} class. */
@RunWith(RobolectricTestRunner.class)
public class FeedActionReaderTest {

  public static final ContentIdGenerators ID_GENERATOR = new ContentIdGenerators();

  public static final ContentId CONTENT_ID = WireProtocolResponseBuilder.createFeatureContentId(1);
  public static final String CONTENT_ID_STRING = ID_GENERATOR.createContentId(CONTENT_ID);
  public static final ContentId CONTENT_ID_2 =
      WireProtocolResponseBuilder.createFeatureContentId(2);
  public static final String CONTENT_ID_STRING_2 = ID_GENERATOR.createContentId(CONTENT_ID_2);
  public static final long DEFAULT_TIME = TimeUnit.DAYS.toSeconds(42);

  @Mock private Store store;
  @Mock private Clock clock;
  @Mock private ProtocolAdapter protocolAdapter;
  @Mock private Configuration configuration;

  private ActionReader actionReader;

  @Before
  public void setUp() throws Exception {
    initMocks(this);

    when(configuration.getValueOrDefault(same(ConfigKey.DEFAULT_ACTION_TTL_SECONDS), any()))
        .thenReturn(TimeUnit.DAYS.toSeconds(3));

    actionReader = new FeedActionReader(store, clock, protocolAdapter, configuration);

    when(protocolAdapter.getWireContentId(CONTENT_ID_STRING))
        .thenReturn(Result.success(CONTENT_ID));
    when(protocolAdapter.getWireContentId(CONTENT_ID_STRING_2))
        .thenReturn(Result.success(CONTENT_ID_2));
  }

  @Test
  public void getAllDismissedActions() {
    when(clock.currentTimeMillis()).thenReturn(DEFAULT_TIME);

    StreamAction dismissAction = buildDismissAction(CONTENT_ID_STRING);
    mockStoreCalls(Collections.singletonList(dismissAction), Collections.emptyList());

    Result<List<DismissActionWithSemanticProperties>> dismissActionsResult =
        actionReader.getDismissActionsWithSemanticProperties();
    assertThat(dismissActionsResult.isSuccessful()).isTrue();
    List<DismissActionWithSemanticProperties> dismissActions = dismissActionsResult.getValue();
    assertThat(dismissActions).hasSize(1);
    assertThat(dismissActions.get(0).getContentId()).isEqualTo(CONTENT_ID);
    assertThat(dismissActions.get(0).getSemanticProperties()).isNull();
  }

  @Test
  public void getAllDismissedActions_storeError_getAllDismissActions() {
    when(clock.currentTimeMillis()).thenReturn(DEFAULT_TIME);
    when(store.getAllDismissActions()).thenReturn(Result.failure());

    Result<List<DismissActionWithSemanticProperties>> dismissActionsResult =
        actionReader.getDismissActionsWithSemanticProperties();
    assertThat(dismissActionsResult.isSuccessful()).isFalse();
  }

  @Test
  public void getAllDismissedActions_storeError_getSemanticProperties() {
    when(clock.currentTimeMillis()).thenReturn(DEFAULT_TIME);
    StreamAction dismissAction = buildDismissAction(CONTENT_ID_STRING);
    when(store.getAllDismissActions())
        .thenReturn(Result.success(Collections.singletonList(dismissAction)));
    when(store.getSemanticProperties(anyList())).thenReturn(Result.failure());

    Result<List<DismissActionWithSemanticProperties>> dismissActionsResult =
        actionReader.getDismissActionsWithSemanticProperties();
    assertThat(dismissActionsResult.isSuccessful()).isFalse();
  }

  @Test
  public void getAllDismissedActions_expired() {
    when(clock.currentTimeMillis())
        .thenReturn(TimeUnit.SECONDS.toMillis(DEFAULT_TIME) + TimeUnit.DAYS.toMillis(3));

    StreamAction dismissAction = buildDismissAction(CONTENT_ID_STRING);
    mockStoreCalls(Collections.singletonList(dismissAction), Collections.emptyList());

    Result<List<DismissActionWithSemanticProperties>> dismissActionsResult =
        actionReader.getDismissActionsWithSemanticProperties();
    assertThat(dismissActionsResult.isSuccessful()).isTrue();
    List<DismissActionWithSemanticProperties> dismissActions = dismissActionsResult.getValue();
    assertThat(dismissActions).hasSize(0);
  }

  @Test
  public void getAllDismissedActions_semanticProperties() {
    when(clock.currentTimeMillis()).thenReturn(DEFAULT_TIME);

    StreamAction dismissAction = buildDismissAction(CONTENT_ID_STRING);
    byte[] semanticData = {12, 41};
    mockStoreCalls(
        Collections.singletonList(dismissAction),
        Collections.singletonList(new SemanticPropertiesWithId(CONTENT_ID_STRING, semanticData)));

    Result<List<DismissActionWithSemanticProperties>> dismissActionsResult =
        actionReader.getDismissActionsWithSemanticProperties();
    assertThat(dismissActionsResult.isSuccessful()).isTrue();
    List<DismissActionWithSemanticProperties> dismissActions = dismissActionsResult.getValue();
    assertThat(dismissActions).hasSize(1);
    assertThat(dismissActions.get(0).getContentId()).isEqualTo(CONTENT_ID);
    assertThat(dismissActions.get(0).getSemanticProperties()).isEqualTo(semanticData);
  }

  @Test
  public void getAllDismissedActions_multipleActions() {
    when(clock.currentTimeMillis()).thenReturn(DEFAULT_TIME);

    StreamAction dismissAction = buildDismissAction(CONTENT_ID_STRING);
    StreamAction dismissAction2 = buildDismissAction(CONTENT_ID_STRING_2);
    mockStoreCalls(Arrays.asList(dismissAction, dismissAction2), Collections.emptyList());

    Result<List<DismissActionWithSemanticProperties>> dismissActionsResult =
        actionReader.getDismissActionsWithSemanticProperties();
    assertThat(dismissActionsResult.isSuccessful()).isTrue();
    List<DismissActionWithSemanticProperties> dismissActions = dismissActionsResult.getValue();

    assertThat(dismissActions).hasSize(2);
    assertThat(dismissActions.get(0).getContentId()).isEqualTo(CONTENT_ID);
    assertThat(dismissActions.get(0).getSemanticProperties()).isNull();
    assertThat(dismissActions.get(1).getContentId()).isEqualTo(CONTENT_ID_2);
    assertThat(dismissActions.get(1).getSemanticProperties()).isNull();
  }

  @Test
  public void getAllDismissedActions_multipleActions_semanticProperties() {
    when(clock.currentTimeMillis()).thenReturn(DEFAULT_TIME);

    StreamAction dismissAction = buildDismissAction(CONTENT_ID_STRING);
    StreamAction dismissAction2 = buildDismissAction(CONTENT_ID_STRING_2);
    byte[] semanticData = {12, 41};
    byte[] semanticData2 = {42, 72};
    mockStoreCalls(
        Arrays.asList(dismissAction, dismissAction2),
        Arrays.asList(
            new SemanticPropertiesWithId(CONTENT_ID_STRING, semanticData),
            new SemanticPropertiesWithId(CONTENT_ID_STRING_2, semanticData2)));

    Result<List<DismissActionWithSemanticProperties>> dismissActionsResult =
        actionReader.getDismissActionsWithSemanticProperties();
    assertThat(dismissActionsResult.isSuccessful()).isTrue();
    List<DismissActionWithSemanticProperties> dismissActions = dismissActionsResult.getValue();

    assertThat(dismissActions).hasSize(2);
    assertThat(dismissActions.get(0).getContentId()).isEqualTo(CONTENT_ID);
    assertThat(dismissActions.get(0).getSemanticProperties()).isEqualTo(semanticData);
    assertThat(dismissActions.get(1).getContentId()).isEqualTo(CONTENT_ID_2);
    assertThat(dismissActions.get(1).getSemanticProperties()).isEqualTo(semanticData2);
  }

  @Test
  public void getAllDismissedActions_multipleActions_someExpired() {
    when(clock.currentTimeMillis())
        .thenReturn(TimeUnit.SECONDS.toMillis(DEFAULT_TIME) + TimeUnit.DAYS.toMillis(3));

    StreamAction dismissAction = buildDismissAction(CONTENT_ID_STRING);
    StreamAction dismissAction2 =
        StreamAction.newBuilder()
            .setAction(ActionType.DISMISS)
            .setFeatureContentId(CONTENT_ID_STRING_2)
            .setTimestampSeconds(DEFAULT_TIME + TimeUnit.DAYS.toSeconds(2))
            .build();
    mockStoreCalls(Arrays.asList(dismissAction, dismissAction2), Collections.emptyList());

    Result<List<DismissActionWithSemanticProperties>> dismissActionsResult =
        actionReader.getDismissActionsWithSemanticProperties();
    assertThat(dismissActionsResult.isSuccessful()).isTrue();
    List<DismissActionWithSemanticProperties> dismissActions = dismissActionsResult.getValue();

    assertThat(dismissActions).hasSize(1);
    assertThat(dismissActions.get(0).getContentId()).isEqualTo(CONTENT_ID_2);
    assertThat(dismissActions.get(0).getSemanticProperties()).isNull();
  }

  private StreamAction buildDismissAction(String contentId) {
    return StreamAction.newBuilder()
        .setAction(ActionType.DISMISS)
        .setFeatureContentId(contentId)
        .setTimestampSeconds(DEFAULT_TIME)
        .build();
  }

  private void mockStoreCalls(
      List<StreamAction> dismissActions, List<SemanticPropertiesWithId> semanticProperties) {
    when(store.getAllDismissActions()).thenReturn(Result.success(dismissActions));
    when(store.getSemanticProperties(anyList())).thenReturn(Result.success(semanticProperties));
  }
}
