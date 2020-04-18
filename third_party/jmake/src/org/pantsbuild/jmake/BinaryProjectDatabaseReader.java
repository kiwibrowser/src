/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.File;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * This class creates the internal representation of the project database from a byte array.
 *
 * @author  Misha Dmitriev
 *  2 March 2005
 */
public class BinaryProjectDatabaseReader extends BinaryFileReader {

    private String stringTable[];
    private Map<String,PCDEntry> pcd;
    private int nOfEntries;
    private int pdbFormat;  // Currently supported values: 0x01030300 (jmake 1.3.3 and newer versions); 1 (all older versions)
    // These are defined in Utils as PDB_FORMAT_CODE_LATEST and PDB_FORMAT_CODE_OLD

    public Map<String,PCDEntry> readProjectDatabaseFromFile(File infile) {
        byte buf[] = Utils.readFileIntoBuffer(infile);
        return readProjectDatabase(buf, infile.toString());
    }

    public Map<String,PCDEntry> readProjectDatabase(byte[] pdbFile,
            String pdbFileFullPath) {
        initBuf(pdbFile, pdbFileFullPath);

        readPreamble();
        readStringTable();
        pcd = new LinkedHashMap<String,PCDEntry>(nOfEntries * 4 / 3);

        for (int i = 0; i < nOfEntries; i++) {
            PCDEntry entry = readPCDEntry();
            pcd.put(entry.className, entry);
        }

        stringTable = null;  // Help the GC
        return pcd;
    }

    private void readPreamble() {
        if (buf.length < Utils.magicLength + 8) {
            pdbCorruptedException("file too short");
        }

        for (int i = 0; i < Utils.magicLength; i++) {
            if (buf[i] != Utils.MAGIC[i]) {
                pdbCorruptedException("wrong project database header");
            }
        }

        curBufPos += Utils.magicLength;
        pdbFormat = nextInt();
        if (pdbFormat != Utils.PDB_FORMAT_CODE_OLD && pdbFormat != Utils.PDB_FORMAT_CODE_LATEST) {
            pdbCorruptedException("wrong version number");
        }

        int pdbSize = nextInt();
        if (buf.length != Utils.MAGIC.length + 8 + pdbSize) {
            pdbCorruptedException("file size does not match stored value");
        }

        nOfEntries = nextInt();
    }

    private void readStringTable() {
        int size = nextInt();
        stringTable = new String[size];
        for (int i = 0; i < size; i++) {
            stringTable[i] = nextString();
        }
    }

    private PCDEntry readPCDEntry() {
        String className = nextStringRef();
        String javaFileFullPath = nextStringRef();
        long classFileLastModified = nextLong();
        long classFileFingerprint = nextLong();
        ClassInfo classInfo = readClassInfo();

        return new PCDEntry(className, javaFileFullPath, classFileLastModified, classFileFingerprint, classInfo);
    }

