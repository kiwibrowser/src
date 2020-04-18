'use strict';

// Converts an ECMAScript String object to an instance of
// mojo_base.mojom.String16.
function stringToMojoString16(string) {
  let array = new Array(string.length);
  for (var i = 0; i < string.length; ++i) {
    array[i] = string.charCodeAt(i);
  }
  return { data: array }
}

// Mocks the CredentialManager interface defined in credential_manager.mojom.
class MockCredentialManager {
  constructor() {
    this.reset();

    this.binding_ = new mojo.Binding(passwordManager.mojom.CredentialManager, this);
    this.interceptor_ = new MojoInterfaceInterceptor(
        passwordManager.mojom.CredentialManager.name);
    this.interceptor_.oninterfacerequest = e => {
      this.binding_.bind(e.handle);
    };
    this.interceptor_.start();
  }

  constructCredentialInfo_(type, id, password, name, icon) {
  return new passwordManager.mojom.CredentialInfo({
      type: type,
      id: stringToMojoString16(id),
      name: stringToMojoString16(name),
      icon: new url.mojom.Url({url: icon}),
      password: stringToMojoString16(password),
      federation: new url.mojom.Origin(
          {scheme: '', host: '', port: 0, unique: true})
    });
  }

  // Mock functions:

  async get(mediation, includePasswords, federations) {
    if (this.error_ == passwordManager.mojom.CredentialManagerError.SUCCESS) {
      return {error: this.error_, credential: this.credentialInfo_};
    } else {
      return {error: this.error_, credential: null};
    }
  }

  async store(credential) {
    return {};
  }

  async preventSilentAccess() {
    return {};
  }

  // Resets state of mock CredentialManager.
  reset() {
    this.error_ = passwordManager.mojom.CredentialManagerError.SUCCESS;
    this.credentialInfo_ = this.constructCredentialInfo_(
        passwordManager.mojom.CredentialType.EMPTY, '', '', '', '');
  }

  setResponse(id, password, name, icon) {
    this.credentialInfo_ = this.constructCredentialInfo_(
        passwordManager.mojom.CredentialType.PASSWORD, id, password, name,
        icon);
  }

  setError(error) {
    this.error_ = error;
  }
}

// Class that mocks Authenticator interface defined in authenticator.mojom.
class MockAuthenticator {
  constructor() {
    this.reset();

    this.binding_ = new mojo.Binding(webauth.mojom.Authenticator, this);
    this.interceptor_ = new MojoInterfaceInterceptor(
        webauth.mojom.Authenticator.name);
    this.interceptor_.oninterfacerequest = e => {
      this.binding_.bind(e.handle);
    };
    this.interceptor_.start();
  }

  // Returns a MakeCredentialResponse to the client.
  async makeCredential(options) {
    var response = null;
    if (this.status_ == webauth.mojom.AuthenticatorStatus.SUCCESS) {
      let info = new webauth.mojom.CommonCredentialInfo(
          { id: this.id_,
            rawId: this.rawId_,
            clientDataJson: this.clientDataJson_,
          });
      response = new webauth.mojom.MakeCredentialAuthenticatorResponse(
          { info: info,
            attestationObject: this.attestationObject_
          });
    }
    let status = this.status_;
    this.reset();
    return {status, credential: response};
  }

  async getAssertion(options) {
    var response = null;
    if (this.status_ == webauth.mojom.AuthenticatorStatus.SUCCESS) {
      let info = new webauth.mojom.CommonCredentialInfo(
          { id: this.id_,
            rawId: this.rawId_,
            clientDataJson: this.clientDataJson_,
          });
      response = new webauth.mojom.GetAssertionAuthenticatorResponse(
          { info: info,
            authenticatorData: this.authenticatorData_,
            signature: this.signature_,
            userHandle: this.userHandle_,
          });
    }
    let status = this.status_;
    this.reset();
    return {status, credential: response};
  }

  // Resets state of mock Authenticator.
  reset() {
    this.status_ = webauth.mojom.AuthenticatorStatus.UNKNOWN_ERROR;
    this.id_ = null;
    this.rawId_ = new Uint8Array(0);
    this.clientDataJson_ = new Uint8Array(0);
    this.attestationObject_ = new Uint8Array(0);
    this.authenticatorData_ = new Uint8Array(0);
    this.signature_ = new Uint8Array(0);
    this.userHandle_ = new Uint8Array(0);
  }

  // Sets everything needed for a MakeCredential success response.
  setDefaultsForSuccessfulMakeCredential() {
    mockAuthenticator.setRawId(RAW_ID);
    mockAuthenticator.setId(ID);
    mockAuthenticator.setClientDataJson(CLIENT_DATA_JSON);
    mockAuthenticator.setAttestationObject(ATTESTATION_OBJECT);
    mockAuthenticator.setAuthenticatorStatus(
        webauth.mojom.AuthenticatorStatus.SUCCESS);
  }

  // Sets everything needed for a GetAssertion success response.
  setDefaultsForSuccessfulGetAssertion() {
    mockAuthenticator.setRawId(RAW_ID);
    mockAuthenticator.setId(ID);
    mockAuthenticator.setClientDataJson(CLIENT_DATA_JSON);
    mockAuthenticator.setAuthenticatorData(AUTHENTICATOR_DATA);
    mockAuthenticator.setSignature(SIGNATURE);
    mockAuthenticator.setAuthenticatorStatus(
        webauth.mojom.AuthenticatorStatus.SUCCESS);
  }

  setAuthenticatorStatus(status) {
    this.status_ = status;
  }

  setId(id) {
    this.id_ = id;
  }

  setRawId(rawId) {
    this.rawId_ = rawId;
  }

  setClientDataJson(clientDataJson) {
    this.clientDataJson_ = clientDataJson;
  }

  setAttestationObject(attestationObject) {
    this.attestationObject_ = attestationObject;
  }

  setAuthenticatorData(authenticatorData) {
    this.authenticatorData_ = authenticatorData;
  }

  setSignature(signature) {
    this.signature_ = signature;
  }

  setUserHandle(userHandle) {
    this.userHandle_ = userHandle;
  }
}

var mockAuthenticator = new MockAuthenticator();
var mockCredentialManager = new MockCredentialManager();
