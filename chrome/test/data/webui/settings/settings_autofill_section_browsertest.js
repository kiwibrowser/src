// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Autofill Settings tests. */

/** @const {string} Path to root from chrome/test/data/webui/settings/. */
const ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture.
GEN_INCLUDE([
  ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js',
]);

/**
 * Test implementation.
 * @implements {settings.address.CountryDetailManager}
 * @constructor
 */
function CountryDetailManagerTestImpl() {}

CountryDetailManagerTestImpl.prototype = {
  /** @override */
  getCountryList: function() {
    return new Promise(function(resolve) {
      resolve([
        {name: 'United States', countryCode: 'US'},  // Default test country.
        {name: 'Israel', countryCode: 'IL'},
        {name: 'United Kingdom', countryCode: 'GB'},
      ]);
    });
  },

  /** @override */
  getAddressFormat: function(countryCode) {
    return new Promise(function(resolve) {
      chrome.autofillPrivate.getAddressComponents(countryCode, resolve);
    });
  },
};

/**
 * Will call |loopBody| for each item in |items|. Will only move to the next
 * item after the promise from |loopBody| resolves.
 * @param {!Array<Object>} items
 * @param {!function(!Object):!Promise} loopBody
 * @return {!Promise}
 */
function asyncForEach(items, loopBody) {
  return new Promise(function(resolve) {
    let index = 0;

    function loop() {
      const item = items[index++];
      if (item)
        loopBody(item).then(loop);
      else
        resolve();
    }

    loop();
  });
}

/**
 * Resolves the promise after the element fires the expected event. Will add and
 * remove the listener so it is only triggered once. |causeEvent| is called
 * after adding a listener to make sure that the event is captured.
 * @param {!Element} element
 * @param {string} eventName
 * @param {function():void} causeEvent
 * @return {!Promise}
 */
function expectEvent(element, eventName, causeEvent) {
  return new Promise(function(resolve) {
    const callback = function() {
      element.removeEventListener(eventName, callback);
      resolve.apply(this, arguments);
    };
    element.addEventListener(eventName, callback);
    causeEvent();
  });
}

/**
 * @constructor
 * @extends {PolymerTest}
 */
function SettingsAutofillSectionBrowserTest() {}

SettingsAutofillSectionBrowserTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  browsePreload:
      'chrome://settings/passwords_and_forms_page/autofill_section.html',

  /** @override */
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    'passwords_and_autofill_fake_data.js',
    'test_util.js',
    'ensure_lazy_loaded.js',
  ]),

  /** @override */
  setUp: function() {
    PolymerTest.prototype.setUp.call(this);

    // Test is run on an individual element that won't have a page language.
    this.accessibilityAuditConfig.auditRulesToIgnore.push('humanLangMissing');

    settings.ensureLazyLoaded();
  },

  /**
   * Creates the autofill section for the given lists.
   * @param {!Array<!chrome.passwordsPrivate.PasswordUiEntry>} passwordList
   * @param {!Array<!chrome.passwordsPrivate.ExceptionEntry>} exceptionList
   * @param {!Object} pref_value
   * @return {!Object}
   * @private
   */
  createAutofillSection_: function(addresses, creditCards, pref_value) {
    // Override the AutofillManagerImpl for testing.
    this.autofillManager = new TestAutofillManager();
    this.autofillManager.data.addresses = addresses;
    this.autofillManager.data.creditCards = creditCards;
    AutofillManagerImpl.instance_ = this.autofillManager;

    const section = document.createElement('settings-autofill-section');
    section.prefs = {autofill: {credit_card_enabled: pref_value}};
    document.body.appendChild(section);
    Polymer.dom.flush();

    return section;
  },

  /**
   * Creates the Edit Address dialog and fulfills the promise when the dialog
   * has actually opened.
   * @param {!chrome.autofillPrivate.AddressEntry} address
   * @return {!Promise<Object>}
   */
  createAddressDialog_: function(address) {
    return new Promise(function(resolve) {
      const section = document.createElement('settings-address-edit-dialog');
      section.address = address;
      document.body.appendChild(section);
      section.addEventListener('on-update-address-wrapper', function() {
        resolve(section);
      });
    });
  },

  /**
   * Creates the Edit Credit Card dialog.
   * @param {!chrome.autofillPrivate.CreditCardEntry} creditCardItem
   * @return {!Object}
   */
  createCreditCardDialog_: function(creditCardItem) {
    const section = document.createElement('settings-credit-card-edit-dialog');
    section.creditCard = creditCardItem;
    document.body.appendChild(section);
    Polymer.dom.flush();
    return section;
  },
};

