/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintStream;
import java.io.Reader;
import java.io.StreamTokenizer;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

/**
 * The main class of the <b>jmake</b> tool.<p>
 *
 * Has several entrypoints: <code>main</code>, <code>mainExternal</code>, <code>mainProgrammatic</code>,
 * <code>mainExternalControlled</code> and <code>mainProgrammaticControlled</code>.
 * The first is not intended to be used by applications other than <b>jmake</b> itself, whereas the
 * rest can be used to call <b>jmake</b> externally with various degrees of control over its behaviour.
 * See method comments for more details.
 *
 * @author Misha Dmitriev
 *  12 October 2004
 */
public class Main {

    static final String DEFAULT_STORE_NAME = "jmake.pdb";
    static final String VERSION = "1.3.8-11";
    private String pdbFileName = null;
    private List<String> allProjectJavaFileNamesList =
            new ArrayList<String>(100);
    private String allProjectJavaFileNames[];
    private String addedJavaFileNames[],  removedJavaFileNames[],  updatedJavaFileNames[];
    private String destDir = "";
    private List<String> javacAddArgs = new ArrayList<String>();
    private String jcPath,  jcMainClass,  jcMethod;
    private String jcExecApp;
    boolean controlledExecution = false;
    Object externalApp = null;
    Method externalCompileSourceFilesMethod = null;
    private boolean failOnDependentJar = false,  noWarnOnDependentJar = false;
    private boolean pdbTextFormat = false;
    private PCDManager pcdm = null;
    private String dependencyFile;
    private static final String optNames[] = {"-h", "-help", "-d", "-pdb", "-C", "-jcpath", "-jcmainclass", "-jcmethod", "-jcexec", "-Xtiming", "-version",
        "-warnlimit", "-failondependentjar", "-nowarnondependentjar", "-classpath", "-projclasspath", "-bootclasspath", "-extdirs", "-vpath", "-pdb-text-format",
        "-depfile"};
    private static final int optArgs[] = {0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1};
    private static final int OPT_H = 0;
    private static final int OPT_HELP = 1;
    private static final int OPT_D = 2;
    private static final int OPT_STORE = 3;
    private static final int OPT_JAVAC_OPT = 4;
    private static final int OPT_JCPATH = 5;
    private static final int OPT_JCMAINCLASS = 6;
    private static final int OPT_JCMETHOD = 7;
    private static final int OPT_JCEXEC = 8;
    private static final int OPT_TIMING = 9;
    private static final int OPT_VERSION = 10;
    private static final int OPT_WARNLIMIT = 11;
    private static final int OPT_FAILONDEPJAR = 12;
    private static final int OPT_NOWARNONDEPJAR = 13;
    private static final int OPT_CLASSPATH = 14;
    private static final int OPT_PROJECTCLASSPATH = 15;
    private static final int OPT_BOOTCLASSPATH = 16;
    private static final int OPT_EXTDIRS = 17;
    private static final int OPT_VPATH = 18;
    private static final int OPT_PDB_TEXT_FORMAT = 19;
    private static final int OPT_DEPENDENCY_FILE = 20;

    /** Construct a new instance of Main. */
    public Main() {
    }

    /**
     * Checks whether the argument is a legal jmake option. Returns its number or
     * -1 if not found.
     */
    private static int inOptions(String option) {
        if (option.startsWith(optNames[OPT_JAVAC_OPT])) {
            return OPT_JAVAC_OPT;
        }
        for (int i = 0; i < optNames.length; i++) {
            if (option.equals(optNames[i])) {
                return i;
            }
        }
        return -1;
    }

