/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * Utility functions used by other classes from this package.
 *
 * @author Misha Dmitriev
 * 23 January 2003
 */
public class Utils {

    static final String REPORT_PROBLEM =
            "Please report this problem to Mikhail.Dmitriev@sun.com";
    static final byte[] MAGIC = {'J', 'a', 'v', 'a', 'm', 'a', 'k', 'e', ' ', 'P', 'r', 'o', 'j', 'e', 'c', 't', ' ', 'D', 'a', 't', 'a', 'b', 'a', 's', 'e', ' ', 'F', 'i', 'l', 'e'};
    static final int magicLength = MAGIC.length;
    static final int PDB_FORMAT_CODE_OLD = 1;
    static final int PDB_FORMAT_CODE_133 = 0x01030300;
    static final int PDB_FORMAT_CODE_LATEST = PDB_FORMAT_CODE_133;
    static final int JAVAC_TARGET_RELEASE_OLDEST = 0x01040000;  // 1.4 and previous versions
    static final int JAVAC_TARGET_RELEASE_15 = 0x01050000;  // if class is compiled with -target 1.5
    static final int JAVAC_TARGET_RELEASE_16 = 0x01060000;  // if class is compiled with -target 1.6
    static final int JAVAC_TARGET_RELEASE_17 = 0x01070000;  // if class is compiled with -target 1.7
    static final int JAVAC_TARGET_RELEASE_18 = 0x01080000;  // if class is compiled with -target 1.8
    static int warningLimit = 20;  // Maximum number of warnings to print
    static final int TIMING_TOTAL = 0;
    static final int TIMING_PDBREAD = 1;
    static final int TIMING_SYNCHRO = 2;
    static final int TIMING_SYNCHRO_CHECK_JAVA_FILES = 3;
    static final int TIMING_FIND_UPDATED_JAVA_FILES = 4;
    static final int TIMING_CLASS_FILE_OBSOLETE_OR_DELETED = 5;
    static final int TIMING_COMPILE = 6;
    static final int TIMING_FIND_UPDATED_CLASSES = 7;
    static final int TIMING_CHECK_UPDATED_CLASSES = 8;
    static final int TIMING_PDBWRITE = 9;
    static final int TIMING_SYNCHRO_CHECK_TMP = 10;
    static final int TIMING_CLASS_FILE_OBSOLETE_TMP = 11;
    static final int TIMING_PDBUPDATE = 12;
    static final int TIMING_ARRAY_LENGTH = 13;
    private static long timings[] = new long[TIMING_ARRAY_LENGTH];
    private static boolean timingOn = false;


    // -------------------------------------------------------------------------------
    // Name manipulation stuff
    // -------------------------------------------------------------------------------
    /**
     * Returns package name for the given class. In case of no package, returns an
     * empty, but non-null string. Returned string is interned.
     */
    public static String getPackageName(String clazzName) {
        int ldi = clazzName.lastIndexOf('/');  // For convenience, we use system-internal slashes, not dots
        if (ldi == -1) {
            return "";
        } else {
            return clazzName.substring(0, ldi).intern();
        }
    }

    /**
     * Returns directly enclosing class name for the given class. If the given class is not a
     * nested class, returns empty, but non-null string. Returned string is interned.
     * NOTE FOR JDK 1.5: this function has to work with both old (1.4 and before) and new (1.5) ways
     * of naming non-member classes. javacTargetRelease determines the javac version for this class;
     * however on rare occasions (when checking a deleted non-project class) it may be 0, denoting
     * that javac version is not known.
     * In that case, we use the old algorithm, which is error-prone due to a bug in nested class
     * naming that existed prior to JDK 1.5, where both a non-member local nested class B of A, and a
     * member nested class B of anonymous class A$1, are named A$1$B.
     */
    public static String getDirectlyEnclosingClass(String clazzName, int javacTargetRelease) {
        int ldi = clazzName.lastIndexOf('$');
        if (ldi == -1) {
            return "";
        }

        if (javacTargetRelease >= JAVAC_TARGET_RELEASE_15) {
            return clazzName.substring(0, ldi).intern();
        } else {   // JAVAC_TARGET_RELEASE_OLDEST or unknown
            // Take into account local classes which are named like "EncClass$1$LocalClass", where EncClass
            // is directly enclosing class.
            int lldi = clazzName.lastIndexOf('$', ldi - 1);
            if (lldi == -1 || !Character.isDigit(clazzName.charAt(lldi + 1))) {
                return clazzName.substring(0, ldi).intern();
            } else {
                return clazzName.substring(0, lldi).intern();
            }
        }
    }

