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
import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.StringUtils;
import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.TestName;

import android.databinding.tool.store.Location;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;


public class BaseCompilationTest {

    private static final String PRINT_ENCODED_ERRORS_PROPERTY
            = "android.injected.invoked.from.ide";
    @Rule
    public TestName name = new TestName();
    static Pattern VARIABLES = Pattern.compile("!@\\{([A-Za-z0-9_-]*)}");

    public static final String KEY_MANIFEST_PACKAGE = "PACKAGE";
    public static final String KEY_DEPENDENCIES = "DEPENDENCIES";
    public static final String KEY_SETTINGS_INCLUDES = "SETTINGS_INCLUDES";
    public static final String DEFAULT_APP_PACKAGE = "com.android.databinding.compilationTest.test";
    public static final String KEY_CLASS_NAME = "CLASSNAME";
    public static final String KEY_CLASS_TYPE = "CLASSTYPE";
    public static final String KEY_IMPORT_TYPE = "IMPORTTYPE";
    public static final String KEY_INCLUDE_ID = "INCLUDEID";
    public static final String KEY_VIEW_ID = "VIEWID";

    protected final File testFolder = new File("./build/build-test");

    protected void copyResourceTo(String name, String path) throws IOException {
        copyResourceTo(name, new File(testFolder, path));
    }

    protected void copyResourceTo(String name, String path, Map<String, String> replacements)
            throws IOException {
        copyResourceTo(name, new File(testFolder, path), replacements);
    }

    protected void copyResourceDirectory(String name, String targetPath)
            throws URISyntaxException, IOException {
        URL dir = getClass().getResource(name);
        assertNotNull(dir);
        assertEquals("file", dir.getProtocol());
        File folder = new File(dir.toURI());
        assertTrue(folder.isDirectory());
        File target = new File(testFolder, targetPath);
        int len = folder.getAbsolutePath().length() + 1;
        for (File item : FileUtils.listFiles(folder, null, true)) {
            if (item.getAbsolutePath().equals(folder.getAbsolutePath())) {
                continue;
            }
            String resourcePath = item.getAbsolutePath().substring(len);

            copyResourceTo(name + "/" + resourcePath, new File(target, resourcePath));
        }
    }

    @Before
    public void clear() throws IOException {
        if (testFolder.exists()) {
            FileUtils.forceDelete(testFolder);
        }
    }

    /**
     * Extracts the text in the given location from the the at the given application path.
     *
     * @param pathInApp The path, relative to the root of the application under test
     * @param location  The location to extract
     * @return The string that is contained in the given location
     * @throws IOException If file is invalid.
     */
    protected String extract(String pathInApp, Location location) throws IOException {
        File file = new File(testFolder, pathInApp);
        assertTrue(file.exists());
        StringBuilder result = new StringBuilder();
        List<String> lines = FileUtils.readLines(file);
        for (int i = location.startLine; i <= location.endLine; i++) {
            if (i > location.startLine) {
                result.append("\n");
            }
            String line = lines.get(i);
            int start = 0;
            if (i == location.startLine) {
                start = location.startOffset;
            }
            int end = line.length() - 1; // inclusive
            if (i == location.endLine) {
                end = location.endOffset;
            }
            result.append(line.substring(start, end + 1));
        }
        return result.toString();
    }

    protected void copyResourceTo(String name, File targetFile) throws IOException {
        File directory = targetFile.getParentFile();
        FileUtils.forceMkdir(directory);
        InputStream contents = getClass().getResourceAsStream(name);
        FileOutputStream fos = new FileOutputStream(targetFile);
        IOUtils.copy(contents, fos);
        IOUtils.closeQuietly(fos);
        IOUtils.closeQuietly(contents);
    }

    protected static Map<String, String> toMap(String... keysAndValues) {
        assertEquals(0, keysAndValues.length % 2);
        Map<String, String> map = new HashMap<String, String>();
        for (int i = 0; i < keysAndValues.length; i += 2) {
            map.put(keysAndValues[i], keysAndValues[i + 1]);
        }
        return map;
    }

    protected void copyResourceTo(String name, File targetFile, Map<String, String> replacements)
            throws IOException {
        if (replacements.isEmpty()) {
            copyResourceTo(name, targetFile);
        }
        InputStream inputStream = getClass().getResourceAsStream(name);
        final String contents = IOUtils.toString(inputStream);
        IOUtils.closeQuietly(inputStream);

        StringBuilder out = new StringBuilder(contents.length());
        final Matcher matcher = VARIABLES.matcher(contents);
        int location = 0;
        while (matcher.find()) {
            int start = matcher.start();
            if (start > location) {
                out.append(contents, location, start);
            }
            final String key = matcher.group(1);
            final String replacement = replacements.get(key);
            if (replacement != null) {
                out.append(replacement);
            }
            location = matcher.end();
        }
        if (location < contents.length()) {
            out.append(contents, location, contents.length());
        }

        FileUtils.writeStringToFile(targetFile, out.toString());
    }