    private void processCommandLine(String args[]) {
        if ((args.length == 0) || (args.length == 1 && args[0].equals(optNames[OPT_HELP]))) {
            printUsage();
            throw new PrivateException(new PublicExceptions.NoActionRequestedException());
        }

        List<String> argsV = new ArrayList<String>();
        for (int i = 0; i < args.length; i++) {
            argsV.add(args[i]);
        }
        int argsSt = 0;
        String arg;

        while (argsSt < argsV.size()) {
            arg = argsV.get(argsSt++);
            if (arg.charAt(0) == '-') {
                int argN = inOptions(arg);
                if (argN != -1) {
                    if (argsSt + optArgs[argN] > argsV.size()) {
                        optRequiresArg("\"" + optNames[argN] + "\"");
                    }
                } else {
                    bailOut(arg + ERR_IS_INVALID_OPTION);
                }

                switch (argN) {
                    case OPT_H:
                    case OPT_HELP:
                        printUsage();
                        throw new PrivateException(new PublicExceptions.NoActionRequestedException());
                    case OPT_D:
                        destDir = argsV.get(argsSt);
                        javacAddArgs.add("-d");
                        javacAddArgs.add(argsV.get(argsSt));
                        argsSt++;
                        break;
                    case OPT_CLASSPATH:
                        try {
                            setClassPath(argsV.get(argsSt++));
                        } catch (PublicExceptions.InvalidCmdOptionException ex) {
                            bailOut(ex.getMessage());
                        }
                        break;
                    case OPT_PROJECTCLASSPATH:
                        try {
                            setProjectClassPath(argsV.get(argsSt++));
                        } catch (PublicExceptions.InvalidCmdOptionException ex) {
                            bailOut(ex.getMessage());
                        }
                        break;
                    case OPT_STORE:
                        pdbFileName = argsV.get(argsSt++);
                        break;
                    case OPT_JAVAC_OPT:
                        String javacArg =
                                (argsV.get(argsSt - 1)).substring(2);
                        if (javacArg.equals("-d") ||
                                javacArg.equals("-classpath") ||
                                javacArg.equals("-bootclasspath") ||
                                javacArg.equals("-extdirs")) {
                            bailOut(javacArg + ERR_SHOULD_BE_EXPLICIT);
                        }
                        javacAddArgs.add(javacArg);
                        break;
                    case OPT_JCPATH:
                        if (jcExecApp != null) {
                            bailOut(ERR_NO_TWO_COMPILER_OPTIONS);
                        }
                        jcPath = argsV.get(argsSt++);
                        break;
                    case OPT_JCMAINCLASS:
                        if (jcExecApp != null) {
                            bailOut(ERR_NO_TWO_COMPILER_OPTIONS);
                        }
                        jcMainClass = argsV.get(argsSt++);
                        break;
                    case OPT_JCMETHOD:
                        if (jcExecApp != null) {
                            bailOut(ERR_NO_TWO_COMPILER_OPTIONS);
                        }
                        jcMethod = argsV.get(argsSt++);
                        break;
                    case OPT_JCEXEC:
                        if (jcPath != null || jcMainClass != null || jcMethod != null) {
                            bailOut(ERR_NO_TWO_COMPILER_OPTIONS);
                        }
                        jcExecApp = argsV.get(argsSt++);
                        break;
                    case OPT_TIMING:
                        Utils.setTimingOn();
                        break;
                    case OPT_VERSION:
                        // Utils.printInfoMessage("jmake version " + VERSION); // Printed anyway at present...
                        throw new PrivateException(new PublicExceptions.NoActionRequestedException());
                    case OPT_WARNLIMIT:
                        try {
                            Utils.warningLimit =
                                    Integer.parseInt(argsV.get(argsSt++));
                        } catch (NumberFormatException e) {
                            Utils.ignore(e);
                            bailOut(argsV.get(argsSt) + ERR_IS_INVALID_OPTION);
                        }
                        break;
                    case OPT_FAILONDEPJAR:
                        if (noWarnOnDependentJar) {
                            bailOut("it is not allowed to use -nowarnondependentjar and -failondependentjar together");
                        }
                        failOnDependentJar = true;
                        break;
                    case OPT_NOWARNONDEPJAR:
                        if (failOnDependentJar) {
                            bailOut("it is not allowed to use -failondependentjar and -nowarnondependentjar together");
                        }
                        noWarnOnDependentJar = true;
                        break;
                    case OPT_BOOTCLASSPATH:
                        try {
                            setBootClassPath(argsV.get(argsSt++));
                        } catch (PublicExceptions.InvalidCmdOptionException ex) {
                            bailOut(ex.getMessage());
                        }
                        break;
                    case OPT_EXTDIRS:
                        try {
                            setExtDirs(argsV.get(argsSt++));
                        } catch (PublicExceptions.InvalidCmdOptionException ex) {
                            bailOut(ex.getMessage());
                        }
                        break;
                    case OPT_VPATH:
                        try {
                            setVirtualPath(argsV.get(argsSt++));
                        } catch (PublicExceptions.InvalidCmdOptionException ex) {
                            bailOut(ex.getMessage());
                        }
                        break;
                    case OPT_PDB_TEXT_FORMAT:
                        pdbTextFormat = true;
                        break;
                    case OPT_DEPENDENCY_FILE:
                        dependencyFile = argsV.get(argsSt++);
                        break;
                    default:
                        bailOut(arg + ERR_IS_INVALID_OPTION);
                }
            } else {    // Not an option, at least does not start with "-". Treat it as a command line file name or source name.
                if (arg.length() > 1 && arg.charAt(0) == '@') {
                    arg = arg.substring(1);
                    loadCmdFile(arg, argsV);
                } else {
                    if (!arg.endsWith(".java")) {
                        bailOut(arg + ERR_IS_INVALID_OPTION);
                    }
                    allProjectJavaFileNamesList.add(arg);
                }
            }
        }
    }

    /** Load @-file that can contain anything that command line can contain. */
    private void loadCmdFile(String name, List<String> argsV) {
        try {
            Reader r = new BufferedReader(new FileReader(name));
            StreamTokenizer st = new StreamTokenizer(r);
            st.resetSyntax();
            st.wordChars(' ', 255);
            st.whitespaceChars(0, ' ');
            st.commentChar('#');
            st.quoteChar('"');
            st.quoteChar('\'');
            while (st.nextToken() != StreamTokenizer.TT_EOF) {
                argsV.add(st.sval);
            }
            r.close();
        } catch (IOException e) {
            throw new PrivateException(new PublicExceptions.CommandFileReadException(name + ":\n" + e.getMessage()));
        }
    }

