// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * The container of the VolumeInfo for each mounted volume.
 * @interface
 */
function VolumeInfoList() {}

/** @type {number} */
VolumeInfoList.prototype.length;

/**
 * Adds the event listener to listen the change of volume info.
 * @param {string} type The name of the event.
 * @param {function(Event)} handler The handler for the event.
 */
VolumeInfoList.prototype.addEventListener = function(type, handler) {};

/**
 * Removes the event listener.
 * @param {string} type The name of the event.
 * @param {function(Event)} handler The handler to be removed.
 */
VolumeInfoList.prototype.removeEventListener = function(type, handler) {};

/**
 * Adds the volumeInfo to the appropriate position. If there already exists,
 * just replaces it.
 * @param {VolumeInfo} volumeInfo The information of the new volume.
 */
VolumeInfoList.prototype.add = function(volumeInfo) {};

/**
 * Removes the VolumeInfo having the given ID.
 * @param {string} volumeId ID of the volume.
 */
VolumeInfoList.prototype.remove = function(volumeId) {};

/**
 * Obtains an index from the volume ID.
 * @param {string} volumeId Volume ID.
 * @return {number} Index of the volume.
 */
VolumeInfoList.prototype.findIndex = function(volumeId) {};

/**
 * Searches the information of the volume that contains the passed entry.
 * @param {!Entry|!FakeEntry} entry Entry on the volume to be found.
 * @return {VolumeInfo} The volume's information, or null if not found.
 */
VolumeInfoList.prototype.findByEntry = function(entry) {};

/**
 * Searches the information of the volume that exists on the given device path.
 * @param {string} devicePath Path of the device to search.
 * @return {VolumeInfo} The volume's information, or null if not found.
 */
VolumeInfoList.prototype.findByDevicePath = function(devicePath) {};

/**
 * Returns a VolumInfo for the volume ID, or null if not found.
 *
 * @param {string} volumeId
 * @return {VolumeInfo} The volume's information, or null if not found.
 */
VolumeInfoList.prototype.findByVolumeId = function(volumeId) {};

/**
 * Returns a promise that will be resolved when volume info, identified
 * by {@code volumeId} is created.
 *
 * @param {string} volumeId
 * @return {!Promise<!VolumeInfo>} The VolumeInfo. Will not resolve
 *     if the volume is never mounted.
 */
VolumeInfoList.prototype.whenVolumeInfoReady = function(volumeId) {};

/**
 * @param {number} index The index of the volume in the list.
 * @return {!VolumeInfo} The VolumeInfo instance.
 */
VolumeInfoList.prototype.item = function(index) {};