TEST_F('SettingsAutofillSectionBrowserTest', 'uiTest', function() {
  suite('AutofillSection', function() {
    test('testAutofillExtensionIndicator', function() {
      // Initializing with fake prefs
      const section = document.createElement('settings-autofill-section');
      section.prefs = {autofill: {enabled: {}, credit_card_enabled: {}}};
      document.body.appendChild(section);

      assertFalse(!!section.$$('#autofillExtensionIndicator'));
      section.set('prefs.autofill.enabled.extensionId', 'test-id');
      Polymer.dom.flush();

      assertTrue(!!section.$$('#autofillExtensionIndicator'));
    });
  });

  mocha.run();
});

TEST_F('SettingsAutofillSectionBrowserTest', 'CreditCardTests', function() {
  const self = this;

  suite('AutofillSection', function() {
    suiteSetup(function() {
      settings.address.CountryDetailManagerImpl.instance_ =
          new CountryDetailManagerTestImpl();
    });

    setup(function() {
      PolymerTest.clearBody();
    });

    test('verifyCreditCardCount', function() {
      const section = self.createAutofillSection_([], [], {});

      const creditCardList = section.$$('#creditCardList');
      assertTrue(!!creditCardList);
      assertEquals(0, creditCardList.querySelectorAll('.list-item').length);

      assertFalse(section.$$('#noCreditCardsLabel').hidden);
      assertTrue(section.$$('#creditCardsHeading').hidden);
      assertTrue(section.$$('#CreditCardsDisabledLabel').hidden);
    });

    test('verifyCreditCardsDisabled', function() {
      const section = self.createAutofillSection_([], [], {value: false});

      assertEquals(0, section.querySelectorAll('#creditCardList').length);
      assertFalse(section.$$('#CreditCardsDisabledLabel').hidden);
    });

    test('verifyCreditCardCount', function() {
      const creditCards = [
        FakeDataMaker.creditCardEntry(),
        FakeDataMaker.creditCardEntry(),
        FakeDataMaker.creditCardEntry(),
        FakeDataMaker.creditCardEntry(),
        FakeDataMaker.creditCardEntry(),
        FakeDataMaker.creditCardEntry(),
      ];

      const section = self.createAutofillSection_([], creditCards, {});
      const creditCardList = section.$$('#creditCardList');
      assertTrue(!!creditCardList);
      assertEquals(
          creditCards.length,
          creditCardList.querySelectorAll('.list-item').length);

      assertTrue(section.$$('#noCreditCardsLabel').hidden);
      assertFalse(section.$$('#creditCardsHeading').hidden);
      assertTrue(section.$$('#CreditCardsDisabledLabel').hidden);
    });

    test('verifyCreditCardFields', function() {
      const creditCard = FakeDataMaker.creditCardEntry();
      const section = self.createAutofillSection_([], [creditCard], {});
      const creditCardList = section.$$('#creditCardList');
      const row = creditCardList.children[0];
      assertTrue(!!row);

      assertEquals(
          creditCard.metadata.summaryLabel,
          row.querySelector('#creditCardLabel').textContent);
      assertEquals(
          creditCard.expirationMonth + '/' + creditCard.expirationYear,
          row.querySelector('#creditCardExpiration').textContent);
    });

    test('verifyCreditCardRowButtonIsDropdownWhenLocal', function() {
      const creditCard = FakeDataMaker.creditCardEntry();
      creditCard.metadata.isLocal = true;
      const section = self.createAutofillSection_([], [creditCard], {});
      const creditCardList = section.$$('#creditCardList');
      const row = creditCardList.children[0];
      assertTrue(!!row);
      const menuButton = row.querySelector('#creditCardMenu');
      assertTrue(!!menuButton);
      const outlinkButton =
          row.querySelector('paper-icon-button-light.icon-external');
      assertFalse(!!outlinkButton);
    });

    test('verifyCreditCardRowButtonIsOutlinkWhenRemote', function() {
      const creditCard = FakeDataMaker.creditCardEntry();
      creditCard.metadata.isLocal = false;
      const section = self.createAutofillSection_([], [creditCard], {});
      const creditCardList = section.$$('#creditCardList');
      const row = creditCardList.children[0];
      assertTrue(!!row);
      const menuButton = row.querySelector('#creditCardMenu');
      assertFalse(!!menuButton);
      const outlinkButton =
          row.querySelector('paper-icon-button-light.icon-external');
      assertTrue(!!outlinkButton);
    });

    test('verifyAddVsEditCreditCardTitle', function() {
      const newCreditCard = FakeDataMaker.emptyCreditCardEntry();
      const newCreditCardDialog = self.createCreditCardDialog_(newCreditCard);
      const oldCreditCard = FakeDataMaker.creditCardEntry();
      const oldCreditCardDialog = self.createCreditCardDialog_(oldCreditCard);

      assertNotEquals(oldCreditCardDialog.title_, newCreditCardDialog.title_);
      assertNotEquals('', newCreditCardDialog.title_);
      assertNotEquals('', oldCreditCardDialog.title_);

      // Wait for dialogs to open before finishing test.
      return Promise.all([
        test_util.whenAttributeIs(newCreditCardDialog.$.dialog, 'open', ''),
        test_util.whenAttributeIs(oldCreditCardDialog.$.dialog, 'open', ''),
      ]);
    });

    test('verifyExpiredCreditCardYear', function() {
      const creditCard = FakeDataMaker.creditCardEntry();

      // 2015 is over unless time goes wobbly.
      const twentyFifteen = 2015;
      creditCard.expirationYear = twentyFifteen.toString();

      const creditCardDialog = self.createCreditCardDialog_(creditCard);

      return test_util.whenAttributeIs(creditCardDialog.$.dialog, 'open', '')
          .then(function() {
            const now = new Date();
            const maxYear = now.getFullYear() + 9;
            const yearOptions = creditCardDialog.$.year.options;

            assertEquals('2015', yearOptions[0].textContent.trim());
            assertEquals(
                maxYear.toString(),
                yearOptions[yearOptions.length - 1].textContent.trim());
            assertEquals(
                creditCard.expirationYear, creditCardDialog.$.year.value);
          });
    });

    test('verifyVeryFutureCreditCardYear', function() {
      const creditCard = FakeDataMaker.creditCardEntry();

      // Expiring 20 years from now is unusual.
      const now = new Date();
      const farFutureYear = now.getFullYear() + 20;
      creditCard.expirationYear = farFutureYear.toString();

      const creditCardDialog = self.createCreditCardDialog_(creditCard);

      return test_util.whenAttributeIs(creditCardDialog.$.dialog, 'open', '')
          .then(function() {
            const yearOptions = creditCardDialog.$.year.options;

            assertEquals(
                now.getFullYear().toString(),
                yearOptions[0].textContent.trim());
            assertEquals(
                farFutureYear.toString(),
                yearOptions[yearOptions.length - 1].textContent.trim());
            assertEquals(
                creditCard.expirationYear, creditCardDialog.$.year.value);
          });
    });

    test('verifyVeryNormalCreditCardYear', function() {
      const creditCard = FakeDataMaker.creditCardEntry();

      // Expiring 2 years from now is not unusual.
      const now = new Date();
      const nearFutureYear = now.getFullYear() + 2;
      creditCard.expirationYear = nearFutureYear.toString();
      const maxYear = now.getFullYear() + 9;

      const creditCardDialog = self.createCreditCardDialog_(creditCard);

      return test_util.whenAttributeIs(creditCardDialog.$.dialog, 'open', '')
          .then(function() {
            const yearOptions = creditCardDialog.$.year.options;

            assertEquals(
                now.getFullYear().toString(),
                yearOptions[0].textContent.trim());
            assertEquals(
                maxYear.toString(),
                yearOptions[yearOptions.length - 1].textContent.trim());
            assertEquals(
                creditCard.expirationYear, creditCardDialog.$.year.value);
          });
    });

    test('verify save disabled for expired credit card', function() {
      const creditCard = FakeDataMaker.emptyCreditCardEntry();

      const now = new Date();
      creditCard.expirationYear = now.getFullYear() - 2;
      // works fine for January.
      creditCard.expirationMonth = now.getMonth() - 1;

      const creditCardDialog = self.createCreditCardDialog_(creditCard);

      return test_util.whenAttributeIs(creditCardDialog.$.dialog, 'open', '')
          .then(function() {
            assertTrue(creditCardDialog.$.saveButton.disabled);
          });
    });

    // Test will timeout if event is not received.
    test('verify save new credit card', function(done) {
      const creditCard = FakeDataMaker.emptyCreditCardEntry();
      const creditCardDialog = self.createCreditCardDialog_(creditCard);

      return test_util.whenAttributeIs(creditCardDialog.$.dialog, 'open', '')
          .then(function() {
            // Not expired, but still can't be saved, because there's no name.
            assertTrue(creditCardDialog.$.expired.hidden);
            assertTrue(creditCardDialog.$.saveButton.disabled);

            // Add a name and trigger the on-input handler.
            creditCardDialog.set('creditCard.name', 'Jane Doe');
            creditCardDialog.onCreditCardNameOrNumberChanged_();
            Polymer.dom.flush();

            assertTrue(creditCardDialog.$.expired.hidden);
            assertFalse(creditCardDialog.$.saveButton.disabled);

            creditCardDialog.addEventListener(
                'save-credit-card', function(event) {
                  assertEquals(creditCard.guid, event.detail.guid);
                  done();
                });
            MockInteractions.tap(creditCardDialog.$.saveButton);
          });
    });

    test('verifyCancelCreditCardEdit', function(done) {
      const creditCard = FakeDataMaker.emptyCreditCardEntry();
      const creditCardDialog = self.createCreditCardDialog_(creditCard);

      return test_util.whenAttributeIs(creditCardDialog.$.dialog, 'open', '')
          .then(function() {
            creditCardDialog.addEventListener('save-credit-card', function() {
              // Fail the test because the save event should not be called when
              // cancel is clicked.
              assertTrue(false);
              done();
            });

            creditCardDialog.addEventListener('close', function() {
              // Test is |done| in a timeout in order to ensure that
              // 'save-credit-card' is NOT fired after this test.
              window.setTimeout(done, 100);
            });

            MockInteractions.tap(creditCardDialog.$.cancelButton);
          });
    });

    test('verifyLocalCreditCardMenu', function() {
      const creditCard = FakeDataMaker.creditCardEntry();

      // When credit card is local, |isCached| will be undefined.
      creditCard.metadata.isLocal = true;
      creditCard.metadata.isCached = undefined;

      const section = self.createAutofillSection_([], [creditCard], {});
      const creditCardList = section.$$('#creditCardList');
      assertTrue(!!creditCardList);
      assertEquals(1, creditCardList.querySelectorAll('.list-item').length);
      const row = creditCardList.children[0];

      // Local credit cards will show the overflow menu.
      assertFalse(!!row.querySelector('#remoteCreditCardLink'));
      const menuButton = row.querySelector('#creditCardMenu');
      assertTrue(!!menuButton);

      menuButton.click();
      Polymer.dom.flush();

      const menu = section.$.creditCardSharedMenu;

      // Menu should have 2 options.
      assertFalse(menu.querySelector('#menuEditCreditCard').hidden);
      assertFalse(menu.querySelector('#menuRemoveCreditCard').hidden);
      assertTrue(menu.querySelector('#menuClearCreditCard').hidden);

      menu.close();
      Polymer.dom.flush();
    });

    test('verifyCachedCreditCardMenu', function() {
      const creditCard = FakeDataMaker.creditCardEntry();

      creditCard.metadata.isLocal = false;
      creditCard.metadata.isCached = true;

      const section = self.createAutofillSection_([], [creditCard], {});
      const creditCardList = section.$$('#creditCardList');
      assertTrue(!!creditCardList);
      assertEquals(1, creditCardList.querySelectorAll('.list-item').length);
      const row = creditCardList.children[0];

      // Cached remote CCs will show overflow menu.
      assertFalse(!!row.querySelector('#remoteCreditCardLink'));
      const menuButton = row.querySelector('#creditCardMenu');
      assertTrue(!!menuButton);

      menuButton.click();
      Polymer.dom.flush();

      const menu = section.$.creditCardSharedMenu;

      // Menu should have 2 options.
      assertFalse(menu.querySelector('#menuEditCreditCard').hidden);
      assertTrue(menu.querySelector('#menuRemoveCreditCard').hidden);
      assertFalse(menu.querySelector('#menuClearCreditCard').hidden);

      menu.close();
      Polymer.dom.flush();
    });

    test('verifyNotCachedCreditCardMenu', function() {
      const creditCard = FakeDataMaker.creditCardEntry();

      creditCard.metadata.isLocal = false;
      creditCard.metadata.isCached = false;

      const section = self.createAutofillSection_([], [creditCard], {});
      const creditCardList = section.$$('#creditCardList');
      assertTrue(!!creditCardList);
      assertEquals(1, creditCardList.querySelectorAll('.list-item').length);
      const row = creditCardList.children[0];

      // No overflow menu when not cached.
      assertTrue(!!row.querySelector('#remoteCreditCardLink'));
      assertFalse(!!row.querySelector('#creditCardMenu'));
    });
  });

  mocha.run();
});