    /**
     * Main entrypoint for applications that want to call <b>jmake</b> externally and are willing
     * to handle exceptions that it may throw.
     *
     * @param  args  command line arguments passed to <b>jmake</b>.
     *
     * @throws PublicExceptions.NoActionRequestedException  if <b>jmake</b> was not requested to do any real work;
     * @throws PublicExceptions.InvalidCmdOptionException   if invalid command line option was detected;
     * @throws PublicExceptions.PDBCorruptedException       if project database is corrupted;
     * @throws PublicExceptions.CommandFileReadException    if there was error reading a command file;
     * @throws PublicExceptions.CompilerInteractionException if there was a problem initializing or calling the compiler,
     *                                                       or compilation errors were detected;
     * @throws PublicExceptions.ClassFileParseException     if there was error parsing a class file;
     * @throws PublicExceptions.ClassNameMismatchException  if there is a mismatch between the deduced and the actual class name;
     * @throws PublicExceptions.InvalidSourceFileExtensionException if a supplied source file has an invalid extension (not .java);
     * @throws PublicExceptions.JarDependsOnSourceException if a class in a <code>JAR</code> is found dependent on a class with the .java source;
     * @throws PublicExceptions.DoubleEntryException        if more than one entry for the same class is found in the project
     * @throws FileNotFoundException                        if a <code>.java</code> or a <code>.class</code> file was not found;
     * @throws IOException                                  if there was an I/O problem of any kind;
     * @throws PublicExceptions.InternalException           if an internal problem that should never happen was detected.
     */
    public void mainProgrammatic(String args[]) throws
            PublicExceptions.NoActionRequestedException,
            PublicExceptions.InvalidCmdOptionException,
            PublicExceptions.PDBCorruptedException,
            PublicExceptions.CommandFileReadException,
            PublicExceptions.CompilerInteractionException,
            PublicExceptions.ClassFileParseException,
            PublicExceptions.ClassNameMismatchException,
            PublicExceptions.InvalidSourceFileExtensionException,
            PublicExceptions.JarDependsOnSourceException,
            PublicExceptions.DoubleEntryException,
            FileNotFoundException,
            IOException,
            PublicExceptions.InternalException {
        try {
            Utils.printInfoMessage("Jmake version " + VERSION);
            if (!controlledExecution) {
                processCommandLine(args);
                String[] projectJars = ClassPath.getProjectJars();
                if (projectJars != null) {
                    for (int i = 0; i < projectJars.length; i++) {
                        allProjectJavaFileNamesList.add(projectJars[i]);
                    }
                }
                allProjectJavaFileNames =
                        allProjectJavaFileNamesList.toArray(new String[allProjectJavaFileNamesList.size()]);
            } else {
                String[] projectJars = ClassPath.getProjectJars();
                if (projectJars != null) {
                    String newNames[] =
                            new String[allProjectJavaFileNames.length + projectJars.length];
                    System.arraycopy(allProjectJavaFileNames, 0, newNames, 0, allProjectJavaFileNames.length);
                    System.arraycopy(projectJars, 0, newNames, allProjectJavaFileNames.length, projectJars.length);
                    allProjectJavaFileNames = newNames;
                }
            }

            Utils.startTiming(Utils.TIMING_PDBREAD);
            PCDContainer pcdc;
            pcdc = PCDContainer.load(pdbFileName, pdbTextFormat);
            Utils.stopAndPrintTiming("DB read", Utils.TIMING_PDBREAD);

            pcdm = new PCDManager(pcdc, allProjectJavaFileNames,
                addedJavaFileNames, removedJavaFileNames, updatedJavaFileNames,
                destDir, javacAddArgs, failOnDependentJar, noWarnOnDependentJar,
                dependencyFile);

            pcdm.initializeCompiler(jcExecApp, jcPath, jcMainClass, jcMethod, externalApp, externalCompileSourceFilesMethod);

            pcdm.run();
        } catch (PrivateException e) {
            Throwable origException = e.getOriginalException();
            if (origException instanceof PublicExceptions.NoActionRequestedException) {
                throw (PublicExceptions.NoActionRequestedException) origException;
            } else if (origException instanceof PublicExceptions.InvalidCmdOptionException) {
                throw (PublicExceptions.InvalidCmdOptionException) origException;
            } else if (origException instanceof PublicExceptions.CommandFileReadException) {
                throw (PublicExceptions.CommandFileReadException) origException;
            } else if (origException instanceof PublicExceptions.PDBCorruptedException) {
                throw (PublicExceptions.PDBCorruptedException) origException;
            } else if (origException instanceof PublicExceptions.CompilerInteractionException) {
                throw (PublicExceptions.CompilerInteractionException) origException;
            } else if (origException instanceof PublicExceptions.ClassFileParseException) {
                throw (PublicExceptions.ClassFileParseException) origException;
            } else if (origException instanceof PublicExceptions.ClassNameMismatchException) {
                throw (PublicExceptions.ClassNameMismatchException) origException;
            } else if (origException instanceof PublicExceptions.InvalidSourceFileExtensionException) {
                throw (PublicExceptions.InvalidSourceFileExtensionException) origException;
            } else if (origException instanceof PublicExceptions.JarDependsOnSourceException) {
                throw (PublicExceptions.JarDependsOnSourceException) origException;
            } else if (origException instanceof PublicExceptions.DoubleEntryException) {
                throw (PublicExceptions.DoubleEntryException) origException;
            } else if (origException instanceof FileNotFoundException) {
                throw (FileNotFoundException) origException;
            } else if (origException instanceof IOException) {
                throw (IOException) origException;
            } else if (origException instanceof PublicExceptions.InternalException) {
                throw (PublicExceptions.InternalException) origException;
            }
        } finally {
            ClassPath.resetOnFinish();
        }
    }

