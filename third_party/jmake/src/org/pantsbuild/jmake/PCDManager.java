/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.StringTokenizer;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.zip.Adler32;

/**
 * This class implements management of the Project Class Directory, automatic tracking
 * of changes and recompilation of .java sources for a project.
 *
 * @author Misha Dmitriev
 *  23 January 2003
 */
public class PCDManager {

    private PCDContainer pcdc;
    private Map<String,PCDEntry> pcd;   // Maps project class names to PCDEntries
    private String projectJavaAndJarFilesArray[];
    private String addedJavaAndJarFilesArray[],  removedJavaAndJarFilesArray[],  updatedJavaAndJarFilesArray[];
    private List<String> newJavaFiles;
    private Set<String> updatedJavaFiles;
    private Set<String> recompiledJavaFiles;
    private Set<String> updatedClasses;       // This set is emptied on every new internal jmake iteration...
    private Set<String> allUpdatedClasses;    // whereas in this one the names of all updated classes found during this jmake invocation are stored.
    private Set<String> updatedAndCheckedClasses;
    private Set<String> deletedClasses;
    private Set<String> updatedJarFiles;
    private Set<String> stableJarFiles;
    private Set<String> newJarFiles;
    private Set<String> deletedJarFiles;
    /* Dependencies from the dependencyFile, if any */
    private Map<String, List<String>> extraDependencies;

    private String destDir;
    private boolean destDirSpecified;
    private List<String> javacAddArgs;
    private Class<?> compilerClass;
    private Method compileMethod;
    private String jcExecApp;
    private Object externalApp;
    private Method externalCompileSourceFilesMethod;
    private Adler32 checkSum;
    private CompatibilityChecker cv;
    private ClassFileReader cfr;
    private boolean newProject = false;
    private String dependencyFile = null;
    private static boolean backSlashFileSeparator = File.separatorChar != '/';

    /**** Interface to the class ****/
    /**
     * Either projectJavaAndJarFilesArray != null and added.. == removed.. == updatedJavaAndJarFilesArray == null,
     * or projectJavaAndJarFilesArray == null and one or more of others != null.
     * When PCDManager is called from Main, this is guaranteed, since separate entrypoint functions initialize
     * either one or another of the above argument groups, but never both.
     */
    public PCDManager(PCDContainer pcdc,
                      String projectJavaAndJarFilesArray[],
                      String addedJavaAndJarFilesArray[],
                      String removedJavaAndJarFilesArray[],
                      String updatedJavaAndJarFilesArray[],
                      String in_destDir,
                      List<String> javacAddArgs,
                      boolean failOnDependentJar,
                      boolean noWarnOnDependentJar,
                      String dependencyFile) {
        this.pcdc = pcdc;
        if (pcdc.pcd == null) {
            pcd = new LinkedHashMap<String,PCDEntry>();
            pcdc.pcd = pcd;
            newProject = true;
        } else {
            pcd = pcdc.pcd;
        }

        this.projectJavaAndJarFilesArray = projectJavaAndJarFilesArray;
        this.addedJavaAndJarFilesArray = addedJavaAndJarFilesArray;
        this.removedJavaAndJarFilesArray = removedJavaAndJarFilesArray;
        this.updatedJavaAndJarFilesArray = updatedJavaAndJarFilesArray;
        this.dependencyFile = dependencyFile;
        newJavaFiles = new ArrayList<String>();
        updatedJavaFiles = new LinkedHashSet<String>();
        recompiledJavaFiles = new LinkedHashSet<String>();
        updatedAndCheckedClasses = new LinkedHashSet<String>();
        deletedClasses = new LinkedHashSet<String>();
        allUpdatedClasses = new LinkedHashSet<String>();

        updatedJarFiles = new LinkedHashSet<String>();
        stableJarFiles = new LinkedHashSet<String>();
        newJarFiles = new LinkedHashSet<String>();
        deletedJarFiles = new LinkedHashSet<String>();

        initializeDestDir(in_destDir);
        this.javacAddArgs = javacAddArgs;

        checkSum = new Adler32();

        cv = new CompatibilityChecker(this, failOnDependentJar, noWarnOnDependentJar);
        cfr = new ClassFileReader();
    }

    public Collection<PCDEntry> entries() {
        return pcd.values();
    }

    public ClassFileReader getClassFileReader() {
        return cfr;
    }

    public ClassInfo getClassInfoForName(int verCode, String className) {
        PCDEntry pcde = pcd.get(className);
        if (pcde != null) {
            return getClassInfoForPCDEntry(verCode, pcde);
        } else {
            return null;
        }
    }

    public boolean isProjectClass(int verCode, String className) {
        if (verCode == ClassInfo.VER_OLD) {
            return pcd.containsKey(className);
        } else {
            PCDEntry pcde = pcd.get(className);
            return (pcde != null && pcde.checkResult != PCDEntry.CV_DELETED);
        }
    }

    /**
     * Get an instance of ClassInfo (load a class file if necessary) for the given version (old or new) of
     * the class determined by pcde. For an old class version, always returns a non-null result; but for a new
     * version, null is returned if class file is not found. In most of the current uses of this method null result
     * is not checked, because it's either called for an old version or it is already known that the .class file
     * should be present; nevertheless, beware!
     */
    public ClassInfo getClassInfoForPCDEntry(int verCode, PCDEntry pcde) {
        if (verCode == ClassInfo.VER_OLD) {
            return pcde.oldClassInfo;
        }

        ClassInfo res = pcde.newClassInfo;
        if (res == null) {
            byte classFileBytes[];
            String classFileFullPath = null;
            if (pcde.javaFileFullPath.endsWith(".java")) {
                File classFile = Utils.checkFileForName(pcde.classFileFullPath);
                if (classFile == null) {
                    return null;  // Class file not found.
                }
                classFileBytes = Utils.readFileIntoBuffer(classFile);
                classFileFullPath = pcde.classFileFullPath;
            } else {
                try {
                    JarFile jarFile = new JarFile(pcde.javaFileFullPath);
                    JarEntry jarEntry =
                            jarFile.getJarEntry(pcde.className + ".class");
                    if (jarEntry == null) {
                        return null;
                    }
                    classFileBytes =
                            Utils.readZipEntryIntoBuffer(jarFile, jarEntry);
                } catch (IOException ex) {
                    throw new PrivateException(ex);
                }
            }
            res =
                    new ClassInfo(classFileBytes, verCode, this, classFileFullPath);
            pcde.newClassInfo = res;
        }
        return res;
    }

    /**
     * Returns null if class is compileable (has a .java source) and not recompiled yet, "" if
     * class has already been recompiled or has been deleted from project, and the class's .jar
     * name if class comes from a jar, hence is uncompileable.
     */
    public String classAlreadyRecompiledOrUncompileable(String className) {
        PCDEntry pcde = pcd.get(className);
        if (pcde == null) {
            //!!!
            for (String keyName : pcd.keySet()) {
                PCDEntry entry = pcd.get(keyName);
                if (entry.className.equals(className)) {
                    System.out.println("ERROR: inconsistent entry: key = " +
                            keyName + ", name in entry = " + entry.className);
                }
            }
            //!!!
            throw internalException(className + " not in project when it should be");
        }
        if (pcde.checkResult == PCDEntry.CV_DELETED) {
            return "";
        }
        if (pcde.javaFileFullPath.endsWith(".jar")) {
            return pcde.javaFileFullPath;
        } else {
            return (recompiledJavaFiles.contains(pcde.javaFileFullPath) ? "" : null);
        }
    }

