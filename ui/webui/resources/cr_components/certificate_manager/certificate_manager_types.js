// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Closure compiler typedefs.
 */

/**
 * The payload of the 'certificate-action' event.
 * @typedef {{
 *   action: !CertificateAction,
 *   subnode: (null|CertificateSubnode|NewCertificateSubNode),
 *   certificateType: !CertificateType,
 *   anchor: !HTMLElement
 * }}
 */
var CertificateActionEventDetail;

/**
 * The payload of the 'certificates-error' event.
 * @typedef {{
 *   error: (null|CertificatesError|CertificatesImportError),
 *   anchor: ?HTMLElement
 * }}
 */
var CertificatesErrorEventDetail;

/**
 * Enumeration of actions that require a popup menu to be shown to the user.
 * @enum {number}
 */
var CertificateAction = {
  DELETE: 0,
  EDIT: 1,
  EXPORT_PERSONAL: 2,
  IMPORT: 3,
};

/**
 * The name of the event fired when a certificate action is selected from the
 * dropdown menu. CertificateActionEventDetail is passed as the event detail.
 */
var CertificateActionEvent = 'certificate-action';
