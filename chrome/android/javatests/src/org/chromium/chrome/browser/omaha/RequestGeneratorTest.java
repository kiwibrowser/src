// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omaha;

import android.accounts.Account;
import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.identity.SettingsSecureBasedIdentificationGenerator;
import org.chromium.chrome.browser.identity.UniqueIdentificationGenerator;
import org.chromium.chrome.browser.identity.UniqueIdentificationGeneratorFactory;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.omaha.AttributeFinder;
import org.chromium.chrome.test.omaha.MockRequestGenerator;
import org.chromium.chrome.test.omaha.MockRequestGenerator.DeviceType;
import org.chromium.chrome.test.omaha.MockRequestGenerator.SignedInStatus;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.test.util.AccountHolder;
import org.chromium.components.signin.test.util.FakeAccountManagerDelegate;

/**
 * Unit tests for the RequestGenerator class.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class RequestGeneratorTest {
    private static final String INSTALL_SOURCE = "install_source";

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testInstallAgeNewInstallation() {
        long currentTimestamp = 201207310000L;
        long installTimestamp = 198401160000L;
        boolean installing = true;
        long expectedAge = RequestGenerator.INSTALL_AGE_IMMEDIATELY_AFTER_INSTALLING;
        checkInstallAge(currentTimestamp, installTimestamp, installing, expectedAge);
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testInstallAge() {
        long currentTimestamp = 201207310000L;
        long installTimestamp = 198401160000L;
        boolean installing = false;
        long expectedAge = 32;
        checkInstallAge(currentTimestamp, installTimestamp, installing, expectedAge);
    }

    /**
     * Checks whether the install age function is behaving according to spec.
     */
    void checkInstallAge(long currentTimestamp, long installTimestamp, boolean installing,
            long expectedAge) {
        long actualAge = RequestGenerator.installAge(currentTimestamp, installTimestamp,
                installing);
        Assert.assertEquals("Install ages differed.", expectedAge, actualAge);
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testConstructorRegistersIdentificationGenerator() {
        Context targetContext = InstrumentationRegistry.getTargetContext();
        AdvancedMockContext context = new AdvancedMockContext(targetContext);

        // First clear the current set of generators.
        UniqueIdentificationGeneratorFactory.clearGeneratorMapForTest();

        // Creating a RequestGenerator should register the identification generator.
        new MockRequestGenerator(context, DeviceType.HANDSET, SignedInStatus.FALSE);

        // Verify the identification generator exists and is of the correct type.
        UniqueIdentificationGenerator instance = UniqueIdentificationGeneratorFactory.getInstance(
                SettingsSecureBasedIdentificationGenerator.GENERATOR_ID);
        Assert.assertTrue(instance instanceof SettingsSecureBasedIdentificationGenerator);
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testHandsetXMLCreationWithInstall() {
        createAndCheckXML(DeviceType.HANDSET, SignedInStatus.FALSE, true);
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testHandsetXMLCreationWithoutInstall() {
        createAndCheckXML(DeviceType.HANDSET, SignedInStatus.FALSE, false);
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testTabletXMLCreationWithInstall() {
        createAndCheckXML(DeviceType.TABLET, SignedInStatus.FALSE, true);
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testTabletXMLCreationWithoutInstall() {
        createAndCheckXML(DeviceType.TABLET, SignedInStatus.FALSE, false);
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testIsSignedIn() {
        createAndCheckXML(DeviceType.HANDSET, SignedInStatus.TRUE, false);
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testIsNotSignedIn() {
        createAndCheckXML(DeviceType.HANDSET, SignedInStatus.FALSE, false);
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testNoGoogleAccountsRetrieved() {
        RequestGenerator generator =
                createAndCheckXML(DeviceType.HANDSET, SignedInStatus.TRUE, false);
        Assert.assertEquals(0, generator.getNumGoogleAccountsOnDevice());
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testOneGoogleAccountRetrieved() {
        RequestGenerator generator = createAndCheckXML(DeviceType.HANDSET, SignedInStatus.TRUE,
                false, new Account("clanktester@this.com", "com.google"));
        Assert.assertEquals(1, generator.getNumGoogleAccountsOnDevice());
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testTwoGoogleAccountsRetrieved() {
        RequestGenerator generator = createAndCheckXML(DeviceType.HANDSET, SignedInStatus.TRUE,
                false, new Account("clanktester@gmail.com", "com.google"),
                new Account("googleguy@elsewhere.com", "com.google"));
        Assert.assertEquals(2, generator.getNumGoogleAccountsOnDevice());
    }

    @Test
    @SmallTest
    @Feature({"Omaha"})
    public void testThreeGoogleAccountsExist() {
        RequestGenerator generator = createAndCheckXML(DeviceType.HANDSET, SignedInStatus.TRUE,
                false, new Account("clanktester@gmail.com", "com.google"),
                new Account("googleguy@elsewhere.com", "com.google"),
                new Account("ImInATest@gmail.com", "com.google"));
        Assert.assertEquals(2, generator.getNumGoogleAccountsOnDevice());
    }

    /**
     * Checks that the XML is being created properly.
     */
    private RequestGenerator createAndCheckXML(DeviceType deviceType, SignedInStatus signInStatus,
            boolean sendInstallEvent, Account... accounts) {
        Context targetContext = InstrumentationRegistry.getTargetContext();
        AdvancedMockContext context = new AdvancedMockContext(targetContext);

        FakeAccountManagerDelegate accountManager = new FakeAccountManagerDelegate(
                FakeAccountManagerDelegate.DISABLE_PROFILE_DATA_SOURCE);
        for (Account account : accounts) {
            accountManager.addAccountHolderExplicitly(AccountHolder.builder(account).build());
        }
        AccountManagerFacade.overrideAccountManagerFacadeForTests(accountManager);

        String sessionId = "random_session_id";
        String requestId = "random_request_id";
        String version = "1.2.3.4";
        long installAge = 42;

        MockRequestGenerator generator =
                new MockRequestGenerator(context, deviceType, signInStatus);

        String xml = null;
        try {
            RequestData data = new RequestData(sendInstallEvent, 0, requestId, INSTALL_SOURCE);
            xml = generator.generateXML(sessionId, version, installAge, data);
        } catch (RequestFailureException e) {
            Assert.fail("XML generation failed.");
        }

        checkForAttributeAndValue(xml, "request", "sessionid", "{" + sessionId + "}");
        checkForAttributeAndValue(xml, "request", "requestid", "{" + requestId + "}");
        checkForAttributeAndValue(xml, "request", "installsource", INSTALL_SOURCE);

        checkForAttributeAndValue(xml, "app", "version", version);
        checkForAttributeAndValue(xml, "app", "lang", generator.getLanguage());
        checkForAttributeAndValue(xml, "app", "brand", generator.getBrand());
        checkForAttributeAndValue(xml, "app", "client", generator.getClient());
        checkForAttributeAndValue(xml, "app", "appid", generator.getAppId());
        checkForAttributeAndValue(xml, "app", "installage", String.valueOf(installAge));
        checkForAttributeAndValue(xml, "app", "ap", generator.getAdditionalParameters());

        if (sendInstallEvent) {
            checkForAttributeAndValue(xml, "event", "eventtype", "2");
            checkForAttributeAndValue(xml, "event", "eventresult", "1");
            Assert.assertFalse(
                    "Ping and install event are mutually exclusive", checkForTag(xml, "ping"));
            Assert.assertFalse("Update check and install event are mutually exclusive",
                    checkForTag(xml, "updatecheck"));
        } else {
            Assert.assertFalse("Update check and install event are mutually exclusive",
                    checkForTag(xml, "event"));
            checkForAttributeAndValue(xml, "ping", "active", "1");
            Assert.assertTrue("Update check and install event are mutually exclusive",
                    checkForTag(xml, "updatecheck"));
        }

        checkForAttributeAndValue(xml, "request", "userid", "{" + generator.getDeviceID() + "}");

        checkForAttributeAndValue(xml, "app", "_numaccounts", "1");
        checkForAttributeAndValue(xml, "app", "_numgoogleaccountsondevice",
                String.valueOf(generator.getNumGoogleAccountsOnDevice()));
        checkForAttributeAndValue(
                xml, "app", "_numsignedin", String.valueOf(generator.getNumSignedIn()));

        return generator;
    }

    private boolean checkForTag(String xml, String tag) {
        return new AttributeFinder(xml, tag, null).isTagFound();
    }

    private void checkForAttributeAndValue(
            String xml, String tag, String attribute, String expectedValue) {
        // Check that the attribute exists for the tag and that the value matches.
        AttributeFinder finder = new AttributeFinder(xml, tag, attribute);
        Assert.assertTrue("Couldn't find tag '" + tag + "'", finder.isTagFound());
        Assert.assertEquals(
                "Bad value found for tag '" + tag + "' and attribute '" + attribute + "'",
                expectedValue, finder.getValue());
    }
}