    /**
     * Returns top-level enclosing class name for the given class. If the given class is not a
     * nested class, returns empty, but non-null string. Returned string is interned.
     */
    public static String getTopLevelEnclosingClass(String clazzName) {
        int fdi = clazzName.indexOf('$');
        if (fdi == -1) {
            return "";
        }

        return clazzName.substring(0, fdi).intern();
    }

    /**
     * Given the full path for the enclosing class file and the full name for the nested class, return the supposed
     * full path for the nested class.
     */
    public static String getClassFileFullPathForNestedClass(String enclosingClassFileFullPath, String nestedClassFullName) {
        String enclosingClassDir = enclosingClassFileFullPath;
        int cutIndex = enclosingClassDir.lastIndexOf(File.separatorChar);
        enclosingClassDir = enclosingClassDir.substring(0, cutIndex + 1); // If slash is present, it's included, otherwise we get ""
        cutIndex = nestedClassFullName.lastIndexOf('/');
        String nestedClassLocalName;
        if (cutIndex < 0) {
            nestedClassLocalName = nestedClassFullName;
        } else {
            nestedClassLocalName = nestedClassFullName.substring(cutIndex + 1);
        }
        return enclosingClassDir + nestedClassLocalName + ".class";
    }

    /**
     * For two strings representing signatures, check if the number of parameters in
     * both is the same.
     */
    public static boolean sameParamNumber(String sig1, String sig2) {
        return getParamNumber(sig1) == getParamNumber(sig2);
    }

    private static int getParamNumber(String sig) {
        char ch;
        int parNo = 0, pos = 0;
        do {
            ch = sig.charAt(++pos);
            if (ch == ')') {
                break;
            }
            while (ch == '[') {
                ch = sig.charAt(++pos);
            }
            parNo++;
            if (ch == '@') {
                // We replaced all "Lclassname;" in signatures with "@classname#"
                while (ch != '#') {
                    ch = sig.charAt(++pos);
                }
            }
        } while (ch != ')');
        return parNo;
    }


    // -------------------------------------------------------------------------------
    // File related stuff
    // -------------------------------------------------------------------------------
    public static File checkFileForName(String name) {
        // For each .java file, a File object is created two times when jmake executes: first when we synchronise the PCD
        // and the supplied .java file list (we make sure that the .java file exists), and second time when we check if a class
        // file was updated (we compare time stamps of the .java and the .class file). I tried to call this routine for a .java
        // class both times, and cached File objects, but it looks as if this does not bring any real speed-up (and in fact may
        // even slow down the application). Most of the time seems to go to the underlying code creating internal File
        // representation; once it is created, it takes little time to execute another "new File()" for it. Also, all operations
        // on files like getCanonicalPath() or lastModified() seem to be quite expensive, so their unnecessary repetition should
        // be avoided as much as possible.
        if (name == null) {
            return null;
        }
        File file = new File(name);
        if (file.exists()) {
            return file;
        }
        return null;
    }

    public static File checkOrCreateDirForName(String name) {
        File file = new File(name);
        if (!file.exists()) {
            file.mkdirs();
        }
        if (file.exists()) {
            if (!file.isDirectory()) {
                throw new PrivateException(new PublicExceptions.InternalException(file + " is not a directory."));
            }
            return file;
        }
        return null;
    }

    public static byte[] readFileIntoBuffer(File file) {
        try {
            InputStream in = new FileInputStream(file);
            int len = (int) file.length();
            return readInputStreamIntoBuffer(in, len);
        } catch (IOException e) {
            throw new PrivateException(e);
        }
    }

