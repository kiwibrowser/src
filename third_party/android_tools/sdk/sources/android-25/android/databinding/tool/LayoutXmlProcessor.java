/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool;

import com.google.common.escape.Escaper;

import org.apache.commons.io.FileUtils;
import org.xml.sax.SAXException;

import android.databinding.BindingBuildInfo;
import android.databinding.tool.store.LayoutFileParser;
import android.databinding.tool.store.ResourceBundle;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;
import android.databinding.tool.util.SourceCodeEscapers;
import android.databinding.tool.writer.JavaFileWriter;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.net.URI;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import javax.xml.bind.JAXBException;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPathExpressionException;

/**
 * Processes the layout XML, stripping the binding attributes and elements
 * and writes the information into an annotated class file for the annotation
 * processor to work with.
 */
public class LayoutXmlProcessor {
    // hardcoded in baseAdapters
    public static final String RESOURCE_BUNDLE_PACKAGE = "android.databinding.layouts";
    public static final String CLASS_NAME = "DataBindingInfo";
    private final JavaFileWriter mFileWriter;
    private final ResourceBundle mResourceBundle;
    private final int mMinSdk;

    private boolean mProcessingComplete;
    private boolean mWritten;
    private final boolean mIsLibrary;
    private final String mBuildId = UUID.randomUUID().toString();
    private final OriginalFileLookup mOriginalFileLookup;

    public LayoutXmlProcessor(String applicationPackage,
            JavaFileWriter fileWriter, int minSdk, boolean isLibrary,
            OriginalFileLookup originalFileLookup) {
        mFileWriter = fileWriter;
        mResourceBundle = new ResourceBundle(applicationPackage);
        mMinSdk = minSdk;
        mIsLibrary = isLibrary;
        mOriginalFileLookup = originalFileLookup;
    }

    private static void processIncrementalInputFiles(ResourceInput input,
            ProcessFileCallback callback)
            throws IOException, ParserConfigurationException, XPathExpressionException,
            SAXException {
        processExistingIncrementalFiles(input.getRootInputFolder(), input.getAdded(), callback);
        processExistingIncrementalFiles(input.getRootInputFolder(), input.getChanged(), callback);
        processRemovedIncrementalFiles(input.getRootInputFolder(), input.getRemoved(), callback);
    }

    private static void processExistingIncrementalFiles(File inputRoot, List<File> files,
            ProcessFileCallback callback)
            throws IOException, XPathExpressionException, SAXException,
            ParserConfigurationException {
        for (File file : files) {
            File parent = file.getParentFile();
            if (inputRoot.equals(parent)) {
                callback.processOtherRootFile(file);
            } else if (layoutFolderFilter.accept(parent, parent.getName())) {
                callback.processLayoutFile(file);
            } else {
                callback.processOtherFile(parent, file);
            }
        }
    }

    private static void processRemovedIncrementalFiles(File inputRoot, List<File> files,
            ProcessFileCallback callback)
            throws IOException {
        for (File file : files) {
            File parent = file.getParentFile();
            if (inputRoot.equals(parent)) {
                callback.processRemovedOtherRootFile(file);
            } else if (layoutFolderFilter.accept(parent, parent.getName())) {
                callback.processRemovedLayoutFile(file);
            } else {
                callback.processRemovedOtherFile(parent, file);
            }
        }
    }

    private static void processAllInputFiles(ResourceInput input, ProcessFileCallback callback)
            throws IOException, XPathExpressionException, SAXException,
            ParserConfigurationException {
        FileUtils.deleteDirectory(input.getRootOutputFolder());
        Preconditions.check(input.getRootOutputFolder().mkdirs(), "out dir should be re-created");
        Preconditions.check(input.getRootInputFolder().isDirectory(), "it must be a directory");
        for (File firstLevel : input.getRootInputFolder().listFiles()) {
            if (firstLevel.isDirectory()) {
                if (layoutFolderFilter.accept(firstLevel, firstLevel.getName())) {
                    callback.processLayoutFolder(firstLevel);
                    for (File xmlFile : firstLevel.listFiles(xmlFileFilter)) {
                        callback.processLayoutFile(xmlFile);
                    }
                } else {
                    callback.processOtherFolder(firstLevel);
                    for (File file : firstLevel.listFiles()) {
                        callback.processOtherFile(firstLevel, file);
                    }
                }
            } else {
                callback.processOtherRootFile(firstLevel);
            }

        }
    }

    /**
     * used by the studio plugin
     */
    public ResourceBundle getResourceBundle() {
        return mResourceBundle;
    }

