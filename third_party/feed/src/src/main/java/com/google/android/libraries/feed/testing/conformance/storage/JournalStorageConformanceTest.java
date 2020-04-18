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

package com.google.android.libraries.feed.testing.conformance.storage;

import static com.google.common.truth.Truth.assertThat;

import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.testing.RequiredConsumer;
import com.google.android.libraries.feed.host.storage.CommitResult;
import com.google.android.libraries.feed.host.storage.JournalMutation;
import com.google.android.libraries.feed.host.storage.JournalStorage;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;

/**
 * Conformance test for {@link JournalStorage}. Hosts who wish to test against this should extend
 * this class and set {@code storage} to the Host implementation.
 */
public abstract class JournalStorageConformanceTest {

  private static final String JOURNAL_NAME = "journal name";
  private static final String JOURNAL_COPY_NAME = "journal copy name";
  private static final byte[] DATA_0 = "data 0".getBytes(Charset.forName("UTF-8"));
  private static final byte[] DATA_1 = "data 1".getBytes(Charset.forName("UTF-8"));

  private final Consumer<Result<List<String>>> isJournal1 =
      result -> {
        assertThat(result.isSuccessful()).isTrue();
        List<String> input = result.getValue();
        assertThat(input).hasSize(1);
        assertThat(input).contains(JOURNAL_NAME);
      };
  private final Consumer<Result<List<String>>> isJournal1AndJournal2 =
      result -> {
        assertThat(result.isSuccessful()).isTrue();
        List<String> input = result.getValue();
        assertThat(input).hasSize(2);
        assertThat(input).contains(JOURNAL_NAME);
        assertThat(input).contains(JOURNAL_COPY_NAME);
      };

  private final Consumer<Result<List<byte[]>>> isData0AndData1 =
      result -> {
        assertThat(result.isSuccessful()).isTrue();
        List<byte[]> input = result.getValue();
        // We should get back one byte array, containing the bytes of DATA_0 and DATA_1.
        assertThat(input).isNotNull();
        assertThat(input).hasSize(2);
        assertThat(Arrays.equals(input.get(0), DATA_0)).isTrue();
        assertThat(Arrays.equals(input.get(1), DATA_1)).isTrue();
      };
  private final Consumer<Result<List<byte[]>>> isEmptyList =
      result -> {
        assertThat(result.isSuccessful()).isTrue();
        List<byte[]> input = result.getValue();

        // The result should be an empty List.
        assertThat(input).isNotNull();
        assertThat(input).isEmpty();
      };
  private final Consumer<CommitResult> isSuccess =
      input -> assertThat(input).isEqualTo(CommitResult.SUCCESS);
  private final Consumer<Result<Boolean>> isFalse =
      result -> {
        assertThat(result.isSuccessful()).isTrue();
        Boolean input = result.getValue();
        assertThat(input).isFalse();
      };
  private final Consumer<Result<Boolean>> isTrue =
      result -> {
        assertThat(result.isSuccessful()).isTrue();
        Boolean input = result.getValue();
        assertThat(input).isTrue();
      };

  protected JournalStorage journalStorage;

  @Test
  public void readOfEmptyJournalReturnsEmptyData() throws Exception {
    // Try to read some blobs from an empty journal store.
    RequiredConsumer<Result<List<byte[]>>> readConsumer = new RequiredConsumer<>(isEmptyList);
    journalStorage.read(JOURNAL_NAME, readConsumer);
    assertThat(readConsumer.isCalled()).isTrue();
  }

  @Test
  public void appendToJournal() throws Exception {
    // Write some data and put the result from the callback into commitResult.
    RequiredConsumer<CommitResult> commitConsumer = new RequiredConsumer<>(isSuccess);
    journalStorage.commit(
        new JournalMutation.Builder(JOURNAL_NAME).append(DATA_0).append(DATA_1).build(),
        commitConsumer);
    assertThat(commitConsumer.isCalled()).isTrue();

    // Read the data back into blobs.
    RequiredConsumer<Result<List<byte[]>>> readConsumer = new RequiredConsumer<>(isData0AndData1);
    journalStorage.read(JOURNAL_NAME, readConsumer);
    assertThat(readConsumer.isCalled()).isTrue();
  }

