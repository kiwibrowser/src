// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testShouldBeDisabled() {
  // Disabled when mode is not set.
  assertTrue(DimmableUIController.shouldBeDisabled(
      undefined /* mode */, undefined /* subMode */, false /* loading */,
      false /* spokenFeedbackEnabled */, false /* renaming */));

  // Disabled in thumbnail mode.
  assertTrue(DimmableUIController.shouldBeDisabled(
      GalleryMode.THUMBNAIL, GallerySubMode.BROWSE, false /* loading */,
      false /* spokenFeedbackEnabled */, false /* renaming */));

  // Disabled in edit mode.
  assertTrue(DimmableUIController.shouldBeDisabled(
      GalleryMode.SLIDE, GallerySubMode.EDIT, false /* loading*/,
      false /* spokenFeedbackEnabled */, false /* renaming */));

  // Shouldn't be disabled while browsing in slide mode.
  assertFalse(DimmableUIController.shouldBeDisabled(
      GalleryMode.SLIDE, GallerySubMode.BROWSE, false /* loading */,
      false /* spokenFeedbackEnabled */, false /* renaming */));

  // Disabled while loading an image in slide mode.
  assertTrue(DimmableUIController.shouldBeDisabled(
      GalleryMode.SLIDE, GallerySubMode.BROWSE, true /* loading */,
      false /* spokenFeedbackEnabled */, false /* renaming */));

  // Disabled when spoken feedback is enabled.
  assertTrue(DimmableUIController.shouldBeDisabled(
      GalleryMode.SLIDE, GallerySubMode.BROWSE, false /* loading */,
      true /* spokenFeedbackEnabled */, false /* renaming */));

  // Disabled when user is renaming an image.
  assertTrue(DimmableUIController.shouldBeDisabled(
      GalleryMode.SLIDE, GallerySubMode.BROWSE, false /* loading */,
      false /* spokenFeedbackEnabled */, true /* renaming */))
}
