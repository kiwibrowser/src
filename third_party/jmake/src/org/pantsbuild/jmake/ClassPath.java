/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * An instance of this class represents a class path, on which binary classes can be looked up.
 * It also provides several static methods to create and utilize several specific class paths used
 * throughout jmake.
 *
 * @author Misha Dmitriev
 *  12 October 2004
 */
public class ClassPath {

    private PathEntry[] paths;
    private static ClassPath projectClassPath;   // Class path (currently it can contain only JARs) containing sourceless project classes.
    // See also the comment to standardClassPath.
    private static ClassPath standardClassPath;  // Class path that the user specifies via the -classpath option. A sum of the
    // standardClassPath, the projectClassPath, and the virtualPath is passed to the compiler. Each of these
    // class paths are also used to look up non-project superclasses/superinterfaces of
    // project classes.
    private static ClassPath bootClassPath,  extClassPath; // Class paths that by default are sun.boot.class.path and all JARs on
    // java.ext.class.path, respectively. They are used to look up non-project
    // superclasses/superinterfaces of project classes. Their values can be changed using
    // setBootClassPath() and setExtDirs().
    private static ClassPath virtualPath; // Class path that the user specifies via the -vpath option.
    private static String compilerUserClassPath; // Class path to be passed to the compiler; equals to the sum of values of parameters of
    // setClassPath() and setProjectClassPath() methods.
    private static String standardClassPathStr,  projectClassPathStr,  bootClassPathStr,  extDirsStr,
            virtualPathStr;
    private static Map<String,ClassInfo> classCache;


    static {
        resetOnFinish();
    }

    /**
     * Needed since some environments, e.g. NetBeans, can keep jmake classes in memory
     * permanently. Thus unchanged class paths from previous, possibly unrelated invocations
     * of jmake, may interfere with the current settings.
     */
    public static void resetOnFinish() {
        projectClassPath = standardClassPath = bootClassPath = extClassPath = virtualPath =
                null;
        compilerUserClassPath = null;
        standardClassPathStr = projectClassPathStr = bootClassPathStr =
                extDirsStr = virtualPathStr = null;
        classCache = new LinkedHashMap<String,ClassInfo>();
    }

    public static void setClassPath(String value) throws PublicExceptions.InvalidCmdOptionException {
        standardClassPathStr = value;
        standardClassPath = new ClassPath(value, false);
    }

    public static void setProjectClassPath(String value) throws PublicExceptions.InvalidCmdOptionException {
        projectClassPathStr = value;
        projectClassPath = new ClassPath(value, true);
    }

    public static void setBootClassPath(String value) throws PublicExceptions.InvalidCmdOptionException {
        bootClassPathStr = value;
        bootClassPath = new ClassPath(value, false);
    }

    public static void setExtDirs(String value) throws PublicExceptions.InvalidCmdOptionException {
        extDirsStr = value;
        // Extension class path needs special handling, since it consists of directories, which contain .jars
        // So we need to find all these .jars in all these dirs and add them to extClassPathElementList
        List<String> extClassPathElements = new ArrayList<String>();
        for (StringTokenizer tok =
                new StringTokenizer(value, File.pathSeparator); tok.hasMoreTokens();) {
            File extDir = new File(tok.nextToken());
            String[] extJars = extDir.list(new FilenameFilter() {

                public boolean accept(File dir, String name) {
                    name = name.toLowerCase(Locale.ENGLISH);
                    return name.endsWith(".zip") || name.endsWith(".jar");
                }
            });
            if (extJars == null) {
                continue;
            }
            for (int i = 0; i < extJars.length; i++) {
                extClassPathElements.add(extDir + File.separator + extJars[i]);
            }
        }
        extClassPath = new ClassPath(extClassPathElements, false);
    }

    public static void setVirtualPath(String value) throws PublicExceptions.InvalidCmdOptionException {
        if (value == null) {
            throw new PublicExceptions.InvalidCmdOptionException("null argument");
        }
        StringTokenizer st = new StringTokenizer(value, File.pathSeparator);
        while (st.hasMoreElements()) {
            String dir = st.nextToken();
            if ( ! (new File(dir)).isDirectory()) {
                throw new PublicExceptions.InvalidCmdOptionException("Virtual path must contain only directories." +
                        " Entry " + dir + " is not a directory.");
            }
        }
        virtualPathStr = value;
        virtualPath = new ClassPath(value, false);
    }

