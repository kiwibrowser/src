// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * This file contains typedefs properties for CrNetworkList, shared by
 * CrNetworkListItem.
 */

var CrNetworkList = {};

/**
 * Generic managed property type. This should match any of the basic managed
 * types in chrome.networkingPrivate, e.g. networkingPrivate.ManagedBoolean.
 * @typedef {{
 *   customItemName: string,
 *   polymerIcon: (string|undefined),
 *   customData: (!Object|undefined),
 * }}
 */
CrNetworkList.CustomItemState;

/** @typedef {CrOnc.NetworkStateProperties|CrNetworkList.CustomItemState} */
CrNetworkList.CrNetworkListItemType;