    protected void prepareProject() throws IOException, URISyntaxException {
        prepareApp(null);
    }

    private Map<String, String> addDefaults(Map<String, String> map) {
        if (map == null) {
            map = new HashMap<String, String>();
        }
        if (!map.containsKey(KEY_MANIFEST_PACKAGE)) {
            map.put(KEY_MANIFEST_PACKAGE, DEFAULT_APP_PACKAGE);
        }
        if (!map.containsKey(KEY_SETTINGS_INCLUDES)) {
            map.put(KEY_SETTINGS_INCLUDES, "include ':app'");
        }
        return map;
    }

    protected void prepareApp(Map<String, String> replacements) throws IOException,
            URISyntaxException {
        replacements = addDefaults(replacements);
        // how to get build folder, pass from gradle somehow ?
        FileUtils.forceMkdir(testFolder);
        copyResourceTo("/AndroidManifest.xml",
                new File(testFolder, "app/src/main/AndroidManifest.xml"), replacements);
        copyResourceTo("/project_build.gradle", new File(testFolder, "build.gradle"), replacements);
        copyResourceTo("/app_build.gradle", new File(testFolder, "app/build.gradle"), replacements);
        copyResourceTo("/settings.gradle", new File(testFolder, "settings.gradle"), replacements);
        File localProperties = new File("../local.properties");
        if (localProperties.exists()) {
            FileUtils.copyFile(localProperties, new File(testFolder, "local.properties"));
        }
        FileUtils.copyFile(new File("../propLoader.gradle"),
                new File(testFolder, "propLoaderClone.gradle"));
        FileUtils.copyFile(new File("../gradlew"), new File(testFolder, "gradlew"));
        FileUtils.copyDirectory(new File("../gradle"), new File(testFolder, "gradle"));
    }

    protected void prepareModule(String moduleName, String packageName,
            Map<String, String> replacements) throws IOException, URISyntaxException {
        replacements = addDefaults(replacements);
        replacements.put(KEY_MANIFEST_PACKAGE, packageName);
        File moduleFolder = new File(testFolder, moduleName);
        if (moduleFolder.exists()) {
            FileUtils.forceDelete(moduleFolder);
        }
        FileUtils.forceMkdir(moduleFolder);
        copyResourceTo("/AndroidManifest.xml",
                new File(moduleFolder, "src/main/AndroidManifest.xml"), replacements);
        copyResourceTo("/module_build.gradle", new File(moduleFolder, "build.gradle"),
                replacements);
    }

    protected CompilationResult runGradle(String... params)
            throws IOException, InterruptedException {
        setExecutable();
        File pathToExecutable = new File(testFolder, "gradlew");
        List<String> args = new ArrayList<String>();
        args.add(pathToExecutable.getAbsolutePath());
        args.add("-P" + PRINT_ENCODED_ERRORS_PROPERTY + "=true");
        if ("true".equals(System.getProperties().getProperty("useReleaseVersion", "false"))) {
            args.add("-PuseReleaseVersion=true");
        }
        if ("true".equals(System.getProperties().getProperty("addRemoteRepos", "false"))) {
            args.add("-PaddRemoteRepos=true");
        }
        args.add("--project-cache-dir");
        args.add(new File("../.caches/", name.getMethodName()).getAbsolutePath());
        Collections.addAll(args, params);
        ProcessBuilder builder = new ProcessBuilder(args);
        builder.environment().putAll(System.getenv());
        String javaHome = System.getProperty("java.home");
        if (StringUtils.isNotBlank(javaHome)) {
            builder.environment().put("JAVA_HOME", javaHome);
        }
        builder.directory(testFolder);
        Process process = builder.start();
        String output = collect(process.getInputStream());
        String error = collect(process.getErrorStream());
        int result = process.waitFor();
        return new CompilationResult(result, output, error);
    }

    private void setExecutable() throws IOException {
        File gw = new File(testFolder, "gradlew");
        gw.setExecutable(true);
    }

    /**
     * Use this instead of IO utils so that we can easily log the output when necessary
     */
    private static String collect(InputStream stream) throws IOException {
        StringBuilder sb = new StringBuilder();
        String line;
        final BufferedReader reader = new BufferedReader(new InputStreamReader(stream));
        while ((line = reader.readLine()) != null) {
            sb.append(line).append("\n");
        }
        return sb.toString();
    }
}
