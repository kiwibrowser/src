/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.i18n.addressinput.common;

/**
 * A simple implementation of ClientCacheManager which doesn't do any caching on its own.
 */
// This is an external class and part of the widget's public API.
// TODO: Review public API for external classes and tidy JavaDoc.
public final class SimpleClientCacheManager implements ClientCacheManager {
  // URL to get public address data.
  static final String PUBLIC_ADDRESS_SERVER = "https://chromium-i18n.appspot.com/ssl-address";

  @Override
  public String get(String key) {
    return "";
  }

  @Override
  public void put(String key, String data) {
  }

  @Override
  public String getAddressServerUrl() {
    return PUBLIC_ADDRESS_SERVER;
  }
}