TEST_F('SettingsAutofillSectionBrowserTest', 'AddressTests', function() {
  const self = this;


  suite('AutofillSection', function() {
    suiteSetup(function() {
      settings.address.CountryDetailManagerImpl.instance_ =
          new CountryDetailManagerTestImpl();
    });

    setup(function() {
      PolymerTest.clearBody();
    });

    test('verifyNoAddresses', function() {
      const section = self.createAutofillSection_([], [], {});

      const addressList = section.$.addressList;
      assertTrue(!!addressList);
      // 1 for the template element.
      assertEquals(1, addressList.children.length);

      assertFalse(section.$.noAddressesLabel.hidden);
    });

    test('verifyAddressCount', function() {
      const addresses = [
        FakeDataMaker.addressEntry(),
        FakeDataMaker.addressEntry(),
        FakeDataMaker.addressEntry(),
        FakeDataMaker.addressEntry(),
        FakeDataMaker.addressEntry(),
      ];

      const section = self.createAutofillSection_(addresses, [], {});

      const addressList = section.$.addressList;
      assertTrue(!!addressList);
      assertEquals(
          addresses.length, addressList.querySelectorAll('.list-item').length);

      assertTrue(section.$.noAddressesLabel.hidden);
    });

    test('verifyAddressFields', function() {
      const address = FakeDataMaker.addressEntry();
      const section = self.createAutofillSection_([address], [], {});
      const addressList = section.$.addressList;
      const row = addressList.children[0];
      assertTrue(!!row);

      const addressSummary =
          address.metadata.summaryLabel + address.metadata.summarySublabel;

      let actualSummary = '';

      // Eliminate white space between nodes!
      const addressPieces = row.querySelector('#addressSummary').children;
      for (let i = 0; i < addressPieces.length; ++i) {
        actualSummary += addressPieces[i].textContent.trim();
      }

      assertEquals(addressSummary, actualSummary);
    });

    test('verifyAddressRowButtonIsDropdownWhenLocal', function() {
      const address = FakeDataMaker.addressEntry();
      address.metadata.isLocal = true;
      const section = self.createAutofillSection_([address], [], {});
      const addressList = section.$.addressList;
      const row = addressList.children[0];
      assertTrue(!!row);
      const menuButton = row.querySelector('#addressMenu');
      assertTrue(!!menuButton);
      const outlinkButton =
          row.querySelector('paper-icon-button-light.icon-external');
      assertFalse(!!outlinkButton);
    });

    test('verifyAddressRowButtonIsOutlinkWhenRemote', function() {
      const address = FakeDataMaker.addressEntry();
      address.metadata.isLocal = false;
      const section = self.createAutofillSection_([address], [], {});
      const addressList = section.$.addressList;
      const row = addressList.children[0];
      assertTrue(!!row);
      const menuButton = row.querySelector('#addressMenu');
      assertFalse(!!menuButton);
      const outlinkButton =
          row.querySelector('paper-icon-button-light.icon-external');
      assertTrue(!!outlinkButton);
    });

    test('verifyAddAddressDialog', function() {
      return self.createAddressDialog_(FakeDataMaker.emptyAddressEntry())
          .then(function(dialog) {
            const title = dialog.$$('[slot=title]');
            assertEquals(
                loadTimeData.getString('addAddressTitle'), title.textContent);
            // Shouldn't be possible to save until something is typed in.
            assertTrue(dialog.$.saveButton.disabled);
          });
    });

    test('verifyEditAddressDialog', function() {
      return self.createAddressDialog_(FakeDataMaker.addressEntry())
          .then(function(dialog) {
            const title = dialog.$$('[slot=title]');
            assertEquals(
                loadTimeData.getString('editAddressTitle'), title.textContent);
            // Should be possible to save when editing because fields are
            // populated.
            assertFalse(dialog.$.saveButton.disabled);
          });
    });

    test('verifyCountryIsSaved', function() {
      const address = FakeDataMaker.emptyAddressEntry();
      return self.createAddressDialog_(address).then(function(dialog) {
        const countrySelect = dialog.$$('select');
        assertEquals('', countrySelect.value);
        assertEquals(undefined, address.countryCode);
        countrySelect.value = 'US';
        countrySelect.dispatchEvent(new CustomEvent('change'));
        Polymer.dom.flush();
        assertEquals('US', countrySelect.value);
        assertEquals('US', address.countryCode);
      });
    });

    test('verifyPhoneAndEmailAreSaved', function() {
      const address = FakeDataMaker.emptyAddressEntry();
      return self.createAddressDialog_(address).then(function(dialog) {
        assertEquals('', dialog.$.phoneInput.value);
        assertFalse(!!(address.phoneNumbers && address.phoneNumbers[0]));

        assertEquals('', dialog.$.emailInput.value);
        assertFalse(!!(address.emailAddresses && address.emailAddresses[0]));

        const phoneNumber = '(555) 555-5555';
        const emailAddress = 'no-reply@chromium.org';

        dialog.$.phoneInput.value = phoneNumber;
        dialog.$.emailInput.value = emailAddress;

        return expectEvent(dialog, 'save-address', function() {
                 MockInteractions.tap(dialog.$.saveButton);
               }).then(function() {
          assertEquals(phoneNumber, dialog.$.phoneInput.value);
          assertEquals(phoneNumber, address.phoneNumbers[0]);

          assertEquals(emailAddress, dialog.$.emailInput.value);
          assertEquals(emailAddress, address.emailAddresses[0]);
        });
      });
    });

    test('verifyPhoneAndEmailAreRemoved', function() {
      const address = FakeDataMaker.emptyAddressEntry();

      const phoneNumber = '(555) 555-5555';
      const emailAddress = 'no-reply@chromium.org';

      address.countryCode = 'US';  // Set to allow save to be active.
      address.phoneNumbers = [phoneNumber];
      address.emailAddresses = [emailAddress];

      return self.createAddressDialog_(address).then(function(dialog) {
        assertEquals(phoneNumber, dialog.$.phoneInput.value);
        assertEquals(emailAddress, dialog.$.emailInput.value);

        dialog.$.phoneInput.value = '';
        dialog.$.emailInput.value = '';

        return expectEvent(dialog, 'save-address', function() {
                 MockInteractions.tap(dialog.$.saveButton);
               }).then(function() {
          assertEquals(0, address.phoneNumbers.length);
          assertEquals(0, address.emailAddresses.length);
        });
      });
    });

    // Test will set a value of 'foo' in each text field and verify that the
    // save button is enabled, then it will clear the field and verify that the
    // save button is disabled. Test passes after all elements have been tested.
    test('verifySaveIsNotClickableIfAllInputFieldsAreEmpty', function() {
      return self.createAddressDialog_(FakeDataMaker.emptyAddressEntry())
          .then(function(dialog) {
            const saveButton = dialog.$.saveButton;
            const testElements =
                dialog.$.dialog.querySelectorAll('paper-input,paper-textarea');

            // Default country is 'US' expecting: Name, Organization,
            // Street address, City, State, ZIP code, Phone, and Email.
            assertEquals(8, testElements.length);

            return asyncForEach(testElements, function(element) {
              return expectEvent(
                         dialog, 'on-update-can-save',
                         function() {
                           assertTrue(saveButton.disabled);
                           element.value = 'foo';
                         })
                  .then(function() {
                    return expectEvent(
                        dialog, 'on-update-can-save', function() {
                          assertFalse(saveButton.disabled);
                          element.value = '';
                        });
                  })
                  .then(function() {
                    assertTrue(saveButton.disabled);
                  });
            });
          });
    });

    // Setting the country should allow the address to be saved.
    test('verifySaveIsNotClickableIfCountryNotSet', function() {
      let dialog = null;

      const simulateCountryChange = function(countryCode) {
        const countrySelect = dialog.$$('select');
        countrySelect.value = countryCode;
        countrySelect.dispatchEvent(new CustomEvent('change'));
      };

      return self.createAddressDialog_(FakeDataMaker.emptyAddressEntry())
          .then(function(d) {
            dialog = d;
            assertTrue(dialog.$.saveButton.disabled);

            return expectEvent(
                dialog, 'on-update-can-save',
                simulateCountryChange.bind(null, 'US'));
          })
          .then(function() {
            assertFalse(dialog.$.saveButton.disabled);

            return expectEvent(
                dialog, 'on-update-can-save',
                simulateCountryChange.bind(null, ''));
          })
          .then(function() {
            assertTrue(dialog.$.saveButton.disabled);
          });
    });

    // Test will timeout if save-address event is not fired.
    test('verifyDefaultCountryIsAppliedWhenSaving', function() {
      const address = FakeDataMaker.emptyAddressEntry();
      address.companyName = 'Google';
      return self.createAddressDialog_(address).then(function(dialog) {
        return expectEvent(dialog, 'save-address', function() {
                 // Verify |countryCode| is not set.
                 assertEquals(undefined, address.countryCode);
                 MockInteractions.tap(dialog.$.saveButton);
               }).then(function(event) {
          // 'US' is the default country for these tests.
          assertEquals('US', event.detail.countryCode);
        });
      });
    });

    test('verifyCancelDoesNotSaveAddress', function(done) {
      self.createAddressDialog_(FakeDataMaker.addressEntry())
          .then(function(dialog) {
            dialog.addEventListener('save-address', function() {
              // Fail the test because the save event should not be called when
              // cancel is clicked.
              assertTrue(false);
            });

            dialog.addEventListener('close', function() {
              // Test is |done| in a timeout in order to ensure that
              // 'save-address' is NOT fired after this test.
              window.setTimeout(done, 100);
            });

            MockInteractions.tap(dialog.$.cancelButton);
          });
    });
  });

  mocha.run();
});