    private ClassInfo readClassInfo() {
        int i, j, len;
        ClassInfo res = new ClassInfo();

        res.name = nextStringRef();
        if (pdbFormat >= Utils.PDB_FORMAT_CODE_133) {
            res.javacTargetRelease = nextInt();
        } else {
            res.javacTargetRelease = Utils.JAVAC_TARGET_RELEASE_OLDEST;
        }

        len = nextChar();
        if (len > 0) {
            String cpoolRefsToClasses[] = new String[len];
            for (i = 0; i < len; i++) {
                cpoolRefsToClasses[i] = nextStringRef();
            }
            res.cpoolRefsToClasses = cpoolRefsToClasses;
            boolean isRefClassArray[] = new boolean[len];
            for (i = 0; i < len; i++) {
                isRefClassArray[i] = (buf[curBufPos++] != 0);
            }
            res.isRefClassArray = isRefClassArray;
        }

        len = nextChar();
        if (len > 0) {
            String cpoolRefsToFieldClasses[] = new String[len];
            for (i = 0; i < len; i++) {
                cpoolRefsToFieldClasses[i] = nextStringRef();
            }
            res.cpoolRefsToFieldClasses = cpoolRefsToFieldClasses;
            String cpoolRefsToFieldNames[] = new String[len];
            for (i = 0; i < len; i++) {
                cpoolRefsToFieldNames[i] = nextStringRef();
            }
            res.cpoolRefsToFieldNames = cpoolRefsToFieldNames;
            String cpoolRefsToFieldSignatures[] = new String[len];
            for (i = 0; i < len; i++) {
                cpoolRefsToFieldSignatures[i] = nextStringRef();
            }
            res.cpoolRefsToFieldSignatures = cpoolRefsToFieldSignatures;
        }

        len = nextChar();
        if (len > 0) {
            String cpoolRefsToMethodClasses[] = new String[len];
            for (i = 0; i < len; i++) {
                cpoolRefsToMethodClasses[i] = nextStringRef();
            }
            res.cpoolRefsToMethodClasses = cpoolRefsToMethodClasses;
            String cpoolRefsToMethodNames[] = new String[len];
            for (i = 0; i < len; i++) {
                cpoolRefsToMethodNames[i] = nextStringRef();
            }
            res.cpoolRefsToMethodNames = cpoolRefsToMethodNames;
            String cpoolRefsToMethodSignatures[] = new String[len];
            for (i = 0; i < len; i++) {
                cpoolRefsToMethodSignatures[i] = nextStringRef();
            }
            res.cpoolRefsToMethodSignatures = cpoolRefsToMethodSignatures;
        }

        res.accessFlags = nextChar();
        res.isNonMemberNestedClass = (buf[curBufPos++] != 0);
        if (!"java/lang/Object".equals(res.name)) {
            res.superName = nextStringRef();
        }

        len = nextChar();
        if (len > 0) {
            String interfaces[] = new String[len];
            for (i = 0; i < len; i++) {
                interfaces[i] = nextStringRef();
            }
            res.interfaces = interfaces;
        }

        len = nextChar();
        if (len > 0) {
            String fieldNames[] = new String[len];
            for (i = 0; i < len; i++) {
                fieldNames[i] = nextStringRef();
            }
            res.fieldNames = fieldNames;
            String fieldSignatures[] = new String[len];
            for (i = 0; i < len; i++) {
                fieldSignatures[i] = nextStringRef();
            }
            res.fieldSignatures = fieldSignatures;
            char fieldAccessFlags[] = new char[len];
            for (i = 0; i < len; i++) {
                fieldAccessFlags[i] = nextChar();
            }
            res.fieldAccessFlags = fieldAccessFlags;
        }

        len = nextChar();
        if (len > 0) {
            Object primitiveConstantInitValues[] = new Object[len];
            for (i = 0; i < len; i++) {
                byte code = buf[curBufPos++];
                switch (code) {
                    case 1:
                        primitiveConstantInitValues[i] = nextStringRef();
                        break;
                    case 2:
                        primitiveConstantInitValues[i] = Integer.valueOf(nextInt());
                        break;
                    case 3:
                        primitiveConstantInitValues[i] = Long.valueOf(nextLong());
                        break;
                    case 4:
                        primitiveConstantInitValues[i] = Float.valueOf(nextFloat());
                        break;
                    case 5:
                        primitiveConstantInitValues[i] =
                                Double.valueOf(nextDouble());
                        break;
                    default:  // Nothing to do
                }
            }
            res.primitiveConstantInitValues = primitiveConstantInitValues;
        }

        len = nextChar();
        if (len > 0) {
            String methodNames[] = new String[len];
            for (i = 0; i < len; i++) {
                methodNames[i] = nextStringRef();
            }
            res.methodNames = methodNames;
            String methodSignatures[] = new String[len];
            for (i = 0; i < len; i++) {
                methodSignatures[i] = nextStringRef();
            }
            res.methodSignatures = methodSignatures;
            char methodAccessFlags[] = new char[len];
            for (i = 0; i < len; i++) {
                methodAccessFlags[i] = nextChar();
            }
            res.methodAccessFlags = methodAccessFlags;
        }

        len = nextChar();
        if (len > 0) {
            String checkedExceptions[][] = new String[len][];
            for (i = 0; i < len; i++) {
                int len1 = nextChar();
                if (len1 > 0) {
                    checkedExceptions[i] = new String[len1];
                    for (j = 0; j < len1; j++) {
                        checkedExceptions[i][j] = nextStringRef();
                    }
                }
            }
            res.checkedExceptions = checkedExceptions;
        }

        len = nextChar();
        if (len > 0) {
            String nestedClasses[] = new String[len];
            for (i = 0; i < len; i++) {
                nestedClasses[i] = nextStringRef();
            }
            res.nestedClasses = nestedClasses;
        }

        res.initializeImmediateTransientFields();
        return res;
    }

    private String nextString() {
        int length = nextChar();
        if (buf.length < curBufPos + length) {
            pdbCorruptedException("data error");
        }
        String res = (new String(buf, curBufPos, length)).intern();
        curBufPos += length;
        return res;
    }

    private String nextStringRef() {
        return stringTable[nextInt()];
    }

    private void pdbCorruptedException(String message) {
        throw new PrivateException(new PublicExceptions.PDBCorruptedException(message));
    }
}