    public boolean processResources(final ResourceInput input)
            throws ParserConfigurationException, SAXException, XPathExpressionException,
            IOException {
        if (mProcessingComplete) {
            return false;
        }
        final LayoutFileParser layoutFileParser = new LayoutFileParser();
        final URI inputRootUri = input.getRootInputFolder().toURI();
        ProcessFileCallback callback = new ProcessFileCallback() {
            private File convertToOutFile(File file) {
                final String subPath = toSystemDependentPath(inputRootUri
                        .relativize(file.toURI()).getPath());
                return new File(input.getRootOutputFolder(), subPath);
            }
            @Override
            public void processLayoutFile(File file)
                    throws ParserConfigurationException, SAXException, XPathExpressionException,
                    IOException {
                final File output = convertToOutFile(file);
                final ResourceBundle.LayoutFileBundle bindingLayout = layoutFileParser
                        .parseXml(file, output, mResourceBundle.getAppPackage(), mOriginalFileLookup);
                if (bindingLayout != null && !bindingLayout.isEmpty()) {
                    mResourceBundle.addLayoutBundle(bindingLayout);
                }
            }

            @Override
            public void processOtherFile(File parentFolder, File file) throws IOException {
                final File outParent = convertToOutFile(parentFolder);
                FileUtils.copyFile(file, new File(outParent, file.getName()));
            }

            @Override
            public void processRemovedLayoutFile(File file) {
                mResourceBundle.addRemovedFile(file);
                final File out = convertToOutFile(file);
                FileUtils.deleteQuietly(out);
            }

            @Override
            public void processRemovedOtherFile(File parentFolder, File file) throws IOException {
                final File outParent = convertToOutFile(parentFolder);
                FileUtils.deleteQuietly(new File(outParent, file.getName()));
            }

            @Override
            public void processOtherFolder(File folder) {
                //noinspection ResultOfMethodCallIgnored
                convertToOutFile(folder).mkdirs();
            }

            @Override
            public void processLayoutFolder(File folder) {
                //noinspection ResultOfMethodCallIgnored
                convertToOutFile(folder).mkdirs();
            }

            @Override
            public void processOtherRootFile(File file) throws IOException {
                final File outFile = convertToOutFile(file);
                if (file.isDirectory()) {
                    FileUtils.copyDirectory(file, outFile);
                } else {
                    FileUtils.copyFile(file, outFile);
                }
            }

            @Override
            public void processRemovedOtherRootFile(File file) throws IOException {
                final File outFile = convertToOutFile(file);
                FileUtils.deleteQuietly(outFile);
            }
        };
        if (input.isIncremental()) {
            processIncrementalInputFiles(input, callback);
        } else {
            processAllInputFiles(input, callback);
        }
        mProcessingComplete = true;
        return true;
    }

    public static String toSystemDependentPath(String path) {
        if (File.separatorChar != '/') {
            path = path.replace('/', File.separatorChar);
        }
        return path;
    }

    public void writeLayoutInfoFiles(File xmlOutDir) throws JAXBException {
        if (mWritten) {
            return;
        }
        for (List<ResourceBundle.LayoutFileBundle> layouts : mResourceBundle.getLayoutBundles()
                .values()) {
            for (ResourceBundle.LayoutFileBundle layout : layouts) {
                writeXmlFile(xmlOutDir, layout);
            }
        }
        for (File file : mResourceBundle.getRemovedFiles()) {
            String exportFileName = generateExportFileName(file);
            FileUtils.deleteQuietly(new File(xmlOutDir, exportFileName));
        }
        mWritten = true;
    }

    private void writeXmlFile(File xmlOutDir, ResourceBundle.LayoutFileBundle layout)
            throws JAXBException {
        String filename = generateExportFileName(layout);
        mFileWriter.writeToFile(new File(xmlOutDir, filename), layout.toXML());
    }

    public String getInfoClassFullName() {
        return RESOURCE_BUNDLE_PACKAGE + "." + CLASS_NAME;
    }

    /**
     * Generates a string identifier that can uniquely identify the given layout bundle.
     * This identifier can be used when we need to export data about this layout bundle.
     */
    private static String generateExportFileName(ResourceBundle.LayoutFileBundle layout) {
        return generateExportFileName(layout.getFileName(), layout.getDirectory());
    }

    private static String generateExportFileName(File file) {
        final String fileName = file.getName();
        return generateExportFileName(fileName.substring(0, fileName.lastIndexOf('.')),
                file.getParentFile().getName());
    }

    public static String generateExportFileName(String fileName, String dirName) {
        return fileName + '-' + dirName + ".xml";
    }

    public static String exportLayoutNameFromInfoFileName(String infoFileName) {
        return infoFileName.substring(0, infoFileName.indexOf('-'));
    }

    public void writeInfoClass(/*Nullable*/ File sdkDir, File xmlOutDir,
            /*Nullable*/ File exportClassListTo) {
        writeInfoClass(sdkDir, xmlOutDir, exportClassListTo, false, false);
    }

    public String getPackage() {
        return mResourceBundle.getAppPackage();
    }

