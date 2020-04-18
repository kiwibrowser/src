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


import org.apache.commons.io.FileUtils;
import org.apache.commons.io.filefilter.PrefixFileFilter;
import org.apache.commons.io.filefilter.SuffixFileFilter;
import org.apache.commons.lang3.StringUtils;
import org.junit.Test;

import android.databinding.tool.processing.ErrorMessages;
import android.databinding.tool.processing.ScopedErrorReport;
import android.databinding.tool.processing.ScopedException;
import android.databinding.tool.store.Location;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.util.Collection;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

@SuppressWarnings("ThrowableResultOfMethodCallIgnored")
public class SimpleCompilationTest extends BaseCompilationTest {

    @Test
    public void listTasks() throws IOException, URISyntaxException, InterruptedException {
        prepareProject();
        CompilationResult result = runGradle("tasks");
        assertEquals(0, result.resultCode);
        assertTrue("there should not be any errors", StringUtils.isEmpty(result.error));
        assertTrue("Test sanity, empty project tasks",
                result.resultContainsText("All tasks runnable from root project"));
    }

    @Test
    public void testEmptyCompilation() throws IOException, URISyntaxException, InterruptedException {
        prepareProject();
        CompilationResult result = runGradle("assembleDebug");
        assertEquals(result.error, 0, result.resultCode);
        assertTrue("there should not be any errors " + result.error,
                StringUtils.isEmpty(result.error));
        assertTrue("Test sanity, should compile fine",
                result.resultContainsText("BUILD SUCCESSFUL"));
    }

    @Test
    public void testMultipleConfigs() throws IOException, URISyntaxException, InterruptedException {
        prepareProject();
        copyResourceTo("/layout/basic_layout.xml",
                "/app/src/main/res/layout/main.xml");
        copyResourceTo("/layout/basic_layout.xml",
                "/app/src/main/res/layout-sw100dp/main.xml");
        CompilationResult result = runGradle("assembleDebug");
        assertEquals(result.error, 0, result.resultCode);
        File debugOut = new File(testFolder,
                "app/build/intermediates/data-binding-layout-out/debug");
        Collection<File> layoutFiles = FileUtils.listFiles(debugOut, new SuffixFileFilter(".xml"),
                new PrefixFileFilter("layout"));
        assertTrue("test sanity", layoutFiles.size() > 1);
        for (File layout : layoutFiles) {
            final String contents = FileUtils.readFileToString(layout);
            if (layout.getParent().contains("sw100")) {
                assertTrue("File has wrong tag:" + layout.getPath(),
                        contents.indexOf("android:tag=\"layout-sw100dp/main_0\"") > 0);
            } else {
                assertTrue("File has wrong tag:" + layout.getPath() + "\n" + contents,
                        contents.indexOf("android:tag=\"layout/main_0\"")
                                > 0);
            }
        }
    }

    private ScopedException singleFileErrorTest(String resource, String targetFile,
            String expectedExtract, String errorMessage)
            throws IOException, URISyntaxException, InterruptedException {
        prepareProject();
        copyResourceTo(resource, targetFile);
        CompilationResult result = runGradle("assembleDebug");
        assertNotEquals(0, result.resultCode);
        ScopedException scopedException = result.getBindingException();
        assertNotNull(result.error, scopedException);
        ScopedErrorReport report = scopedException.getScopedErrorReport();
        assertNotNull(report);
        assertEquals(1, report.getLocations().size());
        Location loc = report.getLocations().get(0);
        if (expectedExtract != null) {
            String extract = extract(targetFile, loc);
            assertEquals(expectedExtract, extract);
        }
        final File errorFile = new File(report.getFilePath());
        assertTrue(errorFile.exists());
        assertEquals(new File(testFolder, targetFile).getCanonicalFile(),
                errorFile.getCanonicalFile());
        if (errorMessage != null) {
            assertEquals(errorMessage, scopedException.getBareMessage());
        }
        return scopedException;
    }

