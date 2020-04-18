/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.compilationTest;

import org.junit.Test;

import android.databinding.tool.processing.ErrorMessages;
import android.databinding.tool.processing.ScopedErrorReport;
import android.databinding.tool.processing.ScopedException;
import android.databinding.tool.store.Location;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

public class MultiLayoutVerificationTest extends BaseCompilationTest {
    @Test
    public void testMultipleLayoutFilesWithNameMismatch()
            throws IOException, URISyntaxException, InterruptedException {
        prepareProject();
        copyResourceTo("/layout/layout_with_class_name.xml",
                "/app/src/main/res/layout/with_class_name.xml", toMap(KEY_CLASS_NAME,
                        "AClassName"));
        copyResourceTo("/layout/layout_with_class_name.xml",
                "/app/src/main/res/layout-land/with_class_name.xml", toMap(KEY_CLASS_NAME,
                        "SomeOtherClassName"));
        CompilationResult result = runGradle("assembleDebug");
        assertNotEquals(result.output, 0, result.resultCode);
        List<ScopedException> exceptions = result.getBindingExceptions();
        assertEquals(result.error, 2, exceptions.size());
        boolean foundNormal = false;
        boolean foundLandscape = false;
        for (ScopedException exception : exceptions) {
            ScopedErrorReport report = exception.getScopedErrorReport();
            assertNotNull(report);
            File file = new File(report.getFilePath());
            assertTrue(file.exists());
            assertEquals(1, report.getLocations().size());
            Location location = report.getLocations().get(0);
            String name = file.getParentFile().getName();
            if ("layout".equals(name)) {
                assertEquals(new File(testFolder,
                        "/app/src/main/res/layout/with_class_name.xml")
                        .getCanonicalFile(), file.getCanonicalFile());
                String extract = extract("/app/src/main/res/layout/with_class_name.xml",
                        location);
                assertEquals(extract, "AClassName");
                assertEquals(String.format(
                        ErrorMessages.MULTI_CONFIG_LAYOUT_CLASS_NAME_MISMATCH,
                        DEFAULT_APP_PACKAGE + ".databinding.AClassName",
                        "layout/with_class_name"), exception.getBareMessage());
                foundNormal = true;
            } else if ("layout-land".equals(name)) {
                    assertEquals(new File(testFolder,
                            "/app/src/main/res/layout-land/with_class_name.xml")
                            .getCanonicalFile(), file.getCanonicalFile());
                    String extract = extract("/app/src/main/res/layout-land/with_class_name.xml",
                            location);
                    assertEquals("SomeOtherClassName", extract);
                    assertEquals(String.format(
                            ErrorMessages.MULTI_CONFIG_LAYOUT_CLASS_NAME_MISMATCH,
                            DEFAULT_APP_PACKAGE + ".databinding.SomeOtherClassName",
                            "layout-land/with_class_name"), exception.getBareMessage());
                    foundLandscape = true;
            } else {
                fail("unexpected error file");
            }
        }
        assertTrue("should find default config error\n" + result.error, foundNormal);
        assertTrue("should find landscape error\n" + result.error, foundLandscape);
    }

    @Test
    public void testMultipleLayoutFilesVariableMismatch()
            throws IOException, URISyntaxException, InterruptedException {
        prepareProject();
        copyResourceTo("/layout/layout_with_variable_type.xml",
                "/app/src/main/res/layout/layout_with_variable_type.xml", toMap(KEY_CLASS_TYPE,
                        "String"));
        copyResourceTo("/layout/layout_with_variable_type.xml",
                "/app/src/main/res/layout-land/layout_with_variable_type.xml", toMap(KEY_CLASS_TYPE,
                        "CharSequence"));
        CompilationResult result = runGradle("assembleDebug");
        assertNotEquals(result.output, 0, result.resultCode);
        List<ScopedException> exceptions = result.getBindingExceptions();
        assertEquals(result.error, 2, exceptions.size());
        boolean foundNormal = false;
        boolean foundLandscape = false;
        for (ScopedException exception : exceptions) {
            ScopedErrorReport report = exception.getScopedErrorReport();
            assertNotNull(report);
            File file = new File(report.getFilePath());
            assertTrue(file.exists());
            assertEquals(result.error, 1, report.getLocations().size());
            Location location = report.getLocations().get(0);
            // validated in switch
            String name = file.getParentFile().getName();
            String config = name;
            String type = "???";
            if ("layout".equals(name)) {
                type = "String";
                foundNormal = true;
            } else if ("layout-land".equals(name)) {
                type = "CharSequence";
                foundLandscape = true;
            } else {
                fail("unexpected error file");
            }
            assertEquals(new File(testFolder,
                    "/app/src/main/res/" + config + "/layout_with_variable_type.xml")
                    .getCanonicalFile(), file.getCanonicalFile());
            String extract = extract("/app/src/main/res/" + config +
                            "/layout_with_variable_type.xml", location);
            assertEquals(extract, "<variable name=\"myVariable\" type=\"" + type + "\"/>");
            assertEquals(String.format(
                    ErrorMessages.MULTI_CONFIG_VARIABLE_TYPE_MISMATCH,
                    "myVariable", type,
                    config + "/layout_with_variable_type"), exception.getBareMessage());
        }
        assertTrue(result.error, foundNormal);
        assertTrue(result.error, foundLandscape);
    }

