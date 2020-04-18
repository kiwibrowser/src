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

package com.google.android.libraries.feed.host.action;

/**
 * The actions API allows the Feed to query the host as to what actions can be performed and
 * instruct the host to perform such actions. In the case that an action is triggered that is not
 * enabled the host has one of two options:
 *
 * <ol>
 *   <li>The host can ignore the action.
 *   <li>The host can throw an exception which the Stream will not catch, crashing the app.
 * </ol>
 *
 * <p>The host is free to log any incorrectly triggered actions.
 */
public interface ActionApi {

  /** Opens the given URL. */
  void openUrl(String url);

  /** Whether the host can open a URL. */
  boolean canOpenUrl();

  /** Opens the given URL in incognito mode. */
  void openUrlInIncognitoMode(String url);

  /** Whether the host can open a URL in incognito mode. */
  boolean canOpenUrlInIncognitoMode();

  /** Opens the given URL in a new tab. */
  void openUrlInNewTab(String url);

  /** Whether the host can open a URL in a new tab. */
  boolean canOpenUrlInNewTab();

  /** Opens the given URL in a new window. */
  void openUrlInNewWindow(String url);

  /** Whether the host can open a URL in a new window. */
  boolean canOpenUrlInNewWindow();

  /** Downloads the given url. */
  void downloadUrl(String url);

  /** Whether the host can download a URL. */
  boolean canDownloadUrl();

  /** Opens the Google Product Help page for the Feed. */
  void learnMore();

  /** Whether the host can open the Google Product Help page. */
  boolean canLearnMore();
}
