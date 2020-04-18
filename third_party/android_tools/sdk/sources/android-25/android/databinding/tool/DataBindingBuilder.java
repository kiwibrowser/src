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

package android.databinding.tool;

import org.apache.commons.io.IOUtils;

import android.databinding.tool.processing.Scope;
import android.databinding.tool.processing.ScopedException;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;
import android.databinding.tool.writer.JavaFileWriter;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;


/**
 * This class is used by Android Gradle plugin.
 */
@SuppressWarnings("unused")
public class DataBindingBuilder {
    Versions mVersions;
    private final static String EXCLUDE_PATTERN = "android/databinding/layouts/*.*";
    public String getCompilerVersion() {
        return getVersions().compiler;
    }

    public String getBaseLibraryVersion(String compilerVersion) {
        return getVersions().baseLibrary;
    }

    public String getLibraryVersion(String compilerVersion) {
        return getVersions().extensions;
    }

    public String getBaseAdaptersVersion(String compilerVersion) {
        return getVersions().extensions;
    }

    public void setPrintMachineReadableOutput(boolean machineReadableOutput) {
        ScopedException.encodeOutput(machineReadableOutput);
    }

    public boolean getPrintMachineReadableOutput() {
        return ScopedException.issEncodeOutput();
    }

    public void setDebugLogEnabled(boolean enableDebugLogs) {
        L.setDebugLog(enableDebugLogs);
    }

    public boolean isDebugLogEnabled() {
        return L.isDebugEnabled();
    }

    private Versions getVersions() {
        if (mVersions != null) {
            return mVersions;
        }
        try {
            Properties props = new Properties();
            InputStream stream = getClass().getResourceAsStream("/data_binding_version_info.properties");
            try {
                props.load(stream);
                mVersions = new Versions(props);
            } finally {
                IOUtils.closeQuietly(stream);
            }
        } catch (IOException exception) {
            L.e(exception, "Cannot read data binding version");
        }
        return mVersions;
    }

    /**
     * Returns the list of classes that should be excluded from package task
     *
     * @param layoutXmlProcessor The LayoutXmlProcessor for this variant
     * @param generatedClassListFile The location of the File into which data binding compiler wrote
     *                               list of generated classes
     *
     * @return The list of classes to exclude. They are already in JNI format.
     */
    public List<String> getJarExcludeList(LayoutXmlProcessor layoutXmlProcessor,
            File generatedClassListFile) {
        List<String> excludes = new ArrayList<String>();
        String appPkgAsClass = layoutXmlProcessor.getPackage().replace('.', '/');
        String infoClassAsClass = layoutXmlProcessor.getInfoClassFullName().replace('.', '/');

        excludes.add(infoClassAsClass + ".class");
        excludes.add(EXCLUDE_PATTERN);
        excludes.add(appPkgAsClass + "/BR.*");
        excludes.add("android/databinding/DynamicUtil.class");
        if (generatedClassListFile != null) {
            List<String> generatedClasses = readGeneratedClasses(generatedClassListFile);
            for (String klass : generatedClasses) {
                excludes.add(klass.replace('.', '/') + ".class");
            }
        }
        Scope.assertNoError();
        return excludes;
    }

    private List<String> readGeneratedClasses(File generatedClassListFile) {
        Preconditions.checkNotNull(generatedClassListFile, "Data binding exclude generated task"
                + " is not configured properly");
        Preconditions.check(generatedClassListFile.exists(),
                "Generated class list does not exist %s", generatedClassListFile.getAbsolutePath());
        FileInputStream fis = null;
        try {
            fis = new FileInputStream(generatedClassListFile);
            return IOUtils.readLines(fis);
        } catch (FileNotFoundException e) {
            L.e(e, "Unable to read generated class list from %s",
                    generatedClassListFile.getAbsoluteFile());
        } catch (IOException e) {
            L.e(e, "Unexpected exception while reading %s",
                    generatedClassListFile.getAbsoluteFile());
        } finally {
            IOUtils.closeQuietly(fis);
        }
        L.e("Could not read data binding generated class list");
        return null;
    }

    public JavaFileWriter createJavaFileWriter(File outFolder) {
        return new GradleFileWriter(outFolder.getAbsolutePath());
    }

    static class GradleFileWriter extends JavaFileWriter {

        private final String outputBase;

        public GradleFileWriter(String outputBase) {
            this.outputBase = outputBase;
        }

        @Override
        public void writeToFile(String canonicalName, String contents) {
            String asPath = canonicalName.replace('.', '/');
            File f = new File(outputBase + "/" + asPath + ".java");
            //noinspection ResultOfMethodCallIgnored
            f.getParentFile().mkdirs();
            FileOutputStream fos = null;
            try {
                fos = new FileOutputStream(f);
                IOUtils.write(contents, fos);
            } catch (IOException e) {
                L.e(e, "cannot write file " + f.getAbsolutePath());
            } finally {
                IOUtils.closeQuietly(fos);
            }
        }
    }

    private static class Versions {
        final String compilerCommon;
        final String compiler;
        final String baseLibrary;
        final String extensions;

        public Versions(Properties properties) {
            compilerCommon = properties.getProperty("compilerCommon");
            compiler = properties.getProperty("compiler");
            baseLibrary = properties.getProperty("baseLibrary");
            extensions = properties.getProperty("extensions");
            Preconditions.checkNotNull(compilerCommon, "cannot read compiler common version");
            Preconditions.checkNotNull(compiler, "cannot read compiler version");
            Preconditions.checkNotNull(baseLibrary, "cannot read baseLibrary version");
            Preconditions.checkNotNull(extensions, "cannot read extensions version");
        }
    }
}