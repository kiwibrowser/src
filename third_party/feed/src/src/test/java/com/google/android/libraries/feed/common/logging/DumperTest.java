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

package com.google.android.libraries.feed.common.logging;

import static com.google.common.truth.Truth.assertThat;

import com.google.android.libraries.feed.common.logging.Dumper.DumperValue;
import java.io.StringWriter;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link Dumper}. */
@RunWith(RobolectricTestRunner.class)
public class DumperTest {
  private static final String KEY1 = "keyOne";
  private static final String KEY2 = "keyTwo";
  private static final String KEY3 = "keyThree";

  @Test
  public void testBaseDumper() {
    Dumper dumper = Dumper.newDefaultDumper();
    dumper.forKey(KEY1).value(13);
    assertThat(dumper.values).hasSize(1);
    DumperValue dumperValue = dumper.values.get(0);
    assertThat(dumperValue).isNotNull();
    assertThat(dumperValue.name).isEqualTo(KEY1);
    assertThat(dumperValue.content.toString()).isEqualTo(Integer.toString(13));
    assertThat(dumperValue.indentLevel).isEqualTo(1);
  }

  @Test
  public void testBaseDumper_childDumper() {
    Dumper dumper = Dumper.newDefaultDumper();
    dumper.forKey(KEY1).value(13);
    assertThat(dumper.values).hasSize(1);

    Dumper childDumper = dumper.getChildDumper();
    childDumper.forKey(KEY2).value(17);

    assertThat(childDumper.values).isEqualTo(dumper.values);
    assertThat(childDumper.values).hasSize(2);

    DumperValue dumperValue = dumper.values.get(1);
    assertThat(dumperValue).isNotNull();
    assertThat(dumperValue.name).isEqualTo(KEY2);
    assertThat(dumperValue.content.toString()).isEqualTo(Integer.toString(17));
    assertThat(dumperValue.indentLevel).isEqualTo(2);
  }

  @Test
  public void testDumpable() {
    Dumper dumper = Dumper.newDefaultDumper();
    TestDumpable testDumpable = new TestDumpable(KEY1);
    dumper.dump(testDumpable);
    testDumpable.assertValue(dumper.values.get(0));
  }

  @Test
  public void testDumpable_nested() {
    Dumper dumper = Dumper.newDefaultDumper();
    TestDumpable dumpableChild = new TestDumpable(KEY1);
    TestDumpable dumpable = new TestDumpable(dumpableChild, KEY2);
    dumper.dump(dumpable);

    assertThat(dumper.values).hasSize(2);
    dumpable.assertValue(dumper.values.get(0));
    dumpableChild.assertValue(dumper.values.get(1));
  }

  @Test
  public void testDumpable_cycle() {
    Dumper dumper = Dumper.newDefaultDumper();
    TestDumpable dumpableChild = new TestDumpable(KEY1);
    TestDumpable dumpable = new TestDumpable(dumpableChild, KEY2);
    dumpableChild.setDumpable(dumpable);

    dumper.dump(dumpable);

    // Two dumpables + the a message that statesa cycle was detected
    assertThat(dumper.values).hasSize(3);
    dumpable.assertValue(dumper.values.get(0));
    dumpableChild.assertValue(dumper.values.get(1));
  }

  @Test
  public void testDumpable_threeChildCycle() {
    Dumper dumper = Dumper.newDefaultDumper();
    TestDumpable dumpableThree = new TestDumpable(KEY1);
    TestDumpable dumpableTwo = new TestDumpable(dumpableThree, KEY2);
    TestDumpable dumpableOne = new TestDumpable(dumpableTwo, KEY3);
    dumpableThree.setDumpable(dumpableOne);

    dumper.dump(dumpableOne);

    // 3 dumpables + the a message that states cycle was detected
    assertThat(dumper.values).hasSize(4);
  }

  @Test
  public void testWrite() throws Exception {
    Dumper dumper = Dumper.newDefaultDumper();
    TestDumpable testDumpable = new TestDumpable(KEY1);
    dumper.dump(testDumpable);
    StringWriter stringWriter = new StringWriter();
    dumper.write(stringWriter);
    String dump = stringWriter.getBuffer().toString();
    assertThat(dump).contains(KEY1);
    assertThat(dump).contains("13");
  }

  @Test
  public void testWrite_compact() throws Exception {
    Dumper dumper = Dumper.newDefaultDumper();
    dumper.forKey(KEY1).value("valueOne");
    dumper.forKey(KEY2).value("valueTwo").compactPrevious();
    StringWriter stringWriter = new StringWriter();
    dumper.write(stringWriter);
    String dump = stringWriter.getBuffer().toString();
    assertThat(dump).contains(" | ");
  }

  private static class TestDumpable implements Dumpable {

    /*@Nullable*/ private Dumpable child;
    private final String key;

    TestDumpable(String key) {
      this(null, key);
    }

    TestDumpable(/*@Nullable*/ Dumpable child, String key) {
      this.child = child;
      this.key = key;
    }

    void setDumpable(Dumpable child) {
      this.child = child;
    }

    @Override
    public void dump(Dumper dumper) {
      dumper.forKey(key).value(13);
      dumper.dump(child);
    }

    void assertValue(DumperValue value) {
      assertThat(value.name).isEqualTo(key);
      assertThat(value.content.toString()).isEqualTo(Integer.toString(13));
    }
  }
}
