// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview JavaScript implementation of the credential management API
 * defined at http://w3c.github.io/webappsec-credential-management
 * This is a minimal implementation that sends data to the app side to
 * integrate with the password manager. When loaded, installs the API onto
 * the window.navigator.object.
 */

// TODO(crbug.com/435046) After get, store, preventSilentAccess are
// implemented app-side, make sure that all tests at
// https://w3c-test.org/credential-management/idl.https.html
// pass.

// TODO(crbug.com/435046) Declare Credential, PasswordCredential,
// FederatedCredential and CredentialsContainer as classes once iOS9 is no
// longer supported.

// Namespace for credential management. __gCrWeb must have already
// been defined.
__gCrWeb.credentialManager = {

  /**
   * Used to apply unique promiseId fields to messages sent to host.
   * Those IDs can be later used to call a corresponding resolver/rejecter.
   * @private {number}
   */
  nextId_: 0,

  /**
   * Stores the functions for resolving Promises returned by
   * navigator.credentials method calls. A resolver for a call with
   * promiseId: |id| is stored at resolvers_[id].
   * @type {!Object<number, function(?Credential)|function()>}
   * @private
   */
  resolvers_: {},

  /**
   * Stores the functions for rejecting Promises returned by
   * navigator.credentials method calls. A rejecter for a call with
   * promiseId: |id| is stored at rejecters_[id].
   * @type {!Object<number, function(?Error)>}
   * @private
   */
  rejecters_: {}
};

__gCrWeb['credentialManager'] = __gCrWeb.credentialManager;

/**
 * Creates and returns a Promise with given |promiseId|. The Promise's executor
 * function stores resolver and rejecter functions in
 * __gCrWeb['credentialManager'] under the key |promiseId| so they can be called
 * from the host after executing app side code.
 * @param {number} promiseId The number assigned to newly created Promise.
 * @return {!Promise<?Credential>|!Promise<!Credential>|!Promise<void>}
 *     The created Promise.
 * @private
 */
__gCrWeb.credentialManager.createPromise_ = function(promiseId) {
  return new Promise(function(resolve, reject) {
    __gCrWeb.credentialManager.resolvers_[promiseId] = resolve;
    __gCrWeb.credentialManager.rejecters_[promiseId] = reject;
  });
};

/**
 * Sends a message to the app side, invoking |command| with the given |options|.
 * @param {string} command The name of the invoked command.
 * @param {Object} options A dictionary of additional properties to forward to
 *     the app.
 * @return {!Promise<?Credential>|!Promise<!Credential>|!Promise<void>}
 *     A promise to be returned by the calling method.
 * @private
 */
__gCrWeb.credentialManager.invokeOnHost_ = function(command, options) {
  var promiseId = __gCrWeb.credentialManager.nextId_++;
  var message = {
    'command': command,
    'promiseId': promiseId
  };
  if (options) {
    Object.assign(message, options);
  }
  __gCrWeb.message.invokeOnHost(message);
  return __gCrWeb.credentialManager.createPromise_(promiseId);
};

/**
 * The Credential interface, for more information see
 * https://w3c.github.io/webappsec-credential-management/#the-credential-interface
 * @constructor
 */
function Credential() {
  /** @type {string} */
  this.id;
  /** @type {string} */
  this.type;
}
Object.defineProperty(Credential.prototype, Symbol.toStringTag,
    { value: 'Credential' });

/**
 * PasswordCredential interace, for more information see
 * https://w3c.github.io/webappsec-credential-management/#passwordcredential-interface
 * @extends {Credential}
 * @param {PasswordCredentialInit} init Either a PasswordCredentialData or
 *     HTMLFormElement to create PasswordCredential from.
 * @constructor
 */