    @Test
    public void testMultipleExceptionsInDifferentFiles()
            throws IOException, URISyntaxException, InterruptedException {
        prepareProject();
        copyResourceTo("/layout/undefined_variable_binding.xml",
                "/app/src/main/res/layout/broken.xml");
        copyResourceTo("/layout/invalid_setter_binding.xml",
                "/app/src/main/res/layout/invalid_setter.xml");
        CompilationResult result = runGradle("assembleDebug");
        assertNotEquals(result.output, 0, result.resultCode);
        List<ScopedException> bindingExceptions = result.getBindingExceptions();
        assertEquals(result.error, 2, bindingExceptions.size());
        File broken = new File(testFolder, "/app/src/main/res/layout/broken.xml");
        File invalidSetter = new File(testFolder, "/app/src/main/res/layout/invalid_setter.xml");
        for (ScopedException exception : bindingExceptions) {
            ScopedErrorReport report = exception.getScopedErrorReport();
            final File errorFile = new File(report.getFilePath());
            String message = null;
            String expectedErrorFile = null;
            if (errorFile.getCanonicalPath().equals(broken.getCanonicalPath())) {
                message = String.format(ErrorMessages.UNDEFINED_VARIABLE, "myVariable");
                expectedErrorFile = "/app/src/main/res/layout/broken.xml";
            } else if (errorFile.getCanonicalPath().equals(invalidSetter.getCanonicalPath())) {
                message = String.format(ErrorMessages.CANNOT_FIND_SETTER_CALL, "android:textx",
                        String.class.getCanonicalName());
                expectedErrorFile = "/app/src/main/res/layout/invalid_setter.xml";
            } else {
                fail("unexpected exception " + exception.getBareMessage());
            }
            assertEquals(1, report.getLocations().size());
            Location loc = report.getLocations().get(0);
            String extract = extract(expectedErrorFile, loc);
            assertEquals("myVariable", extract);
            assertEquals(message, exception.getBareMessage());
        }
    }

    @Test
    public void testBadSyntax() throws IOException, URISyntaxException, InterruptedException {
        singleFileErrorTest("/layout/layout_with_bad_syntax.xml",
                "/app/src/main/res/layout/broken.xml",
                "myVar.length())",
                String.format(ErrorMessages.SYNTAX_ERROR,
                        "extraneous input ')' expecting {<EOF>, ',', '.', '[', '+', '-', '*', '/', "
                                + "'%', '<<', '>>>', '>>', '<=', '>=', '>', '<', 'instanceof', "
                                + "'==', '!=', '&', '^', '|', '&&', '||', '?', '??'}"));
    }

    @Test
    public void testBrokenSyntax() throws IOException, URISyntaxException, InterruptedException {
        singleFileErrorTest("/layout/layout_with_completely_broken_syntax.xml",
                "/app/src/main/res/layout/broken.xml",
                "new String()",
                String.format(ErrorMessages.SYNTAX_ERROR,
                        "mismatched input 'String' expecting {<EOF>, ',', '.', '[', '+', '-', '*', "
                                + "'/', '%', '<<', '>>>', '>>', '<=', '>=', '>', '<', 'instanceof',"
                                + " '==', '!=', '&', '^', '|', '&&', '||', '?', '??'}"));
    }

    @Test
    public void testUndefinedVariable() throws IOException, URISyntaxException,
            InterruptedException {
        ScopedException ex = singleFileErrorTest("/layout/undefined_variable_binding.xml",
                "/app/src/main/res/layout/broken.xml", "myVariable",
                String.format(ErrorMessages.UNDEFINED_VARIABLE, "myVariable"));
    }

    @Test
    public void testInvalidSetterBinding() throws IOException, URISyntaxException,
            InterruptedException {
        prepareProject();
        ScopedException ex = singleFileErrorTest("/layout/invalid_setter_binding.xml",
                "/app/src/main/res/layout/invalid_setter.xml", "myVariable",
                String.format(ErrorMessages.CANNOT_FIND_SETTER_CALL, "android:textx",
                        String.class.getCanonicalName()));
    }

    @Test
    public void testRootTag() throws IOException, URISyntaxException,
            InterruptedException {
        prepareProject();
        copyResourceTo("/layout/root_tag.xml", "/app/src/main/res/layout/root_tag.xml");
        CompilationResult result = runGradle("assembleDebug");
        assertNotEquals(0, result.resultCode);
        assertNotNull(result.error);
        final String expected = String.format(ErrorMessages.ROOT_TAG_NOT_SUPPORTED, "hello");
        assertTrue(result.error.contains(expected));
    }

