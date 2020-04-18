// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('cloudprint', function() {
  'use strict';

  /**
   * Enumeration of cloud destination field names.
   * @enum {string}
   */
  const CloudDestinationField = {
    CAPABILITIES: 'capabilities',
    CONNECTION_STATUS: 'connectionStatus',
    DESCRIPTION: 'description',
    DISPLAY_NAME: 'displayName',
    ID: 'id',
    LAST_ACCESS: 'accessTime',
    TAGS: 'tags',
    TYPE: 'type'
  };

  /**
   * Special tag that denotes whether the destination has been recently used.
   * @const {string}
   */
  const RECENT_TAG = '^recent';

  /**
   * Special tag that denotes whether the destination is owned by the user.
   * @const {string}
   */
  const OWNED_TAG = '^own';

  /**
   * Tag that denotes whether the printer passes the 2018 certificate.
   * @const {string}
   */
  const CERT_TAG = '__cp_printer_passes_2018_cert__=';

  /**
   * Enumeration of cloud destination types that are supported by print preview.
   * @enum {string}
   */
  const DestinationCloudType = {
    ANDROID: 'ANDROID_CHROME_SNAPSHOT',
    DOCS: 'DOCS',
    IOS: 'IOS_CHROME_SNAPSHOT'
  };

  /**
   * Parses the destination type.
   * @param {string} typeStr Destination type given by the Google Cloud Print
   *     server.
   * @return {!print_preview.DestinationType} Destination type.
   * @private
   */
  function parseType(typeStr) {
    if (typeStr == DestinationCloudType.ANDROID ||
        typeStr == DestinationCloudType.IOS) {
      return print_preview.DestinationType.MOBILE;
    }
    if (typeStr == DestinationCloudType.DOCS) {
      return print_preview.DestinationType.GOOGLE_PROMOTED;
    }
    return print_preview.DestinationType.GOOGLE;
  }

  /**
   * @param {!Array<string>} tags The array of tag strings sent by GCP server.
   * @return {!print_preview.DestinationCertificateStatus} The certificate
   *     status indicated by the tag. Returns NONE if certificate tag is not
   *     found.
   */
  function extractCertificateStatus(tags) {
    const certTag = tags.find(tag => tag.startsWith(CERT_TAG));
    if (!certTag)
      return print_preview.DestinationCertificateStatus.NONE;
    const value = /** @type {print_preview.DestinationCertificateStatus} */ (
        certTag.substring(CERT_TAG.length));
    // Only 2 valid values sent by GCP server.
    assert(
        value == print_preview.DestinationCertificateStatus.UNKNOWN ||
        value == print_preview.DestinationCertificateStatus.YES ||
        value == print_preview.DestinationCertificateStatus.NO);
    return value;
  }

  /**
   * Parses a destination from JSON from a Google Cloud Print search or printer
   * response.
   * @param {!Object} json Object that represents a Google Cloud Print search or
   *     printer response.
   * @param {!print_preview.DestinationOrigin} origin The origin of the
   *     response.
   * @param {string} account The account this destination is registered for or
   *     empty string, if origin != COOKIES.
   * @return {!print_preview.Destination} Parsed destination.
   */
  function parseCloudDestination(json, origin, account) {
    if (!json.hasOwnProperty(CloudDestinationField.ID) ||
        !json.hasOwnProperty(CloudDestinationField.TYPE) ||
        !json.hasOwnProperty(CloudDestinationField.DISPLAY_NAME)) {
      throw Error('Cloud destination does not have an ID or a display name');
    }
    const id = json[CloudDestinationField.ID];
    const tags = json[CloudDestinationField.TAGS] || [];
    const connectionStatus = json[CloudDestinationField.CONNECTION_STATUS] ||
        print_preview.DestinationConnectionStatus.UNKNOWN;
    const optionalParams = {
      account: account,
      tags: tags,
      isOwned: arrayContains(tags, OWNED_TAG),
      lastAccessTime:
          parseInt(json[CloudDestinationField.LAST_ACCESS], 10) || Date.now(),
      cloudID: id,
      description: json[CloudDestinationField.DESCRIPTION],
      certificateStatus: extractCertificateStatus(tags),
    };
    const cloudDest = new print_preview.Destination(
        id, parseType(json[CloudDestinationField.TYPE]), origin,
        json[CloudDestinationField.DISPLAY_NAME],
        arrayContains(tags, RECENT_TAG) /*isRecent*/, connectionStatus,
        optionalParams);
    if (json.hasOwnProperty(CloudDestinationField.CAPABILITIES)) {
      cloudDest.capabilities = /** @type {!print_preview.Cdd} */ (
          json[CloudDestinationField.CAPABILITIES]);
    }
    return cloudDest;
  }

  /**
   * Enumeration of invitation field names.
   * @enum {string}
   */
  const InvitationField = {
    PRINTER: 'printer',
    RECEIVER: 'receiver',
    SENDER: 'sender'
  };

  /**
   * Enumeration of cloud destination types that are supported by print preview.
   * @enum {string}
   */
  const InvitationAclType =
      {DOMAIN: 'DOMAIN', GROUP: 'GROUP', PUBLIC: 'PUBLIC', USER: 'USER'};

  /**
   * Parses printer sharing invitation from JSON from GCP invite API response.
   * @param {!Object} json Object that represents a invitation search response.
   * @param {string} account The account this invitation is sent for.
   * @return {!print_preview.Invitation} Parsed invitation.
   */
  function parseInvitation(json, account) {
    if (!json.hasOwnProperty(InvitationField.SENDER) ||
        !json.hasOwnProperty(InvitationField.RECEIVER) ||
        !json.hasOwnProperty(InvitationField.PRINTER)) {
      throw Error('Invitation does not have necessary info.');
    }

    const nameFormatter = function(name, scope) {
      return name && scope ? (name + ' (' + scope + ')') : (name || scope);
    };

    const sender = json[InvitationField.SENDER];
    const senderName = nameFormatter(sender['name'], sender['email']);

    const receiver = json[InvitationField.RECEIVER];
    let receiverName = '';
    const receiverType = receiver['type'];
    if (receiverType == InvitationAclType.USER) {
      // It's a personal invitation, empty name indicates just that.
    } else if (
        receiverType == InvitationAclType.GROUP ||
        receiverType == InvitationAclType.DOMAIN) {
      receiverName = nameFormatter(receiver['name'], receiver['scope']);
    } else {
      throw Error('Invitation of unsupported receiver type');
    }

    const destination = cloudprint.parseCloudDestination(
        json[InvitationField.PRINTER], print_preview.DestinationOrigin.COOKIES,
        account);

    return new print_preview.Invitation(
        senderName, receiverName, destination, receiver, account);
  }

  // Export
  return {
    parseCloudDestination: parseCloudDestination,
    parseInvitation: parseInvitation,
  };
});
