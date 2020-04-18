/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.lang.reflect.Modifier;


/**
 * This class implements reading a byte array representing a class file and converting it into ClassInfo.
 *
 * @author  Misha Dmitriev
 *  2 March 2005
 */
public class ClassFileReader extends BinaryFileReader {

    public static final int JAVA_MAGIC = -889275714;    // 0xCAFEBABE
    public static final int JAVA_MINOR_VERSION = 0;
    public static final int JAVA_MIN_MAJOR_VERSION = 45;
    public static final int JAVA_MIN_MINOR_VERSION = 3;
    public static final int DEFAULT_MAJOR_VERSION = 46;
    public static final int DEFAULT_MINOR_VERSION = 0;
    public static final int JDK14_MAJOR_VERSION = 48;
    public static final int JDK15_MAJOR_VERSION = 49;
    public static final int JDK16_MAJOR_VERSION = 50;
    public static final int JDK17_MAJOR_VERSION = 51;
    public static final int JDK18_MAJOR_VERSION = 52;
    public static final int CONSTANT_Utf8 = 1;
    public static final int CONSTANT_Unicode = 2;
    public static final int CONSTANT_Integer = 3;
    public static final int CONSTANT_Float = 4;
    public static final int CONSTANT_Long = 5;
    public static final int CONSTANT_Double = 6;
    public static final int CONSTANT_Class = 7;
    public static final int CONSTANT_String = 8;
    public static final int CONSTANT_Fieldref = 9;
    public static final int CONSTANT_Methodref = 10;
    public static final int CONSTANT_InterfaceMethodref = 11;
    public static final int CONSTANT_NameandType = 12;
    public static final int CONSTANT_MethodHandle = 15;
    public static final int CONSTANT_MethodType = 16;
    public static final int CONSTANT_InvokeDynamic = 18;
    private ClassInfo classInfo = null;
    private int cpOffsets[];
    private Object cpObjectCache[];
    private byte cpTags[];

    public void readClassFile(byte[] classFile, ClassInfo classInfo, String classFileFullPath, boolean readFullInfo) {
        initBuf(classFile, classFileFullPath);
        this.classInfo = classInfo;

        readPreamble();
        readConstantPool(readFullInfo);
        readIntermediate();
        if (readFullInfo) {
            readFields();
            readMethods();
            readAttributes();
        }
    }

    private int versionWord(int major, int minor) {
        return major * 1000 + minor;
    }

    private void readPreamble() {
        int magic = nextInt();
        if (magic != JAVA_MAGIC) {
            throw classFileParseException("Illegal start of class file");
        }
        int minorVersion = nextChar();
        int majorVersion = nextChar();
        if (majorVersion > JDK14_MAJOR_VERSION ||
                versionWord(majorVersion, minorVersion) <
                versionWord(JAVA_MIN_MAJOR_VERSION, JAVA_MIN_MINOR_VERSION) ) {
            if (majorVersion == JDK18_MAJOR_VERSION) {
                classInfo.javacTargetRelease = Utils.JAVAC_TARGET_RELEASE_18;
            } else if (majorVersion == JDK17_MAJOR_VERSION) {
                classInfo.javacTargetRelease = Utils.JAVAC_TARGET_RELEASE_17;
            } else if (majorVersion == JDK16_MAJOR_VERSION) {
                classInfo.javacTargetRelease = Utils.JAVAC_TARGET_RELEASE_16;
            } else if (majorVersion == JDK15_MAJOR_VERSION) {
                classInfo.javacTargetRelease = Utils.JAVAC_TARGET_RELEASE_15;
            } else {
                throw classFileParseException("Wrong version: " + majorVersion + "." + minorVersion);
            }
        } else {
            classInfo.javacTargetRelease = Utils.JAVAC_TARGET_RELEASE_OLDEST;
        }
    }

