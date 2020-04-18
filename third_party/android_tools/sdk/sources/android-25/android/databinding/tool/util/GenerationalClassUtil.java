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

package android.databinding.tool.util;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.IOUtils;
import org.apache.commons.io.filefilter.TrueFileFilter;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.io.Serializable;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import javax.annotation.processing.ProcessingEnvironment;
import javax.tools.FileObject;
import javax.tools.StandardLocation;

/**
 * A utility class that helps adding build specific objects to the jar file
 * and their extraction later on.
 */
public class GenerationalClassUtil {
    private static List[] sCache = null;
    public static <T extends Serializable> List<T> loadObjects(ExtensionFilter filter) {
        if (sCache == null) {
            buildCache();
        }
        //noinspection unchecked
        return sCache[filter.ordinal()];
    }

    private static void buildCache() {
        L.d("building generational class cache");
        ClassLoader classLoader = GenerationalClassUtil.class.getClassLoader();
        Preconditions.check(classLoader instanceof URLClassLoader, "Class loader must be an"
                + "instance of URLClassLoader. %s", classLoader);
        //noinspection ConstantConditions
        final URLClassLoader urlClassLoader = (URLClassLoader) classLoader;
        sCache = new List[ExtensionFilter.values().length];
        for (ExtensionFilter filter : ExtensionFilter.values()) {
            sCache[filter.ordinal()] = new ArrayList();
        }
        for (URL url : urlClassLoader.getURLs()) {
            L.d("checking url %s for intermediate data", url);
            try {
                final File file = new File(url.toURI());
                if (!file.exists()) {
                    L.d("cannot load file for %s", url);
                    continue;
                }
                if (file.isDirectory()) {
                    // probably exported classes dir.
                    loadFromDirectory(file);
                } else {
                    // assume it is a zip file
                    loadFomZipFile(file);
                }
            } catch (IOException e) {
                L.d("cannot open zip file from %s", url);
            } catch (URISyntaxException e) {
                L.d("cannot open zip file from %s", url);
            }
        }
    }

    private static void loadFromDirectory(File directory) {
        for (File file : FileUtils.listFiles(directory, TrueFileFilter.INSTANCE,
                TrueFileFilter.INSTANCE)) {
            for (ExtensionFilter filter : ExtensionFilter.values()) {
                if (filter.accept(file.getName())) {
                    InputStream inputStream = null;
                    try {
                        inputStream = FileUtils.openInputStream(file);
                        Serializable item = fromInputStream(inputStream);
                        if (item != null) {
                            //noinspection unchecked
                            sCache[filter.ordinal()].add(item);
                            L.d("loaded item %s from file", item);
                        }
                    } catch (IOException e) {
                        L.e(e, "Could not merge in Bindables from %s", file.getAbsolutePath());
                    } catch (ClassNotFoundException e) {
                        L.e(e, "Could not read Binding properties intermediate file. %s",
                                file.getAbsolutePath());
                    } finally {
                        IOUtils.closeQuietly(inputStream);
                    }
                }
            }
        }
    }

    private static void loadFomZipFile(File file)
            throws IOException {
        ZipFile zipFile = new ZipFile(file);
        Enumeration<? extends ZipEntry> entries = zipFile.entries();
        while (entries.hasMoreElements()) {
            ZipEntry entry = entries.nextElement();
            for (ExtensionFilter filter : ExtensionFilter.values()) {
                if (!filter.accept(entry.getName())) {
                    continue;
                }
                InputStream inputStream = null;
                try {
                    inputStream = zipFile.getInputStream(entry);
                    Serializable item = fromInputStream(inputStream);
                    L.d("loaded item %s from zip file", item);
                    if (item != null) {
                        //noinspection unchecked
                        sCache[filter.ordinal()].add(item);
                    }
                } catch (IOException e) {
                    L.e(e, "Could not merge in Bindables from %s", file.getAbsolutePath());
                } catch (ClassNotFoundException e) {
                    L.e(e, "Could not read Binding properties intermediate file. %s",
                            file.getAbsolutePath());
                } finally {
                    IOUtils.closeQuietly(inputStream);
                }
            }
        }
    }

    private static Serializable fromInputStream(InputStream inputStream)
            throws IOException, ClassNotFoundException {
        ObjectInputStream in = new ObjectInputStream(inputStream);
        return (Serializable) in.readObject();

    }

    public static void writeIntermediateFile(ProcessingEnvironment processingEnv,
            String packageName, String fileName, Serializable object) {
        ObjectOutputStream oos = null;
        try {
            FileObject intermediate = processingEnv.getFiler().createResource(
                    StandardLocation.CLASS_OUTPUT, packageName,
                    fileName);
            OutputStream ios = intermediate.openOutputStream();
            oos = new ObjectOutputStream(ios);
            oos.writeObject(object);
            oos.close();
            L.d("wrote intermediate bindable file %s %s", packageName, fileName);
        } catch (IOException e) {
            L.e(e, "Could not write to intermediate file: %s", fileName);
        } finally {
            IOUtils.closeQuietly(oos);
        }
    }

    public enum ExtensionFilter {
        BR("-br.bin"),
        LAYOUT("-layoutinfo.bin"),
        SETTER_STORE("-setter_store.bin");
        private final String mExtension;
        ExtensionFilter(String extension) {
            mExtension = extension;
        }

        public boolean accept(String entryName) {
            return entryName.endsWith(mExtension);
        }

        public String getExtension() {
            return mExtension;
        }
    }
}
