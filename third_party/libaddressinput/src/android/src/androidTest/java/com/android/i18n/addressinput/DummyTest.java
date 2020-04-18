package com.android.i18n.addressinput;

import android.test.ActivityInstrumentationTestCase2;
import com.android.i18n.addressinput.testing.TestActivity;

/**
 * Empty test file. The API level 19 test emulator requires a nonempty dex to run tests.
 * This empty file is included in the srcs attribute of the android_test rule so that the real test
 * source files can be included via the binary_under_test attribute.
 */
public class DummyTest extends ActivityInstrumentationTestCase2<TestActivity> {
  public DummyTest() {
    super(TestActivity.class);
  }
}