    private void readConstantPool(boolean readFullInfo) {
        int classRefsNo = 0;
        int fieldRefsNo = 0;
        int methodRefsNo = 0;

        cpOffsets = new int[nextChar()];
        cpTags = new byte[cpOffsets.length];
        int ofs, len, classIdx, nameAndTypeIdx, nameIdx, sigIdx, utf8Idx;
        int i = 1;
        while (i < cpOffsets.length) {
            byte tag = buf[curBufPos++];
            cpOffsets[i] = curBufPos;
            cpTags[i] = tag;
            i++;
            switch (tag) {
                case CONSTANT_Utf8:
                    len = nextChar();
                    curBufPos += len;
                    break;

                case CONSTANT_Class:
                    classRefsNo++;
                    curBufPos += 2;
                    break;

                case CONSTANT_String:
                case CONSTANT_MethodType:
                    curBufPos += 2;
                    break;

                case CONSTANT_Fieldref:
                    fieldRefsNo++;
                    curBufPos += 4;
                    break;

                case CONSTANT_Methodref:
                case CONSTANT_InterfaceMethodref:
                    methodRefsNo++;
                    curBufPos += 4;
                    break;

                case CONSTANT_MethodHandle:
                    curBufPos += 3;
                    break;

                case CONSTANT_NameandType:
                case CONSTANT_Integer:
                case CONSTANT_Float:
                case CONSTANT_InvokeDynamic:
                    curBufPos += 4;
                    break;

                case CONSTANT_Long:
                case CONSTANT_Double:
                    curBufPos += 8;
                    i++;
                    break;

                default:
                    throw classFileParseException("Bad constant pool tag: " + tag + " at " + Integer.toString(curBufPos - 1));
            }
        }

        cpObjectCache = new Object[cpOffsets.length];
        if (!readFullInfo) {
            return;
        }

        classInfo.cpoolRefsToClasses = new String[classRefsNo];
        classInfo.isRefClassArray = new boolean[classRefsNo];
        classInfo.cpoolRefsToFieldClasses = new String[fieldRefsNo];
        classInfo.cpoolRefsToFieldNames = new String[fieldRefsNo];
        classInfo.cpoolRefsToFieldSignatures = new String[fieldRefsNo];
        classInfo.cpoolRefsToMethodClasses = new String[methodRefsNo];
        classInfo.cpoolRefsToMethodNames = new String[methodRefsNo];
        classInfo.cpoolRefsToMethodSignatures = new String[methodRefsNo];

        int curClassRef = 0;
        int curFieldRef = 0;
        int curMethodRef = 0;

        for (i = 0; i < cpOffsets.length; i++) {
            ofs = cpOffsets[i];
            switch (cpTags[i]) {
                case CONSTANT_Class:
                    utf8Idx = getChar(ofs);
                    classInfo.cpoolRefsToClasses[curClassRef++] =
                            classNameAtCPIndex(utf8Idx, classInfo.isRefClassArray, curClassRef - 1);
                    //System.out.println("Read cpool ref to class: " + classInfo.cpoolRefsToClasses[curClassRef-1]);
                    break;

                case CONSTANT_Fieldref:
                    classIdx = getChar(ofs);
                    nameAndTypeIdx = getChar(ofs + 2);
                    if (cpTags[classIdx] != CONSTANT_Class || cpTags[nameAndTypeIdx] != CONSTANT_NameandType) {
                        badCPReference(ofs, i);
                    }
                    classInfo.cpoolRefsToFieldClasses[curFieldRef] =
                            classNameAtCPIndex(getChar(cpOffsets[classIdx]));

                    ofs = cpOffsets[nameAndTypeIdx];
                    nameIdx = getChar(ofs);
                    sigIdx = getChar(ofs + 2);
                    if (cpTags[nameIdx] != CONSTANT_Utf8 || cpTags[sigIdx] != CONSTANT_Utf8) {
                        badCPReference(ofs, i);
                    }
                    classInfo.cpoolRefsToFieldNames[curFieldRef] =
                            utf8AtCPIndex(nameIdx);
                    classInfo.cpoolRefsToFieldSignatures[curFieldRef] =
                            signatureAtCPIndex(sigIdx);
                    //System.out.println("Read cpool ref to field: " + classInfo.cpoolRefsToFieldNames[curFieldRef] + " " +
                    //                   classInfo.cpoolRefsToFieldSignatures[curFieldRef]);
                    curFieldRef++;
                    break;

                case CONSTANT_Methodref:
                case CONSTANT_InterfaceMethodref:
                    classIdx = getChar(ofs);
                    nameAndTypeIdx = getChar(ofs + 2);
                    if (cpTags[classIdx] != CONSTANT_Class || cpTags[nameAndTypeIdx] != CONSTANT_NameandType) {
                        badCPReference(ofs, i);
                    }
                    classInfo.cpoolRefsToMethodClasses[curMethodRef] =
                            classNameAtCPIndex(getChar(cpOffsets[classIdx]));

                    ofs = cpOffsets[nameAndTypeIdx];
                    nameIdx = getChar(ofs);
                    sigIdx = getChar(ofs + 2);
                    if (cpTags[nameIdx] != CONSTANT_Utf8 || cpTags[sigIdx] != CONSTANT_Utf8) {
                        badCPReference(ofs, i);
                    }
                    classInfo.cpoolRefsToMethodNames[curMethodRef] =
                            utf8AtCPIndex(nameIdx);
                    classInfo.cpoolRefsToMethodSignatures[curMethodRef] =
                            signatureAtCPIndex(sigIdx);
                    //System.out.println("Read cpool ref to method: " + classInfo.cpoolRefsToMethodNames[curMethodRef] + " " +
                    //                   classInfo.cpoolRefsToMethodSignatures[curMethodRef]);
                    curMethodRef++;
                    break;
            }
        }
    }