    /**
     * Compiler initialization depends on compiler type specified.
     * If jcExecApp != null, i.e. an external executable compiler application is used, and nothing has to be done.
     * If externalApp != null, that is, jmake is called by an external application such as Ant, which
     * manages compilation in its own way, and also nothing has to be done.
     * Otherwise, load the compiler class and method (either specified through jcPath, jcMainClass and jcMethod,
     * or the default one.
     */
    public void initializeCompiler(String jcExecApp,
            String jcPath, String jcMainClass, String jcMethod,
            Object externalApp, Method externalCompileSourceFilesMethod) {
        ClassPath.initializeAllClassPaths();

        if (externalApp != null) {
            this.externalApp = externalApp;
            this.externalCompileSourceFilesMethod =
                    externalCompileSourceFilesMethod;
            return;
        }
        if (jcExecApp != null) {
            this.jcExecApp = jcExecApp;
            return;
        }

        if (jcPath == null) {
            String javaHome = System.getProperty("java.home");
            // In my tests it ends with '/jre'. Or it could be ending with '/bin' as well? Let's assume it can be both and delete
            // this latter directory.
            if (javaHome.endsWith(File.separator + "jre") || javaHome.endsWith(File.separator + "bin")) {
                javaHome = javaHome.substring(0, javaHome.length() - 4);
            }
            jcPath = javaHome + "/lib/tools.jar";
        }
        ClassLoader compilerLoader;
        try {
            compilerLoader = ClassPath.getClassLoaderForPath(jcPath);
        } catch (Exception ex) {
            throw compilerInteractionException("error opening compiler path", ex, 0);
        }

        if (jcMainClass == null) {
            jcMainClass = "com.sun.tools.javac.Main";
        }
        if (jcMethod == null) {
            jcMethod = "compile";
        }

        try {
            compilerClass = compilerLoader.loadClass(jcMainClass);
        } catch (ClassNotFoundException e) {
            throw compilerInteractionException("error loading compiler main class " + jcMainClass, e, 0);
        }

        Class<?>[] args = new Class<?>[]{String[].class};
        try {
            compileMethod = compilerClass.getMethod(jcMethod, args);
        } catch (Exception e) {
            throw compilerInteractionException("error getting method com.sun.tools.javac.Main.compile(String args[])", e, 0);
        }
    }

    /** Main entrypoint for this class */
    public void run() {
        Utils.startTiming(Utils.TIMING_SYNCHRO);
        synchronizeProjectFilesAndPCD();
        Utils.stopAndPrintTiming("Synchro", Utils.TIMING_SYNCHRO);
        Utils.printTiming("of which synchro check file", Utils.TIMING_SYNCHRO_CHECK_JAVA_FILES);

        Utils.startTiming(Utils.TIMING_FIND_UPDATED_JAVA_FILES);
        findUpdatedJavaAndJarFiles();
        Utils.stopAndPrintTiming("findUpdatedJavaAndJarFiles", Utils.TIMING_FIND_UPDATED_JAVA_FILES);
        Utils.printTiming("of which classFileObsoleteOrDeleted", Utils.TIMING_CLASS_FILE_OBSOLETE_OR_DELETED);

        // Let's free some memory
        projectJavaAndJarFilesArray = null;

        updatedClasses = new LinkedHashSet<String>();
        dealWithClassesInUpdatedJarFiles();

        int iterNo = 0;
        int res = 0;
        while (iterNo == 0 || updatedJavaFiles.size() != 0 || newJavaFiles.size() != 0) {
            // It may happen that we didn't find any updated or new .java files. However, we still need to enter
            // this loop because there may be some class files that need compatibility checking. This can happen
            // either if somebody had recompiled their sources bypassing jmake, or if their checking during the
            // previous invocation of jmake failed, because their dependent code recompilation failed.
            if (updatedJavaFiles.size() > 0 || newJavaFiles.size() > 0) {
                Utils.startTiming(Utils.TIMING_COMPILE);
                int intermediateRes = recompileUpdatedJavaFiles();
                Utils.stopAndPrintTiming("Compile", Utils.TIMING_COMPILE);
                if (intermediateRes != 0) {
                    res = intermediateRes;
                }
            }

            Utils.startTiming(Utils.TIMING_PDBUPDATE);
            // New classes can be added to pdb only if compilation was successful, i.e. the new project version is consistent.
            if (iterNo++ == 0 && res == 0) {
                findClassFilesForNewJavaAndJarFiles();
                findClassFilesForUpdatedJavaFiles();
                dealWithNestedClassesForUpdatedJavaFiles();
            }
            Utils.stopAndPrintTiming("Entering new classes in PDB", Utils.TIMING_PDBUPDATE);

            updatedJavaFiles.clear();
            newJavaFiles.clear();

            Utils.startTiming(Utils.TIMING_FIND_UPDATED_CLASSES);
            findUpdatedClasses();
            Utils.stopAndPrintTiming("Find updated classes", Utils.TIMING_FIND_UPDATED_CLASSES);

            Utils.startTiming(Utils.TIMING_CHECK_UPDATED_CLASSES);
            checkDeletedClasses();
            checkUpdatedClasses();
            Utils.stopAndPrintTiming("Check updated classes", Utils.TIMING_CHECK_UPDATED_CLASSES);

            updatedClasses = new LinkedHashSet<String>();
            if (ClassPath.getVirtualPath() != null) {
                if (res != 0)
                    break;
            }
        }

        Utils.startTiming(Utils.TIMING_PDBWRITE);
        updateClassFilesInfoInPCD(res);
        pcdc.save();
        Utils.stopAndPrintTiming("PDB write", Utils.TIMING_PDBWRITE);

        if (res != 0) {
            throw compilerInteractionException("compilation error(s)", null, res);
        }
    }

    /**
     * Find the newly-created class files for existing java files.
     */
    private void findClassFilesForUpdatedJavaFiles() {
        if (dependencyFile == null)
            return;

        Set<String> allClasses = new HashSet<String>();

        Map<String, List<String>> dependencies = parseDependencyFile();
        for (String file : updatedJavaFiles) {
            List<String> myDeps = dependencies.get(file);
            if (myDeps != null) {
                PCDEntry parent = getNamedPCDE(file, dependencies);
                for (String dependency : myDeps) {
                    allClasses.add(dependency);
                    if (pcd.containsKey(dependency))
                        continue;
                    findClassFileOnFilesystem(file, parent, dependency, false);
                }
            }
        }
        for (Map.Entry<String, PCDEntry> entry : pcd.entrySet()) {
            String cls = entry.getKey();
            if (!allClasses.contains(cls)) {
                PCDEntry pcde = entry.getValue();
                if (updatedJavaFiles.contains(pcde.javaFileFullPath)) {
                    deletedClasses.add(cls);
                }
            }
        }
    }

    public String[] getAllUpdatedClassesAsStringArray() {
        String[] res = new String[allUpdatedClasses.size()];
        int i = 0;
        for (String updatedClass : allUpdatedClasses) {
            res[i++] = updatedClass.replace('/', '.');
        }
        return res;
    }