    /**
     * Main entrypoint for applications that want to call <b>jmake</b> externally and would prefer
     * receiving an error code instead of an exception in case something goes wrong.<p>
     *
     * @param  args  command line arguments passed to <b>jmake</b>.
     *
     * @return <dl>
     *  <dt><code>  0</code>  if everything was successful;
     *  <dt><code> -1</code>  invalid command line option detected;
     *  <dt><code> -2</code>  error reading command file;
     *  <dt><code> -3</code>  project database corrupted;
     *  <dt><code> -4</code>  error initializing or calling the compiler;
     *  <dt><code> -5</code>  compilation error;
     *  <dt><code> -6</code>  error parsing a class file;
     *  <dt><code> -7</code>  file not found;
     *  <dt><code> -8</code>  I/O exception;
     *  <dt><code> -9</code>  internal jmake exception;
     *  <dt><code>-10</code>  deduced and actual class name mismatch;
     *  <dt><code>-11</code>  invalid source file extension;
     *  <dt><code>-12</code>  a class in a <code>JAR</code> is found dependent on a class with the .java source;
     *  <dt><code>-13</code>  more than one entry for the same class is found in the project
     *  <dt><code>-20</code> internal Java error (caused by <code>java.lang.InternalError</code>);
     *  <dt><code>-30</code> internal Java error (caused by <code>java.lang.RuntimeException</code>).
     *  </dl>
     */
    public int mainExternal(String args[]) {
        try {
            mainProgrammatic(args);
        } catch (PublicExceptions.NoActionRequestedException e0) {
            // Nothing to do
        } catch (PublicExceptions.InvalidCmdOptionException e1) {
            Utils.printErrorMessage(e1.getMessage());
            return -1;
        } catch (PublicExceptions.CommandFileReadException e2) {
            Utils.printErrorMessage("error parsing command file:");
            Utils.printErrorMessage(e2.getMessage());
            return -2;
        } catch (PublicExceptions.PDBCorruptedException e3) {
            Utils.printErrorMessage("project database corrupted: " + e3.getMessage());
            return -3;
        } catch (PublicExceptions.CompilerInteractionException e4) {
            if (e4.getOriginalException() != null) {
                Utils.printErrorMessage("error interacting with the compiler: ");
                Utils.printErrorMessage(e4.getMessage());
                Utils.printErrorMessage("original exception:");
                Utils.printErrorMessage(e4.getOriginalException().getMessage());
                return -4;
            } else { // Otherwise there is a compilation error, and the compiler has already printed a lot...
                return -5;
            }
        } catch (PublicExceptions.ClassFileParseException e6) {
            Utils.printErrorMessage(e6.getMessage());
            return -6;
        } catch (FileNotFoundException e7) {
            Utils.printErrorMessage(e7.getMessage());
            return -7;
        } catch (IOException e8) {
            Utils.printErrorMessage(e8.getMessage());
            return -8;
        } catch (PublicExceptions.InternalException e9) {
            Utils.printErrorMessage("internal jmake exception detected:");
            Utils.printErrorMessage(e9.getMessage());
            Utils.printErrorMessage(Utils.REPORT_PROBLEM);
            Utils.printErrorMessage("the stack trace is as follows:");
            e9.printStackTrace();
            return -9;
        } catch (PublicExceptions.ClassNameMismatchException e10) {
            Utils.printErrorMessage(e10.getMessage());
            return -10;
        } catch (PublicExceptions.InvalidSourceFileExtensionException e11) {
            Utils.printErrorMessage(e11.getMessage());
            return -11;
        } catch (PublicExceptions.JarDependsOnSourceException e12) {
            Utils.printErrorMessage(e12.getMessage());
            return -12;
        } catch (PublicExceptions.DoubleEntryException e13) {
            Utils.printErrorMessage(e13.getMessage());
            return -13;
        } catch (InternalError e20) {
            Utils.printErrorMessage("internal Java error: " + e20);
            Utils.printErrorMessage("Consult the following stack trace for more info:");
            e20.printStackTrace();
            return -20;
        } catch (RuntimeException e30) {
            Utils.printErrorMessage("internal Java exception: " + e30);
            Utils.printErrorMessage("Consult the following stack trace for more info:");
            e30.printStackTrace();
            return -30;
        }

        return 0;
    }