    @Test
    public void testMultipleLayoutFilesImportMismatch()
            throws IOException, URISyntaxException, InterruptedException {
        prepareProject();
        String typeNormal = "java.util.List";
        String typeLand = "java.util.Map";
        copyResourceTo("/layout/layout_with_import_type.xml",
                "/app/src/main/res/layout/layout_with_import_type.xml", toMap(KEY_IMPORT_TYPE,
                        typeNormal));
        copyResourceTo("/layout/layout_with_import_type.xml",
                "/app/src/main/res/layout-land/layout_with_import_type.xml", toMap(KEY_IMPORT_TYPE,
                        typeLand));
        CompilationResult result = runGradle("assembleDebug");
        assertNotEquals(result.output, 0, result.resultCode);
        List<ScopedException> exceptions = result.getBindingExceptions();
        assertEquals(result.error, 2, exceptions.size());
        boolean foundNormal = false;
        boolean foundLandscape = false;
        for (ScopedException exception : exceptions) {
            ScopedErrorReport report = exception.getScopedErrorReport();
            assertNotNull(report);
            File file = new File(report.getFilePath());
            assertTrue(file.exists());
            assertEquals(result.error, 1, report.getLocations().size());
            Location location = report.getLocations().get(0);
            // validated in switch
            String name = file.getParentFile().getName();
            String config = name;
            String type = "???";
            if ("layout".equals(name)) {
                type = typeNormal;
                foundNormal = true;
            } else if ("layout-land".equals(name)) {
                type = typeLand;
                foundLandscape = true;
            } else {
                fail("unexpected error file");
            }
            assertEquals(new File(testFolder,
                    "/app/src/main/res/" + config + "/layout_with_import_type.xml")
                    .getCanonicalFile(), file.getCanonicalFile());
            String extract = extract("/app/src/main/res/" + config + "/layout_with_import_type.xml",
                    location);
            assertEquals(extract, "<import alias=\"Blah\" type=\"" + type + "\"/>");
            assertEquals(String.format(
                    ErrorMessages.MULTI_CONFIG_IMPORT_TYPE_MISMATCH,
                    "Blah", type,
                    config + "/layout_with_import_type"), exception.getBareMessage());
        }
        assertTrue(result.error, foundNormal);
        assertTrue(result.error, foundLandscape);
    }

    @Test
    public void testSameIdInIncludeAndView()
            throws IOException, URISyntaxException, InterruptedException {
        prepareProject();
        copyResourceTo("/layout/basic_layout.xml",
                "/app/src/main/res/layout/basic_layout.xml");
        copyResourceTo("/layout/layout_with_include.xml",
                "/app/src/main/res/layout/foo.xml", toMap(KEY_INCLUDE_ID, "sharedId"));
        copyResourceTo("/layout/layout_with_view_id.xml",
                "/app/src/main/res/layout-land/foo.xml", toMap(KEY_VIEW_ID, "sharedId"));
        CompilationResult result = runGradle("assembleDebug");
        assertNotEquals(result.output, 0, result.resultCode);
        List<ScopedException> exceptions = result.getBindingExceptions();
        assertEquals(result.error, 2, exceptions.size());

        boolean foundNormal = false;
        boolean foundLandscape = false;
        for (ScopedException exception : exceptions) {
            ScopedErrorReport report = exception.getScopedErrorReport();
            assertNotNull(report);
            File file = new File(report.getFilePath());
            assertTrue(file.exists());
            assertEquals(result.error, 1, report.getLocations().size());
            Location location = report.getLocations().get(0);
            // validated in switch
            String config = file.getParentFile().getName();
            if ("layout".equals(config)) {
                String extract = extract("/app/src/main/res/" + config + "/foo.xml", location);
                assertEquals(extract, "<include layout=\"@layout/basic_layout\" "
                        + "android:id=\"@+id/sharedId\" bind:myVariable=\"@{myVariable}\"/>");
                foundNormal = true;
            } else if ("layout-land".equals(config)) {
                String extract = extract("/app/src/main/res/" + config + "/foo.xml", location);
                assertEquals(extract, "<TextView android:layout_width=\"wrap_content\" "
                        + "android:layout_height=\"wrap_content\" android:id=\"@+id/sharedId\" "
                        + "android:text=\"@{myVariable}\"/>");
                foundLandscape = true;
            } else {
                fail("unexpected error file");
            }
            assertEquals(new File(testFolder,
                    "/app/src/main/res/" + config + "/foo.xml").getCanonicalFile(),
                    file.getCanonicalFile());
            assertEquals(String.format(
                    ErrorMessages.MULTI_CONFIG_ID_USED_AS_IMPORT, "@+id/sharedId"),
                    exception.getBareMessage());
        }
        assertTrue(result.error, foundNormal);
        assertTrue(result.error, foundLandscape);
    }


}
