// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testImageView() {
  var mockFileSystem = new MockFileSystem('volumeId');
  var mockEntry = new MockEntry(mockFileSystem, '/test.jpg');

  // Item has full size cache.
  var itemWithFullCache = new GalleryItem(mockEntry, null, {}, null, false);
  itemWithFullCache.contentImage = document.createElement('canvas');
  assertEquals(
      ImageView.LoadTarget.CACHED_MAIN_IMAGE,
      ImageView.getLoadTarget(itemWithFullCache, new ImageView.Effect.None()));

  // Item with content thumbnail.
  var itemWithContentThumbnail = new GalleryItem(
      mockEntry, null, {thumbnail: {url: 'url'}}, null, false);
  assertEquals(
      ImageView.LoadTarget.THUMBNAIL,
      ImageView.getLoadTarget(
          itemWithContentThumbnail, new ImageView.Effect.None()));

  // Item with external thumbnail.
  var itemWithExternalThumbnail = new GalleryItem(
      mockEntry, null, {external: {thumbnailUrl: 'url'}}, null, false);
  assertEquals(
      ImageView.LoadTarget.THUMBNAIL,
      ImageView.getLoadTarget(
          itemWithExternalThumbnail, new ImageView.Effect.None()));

  // Item with external thumbnail but present localy.
  var itemWithExternalThumbnailPresent = new GalleryItem(
      mockEntry, null, {external: {thumbnailUrl: 'url', present: true}}, null,
      false);
  assertEquals(
      ImageView.LoadTarget.MAIN_IMAGE,
      ImageView.getLoadTarget(
          itemWithExternalThumbnailPresent, new ImageView.Effect.None()));

  // Item with external thumbnail shown by slide effect.
  var itemWithExternalThumbnailSlide = new GalleryItem(
      mockEntry, null, {external: {thumbnailUrl: 'url'}}, null, false);
  assertEquals(
      ImageView.LoadTarget.THUMBNAIL,
      ImageView.getLoadTarget(
          itemWithExternalThumbnailSlide, new ImageView.Effect.Slide(1)));

  // Item with external thumbnail shown by zoom to screen effect.
  var itemWithExternalThumbnailZoomToScreen = new GalleryItem(
      mockEntry, null, {external: {thumbnailUrl: 'url'}}, null, false);
  assertEquals(
      ImageView.LoadTarget.THUMBNAIL,
      ImageView.getLoadTarget(
          itemWithExternalThumbnailZoomToScreen,
          new ImageView.Effect.ZoomToScreen(new ImageRect(0, 0, 100, 100))));

  // Item with external thumbnail shown by zoom effect.
  var itemWithExternalThumbnailZoom = new GalleryItem(
      mockEntry, null, {external: {thumbnailUrl: 'url'}}, null, false);
  assertEquals(
      ImageView.LoadTarget.MAIN_IMAGE,
      ImageView.getLoadTarget(
          itemWithExternalThumbnailZoom,
          new ImageView.Effect.Zoom(0, 0, null)));

  // Item without cache/thumbnail.
  var itemWithoutCacheOrThumbnail = new GalleryItem(
      mockEntry, null, {}, null, false);
  assertEquals(
      ImageView.LoadTarget.MAIN_IMAGE,
      ImageView.getLoadTarget(
          itemWithoutCacheOrThumbnail, new ImageView.Effect.None));
}