    /**
     * Synchronize projectJavaAndJarFilesArray and PCD, i.e. leave only those entries in the PCD which have their
     * .java (.jar) files in projectJavaAndJarFilesArray. New .java files in projectJavaAndJarFilesArray (i.e. those
     * for which there are no entries in the PCD yet) are added to newJavaFiles; new .jar files are added to newJarFiles.
     * Alternatively, just use the supplied arrays of added and deleted .java and .jar files.
     *
     * For entries whose .java files are not in the PCD anymore, try to delete .class files. We need to do that before
     * compilation to avoid the situation when a .java file is removed but compilation succeeds because the .class file
     * is still there.
     *
     * Unfortunately, we also need to delete all class files for non-nested classes whose names differ from their .java
     * file name, because we can't tell when they've been removed from their .java files -- but it's only safe to do this
     * for files that originate from java files that we're compiling this round.
     *
     * Upon return from this method, all of the .java and .jar files in the PCD are known to exist.
     */
    private void synchronizeProjectFilesAndPCD() {
        if (projectJavaAndJarFilesArray != null) {
            Set<String> pcdJavaFilesSet = new LinkedHashSet<String>(pcd.size() * 3 / 2);
            for(PCDEntry entry : entries()) {
                pcdJavaFilesSet.add(entry.javaFileFullPath);
            }

            Set<String> canonicalPJF =
                    new LinkedHashSet<String>(projectJavaAndJarFilesArray.length * 3 / 2);

            // Add .java files that are not in PCD to newJavaFiles; add .jar files that are not in PCD to newJarFiles.
            for (int i = 0; i < projectJavaAndJarFilesArray.length; i++) {
                String projFileName = projectJavaAndJarFilesArray[i];
                Utils.startTiming(Utils.TIMING_SYNCHRO_CHECK_TMP);
                File projFile = Utils.checkFileForName(projFileName);
                Utils.stopAndAddTiming(Utils.TIMING_SYNCHRO_CHECK_TMP, Utils.TIMING_SYNCHRO_CHECK_JAVA_FILES);
                if (projFile == null) {
                    throw new PrivateException(new FileNotFoundException("specified source file " + projFileName + " not found."));
                }
                // The main reason for using getAbsolutePath() instead of more reliable getCanonicalPath() is the fact that
                // sometimes users may name the actual files containing Java code in some custom way, and give javac/jmake
                // symbolic links to these files (that have correct .java names) instead. getCanonicalPath(), however, returns the
                // real (i.e. user custom) file name, which will confuse our test below and then javac.
                String absoluteProjFileName = projFile.getAbsolutePath();
                // On Windows, make sure the drive letter is always in lower case
                if (backSlashFileSeparator) {
                    absoluteProjFileName =
                            Utils.convertDriveLetterToLowerCase(absoluteProjFileName);
                }
                canonicalPJF.add(absoluteProjFileName);
                if (!pcdJavaFilesSet.contains(absoluteProjFileName)) {
                    if (absoluteProjFileName.endsWith(".java")) {
                        newJavaFiles.add(absoluteProjFileName);
                    } else if (absoluteProjFileName.endsWith(".jar")) {
                        newJarFiles.add(absoluteProjFileName);
                    } else {
                        throw new PrivateException(new PublicExceptions.InvalidSourceFileExtensionException("specified source file " + projFileName + " has an invalid extension (not .java or .jar)."));
                    }
                }
            }

            // Find the entries containing .java or .jar files that are not in project anymore
            for (Entry<String, PCDEntry> entry : pcd.entrySet()) {
                String key = entry.getKey();
                PCDEntry e = entry.getValue();
                e.oldClassInfo.restorePCDM(this);
                if (canonicalPJF.contains(e.javaFileFullPath)) {
                    if (e.isPackagePrivateClass()) {
                        initializeClassFileFullPath(e);
                        new File(e.classFileFullPath).delete();
                    }
                } else {
                    if (ClassPath.getVirtualPath() == null) {
                        deletedClasses.add(key);
                    } else {
                        // Okay, not found locally, but virtual path was defined, so try it now....
                        if ( (e.oldClassFileFingerprint == projectJavaAndJarFilesArray.length &&
                                newJavaFiles.size() == 0) ||
                                Utils.checkFileForName(e.javaFileFullPath) != null)
                        {
                            e.checkResult = PCDEntry.CV_NEWER_FOUND_NEARER;
                            e.oldClassFileFingerprint = projectJavaAndJarFilesArray.length;
                        }
                        else
                        {
                            String classFound = null;
                            String sourceFound = null;
                            // Find source and class file via virtual path
                            String path = ClassPath.getVirtualPath();
                            // TODO(Eric Ayers): IntelliJ static analysis shows several useless
                            // expressions that make this loop a no-op.
                            for (StringTokenizer st = new StringTokenizer(path, File.pathSeparator);
                                !(classFound != null && sourceFound != null) && st.hasMoreTokens();)
                            {
                                String fullPath = st.nextToken()+File.separator+e.className;
                                if (sourceFound != null && new File(fullPath+".java").exists())
                                {
                                    sourceFound = fullPath + ".java";
                                }
                                if (classFound != null && new File(fullPath+".class").exists())
                                {
                                    classFound = fullPath + ".class";
                                }
                            }
                            // TODO(Eric Ayers): IntelliJ static analysis shows that this expression
                            // is always true.
                            if (classFound == null)
                            {
                                deletedClasses.add(key);
                                if (e.javaFileFullPath.endsWith(".jar"))
                                {
                                    deletedJarFiles.add(e.javaFileFullPath);
                                }
                                else
                                {
                                    initializeClassFileFullPath(e);
                                    (new File(e.classFileFullPath)).delete();
                                }
                            }
                            else if (sourceFound != null)
                            {
                                newJavaFiles.add(sourceFound);
                                e.checkResult = PCDEntry.CV_NEWER_FOUND_NEARER;
                                e.oldClassFileFingerprint = projectJavaAndJarFilesArray.length;
                            }
                            else
                            {
                                classFound = classFound.replace('/', File.separatorChar);
                                throw new PrivateException(new FileNotFoundException("deleted class " + classFound + " still exists."));
                            }
                        }
                    }
                    if (e.javaFileFullPath.endsWith(".jar")) {
                        deletedJarFiles.add(e.javaFileFullPath);
                    } else {  // Try to delete a class file for the removed project class.
                        initializeClassFileFullPath(e);
                        (new File(e.classFileFullPath)).delete();
                    }
                }
            }
        } else { // projectJavaAndJarFilesArray == null - use supplied arrays of added and removed .java and .jar files
            if (addedJavaAndJarFilesArray != null) {
                for (String fileName : addedJavaAndJarFilesArray) {
                    fileName = fileName.intern();
                    if (fileName.endsWith(".java")) {
                        newJavaFiles.add(fileName);
                    } else if (fileName.endsWith(".jar")) {
                        newJarFiles.add(fileName);
                    } else {
                        throw new PrivateException(new PublicExceptions.InvalidSourceFileExtensionException(
                            "specified source file " + fileName + " has an invalid extension (not .java or .jar)."));
                    }
                }
            }

            Set<String> removedJavaAndJarFilesSet = null;
            if (removedJavaAndJarFilesArray != null) {
                removedJavaAndJarFilesSet = new LinkedHashSet<String>();
                for (String fileName : removedJavaAndJarFilesArray) {
                    fileName = fileName.intern();
                    removedJavaAndJarFilesSet.add(fileName);
                    if (fileName.endsWith(".jar")) {
                        deletedJarFiles.add(fileName);
                    }
                }
            }

            for (Entry<String, PCDEntry> entry : pcd.entrySet()) {
                String key = entry.getKey();
                PCDEntry e = entry.getValue();
                e.oldClassInfo.restorePCDM(this);
                if (removedJavaAndJarFilesSet != null &&
                        removedJavaAndJarFilesSet.contains(e.javaFileFullPath)) {
                    deletedClasses.add(key);
                    if (!e.javaFileFullPath.endsWith(".jar")) {  // Try to delete a class file for the removed project class.
                        initializeClassFileFullPath(e);
                        (new File(e.classFileFullPath)).delete();
                    }
                }
            }
        }
    }

    /**
     * In the end of run, update the information in the project database for the class files which have
     * been updated and checked, or deleted. If compilationResult == 0, i.e. all recompilations were
     * successful, information for new versions of all of the classes is made permanent, and entries
     * for deleted classes are removed permanently. Otherwise, information is updated only for those
     * classes whose old and new versions were found source compatible.
     */
    private void updateClassFilesInfoInPCD(int compilationResult) {
        // If the project appears to be inconsistent after changes, make a preliminary pass that will deal with enclosing
        // classes for deleted nested classes. The problem with them can be as follows: we delete a nested class C$X,
        // which is still referenced from somewhere. However, C has not changed at all or at least incompatibly, and
        // thus we update its PCDEntry, which now does not reference C$X. Other parts of jmake require that a nested
        // class is always referenced from its directly enclusing class, thus to keep the PCD consistent we have to remove
        // C$X from the PCD. On the next invocation of jmake, C$X is not in the PDB at all, and thus any classes that
        // may still reference it and have not been updated are not checked => project becomes inconsistent. We could do
        // better by immediately marking enclosing classes incompatible once we detect that a deleted nested class is
        // really referenced from somewhere, but the solution below seems to be more robust.
        if (compilationResult != 0) {
            for (String className : updatedAndCheckedClasses) {
                PCDEntry entry = pcd.get(className);
                if (entry.checkResult == PCDEntry.CV_DELETED &&
                        !"".equals(entry.oldClassInfo.directlyEnclosingClass)) {
                    PCDEntry enclEntry =
                            pcd.get(entry.oldClassInfo.directlyEnclosingClass);
                    enclEntry.checkResult = PCDEntry.CV_INCOMPATIBLE;
                }
            }
        }

        for (String className : updatedAndCheckedClasses) {
            PCDEntry entry = pcd.get(className);
            if (entry.checkResult == PCDEntry.CV_UNCHECKED) {
                continue;
            }
            if (ClassPath.getVirtualPath() != null) {
                if (entry.checkResult == PCDEntry.CV_NEWER_FOUND_NEARER) {
                    continue;
                }
            }
            if (entry.checkResult == PCDEntry.CV_DELETED) {
                if (compilationResult == 0) {
                    pcd.remove(className);  // Only if consistency checking is ok, a deleted class can be safely removed from the PCD
                }
            } else if (entry.checkResult == PCDEntry.CV_COMPATIBLE ||
                    entry.checkResult == PCDEntry.CV_NEW ||
                    (entry.checkResult == PCDEntry.CV_INCOMPATIBLE && compilationResult == 0)) {
                if (entry.newClassInfo == null) {  // "Safety net" for the (hopefully unlikely) case we overlooked something before...
                    Utils.printWarningMessage("Warning: internal information inconsistency detected during pdb updating");
                    Utils.printWarningMessage(Utils.REPORT_PROBLEM);
                    Utils.printWarningMessage("Class name: " + className);
                    if (entry.checkResult == PCDEntry.CV_NEW) {
                        pcd.remove(className);
                    } else {
                        continue;
                    }
                }
                entry.oldClassFileLastModified = entry.newClassFileLastModified;
                entry.oldClassFileFingerprint = entry.newClassFileFingerprint;
                entry.oldClassInfo = entry.newClassInfo;
            }
        }
    }