    /**
     * Main entrypoint for applications such as Ant, that want to have full control over
     * compilations that <b>jmake</b> invokes, and are willing to handle exceptions
     * that it may throw.
     *
     * @param javaFileNames    array of strings that specify <code>.java</code> file names.
     * @param destDirName      name of the destination directory (<b>jmake</b> will look up binary classes
     *                         in there, it should be the same as the one used by the Java compiler method).
     *                         If <code>null</code> is passed, classes will be looked up in the same directories
     *                         as their sources, in agreement with the default Java compiler behaviour.
     * @param pdbFileName      project database file name (if <code>null</code> is passed,
     *                         a file with the default name placed in the current directory will be used).
     * @param externalApp      an object on which to invoke <code>externalCompileSourceFilesMethod</code> method.
     * @param externalCompileSourceFilesMethod    a method of the form <code>int x(String[] args)</code>. It
     *                         should return <code>0</code> if compilation is successful and any non-zero value
     *                         otherwise. <b>jmake</b> passes it a list of the <code>.java</code> files to
     *                         recompile in the form of canonical full path file names.
     *
     * @throws PublicExceptions.NoActionRequestedException  if <b>jmake</b> was not requested to do any real work;
     * @throws PublicExceptions.InvalidCmdOptionException   if invalid command line option was detected;
     * @throws PublicExceptions.PDBCorruptedException       if project database is corrupted;
     * @throws PublicExceptions.CommandFileReadException    if there was error reading a command file;
     * @throws PublicExceptions.CompilerInteractionException if there was a problem initializing or calling the compiler,
     *                                                       or compilation errors were detected;
     * @throws PublicExceptions.ClassFileParseException     if there was error parsing a class file;
     * @throws PublicExceptions.ClassNameMismatchException  if there is a mismatch between the deduced and the actual class name;
     * @throws PublicExceptions.InvalidSourceFileExtensionException if a specified source file has an invalid extension (not .java);
     * @throws PublicExceptions.JarDependsOnSourceException if a class in a <code>JAR</code> is found dependent on a class with the .java source;
     * @throws PublicExceptions.DoubleEntryException        if more than one entry for the same class is found in the project
     * @throws PublicExceptions.InternalException           if an internal problem that should never happen was detected.
     * @throws FileNotFoundException                        if a <code>.java</code> or a <code>.class</code> file was not found;
     * @throws IOException                                  if there was an I/O problem of any kind;
     */
    public void mainProgrammaticControlled(String javaFileNames[], String destDirName, String pdbFileName,
            Object externalApp, Method externalCompileSourceFilesMethod) throws
            PublicExceptions.NoActionRequestedException,
            PublicExceptions.InvalidCmdOptionException,
            PublicExceptions.PDBCorruptedException,
            PublicExceptions.CommandFileReadException,
            PublicExceptions.CompilerInteractionException,
            PublicExceptions.ClassFileParseException,
            PublicExceptions.ClassNameMismatchException,
            PublicExceptions.InvalidSourceFileExtensionException,
            PublicExceptions.JarDependsOnSourceException,
            PublicExceptions.DoubleEntryException,
            PublicExceptions.InternalException,
            FileNotFoundException,
            IOException {

        controlledExecution = true;
        this.pdbFileName = pdbFileName;
        this.destDir = destDirName;
        this.allProjectJavaFileNames = javaFileNames;
        this.externalApp = externalApp;
        this.externalCompileSourceFilesMethod = externalCompileSourceFilesMethod;

        mainProgrammatic(null);
    }

    /**
     * Main entrypoint for applications such as Ant, that want to have full control over
     * compilations that <b>jmake</b> invokes, and do not want to handle exceptions that it
     * may throw. Error codes returned are the same as <code>mainExternal(String[])</code> returns.
     *
     * @param javaFileNames    array of strings that specify <code>.java</code> file names.
     * @param destDirName      name of the destination directory (<b>jmake</b> will look up binary classes
     *                         in there, it should be the same as the one used by the Java compiler method).
     *                         If <code>null</code> is passed, classes will be looked up in the same directories
     *                         as their sources, in agreement with the default Java compiler behaviour.
     * @param pdbFileName      project database file name (if <code>null</code> is passed,
     *                         a file with the default name placed in the current directory will be used).
     * @param externalApp      an object on which to invoke <code>externalCompileSourceFilesMethod</code> method.
     * @param externalCompileSourceFilesMethod    a method of the form <code>int x(String[] args)</code>. It
     *                         should return <code>0</code> if compilation is successful and any non-zero value
     *                         otherwise. <b>jmake</b> passes it a list of the <code>.java</code> files to
     *                         recompile in the form of canonical full path file names.
     *
     * @see #mainExternal(String[])
     */
    public int mainExternalControlled(String javaFileNames[], String destDirName, String pdbFileName,
            Object externalApp, Method externalCompileSourceFilesMethod) {
        controlledExecution = true;
        this.pdbFileName = pdbFileName;
        this.destDir = destDirName;
        this.allProjectJavaFileNames = javaFileNames;
        this.externalApp = externalApp;
        this.externalCompileSourceFilesMethod = externalCompileSourceFilesMethod;

        return mainExternal(null);
    }

