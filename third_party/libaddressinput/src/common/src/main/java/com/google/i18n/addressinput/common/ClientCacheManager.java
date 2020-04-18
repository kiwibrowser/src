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
 * Used by AddressWidget to handle caching in client-specific ways.
 */
// This is an external class and part of the widget's public API.
// TODO: Review public API for external classes and tidy JavaDoc.
public interface ClientCacheManager {
  /** Get the data that is cached for the given key. */
  public String get(String key);
  /** Put the data for the given key into the cache. */
  public void put(String key, String data);
  /** Get the URL of the server that serves address metadata. */
  public String getAddressServerUrl();
}