function PasswordCredential(init) {
  // TODO(crbug.com/435046): When iOS9 is no longer supported, change vars to
  // |let| and |const| and declare at assignment.
  /** @type {!PasswordCredentialData} */
  var data;
  var elements;
  var elementIndex;
  var newPasswordObserved;
  var field;
  var name;
  var autocompleteTokens;
  var token;
  var tokenIndex;

  if (init instanceof HTMLFormElement) {
    // Performs following steps:
    // https://www.w3.org/TR/credential-management-1/#abstract-opdef-create-a-passwordcredential-from-an-htmlformelement
    data = /** @type {!PasswordCredentialData} */ ({});
    elements = init.querySelectorAll( // all submittable elements
        'button, input, object, select, textarea');
    newPasswordObserved = false;
    for (elementIndex = 0; elementIndex < elements.length;
        elementIndex++) {
      field = elements.item(elementIndex);
      if (!field.hasAttribute('autocomplete')) {
        continue;
      }
      name = field.name;
      if (!init[name]) {
        continue;
      }
      autocompleteTokens = field.getAttribute('autocomplete').split(' ');
      for (tokenIndex = 0; tokenIndex < autocompleteTokens.length;
          tokenIndex++) {
        token = autocompleteTokens[tokenIndex];
        if (token.toLowerCase() === 'new-password') {
          data.password = init[name].value;
          newPasswordObserved = true;
        }
        if (token.toLowerCase() === 'current-password') {
          if (!newPasswordObserved) {
            data.password = init[name].value;
          }
        }
        if (token.toLowerCase() === 'photo') {
          data.iconURL = init[name].value;
        }
        if (token.toLowerCase() === 'name') {
          data.name = init[name].value;
        }
        if (token.toLowerCase() === 'nickname') {
          data.name = init[name].value;
        }
        if (token.toLowerCase() === 'username') {
          data.id = init[name].value;
        }
      }
    }
  } else {
    // |init| was not HTMLFormElement so assuming it is PasswordCredentialData.
    // Not checking with instanceof because any dictionary with required fields
    // should be accepted.
    data = /** @type {!PasswordCredentialData} */ (init);
  }

  // Perform following steps:
  // https://w3c.github.io/webappsec-credential-management/#abstract-opdef-create-a-passwordcredential-from-passwordcredentialdata
  if (!data.id || typeof data.id != 'string') {
    throw new TypeError('id must be a non-empty string');
  }
  if (!data.password || typeof data.password != 'string') {
    throw new TypeError('password must be a non-empty string');
  }
  if (data.iconURL && !data.iconURL.startsWith('https://')) {
    throw new SyntaxError('invalid iconURL');
  }
  /** @type {string} */
  this._id = data.id;
  /** @type {string} */
  this._type = 'password';
  /** @type {string} */
  this._password = data.password;
  /** @type {string} */
  this._iconURL = (data.iconURL ? data.iconURL : '');
  /** @type {string} */
  this._name = (data.name ? data.name : '');
}

PasswordCredential.prototype = {
  __proto__: Credential.prototype
};
Object.defineProperty(PasswordCredential, 'prototype', { writable: false });

PasswordCredential.prototype.constructor = PasswordCredential;
Object.defineProperties(
  PasswordCredential.prototype,
  {
    'constructor': {
      enumerable: false
    },
    'id' : {
      get: /** @this {PasswordCredential} */ function() {
        return this._id;
      }
    },
    'type' : {
      get: /** @this {PasswordCredential} */ function() {
        return this._type;
      }
    },
    'password': {
      get: /** @this {PasswordCredential} */ function() {
        if (!(this instanceof PasswordCredential)) {
          throw new TypeError('attempting to get a property on prototype');
        }
        return this._password;
      },
      configurable: true,
      enumerable: true
    },
    'iconURL' : {
      get: /** @this {PasswordCredential} */ function() {
        return this._iconURL;
      }
    },
    'name' : {
      get: /** @this {PasswordCredential} */ function() {
        return this._name;
      }
    }
  }
);
Object.defineProperty(PasswordCredential.prototype, Symbol.toStringTag,
    { value: 'PasswordCredential' });

/**
 * FederatedCredential interface, for more information see
 * https://w3c.github.io/webappsec-credential-management/#federatedcredential-interface
 * @param {FederatedCredentialInit} init Dictionary to create
 *     FederatedCredential from.
 * @extends {Credential}
 * @constructor
 */