    /**
     * Main entrypoint for applications such as IDEs, that themselves keep track of updated/added/removed sources,
     * want to have full control over compilations that <b>jmake</b> invokes, and are willing to handle exceptions
     * that it may throw.
     *
     * @param addedJavaFileNames   names of <code>.java</code> files just added to the project
     * @param removedJavaFileNames names of <code>.java</code> files just removed from the project
     * @param updatedJavaFileNames names of updated project <code>.java</code> files
     * @param destDirName      name of the destination directory (<b>jmake</b> will look up binary classes
     *                         in there, it should be the same as the one used by the Java compiler method).
     *                         If <code>null</code> is passed, classes will be looked up in the same directories
     *                         as their sources, in agreement with the default Java compiler behaviour.
     * @param pdbFileName      project database file name (if <code>null</code> is passed,
     *                         a file with the default name placed in the current directory will be used).
     * @param externalApp      an object on which to invoke <code>externalCompileSourceFilesMethod</code> method.
     * @param externalCompileSourceFilesMethod    a method of the form <code>int x(String[] args)</code>. It
     *                         should return <code>0</code> if compilation is successful and any non-zero value
     *                         otherwise. <b>jmake</b> passes it a list of the <code>.java</code> files to
     *                         recompile in the form of canonical full path file names.
     *
     * @throws PublicExceptions.NoActionRequestedException  if <b>jmake</b> was not requested to do any real work;
     * @throws PublicExceptions.InvalidCmdOptionException   if invalid command line option was detected;
     * @throws PublicExceptions.PDBCorruptedException       if project database is corrupted;
     * @throws PublicExceptions.CommandFileReadException    if there was error reading a command file;
     * @throws PublicExceptions.CompilerInteractionException if there was a problem initializing or calling the compiler,
     *                                                       or compilation errors were detected;
     * @throws PublicExceptions.ClassFileParseException     if there was error parsing a class file;
     * @throws PublicExceptions.ClassNameMismatchException  if there is a mismatch between the deduced and the actual class name;
     * @throws PublicExceptions.InvalidSourceFileExtensionException if a specified source file has an invalid extension (not .java);
     * @throws PublicExceptions.JarDependsOnSourceException if a class in a <code>JAR</code> is found dependent on a class with the .java source;
     * @throws PublicExceptions.DoubleEntryException        if more than one entry for the same class is found in the project
     * @throws PublicExceptions.InternalException           if an internal problem that should never happen was detected.
     * @throws FileNotFoundException                        if a <code>.java</code> or a <code>.class</code> file was not found;
     * @throws IOException                                  if there was an I/O problem of any kind;
     */
    public void mainProgrammaticControlled(String addedJavaFileNames[], String removedJavaFileNames[], String updatedJavaFileNames[],
            String destDirName, String pdbFileName,
            Object externalApp, Method externalCompileSourceFilesMethod) throws
            PublicExceptions.NoActionRequestedException,
            PublicExceptions.InvalidCmdOptionException,
            PublicExceptions.PDBCorruptedException,
            PublicExceptions.CommandFileReadException,
            PublicExceptions.CompilerInteractionException,
            PublicExceptions.ClassFileParseException,
            PublicExceptions.ClassNameMismatchException,
            PublicExceptions.InvalidSourceFileExtensionException,
            PublicExceptions.JarDependsOnSourceException,
            PublicExceptions.DoubleEntryException,
            PublicExceptions.InternalException,
            FileNotFoundException,
            IOException {

        controlledExecution = true;
        this.pdbFileName = pdbFileName;
        this.destDir = destDirName;
        this.addedJavaFileNames = addedJavaFileNames;
        this.removedJavaFileNames = removedJavaFileNames;
        this.updatedJavaFileNames = updatedJavaFileNames;
        this.externalApp = externalApp;
        this.externalCompileSourceFilesMethod = externalCompileSourceFilesMethod;

        mainProgrammatic(null);
    }

    /**
     * Main entrypoint for applications such as IDEs, that themselves keep track of updated/added/removed sources,
     * want to have full control over compilations that <b>jmake</b> invokes, and do not want to handle exceptions
     * that it may throw. Error codes returned are the same as <code>mainExternal(String[])</code> returns.
     *
     * @param addedJavaFileNames   names of <code>.java</code> files just added to the project
     * @param removedJavaFileNames names of <code>.java</code> files just removed from the project
     * @param updatedJavaFileNames names of updated project <code>.java</code> files
     * @param destDirName      name of the destination directory (<b>jmake</b> will look up binary classes
     *                         in there, it should be the same as the one used by the Java compiler method).
     *                         If <code>null</code> is passed, classes will be looked up in the same directories
     *                         as their sources, in agreement with the default Java compiler behaviour.
     * @param pdbFileName      project database file name (if <code>null</code> is passed,
     *                         a file with the default name placed in the current directory will be used).
     * @param externalApp      an object on which to invoke <code>externalCompileSourceFilesMethod</code> method.
     * @param externalCompileSourceFilesMethod    a method of the form <code>int x(String[] args)</code>. It
     *                         should return <code>0</code> if compilation is successful and any non-zero value
     *                         otherwise. <b>jmake</b> passes it a list of the <code>.java</code> files to
     *                         recompile in the form of canonical full path file names.
     *
     * @see #mainExternal(String[])
     */
    public int mainExternalControlled(String addedJavaFileNames[], String removedJavaFileNames[], String updatedJavaFileNames[],
            String destDirName, String pdbFileName,
            Object externalApp, Method externalCompileSourceFilesMethod) {
        controlledExecution = true;
        this.pdbFileName = pdbFileName;
        this.destDir = destDirName;
        this.addedJavaFileNames = addedJavaFileNames;
        this.removedJavaFileNames = removedJavaFileNames;
        this.updatedJavaFileNames = updatedJavaFileNames;
        this.externalApp = externalApp;
        this.externalCompileSourceFilesMethod = externalCompileSourceFilesMethod;

        return mainExternal(null);
    }

    /**
     * Main entrypoint for the standalone <b>jmake</b> application. This method calls does little but calling
     * <code>mainExternal</code>, and its execution always completes with <code>System.exit(code)</code>,
     * where <code>code</code> is the value returned by <code>mainExternal</code>.
     *
     * @see #mainExternal(String[])
     * @see #mainProgrammatic(String[])
     *
     * @param  args  command line arguments passed to <b>jmake</b>
     */
    public static void main(String args[]) {
        Utils.startTiming(Utils.TIMING_TOTAL);

        Main m = new Main();
        int exitCode = m.mainExternal(args);

        Utils.stopAndPrintTiming("Total", Utils.TIMING_TOTAL);
        if ( exitCode != 0 ) {
            System.exit(exitCode);
        }
    }