    public static void initializeAllClassPaths() {
        // First set the compiler class path value
        if (standardClassPathStr == null && projectClassPathStr == null) {
            compilerUserClassPath = ".";
        } else if (standardClassPathStr == null) {
            compilerUserClassPath = projectClassPathStr;
        } else if (projectClassPathStr == null) {
            compilerUserClassPath = standardClassPathStr;
        } else {
            compilerUserClassPath =
                    standardClassPathStr + File.pathSeparator + projectClassPathStr;
        }

        if (virtualPathStr != null) {
            compilerUserClassPath += File.pathSeparator + virtualPathStr;
        }

        if (standardClassPathStr == null) {
            try {
                String tmp = ".";
                if (virtualPathStr != null) {
                    tmp += File.pathSeparator + virtualPathStr;
                }
                standardClassPath = new ClassPath(tmp, false);
            } catch (PublicExceptions.InvalidCmdOptionException ex) { /* Should not happen */ }
        }
        if (projectClassPathStr == null) {
            projectClassPath = new ClassPath();
        }

        // Create the core class path as a combination of sun.boot.class.path and java.ext.dirs contents
        if (bootClassPathStr == null) {
            try {
                bootClassPath =
                        new ClassPath(System.getProperty("sun.boot.class.path"), false);
            } catch (PublicExceptions.InvalidCmdOptionException ex) { /* Shouldn't happen */ }
        // bootClassPathStr should remain null, so that nothing that the user didn't specify is passed to the compiler
        }

        if (extDirsStr == null) {
            try {
                setExtDirs(System.getProperty("java.ext.dirs"));
            } catch (PublicExceptions.InvalidCmdOptionException ex) { /* Shouldn't happen */ }
            // extDirsStr should remain null, so that nothing that the user didn't specify is passed to the compiler
            extDirsStr = null;
        }
    }

    /** Never returns null - if classpath wasn't set explicitly, returns "." */
    public static String getCompilerUserClassPath() {
        return compilerUserClassPath;
    }

    /** Will return null if boot class path wasn't explicitly specified */
    public static String getCompilerBootClassPath() {
        return bootClassPathStr;
    }

    /** Will return null if extdirs weren't explicitly specified */
    public static String getCompilerExtDirs() {
        return extDirsStr;
    }

    /** Will return null if virtualPath wasn't explicitly specified */
    public static String getVirtualPath() {
        return virtualPathStr;
    }

    /**
     * For the given class return the list of all of its superclasses (excluding Object), that can be loaded from
     * projectClassPath or standardClassPath, plus the first superclass that can be loaded from coreClassPath.
     * The latter is an optimization based on the assumption that core classes never change, or rather the programmer
     * will recompile everything when they switch to a new JDK version. The optimization prevents us from wasting time
     * repeatedly loading the same sets of core classes.
     */
    public static void getSuperclasses(String className,
            Collection<String> res, PCDManager pcdm) {
        int iterNo = 0;
        while (!"java/lang/Object".equals(className)) {
            ClassInfo ci = getClassInfoForName(className, pcdm);
            if (ci == null) {
                return;
            }
            if (iterNo++ > 0) {
                res.add(ci.name);
            }
            className = ci.superName;
        }
    }

    /**
     * Add to the given set the names of all interfaces implemented by the given class, that can be loaded from
     * projectClassPath or standardClassPath, plus the first interface on each branch that can be loaded from
     * coreClassPath. It's the same optimization as in getSuperclasses().
     */
    public static void addAllImplementedInterfaceNames(String className,
            Set<String> intfSet, PCDManager pcdm) {
        if ("java/lang/Object".equals(className)) {
            return;
        }
        ClassInfo ci = getClassInfoForName(className, pcdm);
        if (ci == null) {
            return;
        }
        String[] interfaces = ci.interfaces;
        if (interfaces != null) {
            for (int i = 0; i < interfaces.length; i++) {
                intfSet.add(interfaces[i]);
                addAllImplementedInterfaceNames(interfaces[i], intfSet, pcdm);
            }
        }

        String superName = ci.superName;
        if (superName != null) {
            addAllImplementedInterfaceNames(superName, intfSet, pcdm);
        }
    }

    public static String[] getProjectJars() {
        if (projectClassPath == null || projectClassPath.isEmpty()) {
            return null;
        }
        PathEntry paths[] = projectClassPath.paths;
        String[] ret = new String[paths.length];
        for (int i = 0; i < paths.length; i++) {
            ret[i] = paths[i].toString();
        }
        return ret;
    }

    public static ClassInfo getClassInfoForName(String className, PCDManager pcdm) {
        ClassInfo info = classCache.get(className);
        if (info != null) {
            return info;
        }

        byte buf[] = bootClassPath.getBytesForClass(className);
        if (buf == null) {
            buf = extClassPath.getBytesForClass(className);
        }
        if (buf == null) {
            buf = standardClassPath.getBytesForClass(className);
        }
        if (buf == null) {
            buf = projectClassPath.getBytesForClass(className);
        }
        if (buf == null) {
            return null;
        }

        info = new ClassInfo(buf, pcdm, className);
        classCache.put(className, info);
        return info;
    }