function FederatedCredential(init) {
  if (!init.id || typeof init.id != 'string') {
    throw new TypeError('id must be a non-empty string');
  }
  if (!init.provider || typeof init.provider != 'string') {
    throw new TypeError('provider must be a non-empty string');
  }
  if (!init.provider.startsWith('https://') &&
      !init.provider.startsWith('http://')) {
    throw new SyntaxError('invalid provider URL');
  }
  if (init.iconURL && !init.iconURL.startsWith('https://')) {
    throw new SyntaxError('invalid iconURL');
  }
  /** @type {string} */
  this._id = init.id;
  /** @type {string} */
  this._type = 'federated';
  /** @type {string} */
  this._name = (init.name ? init.name : '');
  /** @type {string} */
  this._iconURL = (init.iconURL ? init.iconURL : '');
  /** @type {string} */
  this._provider = init.provider.replace(/\/$/, ''); // strip trailing slash
  /** @type {?string} */
  this._protocol = (init.protocol ? init.protocol : '');
}

FederatedCredential.prototype = {
  __proto__: Credential.prototype
};
Object.defineProperty(FederatedCredential, 'prototype', { writable: false });

FederatedCredential.prototype.constructor = FederatedCredential;
Object.defineProperties(
  FederatedCredential.prototype,
  {
    'constructor': {
      enumerable: false
    },
    'id' : {
      get: /** @this {FederatedCredential} */ function() {
        return this._id;
      }
    },
    'type' : {
      get: /** @this {FederatedCredential} */ function() {
        return this._type;
      }
    },
    'provider': {
      get: /** @this {FederatedCredential} */ function() {
        if (!(this instanceof FederatedCredential)) {
          throw new TypeError('attempting to get a property on prototype');
        }
        return this._provider;
      },
      configurable: true,
      enumerable: true
    },
    'protocol': {
      get: /** @this {FederatedCredential} */ function() {
        if (!(this instanceof FederatedCredential)) {
          throw new TypeError('attempting to get a property on prototype');
        }
        return this._protocol;
      },
      configurable: true,
      enumerable: true
    },
    'iconURL' : {
      get: /** @this {FederatedCredential} */ function() {
        return this._iconURL;
      }
    },
    'name' : {
      get: /** @this {FederatedCredential} */ function() {
        return this._name;
      }
    }
  }
);
Object.defineProperty(FederatedCredential.prototype, Symbol.toStringTag,
    { value: 'FederatedCredential' });

/**
 * CredentialData dictionary
 * https://w3c.github.io/webappsec-credential-management/#dictdef-credentialdata
 * @dict
 * @typedef {{id: string}}
 */
var CredentialData;

/**
 * PasswordCredentialData used for constructing PasswordCredential objects
 * https://w3c.github.io/webappsec-credential-management/#dictdef-passwordcredentialdata
 * @dict
 * @typedef {{
 *     id: string,
 *     name: string,
 *     iconURL: string,
 *     password: string
 * }}
 */
var PasswordCredentialData;

/**
 * Either PasswordCredentialData or HTMLFormElement used for constructing
 * a new PasswordCredential
 * https://www.w3.org/TR/credential-management-1/#typedefdef-passwordcredentialinit
 * @typedef {!PasswordCredentialData|HTMLFormElement}
 */
var PasswordCredentialInit;

/**
 * FederatedCredentialInit used for constructing FederatedCredential objects
 * https://w3c.github.io/webappsec-credential-management/#dictdef-federatedcredentialinit
 * @dict
 * @typedef {{
 *     id: string,
 *     name: string,
 *     iconURL: string,
 *     provider: string,
 *     protocol: string
 * }}
 */
var FederatedCredentialInit;

/**
 * The CredentialRequestOptions dictionary, for more information see
 * https://w3c.github.io/webappsec-credential-management/#credentialrequestoptions-dictionary
 * @dict
 * @typedef {{mediation: string}}
 */