    /**
     * Find all .java files on the filesystem,  for which the .class file does not exist
     * or is newer than the .java file. Also find all .jar files for which the timestamp
     * has changed. Alternatively, just use the supplied array of updated .java/.jar files.
     */
    private void findUpdatedJavaAndJarFiles() {
        boolean projectSpecifiedAsAllSources =
                projectJavaAndJarFilesArray != null;
        for (PCDEntry entry : entries()) {
            if (deletedClasses.contains(entry.className)) {
                continue;
            }
            if (entry.javaFileFullPath.endsWith(".java")) {
                initializeClassFileFullPath(entry);
                if (projectSpecifiedAsAllSources) {
                    if (ClassPath.getVirtualPath() != null) {
                        String paths[] = ClassPath.getVirtualPath().split(File.pathSeparator);
                        String tmpClassName = entry.className;
                        tmpClassName = tmpClassName.replaceAll("\\Q$\\E.*$", "");
                        for (int i=0; i<paths.length; i++) {
                            String tmpFilename = paths[i] + File.separator + tmpClassName + ".java";
                            File tmpFile = new File(tmpFilename);
                            if (tmpFile.exists()) {
                                entry.javaFileFullPath = tmpFile.getAbsolutePath();
                                break;
                            }
                        }
                    }
                    Utils.startTiming(Utils.TIMING_CLASS_FILE_OBSOLETE_TMP);
                    if (classFileObsoleteOrDeleted(entry)) {
                        updatedJavaFiles.add(entry.javaFileFullPath);
                    }
                    Utils.stopAndAddTiming(Utils.TIMING_CLASS_FILE_OBSOLETE_TMP, Utils.TIMING_CLASS_FILE_OBSOLETE_OR_DELETED);
                }
                entry.checked = true;
            } else {  // Class coming from a .jar file. Mark this entry as checked only if its JAR hasn't changed
                if (projectJavaAndJarFilesArray != null) {
                    entry.checked = !checkJarFileForUpdate(entry);
                }
            }
        }

        // Lists of updated/added/deleted source files specified instead of a full list of project sources
        if (!projectSpecifiedAsAllSources && updatedJavaAndJarFilesArray != null) {
            for (int i = 0; i < updatedJavaAndJarFilesArray.length; i++) {
                if (updatedJavaAndJarFilesArray[i].endsWith(".java")) {
                    updatedJavaFiles.add(updatedJavaAndJarFilesArray[i]);
                } else {
                    updatedJarFiles.add(updatedJavaAndJarFilesArray[i]);
                }
            }
        }
    }

    private boolean classFileObsoleteOrDeleted(PCDEntry entry) {
        if (ClassPath.getVirtualPath() != null) {
            File file1 = new File(entry.javaFileFullPath);
            if (!file1.exists())
                throw new PrivateException(new FileNotFoundException("specified source file " +
                        entry.javaFileFullPath + " not found."));
            if (file1.lastModified() < entry.oldClassFileLastModified)
            {
                return false;
            }
        }
        File classFile = Utils.checkFileForName(entry.classFileFullPath);
        if (classFile == null || !classFile.exists()) {
            return true; // Class file has been deleted
        }
        File javaFile = new File(entry.javaFileFullPath); // Guaranteed to exist at this point
        if (classFile.lastModified() <= javaFile.lastModified()) {
            return true;
        }
        return false;
    }

    private boolean checkJarFileForUpdate(PCDEntry entry) {
        String jarFileName = entry.javaFileFullPath;
        if (stableJarFiles.contains(jarFileName)) {
            return false;
        } else if (updatedJarFiles.contains(jarFileName) ||
                newJarFiles.contains(jarFileName) ||
                deletedJarFiles.contains(jarFileName)) {
            return true;
        } else {
            File jarFile = new File(jarFileName); // Guaranteed to exist at this point.
            if (entry.oldClassFileLastModified != jarFile.lastModified()) {
                updatedJarFiles.add(jarFileName);
                return true;
            } else {
                stableJarFiles.add(jarFileName);
                return false;
            }
        }
    }

    public int recompileUpdatedJavaFiles() {
        if (externalApp != null) {
            return recompileUpdatedJavaFilesUsingExternalMethod();
        } else {
            return recompileUpdatedJavaFilesOurselves();
        }
    }

    private int recompileUpdatedJavaFilesOurselves() {
        int filesNo = updatedJavaFiles.size() + newJavaFiles.size();
        int addArgsNo = javacAddArgs.size();
        int argsNo = addArgsNo + filesNo + 2;
        String compilerBootClassPath, compilerExtDirs;
        if ((compilerBootClassPath = ClassPath.getCompilerBootClassPath()) != null) {
            argsNo += 2;
        }
        if ((compilerExtDirs = ClassPath.getCompilerExtDirs()) != null) {
            argsNo += 2;
        }
        if (jcExecApp != null) {
            argsNo++;
        }
        String args[] = new String[argsNo];
        int pos = 0;
        if (jcExecApp != null) {
            args[pos++] = jcExecApp;
        }
        for (int i = 0; i < addArgsNo; i++) {
            args[pos++] = javacAddArgs.get(i);
        }
        args[pos++] = "-classpath";
        args[pos++] = ClassPath.getCompilerUserClassPath();
        if (compilerBootClassPath != null) {
            args[pos++] = "-bootclasspath";
            args[pos++] = compilerBootClassPath;
        }
        if (compilerExtDirs != null) {
            args[pos++] = "-extdirs";
            args[pos++] = compilerExtDirs;
        }
        if (!newProject) {
            Utils.printInfoMessage("Recompiling source files:");
        }
        for (String javaFileFullPath : updatedJavaFiles) {
            if (!newProject) {
                Utils.printInfoMessage(javaFileFullPath);
            }
            recompiledJavaFiles.add(args[pos++] = javaFileFullPath);
        }
        for (int j = 0; j < newJavaFiles.size(); j++) {
            String javaFileFullPath = newJavaFiles.get(j);
            if (!newProject) {
                Utils.printInfoMessage(javaFileFullPath);
            }
            recompiledJavaFiles.add(args[pos++] = javaFileFullPath);
        }

        if (jcExecApp == null) {  // Executing javac or some other compiler within the same JVM
            Object reflectArgs[] = new Object[1];
            reflectArgs[0] = args;
            try {
                Object dummy = compilerClass.newInstance();
                Integer res = (Integer) compileMethod.invoke(dummy, reflectArgs);
                return res.intValue();
            } catch (Exception e) {
                throw compilerInteractionException("exception thrown when trying to invoke the compiler method", e, 0);
            }
        } else {  // Executing an external Java compiler, such as jikes
            int exitCode = 0;
            try {
                Process p = Runtime.getRuntime().exec(args);
                InputStream pErr = p.getErrorStream();
                InputStream pOut = p.getInputStream();
                boolean terminated = false;

                while (!terminated) {
                    try {
                        exitCode = p.exitValue();
                        terminated = true;
                    } catch (IllegalThreadStateException itse) { // Process not yet terminated, wait for some time
                        Utils.ignore(itse);
                        Utils.delay(100);
                    }
                    try {
                        Utils.readAndPrintBytesFromStream(pErr, System.err);
                        Utils.readAndPrintBytesFromStream(pOut, System.out);
                    } catch (IOException ioe1) {
                        throw compilerInteractionException("I/O error when reading the compiler application output", ioe1, exitCode);
                    }
                }
                return exitCode;
            } catch (IOException ioe2) {
                throw compilerInteractionException("I/O error when trying to invoke the compiler application", ioe2, exitCode);
            }
        }
    }

