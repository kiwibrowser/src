// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Wallpaper file system quota.
 */
/** @const */ var WallpaperQuota = 1024 * 1024 * 100;

var wallpaperDirectories = null;

/**
 * Manages custom wallpaper related directories in wallpaper's sandboxed
 * FileSystem.
 * @constructor
 */
function WallpaperDirectories() {
  this.wallpaperDirs_ = {};
  this.wallpaperDirs_[Constants.WallpaperDirNameEnum.ORIGINAL] = null;
  this.wallpaperDirs_[Constants.WallpaperDirNameEnum.THUMBNAIL] = null;
}

/**
 * Gets WallpaperDirectories instance. In case is hasn't been initialized, a new
 * instance is created.
 * @return {WallpaperDirectories} A WallpaperDirectories instance.
 */
WallpaperDirectories.getInstance = function() {
  if (wallpaperDirectories === null)
    wallpaperDirectories = new WallpaperDirectories();
  return wallpaperDirectories;
};

WallpaperDirectories.prototype = {
  /**
   * Returns all custom wallpaper related directory entries.
   */
  get wallpaperDirs() {
    return this.wallpaperDirs_;
  },

  /**
   * If dirName is not requested, gets the directory entry of dirName and cache
   * the result. Calls success callback if success.
   * @param {string} dirName The directory name of requested directory entry.
   * @param {function(DirectoryEntry):void} success Call success with requested
   *     DirectoryEntry.
   * @param {function(e):void} failure Call failure when failed to get the
   *     requested directory.
   */
  requestDir: function(dirName, success, failure) {
    if (dirName != Constants.WallpaperDirNameEnum.ORIGINAL &&
        dirName != Constants.WallpaperDirNameEnum.THUMBNAIL) {
      console.error('Error: Unknow directory name.');
      var e = new Error();
      e.code = FileError.NOT_FOUND_ERR;
      failure(e);
      return;
    }
    var self = this;
    window.webkitRequestFileSystem(
        window.PERSISTENT, WallpaperQuota, function(fs) {
          fs.root.getDirectory(dirName, {create: true}, function(dirEntry) {
            self.wallpaperDirs_[dirName] = dirEntry;
            success(dirEntry);
          }, failure);
        }, failure);
  },

  /**
   * Gets DirectoryEntry associated with dirName from cache. If not in cache try
   * to request it from FileSystem.
   * @param {string} dirName The directory name of requested directory entry.
   * @param {function(DirectoryEntry):void} success Call success with requested
   *     DirectoryEntry.
   * @param {function(e):void} failure Call failure when failed to get the
   *     requested directory.
   */
  getDirectory: function(dirName, success, failure) {
    if (this.wallpaperDirs[dirName])
      success(this.wallpaperDirs[dirName]);
    else
      this.requestDir(dirName, success, failure);
  }
};