    /** Returns the class loader that would load classes from the given class path. */
    public static ClassLoader getClassLoaderForPath(String classPath) throws Exception {
        boolean isWindows = System.getProperty("os.name").startsWith("Win");
        ClassPath cp = new ClassPath(classPath, false);
        PathEntry[] paths = cp.paths;
        URL[] urls = new URL[paths.length];
        for (int i = 0; i < paths.length; i++) {
            String dirOrJar = paths[i].toString();
            if (!(dirOrJar.startsWith("file://") || dirOrJar.startsWith("http://"))) {
                // On Windows, if I have path specified as "file://c:\...", (i.e. with the drive name) URLClassLoader works
                // unbelievably slow. However, if an additional slash is added, like : "file:///c:\...", the speed becomes
                // normal. To me it looks like a bug, but, anyway, I am taking measure here.
                if (isWindows && dirOrJar.charAt(1) == ':') {
                    dirOrJar = "/" + dirOrJar;
                }
                dirOrJar = new File(dirOrJar).toURI().toString();
            }
            if (!(dirOrJar.endsWith(".jar") || dirOrJar.endsWith(".zip") || dirOrJar.endsWith(File.separator))) {
                dirOrJar += File.separator; // So that URLClassLoader correctly handles it as a directory
            }
            urls[i] = new URL(dirOrJar);
        }

        return new URLClassLoader(urls);
    //} catch (java.net.MalformedURLException e) {

    //}
    }


    // ------------------------------------ Private implementation --------------------------------------------
    private ClassPath() {
        paths = new PathEntry[0];
    }

    private ClassPath(String classPath, boolean isJarOnly) throws PublicExceptions.InvalidCmdOptionException {
        if (classPath == null) {
            throw new PublicExceptions.InvalidCmdOptionException("null argument");
        }
        List<String> vec = new ArrayList<String>();

        for (StringTokenizer tok =
                new StringTokenizer(classPath, File.pathSeparator); tok.hasMoreTokens();) {
            String path = tok.nextToken();
            vec.add(path);
        }
        init(vec, isJarOnly);
    }

    private ClassPath(List<String> pathEntries, boolean isJarOnly) throws PublicExceptions.InvalidCmdOptionException {
        init(pathEntries, isJarOnly);
    }

    private void init(List<String> pathEntries, boolean isJarOnly) throws PublicExceptions.InvalidCmdOptionException {
        if (pathEntries == null) {
            throw new PublicExceptions.InvalidCmdOptionException("null argument");
        }
        List<PathEntry> vec = new ArrayList<PathEntry>(pathEntries.size());
        for (int i = 0; i < pathEntries.size(); i++) {
            String path = pathEntries.get(i);
            if (!path.equals("")) {
                File file = new File(path);
                try {
                    if (file.exists() && file.canRead()) {
                        if (file.isDirectory()) {
                            if (isJarOnly) {
                                throw new PublicExceptions.InvalidCmdOptionException("directories are not allowed on this class path: " + path);
                            }
                            vec.add(new Dir(file));
                        } else {
                            vec.add(new Zip(new ZipFile(file)));
                        }
                    } else if (isJarOnly) {
                        throw new IOException("file does not exist");
                    }
                } catch (IOException e) {
                    if (isJarOnly) {
                        throw new PublicExceptions.InvalidCmdOptionException("error initializing class path component " + path + ": " + e.getMessage());
                    }
                }
            }
        }

        paths = new PathEntry[vec.size()];
        vec.toArray(paths);
    }

    private boolean isEmpty() {
        return paths.length == 0;
    }

    private byte[] getBytesForClass(String className) {
        String fileName = className + ".class";
        for (int i = 0; i < paths.length; i++) {
            byte buf[] = paths[i].getBytesForClassFile(fileName);
            if (buf != null) {
                return buf;
            }
        }
        return null;
    }

    public String toString() {
        if (paths == null) {
            return "NULL";
        }
        StringBuilder res = new StringBuilder();
        for (int i = 0; i < paths.length; i++) {
            res.append(paths[i].toString());
        }
        return res.toString();
    }


    // ------------------------------------ Private helper classes --------------------------------------------
    private static abstract class PathEntry {

        abstract byte[] getBytesForClassFile(String fileName);

        public abstract String toString();
    }

    private static class Dir extends PathEntry {

        private String dir;

        Dir(File f) throws IOException {
            dir = f.getCanonicalPath();
        }

        byte[] getBytesForClassFile(String fileName) {
            File file = new File(dir + File.separatorChar + fileName);
            if (file.exists()) {
                return Utils.readFileIntoBuffer(file);
            } else {
                return null;
            }
        }

        public String toString() {
            return dir;
        }
    }

    private static class Zip extends PathEntry {

        private ZipFile zip;

        Zip(ZipFile z) {
            zip = z;
        }

        byte[] getBytesForClassFile(String fileName) {
            ZipEntry entry = zip.getEntry(fileName);
            if (entry != null) {
                return Utils.readZipEntryIntoBuffer(zip, entry);
            } else {
                return null;
            }
        }

        public String toString() {
            return zip.getName();
        }
    }
}