    private void readIntermediate() {
        int i, classIdx, superClassIdx;

        classInfo.accessFlags = nextChar();
        classIdx = nextChar();
        if (cpTags[classIdx] != CONSTANT_Class) {
            throw classFileParseException("Bad reference to this class name");
        }
        classInfo.name = classNameAtCPIndex(getChar(cpOffsets[classIdx]));
        superClassIdx = nextChar();
        if (!"java/lang/Object".equals(classInfo.name)) {
            if (cpTags[superClassIdx] != CONSTANT_Class) {
                throw classFileParseException("Bad reference to super class name");
            }
            classInfo.superName =
                    classNameAtCPIndex(getChar(cpOffsets[superClassIdx]));
        }

        char intfCount = nextChar();
        if (intfCount != 0) {
            classInfo.interfaces = new String[intfCount];
            for (i = 0; i < intfCount; i++) {
                classIdx = nextChar();
                if (cpTags[classIdx] != CONSTANT_Class) {
                    throw classFileParseException("Bad reference to an implemented interface");
                }
                classInfo.interfaces[i] =
                        classNameAtCPIndex(getChar(cpOffsets[classIdx]));
            }
        }
    }

    private void readFields() {
        int i, j;

        char definedFieldCount = nextChar();
        if (definedFieldCount == 0) {
            return;
        }

        String names[] = new String[definedFieldCount];
        String signatures[] = new String[definedFieldCount];
        char accessFlags[] = new char[definedFieldCount];

        // We are not going to record information on private fields which have either primitive or non-project-class
        // (typically core-class) types. Such fields cannot affect anything except their own class, so we don't need them.
        int ri = 0;

        for (i = 0; i < definedFieldCount; i++) {
            char flags = nextChar();
            String name = utf8AtCPIndex(nextChar());
            String sig = signatureAtCPIndex(nextChar());

            boolean recordField =
                    !(Modifier.isPrivate(flags) &&
                    (ClassInfo.isPrimitiveFieldSig(sig) || classInfo.isNonProjectClassTypeFieldSig(sig)));

            int attrCount = nextChar();
            for (j = 0; j < attrCount; j++) {
                int attrNameIdx = nextChar();
                int attrLen = nextInt();
                if (recordField && utf8AtCPIndex(attrNameIdx).equals("ConstantValue") &&
                        Modifier.isFinal(flags)) {
                    if (classInfo.primitiveConstantInitValues == null) {
                        classInfo.primitiveConstantInitValues =
                                new Object[definedFieldCount];
                    }
                    int constValueIdx = nextChar();
                    switch (cpTags[constValueIdx]) {
                        case CONSTANT_String:
                            classInfo.primitiveConstantInitValues[ri] =
                                    utf8AtCPIndex(getChar(cpOffsets[constValueIdx]));
                            break;

                        case CONSTANT_Integer:
                            classInfo.primitiveConstantInitValues[ri] =
                                    Integer.valueOf(getInt(cpOffsets[constValueIdx]));
                            break;

                        case CONSTANT_Long:
                            classInfo.primitiveConstantInitValues[ri] =
                                    Long.valueOf(getLong(cpOffsets[constValueIdx]));
                            break;

                        case CONSTANT_Float:
                            classInfo.primitiveConstantInitValues[ri] =
                                    Float.valueOf(getFloat(cpOffsets[constValueIdx]));
                            break;

                        case CONSTANT_Double:
                            classInfo.primitiveConstantInitValues[ri] =
                                    Double.valueOf(getDouble(cpOffsets[constValueIdx]));
                            break;

                        default:
                            badCPEntry(constValueIdx);
                    }

                } else {
                    curBufPos += attrLen;
                }
            }

            if (recordField) {
                names[ri] = name;
                signatures[ri] = sig;
                accessFlags[ri] = flags;
                ri++;
            }
        }

        if (ri == definedFieldCount) {
            classInfo.fieldNames = names;
            classInfo.fieldSignatures = signatures;
            classInfo.fieldAccessFlags = accessFlags;
        } else if (ri > 0) {
            classInfo.fieldNames = new String[ri];
            classInfo.fieldSignatures = new String[ri];
            classInfo.fieldAccessFlags = new char[ri];
            System.arraycopy(names, 0, classInfo.fieldNames, 0, ri);
            System.arraycopy(signatures, 0, classInfo.fieldSignatures, 0, ri);
            System.arraycopy(accessFlags, 0, classInfo.fieldAccessFlags, 0, ri);
        }
    }