var CredentialRequestOptions;

/**
 * The FederatedCredentialRequestOptions dictionary, for more information see
 * https://w3c.github.io/webappsec-credential-management/#dictdef-federatedcredentialrequestoptions
 * @dict
 * @typedef {{
 *     providers: !Array<string>,
 *     protocols: !Array<string>
 * }}
 */
var FederatedCredentialRequestOptions;

/**
 * The CredentialCreationOptions dictionary, for more information see
 * https://w3c.github.io/webappsec-credential-management/#credentialcreationoptions-dictionary
 * @dict
 * @typedef {{
 *     password: ?PasswordCredentialInit,
 *     federated: ?FederatedCredentialInit
 * }}
 */
var CredentialCreationOptions;

/**
 * Implements the public Credential Management API. For more information, see
 * https://w3c.github.io/webappsec-credential-management/#credentialscontainer
 * @constructor
 */
function CredentialsContainer() {}

Object.defineProperty(CredentialsContainer, 'prototype', { writable: false });

CredentialsContainer.prototype.constructor = CredentialsContainer;
Object.defineProperty(
    CredentialsContainer.prototype, 'constructor', { enumerable: false });
Object.defineProperty(CredentialsContainer.prototype, Symbol.toStringTag,
    { value: 'CredentialsContainer' });

/**
 * Performs the Request A Credential action described at
 * https://w3c.github.io/webappsec-credential-management/#abstract-opdef-request-a-credential
 * @param {!CredentialRequestOptions} options An optional dictionary of
 *     parameters for the request.
 * @return {!Promise<?Credential>} A promise for retrieving the result
 *     of the request.
 */
CredentialsContainer.prototype.get = function(options) {
  return __gCrWeb.credentialManager.invokeOnHost_(
      'credentials.get', options);
};

/**
 * Performs the Store A Credential action described at
 * https://w3c.github.io/webappsec-credential-management/#abstract-opdef-store-a-credential
 * @param {!Credential} credential A credential object to store.
 * @return {!Promise<void>} A promise for retrieving the result
 *     of the request.
 */
CredentialsContainer.prototype.store = function(credential) {
  return __gCrWeb.credentialManager.invokeOnHost_(
      'credentials.store', credential);
};

/**
 * Performs the Prevent Silent Access action described at
 * https://w3c.github.io/webappsec-credential-management/#abstract-opdef-prevent-silent-access
 * @return {!Promise<void>} A promise for retrieving the result
 *     of the request.
 */
CredentialsContainer.prototype.preventSilentAccess = function() {
  return __gCrWeb.credentialManager.invokeOnHost_(
      'credentials.preventSilentAccess', {});
};

/**
 * Performs the Create A Credential action described at
 * https://w3c.github.io/webappsec-credential-management/#abstract-opdef-create-a-credential
 * @param {!CredentialCreationOptions} options An optional dictionary of
 *     of params for creating a new Credential object.
 * @return {!Promise<?Credential>} A promise for retrieving the result
 *     of the request.
 */
CredentialsContainer.prototype.create = function(options) {
  // According to
  // https://w3c.github.io/webappsec-credential-management/#abstract-opdef-create-a-credential,
  // we should also check for secure context. Instead it is done only in
  // browser-side methods. Since public JS interface is exposed, user can create
  // a Credential using a constructor anyway.
  return new Promise(function(resolve, reject) {
    try {
      if (options && options.password && !options.federated) {
        resolve(new PasswordCredential(options.password));
        return;
      }
      if (options && options.federated && !options.password) {
        resolve(new FederatedCredential(options.federated));
        return;
      }
    } catch (err) {
      reject(err);
      return;
    }
    reject(Object.create(DOMException.prototype, {
      name: {value: DOMException.NOT_SUPPORTED_ERR},
      message: {value: 'Invalid CredentialRequestOptions'}
    }));
  });
};

/**
 * Install the public interface.
 * @type {!CredentialsContainer}
 */
window.navigator.credentials = new CredentialsContainer();
