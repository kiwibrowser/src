// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

describe('PassphraseManager', function() {
  var passphraseWindow = null;
  var passphraseWindowLoadCallback = null;

  beforeEach(function() {
    // This API is not available in Karma unit tests, so stub it using
    // window.open.
    chrome.app.window = {
      create: function(url, options, callback) {
        var closeCallbacks = [];
        passphraseWindow = window.open(url);
        passphraseWindow.onload = function() {
          passphraseWindow.onEverythingLoaded = function() {
            if (passphraseWindowLoadCallback)
              passphraseWindowLoadCallback();
          };
          passphraseWindow.onunload = function() {
            closeCallbacks.forEach(function(callback) {
              callback();
            });
          };
        };
        callback({
          contentWindow: passphraseWindow,
          onClosed: {
            addListener: function(onClosedCallback) {
              closeCallbacks.push(onClosedCallback);
            }
          }
        });
      }
    };
  });

  describe('that has an initial passphrase', function() {
    var TEST_PASSPHRASE = 'hello-world';
    var passphraseManager = new unpacker.PassphraseManager(TEST_PASSPHRASE);

    afterEach(function() {
      passphraseWindowLoadCallback = null;
      if (passphraseWindow)
        passphraseWindow.close();
      passphraseWindow = null;
    });

    it('should return it immediately', function(done) {
      passphraseManager.getPassphrase()
          .then(function(passphrase) {
            expect(passphrase).to.equal(TEST_PASSPHRASE);
            expect(passphraseManager.rememberedPassphrase)
                .to.equal(TEST_PASSPHRASE);
            done();
          })
          .catch(test_utils.forceFailure);
    });

    it('should not return it again for the second call', function(done) {
      passphraseWindowLoadCallback = function() {
        // Close the window as soon as it's loaded.
        passphraseWindow.close();
      };
      passphraseManager.getPassphrase()
          .then(test_utils.forceFailure)
          .catch(function(error) {
            expect(error).to.equal('FAILED');
            done();
          });
    });
  });

  describe('without an initial passphrase', function() {
    var USER_PASSPHRASE = 'hello-kitty';
    var passphraseManager = new unpacker.PassphraseManager(null);

    afterEach(function() {
      passphraseWindowLoadCallback = null;
      if (passphraseWindow)
        passphraseWindow.close();
      passphraseWindow = null;
    });

    it('should accept a passphrase from a user with remembering',
       function(done) {
         passphraseWindowLoadCallback = function() {
           // Fill out the password field and close the dialog as soon as it's
           // loaded.
           passphraseWindow.document.querySelector('html /deep/ #input').value =
               USER_PASSPHRASE;
           passphraseWindow.document.querySelector('html /deep/ #remember')
               .checked = true;
           passphraseWindow.document.querySelector('html /deep/ #acceptButton')
               .click();
         };
         passphraseManager.getPassphrase()
             .then(function(passphrase) {
               expect(passphrase).to.equal(USER_PASSPHRASE);
               expect(passphraseManager.rememberedPassphrase)
                   .to.equal(USER_PASSPHRASE);
               done();
             })
             .catch(test_utils.forceFailure);
       });

    it('should accept a passphrase from a user without remembering',
       function(done) {
         passphraseWindowLoadCallback = function() {
           // Fill out the password field and close the dialog as soon as it's
           // loaded.
           passphraseWindow.document.querySelector('html /deep/ #input').value =
               USER_PASSPHRASE;
           passphraseWindow.document.querySelector('html /deep/ #acceptButton')
               .click();
         };
         passphraseManager.getPassphrase()
             .then(function(passphrase) {
               expect(passphrase).to.equal(USER_PASSPHRASE);
               expect(passphraseManager.rememberedPassphrase).to.be.null;
               done();
             })
             .catch(test_utils.forceFailure);
       });

    it('should reject password on window cancel button', function(done) {
      passphraseWindowLoadCallback = function() {
        passphraseWindow.document.querySelector('html /deep/ #input').value =
            USER_PASSPHRASE;
        passphraseWindow.document.querySelector('html /deep/ #remember')
            .checked = true;
        passphraseWindow.document.querySelector('html /deep/ #cancelButton')
            .click();
      };
      passphraseManager.getPassphrase().then(
          test_utils.forceFailure, function(error) {
            expect(error).to.equal('FAILED');
            expect(passphraseManager.rememberedPassphrase).to.be.null;
            done();
          });
    });

    it('should reject password on window close', function(done) {
      passphraseWindowLoadCallback = function() {
        passphraseWindow.document.querySelector('html /deep/ #input').value =
            USER_PASSPHRASE;
        passphraseWindow.document.querySelector('html /deep/ #remember')
            .checked = true;
        passphraseWindow.close();
      };
      passphraseManager.getPassphrase().then(
          test_utils.forceFailure, function(error) {
            expect(error).to.equal('FAILED');
            expect(passphraseManager.rememberedPassphrase).to.be.null;
            done();
          });
    });
  });
});