    public void writeInfoClass(/*Nullable*/ File sdkDir, File xmlOutDir, File exportClassListTo,
            boolean enableDebugLogs, boolean printEncodedErrorLogs) {
        Escaper javaEscaper = SourceCodeEscapers.javaCharEscaper();
        final String sdkPath = sdkDir == null ? null : javaEscaper.escape(sdkDir.getAbsolutePath());
        final Class annotation = BindingBuildInfo.class;
        final String layoutInfoPath = javaEscaper.escape(xmlOutDir.getAbsolutePath());
        final String exportClassListToPath = exportClassListTo == null ? "" :
                javaEscaper.escape(exportClassListTo.getAbsolutePath());
        String classString = "package " + RESOURCE_BUNDLE_PACKAGE + ";\n\n" +
                "import " + annotation.getCanonicalName() + ";\n\n" +
                "@" + annotation.getSimpleName() + "(buildId=\"" + mBuildId + "\", " +
                "modulePackage=\"" + mResourceBundle.getAppPackage() + "\", " +
                "sdkRoot=" + "\"" + (sdkPath == null ? "" : sdkPath) + "\"," +
                "layoutInfoDir=\"" + layoutInfoPath + "\"," +
                "exportClassListTo=\"" + exportClassListToPath + "\"," +
                "isLibrary=" + mIsLibrary + "," +
                "minSdk=" + mMinSdk + "," +
                "enableDebugLogs=" + enableDebugLogs + "," +
                "printEncodedError=" + printEncodedErrorLogs + ")\n" +
                "public class " + CLASS_NAME + " {}\n";
        mFileWriter.writeToFile(RESOURCE_BUNDLE_PACKAGE + "." + CLASS_NAME, classString);
    }

    private static final FilenameFilter layoutFolderFilter = new FilenameFilter() {
        @Override
        public boolean accept(File dir, String name) {
            return name.startsWith("layout");
        }
    };

    private static final FilenameFilter xmlFileFilter = new FilenameFilter() {
        @Override
        public boolean accept(File dir, String name) {
            return name.toLowerCase().endsWith(".xml");
        }
    };

    /**
     * Helper interface that can find the original copy of a resource XML.
     */
    public interface OriginalFileLookup {

        /**
         * @param file The intermediate build file
         * @return The original file or null if original File cannot be found.
         */
        File getOriginalFileFor(File file);
    }

    /**
     * API agnostic class to get resource changes incrementally.
     */
    public static class ResourceInput {
        private final boolean mIncremental;
        private final File mRootInputFolder;
        private final File mRootOutputFolder;

        private List<File> mAdded = new ArrayList<File>();
        private List<File> mRemoved = new ArrayList<File>();
        private List<File> mChanged = new ArrayList<File>();

        public ResourceInput(boolean incremental, File rootInputFolder, File rootOutputFolder) {
            mIncremental = incremental;
            mRootInputFolder = rootInputFolder;
            mRootOutputFolder = rootOutputFolder;
        }

        public void added(File file) {
            mAdded.add(file);
        }
        public void removed(File file) {
            mRemoved.add(file);
        }
        public void changed(File file) {
            mChanged.add(file);
        }

        public boolean shouldCopy() {
            return !mRootInputFolder.equals(mRootOutputFolder);
        }

        List<File> getAdded() {
            return mAdded;
        }

        List<File> getRemoved() {
            return mRemoved;
        }

        List<File> getChanged() {
            return mChanged;
        }

        File getRootInputFolder() {
            return mRootInputFolder;
        }

        File getRootOutputFolder() {
            return mRootOutputFolder;
        }

        public boolean isIncremental() {
            return mIncremental;
        }

        @Override
        public String toString() {
            StringBuilder out = new StringBuilder();
            out.append("ResourceInput{")
                    .append("mIncremental=").append(mIncremental)
                    .append(", mRootInputFolder=").append(mRootInputFolder)
                    .append(", mRootOutputFolder=").append(mRootOutputFolder);
            logFiles(out, "added", mAdded);
            logFiles(out, "removed", mRemoved);
            logFiles(out, "changed", mChanged);
            return out.toString();

        }

        private static void logFiles(StringBuilder out, String name, List<File> files) {
            out.append("\n  ").append(name);
            for (File file : files) {
                out.append("\n   - ").append(file.getAbsolutePath());
            }
        }
    }

    private interface ProcessFileCallback {
        void processLayoutFile(File file)
                throws ParserConfigurationException, SAXException, XPathExpressionException,
                IOException;
        void processOtherFile(File parentFolder, File file) throws IOException;
        void processRemovedLayoutFile(File file);
        void processRemovedOtherFile(File parentFolder, File file) throws IOException;

        void processOtherFolder(File folder);

        void processLayoutFolder(File folder);

        void processOtherRootFile(File file) throws IOException;

        void processRemovedOtherRootFile(File file) throws IOException;
    }
}
