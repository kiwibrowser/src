// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.multidex.ShadowMultiDex;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.components.offline_items_collection.ContentId;
import org.chromium.components.offline_items_collection.LegacyHelpers;

import java.util.UUID;

/** Unit tests for {@link DownloadSharedPreferenceEntry}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, shadows = {ShadowMultiDex.class})
public class DownloadSharedPreferenceEntryTest {
    private String newUUID() {
        return UUID.randomUUID().toString();
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringVersion1() {
        String uuid = newUUID();
        String notificationString = "1,2,1,1," + uuid + ",test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(2, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(false, uuid), entry.id);
        assertFalse(entry.isOffTheRecord);
        assertTrue(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,2.pdf", entry.fileName);

        String uuid2 = newUUID();
        notificationString = "1,3,0,0," + uuid2 + ",test,4.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(3, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(false, uuid2), entry.id);
        assertTrue(entry.isOffTheRecord);
        assertFalse(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,4.pdf", entry.fileName);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringVersion2() {
        String uuid = newUUID();
        String notificationString = "2,2,0,1," + uuid + ",test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(2, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(false, uuid), entry.id);
        assertFalse(entry.isOffTheRecord);
        assertTrue(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,2.pdf", entry.fileName);

        String uuid2 = newUUID();
        notificationString = "2,3,1,0," + uuid2 + ",test,4.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(3, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(false, uuid2), entry.id);
        assertTrue(entry.isOffTheRecord);
        assertFalse(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,4.pdf", entry.fileName);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringVersion3_Download() {
        String uuid = newUUID();
        String notificationString = "3,2,1,0,1," + uuid + ",test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(2, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(false, uuid), entry.id);
        assertFalse(entry.isOffTheRecord);
        assertTrue(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,2.pdf", entry.fileName);

        String uuid2 = newUUID();
        notificationString = "3,3,1,1,0," + uuid2 + ",test,4.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(3, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(false, uuid2), entry.id);
        assertTrue(entry.isOffTheRecord);
        assertFalse(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,4.pdf", entry.fileName);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringVersion3_OfflinePage() {
        String uuid = newUUID();
        String notificationString = "3,2,2,0,1," + uuid + ",test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(2, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(true, uuid), entry.id);
        assertFalse(entry.isOffTheRecord);
        assertTrue(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,2.pdf", entry.fileName);

        String uuid2 = newUUID();
        notificationString = "3,3,2,1,0," + uuid2 + ",test,4.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(3, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(true, uuid2), entry.id);
        assertTrue(entry.isOffTheRecord);
        assertFalse(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,4.pdf", entry.fileName);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringVersion4_Download() {
        String uuid = newUUID();
        String notificationString = "4,2,1,0,1,1," + uuid + ",test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(2, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(false, uuid), entry.id);
        assertFalse(entry.isOffTheRecord);
        assertTrue(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,2.pdf", entry.fileName);

        String uuid2 = newUUID();
        notificationString = "4,3,1,1,0,0," + uuid2 + ",test,4.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(3, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(false, uuid2), entry.id);
        assertTrue(entry.isOffTheRecord);
        assertFalse(entry.canDownloadWhileMetered);
        assertFalse(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,4.pdf", entry.fileName);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringVersion5() {
        String uuid = newUUID();
        String notificationString =
                "5,2," + LegacyHelpers.LEGACY_DOWNLOAD_NAMESPACE + "," + uuid + ",0,1,1,test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(2, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(false, uuid), entry.id);
        assertFalse(entry.isOffTheRecord);
        assertTrue(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,2.pdf", entry.fileName);

        String uuid2 = newUUID();
        notificationString = "5,3," + LegacyHelpers.LEGACY_DOWNLOAD_NAMESPACE + "," + uuid2
                + ",1,0,0,test,4.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(3, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(false, uuid2), entry.id);
        assertTrue(entry.isOffTheRecord);
        assertFalse(entry.canDownloadWhileMetered);
        assertFalse(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,4.pdf", entry.fileName);

        String uuid3 = newUUID();
        notificationString = "5,3," + LegacyHelpers.LEGACY_OFFLINE_PAGE_NAMESPACE + "," + uuid3
                + ",1,0,0,test,4.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(3, entry.notificationId);
        assertEquals(LegacyHelpers.buildLegacyContentId(true, uuid3), entry.id);
        assertTrue(entry.isOffTheRecord);
        assertFalse(entry.canDownloadWhileMetered);
        assertFalse(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,4.pdf", entry.fileName);

        String uuid4 = newUUID();
        notificationString = "5,3,test_namespace," + uuid4 + ",1,0,0,test,4.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(3, entry.notificationId);
        assertEquals(new ContentId("test_namespace", uuid4), entry.id);
        assertTrue(entry.isOffTheRecord);
        assertFalse(entry.canDownloadWhileMetered);
        assertFalse(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,4.pdf", entry.fileName);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringVersion6() {
        String id1 = newUUID();
        String notificationString = "6,2,test_namespace," + id1 + ",0,1,1,1,test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(2, entry.notificationId);
        assertEquals(new ContentId("test_namespace", id1), entry.id);
        assertFalse(entry.isOffTheRecord);
        assertTrue(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertTrue(entry.isTransient);
        assertEquals("test,2.pdf", entry.fileName);

        String id2 = "notaguidhurray";
        notificationString = "6,3,test_namespace," + id2 + ",1,0,0,0,test,4.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(3, entry.notificationId);
        assertEquals(new ContentId("test_namespace", id2), entry.id);
        assertTrue(entry.isOffTheRecord);
        assertFalse(entry.canDownloadWhileMetered);
        assertFalse(entry.isAutoResumable);
        assertFalse(entry.isTransient);
        assertEquals("test,4.pdf", entry.fileName);
    }

    @Test
    @Feature({"Download"})
    public void testGetSharedPreferencesString() {
        String uuid = newUUID();
        String notificationString = "6,2," + LegacyHelpers.LEGACY_OFFLINE_PAGE_NAMESPACE + ","
                + uuid + ",0,1,1,1,test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(notificationString, entry.getSharedPreferenceString());

        String uuid2 = newUUID();
        notificationString = "6,3," + LegacyHelpers.LEGACY_OFFLINE_PAGE_NAMESPACE + "," + uuid2
                + ",1,0,0,0,test,4.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(notificationString, entry.getSharedPreferenceString());
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringInvalidVersion() {
        String uuid = newUUID();
        // Version 0 is invalid.
        String notificationString = "0,2,0,1," + uuid + ",test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Version 4 has to have 8 field.
        notificationString = "4,2,0,1,0," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Version number is missing.
        notificationString = ",2,2,0,1," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not possible to parse version number.
        notificationString = "xxx,2,2,0,1," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringNotEnoughValues() {
        String uuid = newUUID();
        // Version 1 requires at least 6.
        String notificationString = "1,2,0," + uuid + ",test.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Version 2 requires at least 6.
        notificationString = "2,2,0," + uuid + ",test.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Version 3 requires at least 7.
        notificationString = "3,2,2,0," + uuid + ",test.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Version 4 requires at least 8.
        notificationString = "4,2,2,0,1," + uuid + ",test.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Version 5 requires at least 8.
        notificationString = "5,2,test_namespace," + uuid + "0,1,test.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Version 6 requires at least 9.
        notificationString = "6,2,test_namespace," + uuid + "0,1,1,test.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringFailedToParseNotificationId() {
        String uuid = newUUID();
        // Notification ID missing in version 1.
        String notificationString = "1,,1,0," + uuid + ",test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Notification ID missing in version 2.
        notificationString = "2,,1,0," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Notification ID missing in version 3.
        notificationString = "3,,2,0,1," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Notification ID missing in version 4.
        notificationString = "4,,2,0,1,1," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Notification ID missing in version 5.
        notificationString = "5,,test_namespace," + uuid + ",0,1,1,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Notification ID missing in version 6.
        notificationString = "6,,test_namespace," + uuid + ",0,1,1,1,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse notification ID in version 1.
        notificationString = "1,xxx,0,1," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse notification ID in version 2.
        notificationString = "2,xxx,1,0," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse notification ID in version 3.
        notificationString = "3,xxx,2,0,1," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse notification ID in version 4.
        notificationString = "4,xxx,2,0,1,1," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse notification ID in version 5.
        notificationString = "5,xxx,test_namespace," + uuid + ",0,1,1,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse notification ID in version 6.
        notificationString = "6,xxx,test_namespace," + uuid + ",0,1,1,1,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringFailedToParseItemType() {
        String uuid = newUUID();
        // Item type is only present in version 3. (Position 3)
        // Invalid item type.
        String notificationString = "3,2,0,1,0," + uuid + ",test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Invalid item type.
        notificationString = "3,2,3,0,1," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse item type.
        notificationString = "3,2,xxx,0,1," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Missing item type.
        notificationString = "3,2,,0,1," + uuid + ",test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringInvalidGuid() {
        // GUID missing in version 1.
        String notificationString = "1,2,1,0,,test,2.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // GUID missing in version 2.
        notificationString = "2,2,1,0,,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // GUID missing in version 3.
        notificationString = "3,2,2,0,1,,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // GUID missing in version 4.
        notificationString = "4,2,2,0,1,1,,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // GUID missing in version 5.
        notificationString = "5,2,test_namespace,,0,1,1,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // GUID missing in version 6.
        notificationString = "6,2,test_namespace,,0,1,1,1,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse GUID in version 1.
        notificationString = "1,2,0,1,xxx,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse GUID in version 2.
        notificationString = "2,2,1,0,xxx,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse GUID in version 3.
        notificationString = "3,2,2,0,1,xxx,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse GUID in version 4.
        notificationString = "4,2,2,0,1,1,xxx,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Not able to parse GUID in version 5.
        notificationString = "5,2,test_namespace,xxx,0,1,1,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);

        // Able to load an invalid GUID in version 6.
        notificationString = "6,2,test_namespace,xxx,0,1,1,1,test,2.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(2, entry.notificationId);
        assertEquals(new ContentId("test_namespace", "xxx"), entry.id);
        assertFalse(entry.isOffTheRecord);
        assertTrue(entry.canDownloadWhileMetered);
        assertTrue(entry.isAutoResumable);
        assertTrue(entry.isTransient);
        assertEquals("test,2.pdf", entry.fileName);
    }

    @Test
    @Feature({"Download"})
    public void testParseFromStringMissingNamespace() {
        // Not able to parse namespace in version 5.
        String notificationString = "6,3,," + newUUID() + ",1,0,0,0,test,4.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(DownloadSharedPreferenceEntry.INVALID_ENTRY, entry);
    }

    @Test
    @Feature({"Download"})
    public void testConvertFromEarlierVersions() {
        // Convert from version 1.
        String uuid = newUUID();
        String notificationString = "1,1,1,0," + uuid + ",test.pdf";
        String v6NotificationString =
                "6,1," + LegacyHelpers.LEGACY_DOWNLOAD_NAMESPACE + "," + uuid + ",0,0,1,0,test.pdf";
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(v6NotificationString, entry.getSharedPreferenceString());

        // Convert from version 2.
        notificationString = "2,1,1,0," + uuid + ",test.pdf";
        v6NotificationString =
                "6,1," + LegacyHelpers.LEGACY_DOWNLOAD_NAMESPACE + "," + uuid + ",1,0,1,0,test.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(v6NotificationString, entry.getSharedPreferenceString());

        // Convert from version 3.
        notificationString = "3,2,2,1,0," + uuid + ",test.pdf";
        v6NotificationString = "6,2," + LegacyHelpers.LEGACY_OFFLINE_PAGE_NAMESPACE + "," + uuid
                + ",1,0,1,0,test.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(v6NotificationString, entry.getSharedPreferenceString());

        // Convert from version 4.
        notificationString = "4,2,2,1,0,0," + uuid + ",test.pdf";
        v6NotificationString = "6,2," + LegacyHelpers.LEGACY_OFFLINE_PAGE_NAMESPACE + "," + uuid
                + ",1,0,0,0,test.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(v6NotificationString, entry.getSharedPreferenceString());

        // Convert from version 5.
        notificationString = "5,2,test_namespace," + uuid + ",1,0,0,test.pdf";
        v6NotificationString = "6,2,test_namespace," + uuid + ",1,0,0,0,test.pdf";
        entry = DownloadSharedPreferenceEntry.parseFromString(notificationString);
        assertEquals(v6NotificationString, entry.getSharedPreferenceString());
    }
}
