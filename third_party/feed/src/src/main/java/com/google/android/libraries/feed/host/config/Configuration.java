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

import android.support.annotation.StringDef;
import java.util.HashMap;

/**
 * Contains an immutable collection of {@link ConfigKey} {@link String}, {@link Object} pairs.
 *
 * <p>Note: this class should not be mocked. Use the {@link Builder} instead.
 */
public class Configuration {
  /** A unique string identifier for a config value */
  @StringDef({
    ConfigKey.FEED_SERVER_HOST,
    ConfigKey.FEED_SERVER_PATH_AND_PARAMS,
    ConfigKey.SESSION_LIFETIME_MS,
    ConfigKey.MOCK_SERVER_DELAY_MS,
    ConfigKey.INITIAL_NON_CACHED_PAGE_SIZE,
    ConfigKey.NON_CACHED_PAGE_SIZE,
    ConfigKey.NON_CACHED_MIN_PAGE_SIZE,
    ConfigKey.DEFAULT_ACTION_TTL_SECONDS,
    ConfigKey.TRIGGER_IMMEDIATE_PAGINATION,
    ConfigKey.MINIMUM_VALID_ACTION_RATIO,
  })
  public @interface ConfigKey {
    String FEED_SERVER_HOST = "feed_server_host";
    String FEED_SERVER_PATH_AND_PARAMS = "feed_server_path_and_params";
    String SESSION_LIFETIME_MS = "session_lifetime_ms";
    String MOCK_SERVER_DELAY_MS = "mock_server_delay_ms";
    String INITIAL_NON_CACHED_PAGE_SIZE = "initial_non_cached_page_size";
    String NON_CACHED_PAGE_SIZE = "non_cached_page_size";
    String NON_CACHED_MIN_PAGE_SIZE = "non_cached_min_page_size";
    String DEFAULT_ACTION_TTL_SECONDS = "default_action_ttl_seconds";
    String TRIGGER_IMMEDIATE_PAGINATION = "trigger_immediate_pagination_bool";
    String MINIMUM_VALID_ACTION_RATIO = "minimum_valid_action_ratio";
  }

  private final HashMap<String, Object> values;

  private Configuration(HashMap<String, Object> values) {
    this.values = values;
  }

  /**
   * Returns the value if it exists, or {@code defaultValue} otherwise.
   *
   * @throws ClassCastException if the value can't be cast to {@code T}.
   */
  public <T> T getValueOrDefault(String key, T defaultValue) {
    if (values.containsKey(key)) {
      // The caller assumes the responsibility of ensuring this cast succeeds
      @SuppressWarnings("unchecked")
      T castedValue = (T) values.get(key);
      return castedValue;
    } else {
      return defaultValue;
    }
  }

  /** Returns true if a value exists for the {@code key}. */
  public boolean hasValue(String key) {
    return values.containsKey(key);
  }

  /** Builder class used to create {@link Configuration} objects. */
  public static class Builder {
    private final HashMap<String, Object> values = new HashMap<>();

    public Builder put(@ConfigKey String key, Object value) {
      values.put(key, value);
      return this;
    }

    public Configuration build() {
      return new Configuration(values);
    }
  }
}