    private void readMethods() {
        int i, j;

        char methodCount = nextChar();
        if (methodCount == 0) {
            return;
        }

        String names[] = new String[methodCount];
        String signatures[] = new String[methodCount];
        char accessFlags[] = new char[methodCount];

        for (i = 0; i < methodCount; i++) {
            accessFlags[i] = nextChar();
            names[i] = utf8AtCPIndex(nextChar());
            signatures[i] = signatureAtCPIndex(nextChar());

            int attrCount = nextChar();
            for (j = 0; j < attrCount; j++) {
                int attrNameIdx = nextChar();
                int attrLen = nextInt();
                if (utf8AtCPIndex(attrNameIdx).equals("Exceptions")) {
                    if (classInfo.checkedExceptions == null) {
                        classInfo.checkedExceptions = new String[methodCount][];
                    }
                    int nExceptions = nextChar();
                    String exceptions[] = new String[nExceptions];
                    for (int k = 0; k < nExceptions; k++) {
                        int excClassIdx = nextChar();
                        if (cpTags[excClassIdx] != CONSTANT_Class) {
                            badCPEntry(excClassIdx);
                        }
                        exceptions[k] =
                                classNameAtCPIndex(getChar(cpOffsets[excClassIdx]));
                    }
                    classInfo.checkedExceptions[i] = exceptions;
                } else {
                    curBufPos += attrLen;
                }
            }
        }

        classInfo.methodNames = names;
        classInfo.methodSignatures = signatures;
        classInfo.methodAccessFlags = accessFlags;
    }