    /** Execution under complete control of external app - use externally supplied method to recompile classes */
    private int recompileUpdatedJavaFilesUsingExternalMethod() {
        int filesNo = updatedJavaFiles.size() + newJavaFiles.size();
        String[] fileNames = new String[filesNo];
        int i = 0;
        for (String updatedFile : updatedJavaFiles) {
            recompiledJavaFiles.add(fileNames[i] = updatedFile);
        }
        for (int j = 0; j < newJavaFiles.size(); j++) {
            recompiledJavaFiles.add(fileNames[i++] = newJavaFiles.get(j));
        }

        try {
            Integer res =
                    (Integer) externalCompileSourceFilesMethod.invoke(externalApp, new Object[]{fileNames});
            return res.intValue();
        } catch (IllegalAccessException e1) {
            throw compilerInteractionException("compiler method is not accessible", e1, 0);
        } catch (IllegalArgumentException e2) {
            throw compilerInteractionException("illegal arguments passed to compiler method", e2, 0);
        } catch (InvocationTargetException e3) {
            throw compilerInteractionException("exception when executing the compiler method", e3, 0);
        }
    }

    /**
     * For each .java file from newJavaFiles, find all of the .class files, the names of which we can
     * logically deduce (a top-level class with the same name, and all of the nested classes),
     * and put the info on them into the PCD. Also include any class files from the dependencyFile,
     * if any. For each .jar file from newJarFiles, find all of the .class files in that archive and
     * put info on them into the PCD.
     */
    private void findClassFilesForNewJavaAndJarFiles() {
        for (String javaFileFullPath : newJavaFiles) {
            PCDEntry pcde =
                    findClassFileOnFilesystem(javaFileFullPath, null, null, false);

            if (pcde == null) {
                // .class file not found - possible compilation error
                if (missingClassIsOk(javaFileFullPath)) {
                    continue;
                } else {
                    throw new PrivateException(new PublicExceptions.ClassNameMismatchException(
                            "Could not find class file for " + javaFileFullPath));
                }
            }
            Set<String> entries = new HashSet<String>();
            if (pcde.checkResult == PCDEntry.CV_NEW) {  // It's really a new .java file, not a moved one
                entries.addAll(findAndUpdateAllNestedClassesForClass(pcde, false));
            } else {
                entries.addAll(findAndUpdateAllNestedClassesForClass(pcde, true));
            }
            entries.add(pcde.className);
            if (dependencyFile != null) {
                Map<String, List<String>> dependencies = parseDependencyFile();
                List<String> myDeps = dependencies.get(javaFileFullPath);
                if (myDeps != null) {
                    for (String dependency : myDeps) {
                        if (entries.contains(dependency))
                            continue;
                        findClassFileOnFilesystem(javaFileFullPath, pcde,
                                dependency, false);
                    }
                }
            }
        }

        for (String newJarFile : newJarFiles) {
            processAllClassesFromJarFile(newJarFile);
        }
    }