  @Test
  public void copyJournal() throws Exception {
    // Write some data.
    RequiredConsumer<CommitResult> commitConsumer = new RequiredConsumer<>(isSuccess);
    journalStorage.commit(
        new JournalMutation.Builder(JOURNAL_NAME).append(DATA_0).append(DATA_1).build(),
        commitConsumer);
    assertThat(commitConsumer.isCalled()).isTrue();

    // Copy the data into a new journal and put the result from the callback into commitResult.
    commitConsumer = new RequiredConsumer<>(isSuccess);
    journalStorage.commit(
        new JournalMutation.Builder(JOURNAL_NAME).copy(JOURNAL_COPY_NAME).build(), commitConsumer);
    assertThat(commitConsumer.isCalled()).isTrue();

    // Read the data back into blobs.
    RequiredConsumer<Result<List<byte[]>>> readConsumer = new RequiredConsumer<>(isData0AndData1);
    journalStorage.read(JOURNAL_COPY_NAME, readConsumer);
    assertThat(readConsumer.isCalled()).isTrue();
  }

  @Test
  public void deleteJournal() throws Exception {
    // Write some data.
    RequiredConsumer<CommitResult> commitConsumer = new RequiredConsumer<>(isSuccess);
    journalStorage.commit(
        new JournalMutation.Builder(JOURNAL_NAME).append(DATA_0).append(DATA_1).build(),
        commitConsumer);
    assertThat(commitConsumer.isCalled()).isTrue();

    // Delete the journal and put the result from the callback into commitResult.
    commitConsumer = new RequiredConsumer<>(isSuccess);
    journalStorage.commit(
        new JournalMutation.Builder(JOURNAL_NAME).delete().build(), commitConsumer);
    assertThat(commitConsumer.isCalled()).isTrue();

    // Try to read the deleted journal.
    RequiredConsumer<Result<List<byte[]>>> readConsumer = new RequiredConsumer<>(isEmptyList);
    journalStorage.read(JOURNAL_NAME, readConsumer);
    assertThat(readConsumer.isCalled()).isTrue();
  }

  @Test
  public void exists() throws Exception {
    // Write some data.
    RequiredConsumer<CommitResult> commitConsumer = new RequiredConsumer<>(isSuccess);
    journalStorage.commit(
        new JournalMutation.Builder(JOURNAL_NAME).append(DATA_0).append(DATA_1).build(),
        commitConsumer);
    assertThat(commitConsumer.isCalled()).isTrue();

    RequiredConsumer<Result<Boolean>> existsConsumer = new RequiredConsumer<>(isTrue);
    journalStorage.exists(JOURNAL_NAME, existsConsumer);
    assertThat(existsConsumer.isCalled()).isTrue();
  }

  @Test
  public void exists_doesNotExist() throws Exception {
    RequiredConsumer<Result<Boolean>> existsConsumer = new RequiredConsumer<>(isFalse);
    journalStorage.exists(JOURNAL_NAME, existsConsumer);
    assertThat(existsConsumer.isCalled()).isTrue();
  }

  @Test
  public void getAllJournals_singleJournal() throws Exception {
    // Write some data.
    RequiredConsumer<CommitResult> commitConsumer = new RequiredConsumer<>(isSuccess);
    journalStorage.commit(
        new JournalMutation.Builder(JOURNAL_NAME).append(DATA_0).append(DATA_1).build(),
        commitConsumer);
    assertThat(commitConsumer.isCalled()).isTrue();

    RequiredConsumer<Result<List<String>>> getAllJournalsConsumer =
        new RequiredConsumer<>(isJournal1);
    journalStorage.getAllJournals(getAllJournalsConsumer);
    assertThat(getAllJournalsConsumer.isCalled()).isTrue();
  }

  @Test
  public void getAllJournals_multipleJournals() throws Exception {
    // Write some data.
    RequiredConsumer<CommitResult> commitConsumer = new RequiredConsumer<>(isSuccess);
    journalStorage.commit(
        new JournalMutation.Builder(JOURNAL_NAME)
            .append(DATA_0)
            .append(DATA_1)
            .copy(JOURNAL_COPY_NAME)
            .build(),
        commitConsumer);
    assertThat(commitConsumer.isCalled()).isTrue();

    RequiredConsumer<Result<List<String>>> getAllJournalsConsumer =
        new RequiredConsumer<>(isJournal1AndJournal2);
    journalStorage.getAllJournals(getAllJournalsConsumer);
    assertThat(getAllJournalsConsumer.isCalled()).isTrue();
  }
}