    @Test
    public void testInvalidVariableType() throws IOException, URISyntaxException,
            InterruptedException {
        prepareProject();
        ScopedException ex = singleFileErrorTest("/layout/invalid_variable_type.xml",
                "/app/src/main/res/layout/invalid_variable.xml", "myVariable",
                String.format(ErrorMessages.CANNOT_RESOLVE_TYPE, "myVariable~"));
    }

    @Test
    public void testSingleModule() throws IOException, URISyntaxException, InterruptedException {
        prepareApp(toMap(KEY_DEPENDENCIES, "compile project(':module1')",
                KEY_SETTINGS_INCLUDES, "include ':app'\ninclude ':module1'"));
        prepareModule("module1", "com.example.module1", toMap());
        copyResourceTo("/layout/basic_layout.xml", "/module1/src/main/res/layout/module_layout.xml");
        copyResourceTo("/layout/basic_layout.xml", "/app/src/main/res/layout/app_layout.xml");
        CompilationResult result = runGradle("assembleDebug");
        assertEquals(result.error, 0, result.resultCode);
    }

    @Test
    public void testModuleDependencyChange() throws IOException, URISyntaxException,
            InterruptedException {
        prepareApp(toMap(KEY_DEPENDENCIES, "compile project(':module1')",
                KEY_SETTINGS_INCLUDES, "include ':app'\ninclude ':module1'"));
        prepareModule("module1", "com.example.module1", toMap(
                KEY_DEPENDENCIES, "compile 'com.android.support:appcompat-v7:23.1.1'"
        ));
        copyResourceTo("/layout/basic_layout.xml", "/module1/src/main/res/layout/module_layout.xml");
        copyResourceTo("/layout/basic_layout.xml", "/app/src/main/res/layout/app_layout.xml");
        CompilationResult result = runGradle("assembleDebug");
        assertEquals(result.error, 0, result.resultCode);
        File moduleFolder = new File(testFolder, "module1");
        copyResourceTo("/module_build.gradle", new File(moduleFolder, "build.gradle"),
                toMap());
        result = runGradle("assembleDebug");
        assertEquals(result.error, 0, result.resultCode);
    }

    @Test
    public void testTwoLevelDependency() throws IOException, URISyntaxException, InterruptedException {
        prepareApp(toMap(KEY_DEPENDENCIES, "compile project(':module1')",
                KEY_SETTINGS_INCLUDES, "include ':app'\ninclude ':module1'\n"
                        + "include ':module2'"));
        prepareModule("module1", "com.example.module1", toMap(KEY_DEPENDENCIES,
                "compile project(':module2')"));
        prepareModule("module2", "com.example.module2", toMap());
        copyResourceTo("/layout/basic_layout.xml",
                "/module2/src/main/res/layout/module2_layout.xml");
        copyResourceTo("/layout/basic_layout.xml", "/module1/src/main/res/layout/module1_layout.xml");
        copyResourceTo("/layout/basic_layout.xml", "/app/src/main/res/layout/app_layout.xml");
        CompilationResult result = runGradle("assembleDebug");
        assertEquals(result.error, 0, result.resultCode);
    }

    @Test
    public void testIncludeInMerge() throws Throwable {
        prepareProject();
        copyResourceTo("/layout/merge_include.xml", "/app/src/main/res/layout/merge_include.xml");
        CompilationResult result = runGradle("assembleDebug");
        assertNotEquals(0, result.resultCode);
        List<ScopedException> errors = ScopedException.extractErrors(result.error);
        assertEquals(result.error, 1, errors.size());
        final ScopedException ex = errors.get(0);
        final ScopedErrorReport report = ex.getScopedErrorReport();
        final File errorFile = new File(report.getFilePath());
        assertTrue(errorFile.exists());
        assertEquals(
                new File(testFolder, "/app/src/main/res/layout/merge_include.xml")
                        .getCanonicalFile(),
                errorFile.getCanonicalFile());
        assertEquals("Merge shouldn't support includes as root. Error message was '" + result.error,
                ErrorMessages.INCLUDE_INSIDE_MERGE, ex.getBareMessage());
    }
}