TEST_F('SettingsAutofillSectionBrowserTest', 'AddressLocaleTests', function() {
  const self = this;

  suite('AutofillSection', function() {
    suiteSetup(function() {
      settings.address.CountryDetailManagerImpl.instance_ =
          new CountryDetailManagerTestImpl();
    });

    setup(function() {
      PolymerTest.clearBody();
    });

    // US address has 3 fields on the same line.
    test('verifyEditingUSAddress', function() {
      const address = FakeDataMaker.emptyAddressEntry();

      address.fullNames = ['Name'];
      address.companyName = 'Organization';
      address.addressLines = 'Street address';
      address.addressLevel2 = 'City';
      address.addressLevel1 = 'State';
      address.postalCode = 'ZIP code';
      address.countryCode = 'US';
      address.phoneNumbers = ['Phone'];
      address.emailAddresses = ['Email'];

      return self.createAddressDialog_(address).then(function(dialog) {
        const rows = dialog.$.dialog.querySelectorAll('.address-row');
        assertEquals(6, rows.length);

        // Name
        let row = rows[0];
        let cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.fullNames[0], cols[0].value);
        // Organization
        row = rows[1];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.companyName, cols[0].value);
        // Street address
        row = rows[2];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.addressLines, cols[0].value);
        // City, State, ZIP code
        row = rows[3];
        cols = row.querySelectorAll('.address-column');
        assertEquals(3, cols.length);
        assertEquals(address.addressLevel2, cols[0].value);
        assertEquals(address.addressLevel1, cols[1].value);
        assertEquals(address.postalCode, cols[2].value);
        // Country
        row = rows[4];
        const countrySelect = row.querySelector('select');
        assertTrue(!!countrySelect);
        assertEquals(
            'United States',
            countrySelect.selectedOptions[0].textContent.trim());
        // Phone, Email
        row = rows[5];
        cols = row.querySelectorAll('.address-column');
        assertEquals(2, cols.length);
        assertEquals(address.phoneNumbers[0], cols[0].value);
        assertEquals(address.emailAddresses[0], cols[1].value);
      });
    });

    // GB address has 1 field per line for all lines that change.
    test('verifyEditingGBAddress', function() {
      const address = FakeDataMaker.emptyAddressEntry();

      address.fullNames = ['Name'];
      address.companyName = 'Organization';
      address.addressLines = 'Street address';
      address.addressLevel2 = 'Post town';
      address.postalCode = 'Postal code';
      address.countryCode = 'GB';
      address.phoneNumbers = ['Phone'];
      address.emailAddresses = ['Email'];

      return self.createAddressDialog_(address).then(function(dialog) {
        const rows = dialog.$.dialog.querySelectorAll('.address-row');
        assertEquals(7, rows.length);

        // Name
        let row = rows[0];
        let cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.fullNames[0], cols[0].value);
        // Organization
        row = rows[1];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.companyName, cols[0].value);
        // Street address
        row = rows[2];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.addressLines, cols[0].value);
        // Post Town
        row = rows[3];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.addressLevel2, cols[0].value);
        // Postal code
        row = rows[4];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.postalCode, cols[0].value);
        // Country
        row = rows[5];
        const countrySelect = row.querySelector('select');
        assertTrue(!!countrySelect);
        assertEquals(
            'United Kingdom',
            countrySelect.selectedOptions[0].textContent.trim());
        // Phone, Email
        row = rows[6];
        cols = row.querySelectorAll('.address-column');
        assertEquals(2, cols.length);
        assertEquals(address.phoneNumbers[0], cols[0].value);
        assertEquals(address.emailAddresses[0], cols[1].value);
      });
    });

    // IL address has 2 fields on the same line and is an RTL locale.
    // RTL locale shouldn't affect this test.
    test('verifyEditingILAddress', function() {
      const address = FakeDataMaker.emptyAddressEntry();

      address.fullNames = ['Name'];
      address.companyName = 'Organization';
      address.addressLines = 'Street address';
      address.addressLevel2 = 'City';
      address.postalCode = 'Postal code';
      address.countryCode = 'IL';
      address.phoneNumbers = ['Phone'];
      address.emailAddresses = ['Email'];

      return self.createAddressDialog_(address).then(function(dialog) {
        const rows = dialog.$.dialog.querySelectorAll('.address-row');
        assertEquals(6, rows.length);

        // Name
        let row = rows[0];
        let cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.fullNames[0], cols[0].value);
        // Organization
        row = rows[1];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.companyName, cols[0].value);
        // Street address
        row = rows[2];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.addressLines, cols[0].value);
        // City, Postal code
        row = rows[3];
        cols = row.querySelectorAll('.address-column');
        assertEquals(2, cols.length);
        assertEquals(address.addressLevel2, cols[0].value);
        assertEquals(address.postalCode, cols[1].value);
        // Country
        row = rows[4];
        const countrySelect = row.querySelector('select');
        assertTrue(!!countrySelect);
        assertEquals(
            'Israel', countrySelect.selectedOptions[0].textContent.trim());
        // Phone, Email
        row = rows[5];
        cols = row.querySelectorAll('.address-column');
        assertEquals(2, cols.length);
        assertEquals(address.phoneNumbers[0], cols[0].value);
        assertEquals(address.emailAddresses[0], cols[1].value);
      });
    });

    // US has an extra field 'State'. Validate that this field is
    // persisted when switching to IL then back to US.
    test('verifyAddressPersistanceWhenSwitchingCountries', function() {
      const address = FakeDataMaker.emptyAddressEntry();
      address.countryCode = 'US';

      return self.createAddressDialog_(address).then(function(dialog) {
        const city = 'Los Angeles';
        const state = 'CA';
        const zip = '90291';
        const countrySelect = dialog.$$('select');

        return expectEvent(
                   dialog, 'on-update-address-wrapper',
                   function() {
                     // US:
                     const rows =
                         dialog.$.dialog.querySelectorAll('.address-row');
                     assertEquals(6, rows.length);

                     // City, State, ZIP code
                     const row = rows[3];
                     const cols = row.querySelectorAll('.address-column');
                     assertEquals(3, cols.length);
                     cols[0].value = city;
                     cols[1].value = state;
                     cols[2].value = zip;

                     countrySelect.value = 'IL';
                     countrySelect.dispatchEvent(new CustomEvent('change'));
                   })
            .then(function() {
              return expectEvent(
                  dialog, 'on-update-address-wrapper', function() {
                    // IL:
                    rows = dialog.$.dialog.querySelectorAll('.address-row');
                    assertEquals(6, rows.length);

                    // City, Postal code
                    row = rows[3];
                    cols = row.querySelectorAll('.address-column');
                    assertEquals(2, cols.length);
                    assertEquals(city, cols[0].value);
                    assertEquals(zip, cols[1].value);

                    countrySelect.value = 'US';
                    countrySelect.dispatchEvent(new CustomEvent('change'));
                  });
            })
            .then(function() {
              // US:
              const rows = dialog.$.dialog.querySelectorAll('.address-row');
              assertEquals(6, rows.length);

              // City, State, ZIP code
              row = rows[3];
              cols = row.querySelectorAll('.address-column');
              assertEquals(3, cols.length);
              assertEquals(city, cols[0].value);
              assertEquals(state, cols[1].value);
              assertEquals(zip, cols[2].value);
            });
      });
    });
  });

  mocha.run();
});
