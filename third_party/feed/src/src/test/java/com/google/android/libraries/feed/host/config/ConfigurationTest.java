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

package com.google.android.libraries.feed.host.config;

import static com.google.common.truth.Truth.assertThat;
import static junit.framework.Assert.fail;

import com.google.android.libraries.feed.host.config.Configuration.ConfigKey;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link Configuration}. */
@RunWith(RobolectricTestRunner.class)
public class ConfigurationTest {

  @ConfigKey private static final String CONFIG_KEY_WITH_DEFAULT = "config-test";

  private static final boolean CONFIG_KEY_WITH_DEFAULT_VALUE = false;

  @Test
  public void defaultConfig_hasDefaultValues() {
    Configuration configuration =
        new Configuration.Builder()
            .put(CONFIG_KEY_WITH_DEFAULT, CONFIG_KEY_WITH_DEFAULT_VALUE)
            .build();

    boolean valueOrDefault =
        configuration.getValueOrDefault(CONFIG_KEY_WITH_DEFAULT, !CONFIG_KEY_WITH_DEFAULT_VALUE);

    assertThat(valueOrDefault).isEqualTo(CONFIG_KEY_WITH_DEFAULT_VALUE);
  }

  @Test
  public void hasValue_forValueThatDoesNotExist_returnsFalse() {
    Configuration configuration = new Configuration.Builder().build();

    assertThat(configuration.hasValue(ConfigKey.FEED_SERVER_HOST)).isFalse();
  }

  @Test
  public void getValueOrDefault_forValueThatDoesNotExist_returnsSpecifiedDefault() {
    final String defaultString = "defaultString";
    Configuration configuration = new Configuration.Builder().build();

    assertThat(configuration.getValueOrDefault(ConfigKey.FEED_SERVER_HOST, defaultString))
        .isEqualTo(defaultString);
  }

  @Test
  public void getValueOrDefault_forValueThatExists_returnsValue() {
    final String someValue = "someValue";
    Configuration configuration =
        new Configuration.Builder().put(ConfigKey.FEED_SERVER_HOST, someValue).build();

    assertThat(configuration.getValueOrDefault(ConfigKey.FEED_SERVER_HOST, ""))
        .isEqualTo(someValue);
  }

  @Test
  public void getValueOrDefaultWithWrongType_throwsClassCastException() {
    Configuration configuration =
        new Configuration.Builder().put(ConfigKey.FEED_SERVER_HOST, "someString").build();

    try {
      @SuppressWarnings("unused") // Used for type inference
      Boolean ignored = configuration.getValueOrDefault(ConfigKey.FEED_SERVER_HOST, false);
      fail();
    } catch (ClassCastException ignored) {
      // expected
    }
  }

  @Test
  public void getValue_forValueThatWasOverridden_ReturnsOverriddenValue() {
    Configuration configuration =
        new Configuration.Builder()
            .put(CONFIG_KEY_WITH_DEFAULT, !CONFIG_KEY_WITH_DEFAULT_VALUE)
            .build();

    boolean valueOrDefault =
        configuration.getValueOrDefault(CONFIG_KEY_WITH_DEFAULT, !CONFIG_KEY_WITH_DEFAULT_VALUE);

    assertThat(valueOrDefault).isEqualTo(!CONFIG_KEY_WITH_DEFAULT_VALUE);
  }
}