    /**
     * This method actually reads only the information related to the nested classes, and
     * records only those of them which are first level nested classes of this class. The class
     * may also reference other classes which are not package members through the same
     * InnerClasses attribute - their names would be processed when their respective enclosing
     * classes are read.
     */
    private void readAttributes() {
        String nestedClassPrefix = classInfo.name + "$";

        char attrCount = nextChar();

        for (int i = 0; i < attrCount; i++) {
            int attrNameIdx = nextChar();
            int attrLen = nextInt();
            if (utf8AtCPIndex(attrNameIdx).equals("InnerClasses")) {
                int nOfClasses = nextChar();
                String nestedClasses[] = new String[nOfClasses];
                char nestedClassAccessFlags[] = new char[nOfClasses];
                boolean nestedClassNonMember[] = new boolean[nOfClasses];
                int curIdx = 0;
                for (int j = 0; j < nOfClasses; j++) {
                    int innerClassInfoIdx = nextChar();
                    int outerClassInfoIdx = nextChar();
                    int innerClassNameIdx = nextChar();
                    char innerClassAccessFlags = nextChar();

                    // Even if a class is private or non-member (innerClassAccessFlags has private bit set or
                    // outerClassInfoIdx == 0), we still should take this class into account, since it may e.g. extend
                    // a public class/implement a public interface, which, in turn, may be changed incompatibly.

                    String nestedClassFullName = classNameAtCPIndex(getChar(cpOffsets[innerClassInfoIdx]));

                    // We are only interested the nested classes whose enclosing class is this one.
                    if (!nestedClassFullName.startsWith(nestedClassPrefix))
                        continue;

                    // We are only interested in the directly nested classes of this class.
                    String nestedClassNameSuffix = nestedClassFullName.substring(nestedClassPrefix.length());

                    if (innerClassNameIdx == 0) {
                        // Nested class is anonymous. Suffix must be all digits.
                        if (findFirstNonDigit(nestedClassNameSuffix) != -1)
                            continue;
                    } else {
                        // Nested class is named.
                        String nestedClassSimpleName = utf8AtCPIndex(innerClassNameIdx);
                        // The simple case is Outer$Inner.
                        if (!nestedClassNameSuffix.equals(nestedClassSimpleName)) {
                            // The more complicated case is a local class. In JDK 1.5+ These are named,
                            // e.g., Outer$1Inner. Pre-JDK 1.5 they are named e.g., Outer$1$Inner.
                            int p = findFirstNonDigit(nestedClassNameSuffix);
                            if (p == -1)
                                continue;
                            if (classInfo.javacTargetRelease == Utils.JAVAC_TARGET_RELEASE_OLDEST &&
                                nestedClassNameSuffix.charAt(p++) != '$')
                                continue;
                            if (!nestedClassNameSuffix.substring(p).equals(nestedClassSimpleName))
                                continue;
                        }
                    }

                    // The name has passed all checks, so register it.

                    nestedClasses[curIdx] = nestedClassFullName;
                    nestedClassAccessFlags[curIdx] = innerClassAccessFlags;
                    nestedClassNonMember[curIdx] = (outerClassInfoIdx == 0);
                    curIdx++;
                }
                if (curIdx == nOfClasses) {
                    classInfo.nestedClasses = nestedClasses;
                    classInfo.nestedClassAccessFlags = nestedClassAccessFlags;
                    classInfo.nestedClassNonMember = nestedClassNonMember;
                } else if (curIdx > 0) {
                    // We found fewer nested classes for this class than we originally expected, but still more than 0.
                    // Create a new array to fit their number exactly.
                    classInfo.nestedClasses = new String[curIdx];
                    classInfo.nestedClassAccessFlags = new char[curIdx];
                    classInfo.nestedClassNonMember = new boolean[curIdx];
                    System.arraycopy(nestedClasses, 0, classInfo.nestedClasses, 0, curIdx);
                    System.arraycopy(nestedClassAccessFlags, 0, classInfo.nestedClassAccessFlags, 0, curIdx);
                    System.arraycopy(nestedClassNonMember, 0, classInfo.nestedClassNonMember, 0, curIdx);
                }
            } else {
                curBufPos += attrLen;
            }
        }
    }