    /**
     * Customize the output of <b>jmake</b>.
     *
     * @see #setOutputStreams(PrintStream, PrintStream, PrintStream)
     *
     * @param printInfoMessages    specify whether to print information messages
     * @param printWarningMessages specify whether to print warning messages
     * @param printErrorMessages   specify whether to print error messages
     */
    public static void customizeOutput(boolean printInfoMessages,
            boolean printWarningMessages,
            boolean printErrorMessages) {
        Utils.customizeOutput(printInfoMessages, printWarningMessages, printErrorMessages);
    }

    /**
     * Set the class path to be used by the compiler, and also by the dependency checker for the purposes of
     * superclass/superinterface change tracking. For the compiler, this class path will be merged with the
     * project class path (set via setProjectClassPath(String)). Other than that, its value will be used only to
     * look up superclasses/superinterfaces of project classes. Note that non-project superclasses and
     * superinterfaces are first looked up at the boot class path, then on the extension class path, and then
     * on this class path.
     *
     * @see #setProjectClassPath(String)
     * @see #setBootClassPath(String)
     * @see #setExtDirs(String)
     *
     * @param classPath  the value of the class path, in the usual format (i.e. entries that are directories
     *                   or JARs, separated by colon or semicolon depending on the platform).
     *
     * @throws PublicExceptions.InvalidCmdOptionException   if invalid class path value is specified.
     */
    public static void setClassPath(String classPath) throws PublicExceptions.InvalidCmdOptionException {
        ClassPath.setClassPath(classPath);
    }

    /**
     * Set the class path to be used by the compiler, and also by the dependency checker for the purposes of
     * superclass/superinterface change tracking and sourceless class dependency checking. For the compiler,
     * and also in order to look up superclasses/superinterfaces of project classes, this class path will be
     * merged with the standard class path (set via setClassPath(String)). But in addition, all binary classes
     * that are on this class path are stored in the project database and checked for updates every time jmake
     * is invoked. Any changes to these classes trigger the standard dependency checking procedure. However,
     * dependent classes are looked up only among the "normal" project classes, i.e. those that have sources.
     * Therefore sourceless classes are assumed to always be mutually consistent.
     *
     * Currently only JAR files can be present on this class path.
     *
     * @see #setClassPath(String)
     *
     * @param projectClassPath  the value of the class path, in the usual format (i.e. entries that are directories
     *                          or JARs, separated by colon or semicolon depending on the platform).
     *
     * @throws PublicExceptions.InvalidCmdOptionException   if invalid class path value is specified.
     */
    public static void setProjectClassPath(String projectClassPath) throws PublicExceptions.InvalidCmdOptionException {
        ClassPath.setProjectClassPath(projectClassPath);
    }

    /**
     * Set the boot class path to be used by the compiler (-bootclasspath option) and also by the dependency
     * checker (by default, the value of "sun.boot.class.path" property is used).
     *
     * @see #setClassPath(String)
     *
     * @param classPath   the value of the boot class path, in the usual format (i.e. entries that are directories
     *                   or JARs, separated by colon or semicolon depending on the platform).
     *
     * @throws PublicExceptions.InvalidCmdOptionException   if invalid class path value is specified.
     */
    public static void setBootClassPath(String classPath) throws PublicExceptions.InvalidCmdOptionException {
        ClassPath.setBootClassPath(classPath);
    }

    /**
     * Set the extensions location to be used by the compiler (-extdirs option) and also by the dependency
     * checker (by default, the value of "java.ext.dirs" property is used).
     *
     * @see #setClassPath(String)
     *
     * @param dirs   the value of extension directories, in the usual format (one or more directory names
     *               separated by colon or semicolon depending on the platform).
     *
     * @throws PublicExceptions.InvalidCmdOptionException   if invalid class path value is specified.
     */
    public static void setExtDirs(String dirs) throws PublicExceptions.InvalidCmdOptionException {
        ClassPath.setExtDirs(dirs);
    }

    /**
     * Set the virtual path used to find both source and class files that are part of the project
     * but are not in the local directory.
     *
     * @see #setClassPath(String)
     *
     * @param dirs   the value of extension directories, in the usual format (one or more directory names
     *               separated by colon or semicolon depending on the platform).
     *
     * @throws PublicExceptions.InvalidCmdOptionException   if invalid path value is specified.
     */
    public static void setVirtualPath(String dirs) throws PublicExceptions.InvalidCmdOptionException {
        ClassPath.setVirtualPath(dirs);
    }

    /** Produce no warning or error message upon a dependent <code>JAR</code> detection. */
    public static final int DEPJAR_NOWARNORERROR = 0;
    /** Produce a warning upon a dependent <code>JAR</code> detection. */
    public static final int DEPJAR_WARNING = 1;
    /** Produce an error message (throw an exception) upon a dependent <code>JAR</code> detection. */
    public static final int DEPJAR_ERROR = 2;