    /**
     * Parse an extra dependency file.  The format of the file is a series of lines,
     * each consisting of:
     * SourceFileName.java -> ClassName
     * (these file names are relative to destDir)
     */
    private Map<String, List<String>> parseDependencyFile() {
        if (!destDirSpecified)
            throw new RuntimeException("Dependency files require destDir");
        if (extraDependencies != null)
            return extraDependencies;
        BufferedReader in = null;
        try {
            extraDependencies = new HashMap<String, List<String>>();
            in = new BufferedReader(new FileReader(dependencyFile));
            int lineNumber = 0;
            while (true) {
                lineNumber ++;
                String line = in.readLine();
                if (line == null)
                    break;
                String[] parts = line.split("->");
                if (parts.length != 2) {
                    throw new RuntimeException("Failed to parse line " + lineNumber + " of " + dependencyFile
                                               + ". Expected {foo.java} -> {classname}.");
                }
                String src = parts[0].trim();
                src = new File(destDir, src).getCanonicalPath();
                String cls = parts[1].trim();
                List<String> classes = extraDependencies.get(src);
                if (classes == null) {
                    classes = new ArrayList<String>();
                    extraDependencies.put(src, classes);
                }
                cls = cls.substring(0, cls.length() - 6); // strip trailing ".class"
                classes.add(cls);
            }
        } catch (IOException e) {
            throw new PrivateException(e);
        } finally {
            if (in != null)
                try {
                    in.close();
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
        }
        return extraDependencies;
    }

    /**
     * In most cases we want to fail the build if a class cannot be found.
     *
     * However there is one common valid case where a .java file might not contain
     * a class: package-info.java files.
     *
     * See this doc for more info: http://docs.oracle.com/javase/specs/jls/se7/html/jls-7.html
     */
    private boolean missingClassIsOk(String javaFileFullPath) {
        return javaFileFullPath != null && "package-info.java".equals(new File(javaFileFullPath).getName());
    }

    /**
     * Find the .class file for the given javaFileFullPath and create a new PCDEntry for it.
     * If enclosingClassPCDE is null, the named top-level class for the given .java file is looked up.
     * Otherwise, the specified class specified by nestedClassFullName is looked up.
     */
    private PCDEntry findClassFileOnFilesystem(String javaFileFullPath, PCDEntry enclosingClassPCDE, String nestedClassFullName, boolean isNested) {
        String classFileFullPath = null;
        String fullClassName;
        File classFile = null;

        if (enclosingClassPCDE == null) { // Looking for a top-level class. May need to locate an appropriate directory.
            // Remove the ".java" suffix. A Windows disk-name prefix, such as 'c:', will be cut off later automatically
            fullClassName =
                    javaFileFullPath.substring(0, javaFileFullPath.length() - 5);
            if (destDirSpecified) {
                // Search for the .class file. We first assume the longest possible name. In case of failure,
                // we cut the assumed top-most package from it and repeat the search.
                while (classFile == null) {
                    classFileFullPath = destDir + fullClassName + ".class";
                    classFile = Utils.checkFileForName(classFileFullPath);
                    if (classFile == null) {
                        int cutIndex = fullClassName.indexOf(File.separatorChar);
                        if (cutIndex == -1) {
                            // Most probably, there was an error during compilation of this file.
                            // This does not prevent us from continuing.
                            Utils.printWarningMessage("Warning: unable to find .class file corresponding to source " + javaFileFullPath + ": expected " + classFileFullPath);

                            return null;
                        }
                        fullClassName = fullClassName.substring(cutIndex + 1);
                    }
                }
            } else {
                classFileFullPath = fullClassName + ".class";
                classFile = Utils.checkFileForName(classFileFullPath);
                if (classFile == null) {
                    Utils.printWarningMessage("Warning: unable to find .class file corresponding to source " + javaFileFullPath);
                    return null;
                }
            }
        } else {  // Looking for a nested class, which always sits in the same directory as its enclosing class
            classFileFullPath =
                    Utils.getClassFileFullPathForNestedClass(enclosingClassPCDE.classFileFullPath, nestedClassFullName);
            classFile = Utils.checkFileForName(classFileFullPath);
            if (classFile == null) {
                Utils.printWarningMessage("Warning: unable to find .class file corresponding to nested class " + nestedClassFullName);
                return null;
            }
            fullClassName = nestedClassFullName;
        }

        if (backSlashFileSeparator) {
            fullClassName = fullClassName.replace(File.separatorChar, '/');
        }

        byte classFileBytes[] = Utils.readFileIntoBuffer(classFile);
        ClassInfo classInfo =
                new ClassInfo(classFileBytes, ClassInfo.VER_NEW, this, classFileFullPath);
        if (isNested) {
            if (!classInfo.directlyEnclosingClass.equals(enclosingClassPCDE.newClassInfo.name)) {
                // Check if the above strings are like A and A$1. If so, there is actually no problem - the correct
                // answer is A$1. The reason why just A was determined as a directly enclosing class when parsing
                // class classInfo is due to the ambiguous interpretation of names like A$1$B. Such a name may mean
                // (1) a non-member local nested class B of A, or (2) a member class B of an anonymous nested class A$1.
                // When parsing any non-toplevel class, the first interpretation is always used.
                // NOTE FOR JDK 1.5 - starting from this version, there is no ambiguity anymore.
                // (1) will be called A$1B, and (2) will still be A$1$B
                String a = classInfo.directlyEnclosingClass;
                String ad1 = enclosingClassPCDE.newClassInfo.name;
                if (!((classInfo.javacTargetRelease == Utils.JAVAC_TARGET_RELEASE_OLDEST) &&
                        (ad1.startsWith(a + "$") && Character.isDigit(ad1.charAt(a.length() + 1))))) {
                    throw new PrivateException(new PublicExceptions.ClassFileParseException(
                            "Enclosing class names for class " + classInfo.name + " don't match:\n" +
                            classInfo.directlyEnclosingClass + " and " + enclosingClassPCDE.newClassInfo.name));
                }
            }
        }

        // If dest dir was specified, check if the deduced name is equal to the one in this class (in this case
        // they should necessarily match). Otherwise, without parsing the .java file, we can't reliably say what the
        // full class name (actually, its package part) should be - so we just note the name.
        if (destDirSpecified) {
            if (!fullClassName.equals(classInfo.name)) {
                throw new PrivateException(new PublicExceptions.ClassNameMismatchException(
                        "Error: deduced class name is different from the real one for source " +
                        javaFileFullPath + "\n" + fullClassName + " and " + classInfo.name));
            }
        } else {
            fullClassName = classInfo.name;
        }

        if (enclosingClassPCDE != null) {
            javaFileFullPath = enclosingClassPCDE.javaFileFullPath;
        }
        long classFileLastMod = classFile.lastModified();
        long classFileFP = computeFP(classFileBytes);

        if (pcd.containsKey(fullClassName)) {
            PCDEntry pcde = pcd.get(fullClassName);
            // If this entry has already been checked, it's a second entry for the same class, which is illegal.
            if (pcde.checkResult == PCDEntry.CV_NEWER_FOUND_NEARER) {
                // Newer copy of same file found in closer layer
                // Reset to CV_UNCHECKED and skip redundnacy check
                // as we know this would be redundant
                pcde.checkResult = PCDEntry.CV_UNCHECKED;
            } else {
                if (pcde.checked) {
                    throw new PrivateException(new PublicExceptions.DoubleEntryException(
                            "Two entries for class " + classInfo.name + " detected: " + pcde.javaFileFullPath + " and " + javaFileFullPath));
                }
            }
            // Otherwise, it means that the .java file for this class has been moved. jmake initially interprets
            // a new source file name as a new class, and it's only at this point that we can actually see that it was
            // only a move. We update javaFileFullPath for nested classes after we return from here.
            pcde.javaFileFullPath = javaFileFullPath;
            pcde.classFileFullPath = classFileFullPath;
            pcde.newClassInfo = classInfo;
            if (deletedClasses.contains(fullClassName)) {
                deletedClasses.remove(fullClassName);
            }
            return pcde;
        }

        PCDEntry pcde = new PCDEntry(fullClassName,
                javaFileFullPath,
                classFileFullPath, classFileLastMod, classFileFP,
                classInfo);
        pcde.checkResult = PCDEntry.CV_NEW;          // So that later it's promoted into oldClassInfo correctly
        updatedAndCheckedClasses.add(fullClassName); // So that the above happens
        pcd.put(fullClassName, pcde);
        return pcde;
    }

    /**
     * For the given class, find all direct nested classes (which may include reading their .class files from the
     * class path) and set their access flags (contained in this, enclosing class, object) appropriately. If
     * this class is a one coming from a .java source, repeat the procedure for each nested class in turn.
     * Otherwise, i.e. if a class comes from a .jar, don't bother, since we will come across each of these
     * classes anyway - when scanning their .jar. If 'move' parameter is true, it means that this method is called for
     * a class that is not new, but has been moved (and possibly updated).
     */
    private Set<String> findAndUpdateAllNestedClassesForClass(PCDEntry pcde, boolean move) {
        ClassInfo classInfo = pcde.newClassInfo;
        if (classInfo.nestedClasses == null) {
            return Collections.emptySet();
        }
        Set<String> entries = new LinkedHashSet<String>();
        String nestedClasses[] = classInfo.nestedClasses;
        String javaFileFullPath = pcde.javaFileFullPath;
        String enclosingClassFileFullPath = pcde.classFileFullPath;
        boolean isJavaSourceFile = javaFileFullPath.endsWith(".java");

        for (int i = 0; i < nestedClasses.length; i++) {
            PCDEntry nestedPCDE = pcd.get(nestedClasses[i]);
            if (nestedPCDE == null) {
                if (isJavaSourceFile) {
                    nestedPCDE =
                            findClassFileOnFilesystem(null, pcde, nestedClasses[i], true);
                }
                // For classes that come from a .jar, pcde should already be there. Otherwise this class just doesn't exist.
                if (nestedPCDE == null) {
                    // Probably a compilation error, such that enclosing class is compiled but nested is not.
                    throw new PrivateException(new PublicExceptions.ClassNameMismatchException(
                            "Could not find class file for " + pcde.toString()));
                }
            }
            if (move) {
                if (deletedClasses.contains(nestedClasses[i])) {
                    deletedClasses.remove(nestedClasses[i]);
                }
                nestedPCDE.javaFileFullPath = javaFileFullPath;
                if (javaFileFullPath.endsWith(".java")) {
                    nestedPCDE.classFileFullPath =
                            Utils.getClassFileFullPathForNestedClass(enclosingClassFileFullPath, nestedClasses[i]);
                } else {
                    nestedPCDE.classFileFullPath = javaFileFullPath;
                }
            }
            if (nestedPCDE.newClassInfo == null) {
                getClassInfoForPCDEntry(ClassInfo.VER_NEW, nestedPCDE);
            }
            nestedPCDE.newClassInfo.accessFlags =
                    pcde.newClassInfo.nestedClassAccessFlags[i];
            nestedPCDE.newClassInfo.isNonMemberNestedClass =
                    pcde.newClassInfo.nestedClassNonMember[i];

            entries.add(nestedPCDE.className);
            entries.addAll(findAndUpdateAllNestedClassesForClass(nestedPCDE, move));
        }
        return entries;
    }

    /**
     * Take care of new nested classes that could have been generated from already existing .java sources,
     * and of nested classes that do not exist anymore because they were deleted from these sources.
     */
    private void dealWithNestedClassesForUpdatedJavaFiles() {
        if (updatedJavaFiles.size() == 0) {
            return;
        }

        // First put PCDEntries for all updated classes that have nested classes into a temporary list.
        // That's because we can then find new nested classes, which we will need to add to the PCD, which
        // may probably conflict with us still iterating over it.
        List<PCDEntry> updatedEntries = new ArrayList<PCDEntry>();
        for (PCDEntry pcde : entries()) {
            if (pcde.checkResult == PCDEntry.CV_NEW) {
                continue;  // This class has just been added to the PCD
            }
            if (updatedJavaFiles.contains(pcde.javaFileFullPath)) {
                ClassInfo oldClassInfo = pcde.oldClassInfo;
                ClassInfo newClassInfo =
                        getClassInfoForPCDEntry(ClassInfo.VER_NEW, pcde);
                if (newClassInfo == null) {
                    deletedClasses.add(pcde.className);
                    continue; // Class file deleted then not re-created due to a compilation error somewhere.
                }
                if (oldClassInfo.nestedClasses != null || newClassInfo.nestedClasses != null) {
                    updatedEntries.add(pcde);
                }
            }
        }

        if (dependencyFile != null) {
            Map<String, List<String>> dependencies = parseDependencyFile();
            for (String file : updatedJavaFiles) {
                List<String> myDeps = dependencies.get(file);
                if (myDeps == null)
                    continue;
                PCDEntry pcde = getNamedPCDE(file, dependencies);
                for (String dependency : myDeps) {
                    PCDEntry dep = pcd.get(dependency);
                    if (dep != null)
                        // This is an existing dep.
                        continue;
                    dep = findClassFileOnFilesystem(file, pcde, dependency,  false);
                    getClassInfoForPCDEntry(ClassInfo.VER_NEW, dep);
                    if (dep.newClassInfo.nestedClasses != null)
                        updatedEntries.add(dep);
                }
            }
        }
        dealWithNestedClassesForUpdatedPCDEntries(updatedEntries, false);
    }

    private PCDEntry getNamedPCDE(String file, Map<String, List<String>> dependencies) {
        List<String> depsForFile = dependencies.get(file);
        PCDEntry pcde = null;
        // Find a non-nested class for this java file for which we already have
        // a pcde
        for (String dependency : depsForFile) {
            if (dependency.indexOf('$') != -1)
                continue;
            pcde = pcd.get(dependency);
            if (pcde != null)
                break;
        }
        if (pcde == null) {
            throw new PrivateException(new PublicExceptions.InternalException(file
                    + " was supposed to be an updated file, but there are no PCDEntries for any of its deps"));
        }
        return pcde;
    }

    private void dealWithNestedClassesForUpdatedPCDEntries(List<PCDEntry> entries, boolean move) {
        for (int i = 0; i < entries.size(); i++) {
            PCDEntry pcde = entries.get(i);
            ClassInfo oldClassInfo = pcde.oldClassInfo;
            ClassInfo newClassInfo = pcde.newClassInfo;
            if (newClassInfo.nestedClasses != null) {
                Set<String> nested = findAndUpdateAllNestedClassesForClass(pcde, move);
                if (oldClassInfo.nestedClasses != null) {  // Check if any old nested classes don't exist anymore
                    for (int j = 0; j < oldClassInfo.nestedClasses.length; j++) {
                        boolean found = false;
                        String oldNestedClass = oldClassInfo.nestedClasses[j];
                        for (int k = 0; k < newClassInfo.nestedClasses.length; k++) {
                            if (oldNestedClass.equals(newClassInfo.nestedClasses[k])) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            deletedClasses.add(oldNestedClass);
                        }
                    }
                }
            } else {  // newNestedClasses == null and oldNestedClasses != null, so all nested classes have been removed in the new version
                for (int j = 0; j < oldClassInfo.nestedClasses.length; j++) {
                    deletedClasses.add(oldClassInfo.nestedClasses[j]);
                }
            }
        }
    }

    private void findUpdatedClasses() {
        // This (iterating over all of the classes once again after performing that in classFileObsoleteOrDeleted()) may
        // seem time-consuming, but in reality it isn't, since the most time-consuming operation of obtaining internal
        // file handles for class files has already been performed in classFileObsoleteOrDeleted(). Once we have done that,
        // this re-iteration takes very small amount of time. However, if we switch from "class file older than .java
        // file" to ".java file timestamp changed" condition for recompilation, this will have to be changed as well.
         for (PCDEntry entry : entries()) {
            String className = entry.className;
            if (updatedAndCheckedClasses.contains(className) ||
                    deletedClasses.contains(className)) {
                continue;
            }
            if (!entry.javaFileFullPath.endsWith(".java")) {
                continue; // classes from (updated) .jars have been dealt with separately
            }
            //DAB TODO understand this bit better.  It is needed to support -vpath, I'm just not sure why....
            if (entry.checkResult != PCDEntry.CV_NEWER_FOUND_NEARER &&
                    !updatedAndCheckedClasses.contains(className) &&
                    !deletedClasses.contains(className) &&
                    entry.javaFileFullPath.endsWith(".java") &&
                    classFileUpdated(entry))
            {
            //DAB TODO this is the old way....
            //DAB    if (classFileUpdated(entry)) {
                updatedClasses.add(className);
                allUpdatedClasses.add(className);
            }
        }
    }

    private boolean classFileUpdated(PCDEntry entry) {
        File classFile = Utils.checkFileForName(entry.classFileFullPath);
        if (classFile == null) {
            return false;
        }
        // The only case when the above can happen is if class file was first deleted, and then there
        // was an error recompiling its source

        long classFileLastMod = classFile.lastModified();

        if (classFileLastMod > entry.oldClassFileLastModified) {
            entry.newClassFileLastModified = classFileLastMod;
            // Check if the class was actually modified, to avoid the costly procedure of detailed version compare
            long classFileFP = computeFP(classFile);
            if (classFileFP != entry.oldClassFileFingerprint) {
                entry.newClassFileFingerprint = classFileFP;
                return true;
            }
        }
        return false;
    }

    /**
     * Compare old (preserved in pdb) and new (file system) versions of updated classes, and find all
     * potentially affected dependent classes.
     */
    private void checkUpdatedClasses() {
        for (String className : updatedClasses) {
            PCDEntry pcde = pcd.get(className);
            getClassInfoForPCDEntry(ClassInfo.VER_NEW, pcde);
            if (!"".equals(pcde.oldClassInfo.directlyEnclosingClass)) {
                // The following problem can occur with nested classes. A C.java source has been changed, so that C.class is
                // not changed or changed in a compatible way, whereas the access modifiers of C$X.class are changed in an
                // incompatible way, so that something is broken in the project. When jmake is called for the first time,
                // it reports the problem, then saves the info on the new version of C in the pdb. Of course, the record for
                // C$X in the pdb is not updated, since the change to it is incompatible and recompilation of dependent sources
                // has failed. Suppose we don't change anything and invoke jmake again. C$X is found different from its old
                // version and is checked here again. The outcome should be the same. But since C has not changed, C.class is
                // not read from disk and the access flags of C$X, which are stored in C.class, are not set appropriately. So
                // in such circumstances we have wrong access flags for C$X here. To fix the problem we need to load C explicitly.
                ClassInfo enclosingClassInfo =
                        getClassInfoForName(ClassInfo.VER_NEW, pcde.oldClassInfo.directlyEnclosingClass);
                //if (enclosingClassInfo == null || enclosingClassInfo.nestedClasses == null) {
                //  System.out.println("!!! Suspicious updated class name = " + className);
                //  System.out.println("!!! enclosingClassInfo for it = " + enclosingClassInfo);
                //  if (enclosingClassInfo != null) {
                //    System.out.println("!!! enclosingClassInfo.name = " + enclosingClassInfo.name);
                //    if (enclosingClassInfo.nestedClasses == null) System.out.println("!!! enclosingClassInfo.nestedClasses = null");
                //  }
                //}
                if (enclosingClassInfo.nestedClasses != null) {  // Can be that this nested class was the only one for enclosing class, and it's deleted now
                    for (int i = 0; i < enclosingClassInfo.nestedClasses.length; i++) {
                        if (className.equals(enclosingClassInfo.nestedClasses[i])) {
                            pcde.newClassInfo.accessFlags =
                                    enclosingClassInfo.nestedClassAccessFlags[i];
                            pcde.newClassInfo.isNonMemberNestedClass =
                                    enclosingClassInfo.nestedClassNonMember[i];
                            break;
                        }
                    }
                }
            }
            if (!(pcde.oldClassInfo.isNonMemberNestedClass && pcde.newClassInfo.isNonMemberNestedClass)) {
                Utils.printInfoMessage("Checking " + pcde.className);
                pcde.checkResult = cv.compareClassVersions(pcde) ? PCDEntry.CV_COMPATIBLE
                        : PCDEntry.CV_INCOMPATIBLE;
                String affectedClasses[] = cv.getAffectedClasses();
                if (affectedClasses != null) {
                    for (int i = 0; i < affectedClasses.length; i++) {
                        PCDEntry affEntry = pcd.get(affectedClasses[i]);
                        updatedJavaFiles.add(affEntry.javaFileFullPath);
                    }
                }
            } else {
                // A non-member nested class can not be referenced by the source code of any class defined outside the
                // immediately enclosing source code block for this class. Therefore, any incompatibility in the new
                // version of this class can affect only classes that are defined in the same source file - and they
                // are necessarily recompiled together with this class. So there is no point in initiating version
                // compare for this class. However, the new class version should always tembe promoted into the store, since
                // this class itself may depend on other changing classes.
                pcde.checkResult = PCDEntry.CV_COMPATIBLE;
            }

            updatedAndCheckedClasses.add(className);
        }
    }

    /** Find all dependent classes for deleted classes. */
    private void checkDeletedClasses() {
        for (String className : deletedClasses) {
            PCDEntry pcde = pcd.get(className);

            if (pcde == null) {  // "Safety net" for the (hopefully unlikely) case. I observed it just once and couldn't identify the reason
                Utils.printWarningMessage("Warning: internal information inconsistency when checking deleted classes");
                Utils.printWarningMessage(Utils.REPORT_PROBLEM);
                Utils.printWarningMessage("Class name: " + className);
                continue;
            }

            ClassInfo oldCI = pcde.oldClassInfo;
            if (!oldCI.isNonMemberNestedClass) { // See the comment above
                Utils.printInfoMessage("Checking deleted class " + oldCI.name);
                cv.checkDeletedClass(pcde);
                String[] affectedClasses = cv.getAffectedClasses();
                if (affectedClasses != null) {
                    for (int i = 0; i < affectedClasses.length; i++) {
                        PCDEntry affEntry = pcd.get(affectedClasses[i]);
                        if (deletedClasses.contains(affEntry.className)) {
                            continue;
                        }
                        updatedJavaFiles.add(affEntry.javaFileFullPath);
                    }
                }
            }
            pcde.checkResult = PCDEntry.CV_DELETED;
            updatedAndCheckedClasses.add(className);
        }
        deletedClasses.clear();
    }

    /**
     * Determine what classes in the given .jar (which may be an existing updated one, or a new one) are new,
     * updated, or moved, and treat them accordingly.
     */
    private void processAllClassesFromJarFile(String jarFileName) {
        JarFile jarFile;
        long jarFileLastMod = 0;
        try {
            File file = new File(jarFileName);
            jarFileLastMod = file.lastModified();
            jarFile = new JarFile(jarFileName);
        } catch (IOException ex) {
            throw new PrivateException(ex);
        }

        List<PCDEntry> newEntries = new ArrayList<PCDEntry>();
        List<PCDEntry> updatedEntries = new ArrayList<PCDEntry>();
        List<PCDEntry> movedEntries = new ArrayList<PCDEntry>();

        for (Enumeration<JarEntry> entries = jarFile.entries(); entries.hasMoreElements();) {
            JarEntry jarEntry = entries.nextElement();
            String fullClassName = jarEntry.getName();
            if (!fullClassName.endsWith(".class")) {
                continue;
            }
            fullClassName =
                    fullClassName.substring(0, fullClassName.length() - 6).intern();
            byte classFileBytes[];
            classFileBytes = Utils.readZipEntryIntoBuffer(jarFile, jarEntry);
            long classFileFP = computeFP(classFileBytes);

            PCDEntry pcde = pcd.get(fullClassName);
            if (pcde != null) {
                if (pcde.checked) {
                    throw new PrivateException(new PublicExceptions.DoubleEntryException(
                            "Two entries for class " + fullClassName + " detected: " + pcde.javaFileFullPath + " and " + jarFileName));
                }
                pcde.checked = true;
                pcde.newClassFileLastModified = jarFileLastMod;
                // If we are scanning an existing updated .jar file, and there is no change to the class itself,
                // and it previously was located in the same .jar, do nothing.
                if (pcde.oldClassFileFingerprint == classFileFP &&
                        pcde.javaFileFullPath.equals(jarFileName)) {
                    pcde.oldClassFileLastModified = jarFileLastMod;   // So that next time jmake is inoked, checking
                    continue;                                         // of this.jar is not triggered.
                }
                if (pcde.oldClassFileFingerprint != classFileFP) {  // This class has been updated
                    updatedClasses.add(fullClassName);
                    allUpdatedClasses.add(fullClassName);
                    pcde.newClassFileLastModified = jarFileLastMod;
                    pcde.newClassFileFingerprint = classFileFP;
                    pcde.newClassInfo =
                            new ClassInfo(classFileBytes, ClassInfo.VER_NEW, this, fullClassName);
                    if (pcde.oldClassInfo.nestedClasses != null || pcde.newClassInfo.nestedClasses != null) {
                        updatedEntries.add(pcde);
                    }
                } else {
                    pcde.oldClassFileLastModified = jarFileLastMod;
                }
                if (!pcde.javaFileFullPath.equals(jarFileName)) {
                    // Found an existing class in a different .jar file.
                    // May happen if the class file has been moved from one .jar to another (or into a .jar, losing its
                    // .java source). It's only at this point that we can actually see that it was really a move.
                    if (deletedClasses.contains(fullClassName)) {
                        deletedClasses.remove(fullClassName);
                    }
                    if (pcde.oldClassInfo.nestedClasses != null) {
                        movedEntries.add(pcde);
                        pcde.newClassInfo =
                                new ClassInfo(classFileBytes, ClassInfo.VER_NEW, this, fullClassName);
                    }
                }
                pcde.javaFileFullPath = jarFileName;
            } else {  // New class file
                ClassInfo classInfo =
                        new ClassInfo(classFileBytes, ClassInfo.VER_NEW, this, fullClassName);
                pcde = new PCDEntry(fullClassName,
                        jarFileName,
                        jarFileName, jarFileLastMod, classFileFP,
                        classInfo);
                pcde.checkResult = PCDEntry.CV_NEW;          // So that later it's promoted into oldClassInfo correctly
                updatedAndCheckedClasses.add(fullClassName); // So that the above happens
                pcd.put(fullClassName, pcde);
                if (pcde.newClassInfo.nestedClasses != null) {
                    newEntries.add(pcde);
                }
            }
        }

        dealWithNestedClassesForUpdatedPCDEntries(updatedEntries, false);
        dealWithNestedClassesForUpdatedPCDEntries(movedEntries, true);
        for (int i = 0; i < newEntries.size(); i++) {
            findAndUpdateAllNestedClassesForClass(newEntries.get(i), false);
        }
    }

    /** Determine new, deleted and updated classes coming from updated .jar files. */
    private void dealWithClassesInUpdatedJarFiles() {
        if (updatedJarFiles.size() == 0) {
            return;
        }

        for (String updatedJarFile : updatedJarFiles) {
            processAllClassesFromJarFile(updatedJarFile);
        }

        // Now scan the PCD to check which classes that come from updated .jar files have not been marked as checked
        for (PCDEntry pcde : entries()) {
            if (updatedJarFiles.contains(pcde.javaFileFullPath)) {
                if (!pcde.checked) {
                    deletedClasses.add(pcde.className);
                }
            }
        }
    }

    /** Check if the destination directory exists, and get the canonical path for it. */
    private void initializeDestDir(String inDestDir) {
        if (!(inDestDir == null || inDestDir.equals(""))) {
            File dir = Utils.checkOrCreateDirForName(inDestDir);
            if (dir == null) {
                throw new PrivateException(new IOException("specified directory " + inDestDir + " cannot be created."));
            }
            inDestDir = getCanonicalPath(dir);
            if (!inDestDir.endsWith(File.separator)) {
                inDestDir += File.separatorChar;
            }
            destDir = inDestDir;
            destDirSpecified = true;
        } else {
            destDirSpecified = false;
        }
    }

    /**
     * For the given PCDEntry, set the entry.classFileFullPath according to the value of the .java file full
     * path and the value of the "-d" option at this particular jmake invocation
     */
    private void initializeClassFileFullPath(PCDEntry entry) {
        String classFileFullPath;
        if (destDirSpecified) {
            classFileFullPath = destDir + entry.className + ".class";
        } else {
            String javaFileDir = entry.javaFileFullPath;
            int cutIndex = javaFileDir.lastIndexOf(File.separatorChar);
            if (cutIndex != -1) {
                javaFileDir = javaFileDir.substring(0, cutIndex + 1);
            }
            String classFileName = entry.className;
            cutIndex = classFileName.lastIndexOf('/');
            if (cutIndex != -1) {
                classFileName = classFileName.substring(cutIndex + 1);
            }
            classFileFullPath = javaFileDir + classFileName + ".class";
        }
        if (backSlashFileSeparator) {
            classFileFullPath =
                    classFileFullPath.replace('/', File.separatorChar);
        }
        entry.classFileFullPath = classFileFullPath;
    }

    private static String getCanonicalPath(File file) {
        try {
            return file.getCanonicalPath().intern();
        } catch (IOException e) {
            throw new PrivateException(e);
        }
    }

    private long computeFP(File file) {
        byte buf[] = Utils.readFileIntoBuffer(file);
        return computeFP(buf);
    }

    private long computeFP(byte[] buf) {
        checkSum.reset();
        checkSum.update(buf);
        return checkSum.getValue();
    }

    private PrivateException compilerInteractionException(String message, Exception origException, int errCode) {
        return new PrivateException(new PublicExceptions.CompilerInteractionException(message, origException, errCode));
    }

    private PrivateException internalException(String message) {
        return new PrivateException(new PublicExceptions.InternalException(message));
    }
}
