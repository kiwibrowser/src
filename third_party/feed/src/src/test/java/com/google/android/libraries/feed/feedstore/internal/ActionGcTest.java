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
package com.google.android.libraries.feed.feedstore.internal;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.store.ActionMutation.ActionType;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.host.storage.JournalMutation;
import com.google.android.libraries.feed.host.storage.JournalOperation;
import com.google.android.libraries.feed.host.storage.JournalOperation.Append;
import com.google.android.libraries.feed.host.storage.JournalOperation.Type;
import com.google.android.libraries.feed.host.storage.JournalStorage;
import com.google.search.now.feed.client.StreamDataProto.StreamAction;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.TimeUnit;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link ActionGc} class. */
@RunWith(RobolectricTestRunner.class)
public class ActionGcTest {

  private static final String DISMISS_JOURNAL_NAME = "DISMISS";
  private static final String CONTENT_ID_1 = "contentId1";
  private static final String CONTENT_ID_2 = "contentId2";

  @Mock private JournalStorage journalStorage;
  private MainThreadRunner mainThreadRunner = new MainThreadRunner();
  private TimingUtils timingUtils = new TimingUtils();

  @Before
  public void setUp() throws Exception {
    initMocks(this);
  }

  @Test
  public void gc_empty() throws Exception {
    // Probably won't be called with empty actions
    List<StreamAction> allActions = Collections.emptyList();
    List<String> validContentIds = Collections.emptyList();

    ArgumentCaptor<JournalMutation> journalMutationCaptor =
        ArgumentCaptor.forClass(JournalMutation.class);

    ActionGc actionGc =
        new ActionGc(
            allActions,
            validContentIds,
            journalStorage,
            mainThreadRunner,
            timingUtils,
            DISMISS_JOURNAL_NAME);
    actionGc.gc();

    verify(journalStorage).commit(journalMutationCaptor.capture(), any(Consumer.class));

    JournalMutation journalMutation = journalMutationCaptor.getValue();
    assertThat(journalMutation.getJournalName()).isEqualTo(DISMISS_JOURNAL_NAME);
    List<JournalOperation> journalOperations = journalMutation.getOperations();
    assertThat(journalOperations).hasSize(1);
    assertThat(journalOperations.get(0).getType()).isEqualTo(Type.DELETE);
  }

  @Test
  public void gc_allValid() throws Exception {
    StreamAction action1 =
        StreamAction.newBuilder()
            .setAction(ActionType.DISMISS)
            .setFeatureContentId(CONTENT_ID_1)
            .setTimestampSeconds(TimeUnit.DAYS.toSeconds(43))
            .build();
    StreamAction action2 =
        StreamAction.newBuilder()
            .setAction(ActionType.DISMISS)
            .setFeatureContentId(CONTENT_ID_2)
            .setTimestampSeconds(TimeUnit.DAYS.toSeconds(44))
            .build();
    List<StreamAction> allActions = Arrays.asList(action1, action2);
    List<String> validContentIds = Arrays.asList(CONTENT_ID_1, CONTENT_ID_2);

    ArgumentCaptor<JournalMutation> journalMutationCaptor =
        ArgumentCaptor.forClass(JournalMutation.class);

    ActionGc actionGc =
        new ActionGc(
            allActions,
            validContentIds,
            journalStorage,
            mainThreadRunner,
            timingUtils,
            DISMISS_JOURNAL_NAME);
    actionGc.gc();

    verify(journalStorage).commit(journalMutationCaptor.capture(), any(Consumer.class));

    JournalMutation journalMutation = journalMutationCaptor.getValue();
    assertThat(journalMutation.getJournalName()).isEqualTo(DISMISS_JOURNAL_NAME);
    List<JournalOperation> journalOperations = journalMutation.getOperations();
    assertThat(journalOperations).hasSize(3);
    assertThat(journalOperations.get(0).getType()).isEqualTo(Type.DELETE);
    assertThat(journalOperations.get(1).getType()).isEqualTo(Type.APPEND);
    assertThat(((Append) journalOperations.get(1)).getValue()).isEqualTo(action1.toByteArray());
    assertThat(journalOperations.get(2).getType()).isEqualTo(Type.APPEND);
    assertThat(((Append) journalOperations.get(2)).getValue()).isEqualTo(action2.toByteArray());
  }

  @Test
  public void gc_allInvalid() throws Exception {
    StreamAction action1 =
        StreamAction.newBuilder()
            .setAction(ActionType.DISMISS)
            .setFeatureContentId(CONTENT_ID_1)
            .setTimestampSeconds(TimeUnit.DAYS.toSeconds(43))
            .build();
    StreamAction action2 =
        StreamAction.newBuilder()
            .setAction(ActionType.DISMISS)
            .setFeatureContentId(CONTENT_ID_2)
            .setTimestampSeconds(TimeUnit.DAYS.toSeconds(44))
            .build();
    List<StreamAction> allActions = Arrays.asList(action1, action2);
    List<String> validContentIds = Collections.emptyList();

    ArgumentCaptor<JournalMutation> journalMutationCaptor =
        ArgumentCaptor.forClass(JournalMutation.class);

    ActionGc actionGc =
        new ActionGc(
            allActions,
            validContentIds,
            journalStorage,
            mainThreadRunner,
            timingUtils,
            DISMISS_JOURNAL_NAME);
    actionGc.gc();

    verify(journalStorage).commit(journalMutationCaptor.capture(), any(Consumer.class));

    JournalMutation journalMutation = journalMutationCaptor.getValue();
    assertThat(journalMutation.getJournalName()).isEqualTo(DISMISS_JOURNAL_NAME);
    List<JournalOperation> journalOperations = journalMutation.getOperations();
    assertThat(journalOperations).hasSize(1);
    assertThat(journalOperations.get(0).getType()).isEqualTo(Type.DELETE);
  }

  @Test
  public void gc_someValid() throws Exception {
    StreamAction action1 =
        StreamAction.newBuilder()
            .setAction(ActionType.DISMISS)
            .setFeatureContentId(CONTENT_ID_1)
            .setTimestampSeconds(TimeUnit.DAYS.toSeconds(43))
            .build();
    StreamAction action2 =
        StreamAction.newBuilder()
            .setAction(ActionType.DISMISS)
            .setFeatureContentId(CONTENT_ID_2)
            .setTimestampSeconds(TimeUnit.DAYS.toSeconds(44))
            .build();
    List<StreamAction> allActions = Arrays.asList(action1, action2);
    List<String> validContentIds = Collections.singletonList(CONTENT_ID_2);

    ArgumentCaptor<JournalMutation> journalMutationCaptor =
        ArgumentCaptor.forClass(JournalMutation.class);

    ActionGc actionGc =
        new ActionGc(
            allActions,
            validContentIds,
            journalStorage,
            mainThreadRunner,
            timingUtils,
            DISMISS_JOURNAL_NAME);
    actionGc.gc();

    verify(journalStorage).commit(journalMutationCaptor.capture(), any(Consumer.class));

    JournalMutation journalMutation = journalMutationCaptor.getValue();
    assertThat(journalMutation.getJournalName()).isEqualTo(DISMISS_JOURNAL_NAME);
    List<JournalOperation> journalOperations = journalMutation.getOperations();
    assertThat(journalOperations).hasSize(2);
    assertThat(journalOperations.get(0).getType()).isEqualTo(Type.DELETE);
    assertThat(journalOperations.get(1).getType()).isEqualTo(Type.APPEND);
    assertThat(((Append) journalOperations.get(1)).getValue()).isEqualTo(action2.toByteArray());
  }
}