    /**
     * Set the response of <b>jmake</b> in case a dependence of a class located in a <code>JAR</code> file on a
     * class with a <code>.java</code> source is detected (such dependencies are highly discouraged, since it is not
     * possible to recompile a class in the <code>JAR</code> that has no source).
     *
     * @param code  response type: DEPJAR_NOWARNORERROR, DEPJAR_WARNING (default behaviour) or DEPJAR_ERROR.
     */
    public void setResponseOnDependentJar(int code) {
        switch (code) {
            case DEPJAR_NOWARNORERROR:
                noWarnOnDependentJar = true;
                failOnDependentJar = false;
                break;
            case DEPJAR_WARNING:
                noWarnOnDependentJar = false;
                failOnDependentJar = false;
                break;
            case DEPJAR_ERROR:
                noWarnOnDependentJar = false;
                failOnDependentJar = true;
                break;
        }
    }

    /**
     * Return the names of all classes that <b>jmake</b>, on this invocation, found updated - either because
     * <b>jmake</b> itself recompiled them or because they were updated independently (their timestamp/checksum
     * found different from those contained in the project database).
     */
    public String[] getUpdatedClasses() {
        return pcdm.getAllUpdatedClassesAsStringArray();
    }

    /**
     * Set the output print streams to be used by <b>jmake</b>.
     *
     * @see #customizeOutput(boolean, boolean, boolean)
     *
     * @param out   print stream to be used for information ("logging") messages that <b>jmake</b> emits
     * @param warn  print stream to be used for warning messages
     * @param err   print stream to be used for error messages
     */
    public static void setOutputStreams(PrintStream out, PrintStream warn, PrintStream err) {
        Utils.setOutputStreams(out, warn, err);
    }

    /** Get the version of this copy of <b>jmake</b> */
    public static String getVersion() {
        return VERSION;
    }
    private static final String ERR_IS_INVALID_OPTION =
            " is an invalid option or argument.";
    private static final String ERR_NO_TWO_COMPILER_OPTIONS =
            "You may not specify both compiler class and compiler executable application";
    private static final String ERR_SHOULD_BE_EXPLICIT =
            " compiler option should be specified directly as a jmake option";

    private static void bailOut(String s) {
        throw new PrivateException(new PublicExceptions.InvalidCmdOptionException("jmake: " + s + "\nRun \"jmake -h\" for help."));
    }

    private static void optRequiresArg(String s) {
        bailOut("the " + s + " option requires an argument.");
    }

    private static void printUsage() {
        Utils.printInfoMessage("Usage: jmake <options> <.java files> <@files>");
        Utils.printInfoMessage("where possible options include:");
        Utils.printInfoMessage("  -h, -help             print this help message");
        Utils.printInfoMessage("  -version              print the product version number");
        Utils.printInfoMessage("  -pdb <file name>      specify non-default project database file");
        Utils.printInfoMessage("  -pdb-text-format      if specified, pdb file is stored in text format");
        Utils.printInfoMessage("  -d <directory>        specify where to place generated class files");
        Utils.printInfoMessage("  -classpath <path>     specify where to find user class files");
        Utils.printInfoMessage("  -projclasspath <path> specify where to find sourceless project classes");
        Utils.printInfoMessage("                        (currently only JARs are allowed on this path)");
        Utils.printInfoMessage("  -C<option>            specify an option to be passed to the Java compiler");
        Utils.printInfoMessage("                        (this option's arguments should also be preceded by -C)");
        Utils.printInfoMessage("  -jcpath <path>        specify the class path for a non-default Java compiler");
        Utils.printInfoMessage("                        (default is <JAVAHOME>/lib/tools.jar)");
        Utils.printInfoMessage("  -jcmainclass <class>  specify the main class for a non-default Java compiler");
        Utils.printInfoMessage("                        (default is com.sun.tools.javac.Main)");
        Utils.printInfoMessage("  -jcmethod <method>    specify the method to call in the Java compiler class");
        Utils.printInfoMessage("                        (default is \"compile(String args[])\")");
        Utils.printInfoMessage("  -jcexec <file name>   specify a binary non-default Java compiler application");
        Utils.printInfoMessage("  -failondependentjar   fail if a class on projectclasspath depends on a class");
        Utils.printInfoMessage("                        with .java source (by default, a warning is issued)");
        Utils.printInfoMessage("  -nowarnondependentjar no warning or error if a class on projectclasspath");
        Utils.printInfoMessage("                        depends on a class with a .java source (use with care)");
        Utils.printInfoMessage("  -warnlimit <number>   specify the maximum number of warnings (20 by default)");
        Utils.printInfoMessage("  -bootclasspath <path> override location of bootstrap class files");
        Utils.printInfoMessage("  -extdirs <dirs>       override location of installed extensions");
        Utils.printInfoMessage("  -vpath <dirs>         a list of directories to search for Java and class files similar to GNUMake's VPATH");
        Utils.printInfoMessage("  -depfile <path>       a file generated by the compiler containing additional java->class mappings");
        Utils.printInfoMessage("");
        Utils.printInfoMessage("Examples:");
        Utils.printInfoMessage("  jmake -d classes -classpath .;mylib.jar X.java Y.java Z.java");
        Utils.printInfoMessage("  jmake -pdb myproject.pdb -jcexec c:\\java\\jikes\\jikes.exe @myproject.src");
    }
}