    private int findFirstNonDigit(String s) {
        for (int i = 0; i < s.length(); i++) {
            if (!Character.isDigit(s.charAt(i)))
                return i;
        }
        return -1;
    }

    private String utf8AtCPIndex(int idx) {
        if (cpTags[idx] != CONSTANT_Utf8) {
            throw classFileParseException("Constant pool entry " + idx + " should be UTF8 constant");
        }
        if (cpObjectCache[idx] == null) {
            int utf8Len = getChar(cpOffsets[idx]);
            // String interning reduces the size of the disk database very significantly
            // (by one-third in one observed case), and also speeds up database search.
            cpObjectCache[idx] =
                    (new String(buf, cpOffsets[idx] + 2, utf8Len)).intern();
        }
        return (String) cpObjectCache[idx];
    }

    private String classNameAtCPIndex(int idx) {
        return classNameAtCPIndex(idx, null, 0);
    }

    /**
     * Read class name at the given CONSTANT_Utf8 constant pool index, and return it
     * trimmed of the possible '[' and 'L' prefixes and the ';' suffix.
     */
    private String classNameAtCPIndex(int idx, boolean isRefClassArray[], int isArrayIdx) {
        if (cpTags[idx] != CONSTANT_Utf8) {
            throw classFileParseException("Constant pool entry " + idx + " should be UTF8 constant");
        }
        boolean isArray = false;
        if (cpObjectCache[idx] == null) {
            int utf8Len = getChar(cpOffsets[idx]);
            int stPos = cpOffsets[idx] + 2;
            int initStPos = stPos;
            while (buf[stPos] == '[') {
                stPos++;
            }
            if (stPos != initStPos) {
                isArray = true;
                if (buf[stPos] == 'L') {
                    stPos++;
                    utf8Len--;  // To get rid of the terminating ';'
                }
            }
            utf8Len = utf8Len - (stPos - initStPos);
            cpObjectCache[idx] = (new String(buf, stPos, utf8Len)).intern();
            if (isRefClassArray != null) {
                isRefClassArray[isArrayIdx] = isArray;
            }
        }
        return (String) cpObjectCache[idx];
    }

    // We replace all "Lclassname;" in signatures with "@classname#" to simplify signature parsing during reference checking
    private String signatureAtCPIndex(int idx) {
        if (cpTags[idx] != CONSTANT_Utf8) {
            throw classFileParseException("Constant pool entry " + idx + " should be UTF8 constant");
        }
        if (cpObjectCache[idx] == null) {
            int utf8Len = getChar(cpOffsets[idx]);
            byte tmp[] = new byte[utf8Len];
            System.arraycopy(buf, cpOffsets[idx] + 2, tmp, 0, utf8Len);
            boolean inClassName = false;
            for (int i = 0; i < utf8Len; i++) {
                if (!inClassName) {
                    if (tmp[i] == 'L') {
                        tmp[i] = '@';
                        inClassName = true;
                    }
                } else if (tmp[i] == ';') {
                    tmp[i] = '#';
                    inClassName = false;
                }
            }
            cpObjectCache[idx] = (new String(tmp)).intern();
        }
        return (String) cpObjectCache[idx];
    }

    private void badCPReference(int ofs, int i) {
        throw classFileParseException("Bad constant pool reference: " + ofs + " from entry " + i);
    }

    private void badCPEntry(int entryNo) {
        throw classFileParseException("Constant pool entry " + entryNo + " : invalid type");
    }

    private PrivateException classFileParseException(String msg) {
        return new PrivateException(new PublicExceptions.ClassFileParseException(
                "Error reading class file " + fileFullPath + ":\n" + msg));
    }
}