    public static byte[] readZipEntryIntoBuffer(ZipFile file, ZipEntry entry) {
        try {
            InputStream in = file.getInputStream(entry);
            int len = (int) entry.getSize();
            return Utils.readInputStreamIntoBuffer(in, len);
        } catch (IOException e) {
            throw new PrivateException(e);
        }
    }

    public static byte[] readInputStreamIntoBuffer(InputStream in, int len) throws IOException {
        byte buf[] = new byte[len];
        int readBytes, ofs = 0, remBytes = len;
        do {
            readBytes = in.read(buf, ofs, remBytes);
            ofs += readBytes;
            remBytes -= readBytes;
        } while (ofs < len);
        in.close();
        return buf;
    }

    public static void readAndPrintBytesFromStream(InputStream in, OutputStream out) throws IOException {
        int avail = in.available();
        if (avail > 0) {
            byte outbytes[] = new byte[avail];
            int realOutBytes = in.read(outbytes);
            out.write(outbytes, 0, realOutBytes);
        }
    }

    /** For a Windows path, convert the drive letter to the lower case */
    public static String convertDriveLetterToLowerCase(String path) {
        if (path.charAt(1) != ':') {
            return path;
        }
        char drive = path.charAt(0);
        if (Character.isUpperCase(drive)) {
            drive = Character.toLowerCase(drive);
            char[] chars = path.toCharArray();
            chars[0] = drive;
            path = new String(chars);
        }
        return path;
    }

    public static void ignore(Exception e) {
        // Ignore this exception
    }

    /** Used when invoking a third-party executable compiler app */
    public static void delay(int ms) {
        Object o = new Object();
        synchronized (o) {
            try {
                o.wait(ms);
            } catch (InterruptedException e) {
            }
        }
    }
    // -------------------------------------------------------------------------------
    // Custom printing stuff
    // -------------------------------------------------------------------------------
    private static PrintStream out = System.out;
    private static PrintStream warn = System.out;
    private static PrintStream err = System.err;
    private static boolean printInfoMessages = true;
    private static boolean printWarningMessages = true;
    private static boolean printErrorMessages = true;
    private static int warningNo;

    public static void setOutputStreams(PrintStream out, PrintStream warn, PrintStream err) {
        Utils.out = out;
        Utils.warn = warn;
        Utils.err = err;
    }

    public static void customizeOutput(boolean printInfoMessages, boolean printWarningMessages, boolean printErrorMessages) {
        Utils.printInfoMessages = printInfoMessages;
        Utils.printWarningMessages = printWarningMessages;
        Utils.printErrorMessages = printErrorMessages;
    }

    public static void printInfoMessage(String message) {
        if (printInfoMessages) {
            out.println(message);
        }
    }

    public static void printInfoMessageNoEOL(String message) {
        if (printInfoMessages) {
            out.print(message);
        }
    }

    public static void printWarningMessage(String message) {
        if (!printWarningMessages) {
            return;
        }
        if (warningNo < warningLimit) {
            warn.println(message);
        } else if (warningNo == warningLimit) {
            warn.println("jmake: more than " + warningLimit + " warnings.");
        }
        warningNo++;
    }

    public static void printErrorMessage(String message) {
        if (printErrorMessages) {
            err.println("jmake: " + message);
        }
    }

    // -------------------------------------------------------------------------------
    // Measuring stuff
    // -------------------------------------------------------------------------------
    public static void setTimingOn() {
        timingOn = true;
    }

    public static void startTiming(int slot) {
        timings[slot] = System.currentTimeMillis();
    }

    public static void stopAndPrintTiming(String message, int slot) {
        if (timingOn) {
            long time = System.currentTimeMillis() - timings[slot];
            printInfoMessage("========== " + message + " time = " + time);
        }
    }

    public static void printTiming(String message, int slot) {
        if (timingOn) {
            printInfoMessage("========== " + message + " time = " + timings[slot]);
        }
    }

    public static void stopAndAddTiming(int slot1, int slot2) {
        if (timingOn) {
            long time = System.currentTimeMillis() - timings[slot1];
            timings[slot2] += time;
        }
    }
}
