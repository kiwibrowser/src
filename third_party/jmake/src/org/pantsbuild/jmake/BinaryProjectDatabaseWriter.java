/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Map;

/**
 * This class implements writing into a byte array representing a project database
 *
 * @author  Misha Dmitriev
 *  2 March 2005
 */
public class BinaryProjectDatabaseWriter extends BinaryFileWriter {

    private Map<String, PCDEntry> pcd = null;
    private int nOfEntries;
    private byte[] stringBuf;
    private int curStringBufPos,  stringBufInc,  curStringBufWatermark,  stringCount;
    private StringHashTable stringHashTable = null;

    public void writeProjectDatabaseToFile(File outfile, Map<String, PCDEntry> pcd) {
        try {
            byte[] buf = new BinaryProjectDatabaseWriter().writeProjectDatabase(pcd);
            FileOutputStream out = new FileOutputStream(outfile);
            out.write(buf);
            out.close();
        } catch (IOException e) {
            throw new PrivateException(e);
        }
    }

    public byte[] writeProjectDatabase(Map<String, PCDEntry> pcd) {
        this.pcd = pcd;
        nOfEntries = pcd.size();

        // So far the constant here is chosen rather arbitrarily
        initBuf(nOfEntries * 1000);

        stringBuf = new byte[nOfEntries * 300];
        stringBufInc = stringBuf.length / 5;
        curStringBufWatermark = stringBuf.length - 20;
        stringHashTable = new StringHashTable(stringBuf.length / 8);

        for (PCDEntry entry : pcd.values()) {
            writePCDEntry(entry);
        }

        // Now we have the string buffer and the main buffer. Write the end result
        byte[] mainBuf = buf;
        int mainBufSize = curBufPos;
        int preambleSize = Utils.MAGIC.length + 8;
        int stringBufSize = curStringBufPos;
        int pdbSize = stringBufSize + mainBufSize + 8;  // 8 is for nOfEntries and string table size
        initBuf(preambleSize + pdbSize);
        setBufferIncreaseMode(false);

        writePreamble(pdbSize);
        writeStringTable(stringBufSize);
        System.arraycopy(mainBuf, 0, buf, curBufPos, mainBufSize);
        return buf;
    }

    private void writePreamble(int pdbSize) {
        System.arraycopy(Utils.MAGIC, 0, buf, 0, Utils.MAGIC.length);
        curBufPos += Utils.MAGIC.length;

        writeInt(Utils.PDB_FORMAT_CODE_LATEST); // Version number
        writeInt(pdbSize);
        writeInt(pcd.size());
    }

    private void writeStringTable(int stringBufSize) {
        writeInt(stringCount);
        System.arraycopy(stringBuf, 0, buf, curBufPos, stringBufSize);
        curBufPos += stringBufSize;
    }

    private void writePCDEntry(PCDEntry entry) {
        writeStringRef(entry.className);
        writeStringRef(entry.javaFileFullPath);
        writeLong(entry.oldClassFileLastModified);
        writeLong(entry.oldClassFileFingerprint);
        writeClassInfo(entry.oldClassInfo);
    }

    private void writeClassInfo(ClassInfo ci) {
        int i, j, len;

        writeStringRef(ci.name);
        writeInt(ci.javacTargetRelease);

        len = ci.cpoolRefsToClasses != null ? ci.cpoolRefsToClasses.length : 0;
        writeChar(len);
        if (len > 0) {
            String cpoolRefsToClasses[] = ci.cpoolRefsToClasses;
            for (i = 0; i < len; i++) {
                writeStringRef(cpoolRefsToClasses[i]);
            }
            boolean isRefClassArray[] = ci.isRefClassArray;
            for (i = 0; i < len; i++) {
                byte b = isRefClassArray[i] ? (byte) 1 : (byte) 0;
                writeByte(b);
            }
        }

        len = ci.cpoolRefsToFieldClasses != null ? ci.cpoolRefsToFieldClasses.length
                : 0;
        writeChar(len);
        if (len > 0) {
            String cpoolRefsToFieldClasses[] = ci.cpoolRefsToFieldClasses;
            for (i = 0; i < len; i++) {
                writeStringRef(cpoolRefsToFieldClasses[i]);
            }
            String cpoolRefsToFieldNames[] = ci.cpoolRefsToFieldNames;
            for (i = 0; i < len; i++) {
                writeStringRef(cpoolRefsToFieldNames[i]);
            }
            String cpoolRefsToFieldSignatures[] = ci.cpoolRefsToFieldSignatures;
            for (i = 0; i < len; i++) {
                writeStringRef(cpoolRefsToFieldSignatures[i]);
            }
        }

        len = ci.cpoolRefsToMethodClasses != null ? ci.cpoolRefsToMethodClasses.length
                : 0;
        writeChar(len);
        if (len > 0) {
            String cpoolRefsToMethodClasses[] = ci.cpoolRefsToMethodClasses;
            for (i = 0; i < len; i++) {
                writeStringRef(cpoolRefsToMethodClasses[i]);
            }
            String cpoolRefsToMethodNames[] = ci.cpoolRefsToMethodNames;
            for (i = 0; i < len; i++) {
                writeStringRef(cpoolRefsToMethodNames[i]);
            }
            String cpoolRefsToMethodSignatures[] =
                    ci.cpoolRefsToMethodSignatures;
            for (i = 0; i < len; i++) {
                writeStringRef(cpoolRefsToMethodSignatures[i]);
            }
        }

        writeChar(ci.accessFlags);
        byte b = ci.isNonMemberNestedClass ? (byte) 1 : (byte) 0;
        writeByte(b);
        if (!"java/lang/Object".equals(ci.name)) {
            writeStringRef(ci.superName);
        }

        len = ci.interfaces != null ? ci.interfaces.length : 0;
        writeChar(len);
        if (len > 0) {
            String interfaces[] = ci.interfaces;
            for (i = 0; i < len; i++) {
                writeStringRef(interfaces[i]);
            }
        }

        len = ci.fieldNames != null ? ci.fieldNames.length : 0;
        writeChar(len);
        if (len > 0) {
            String fieldNames[] = ci.fieldNames;
            for (i = 0; i < len; i++) {
                writeStringRef(fieldNames[i]);
            }
            String fieldSignatures[] = ci.fieldSignatures;
            for (i = 0; i < len; i++) {
                writeStringRef(fieldSignatures[i]);
            }
            char fieldAccessFlags[] = ci.fieldAccessFlags;
            for (i = 0; i < len; i++) {
                writeChar(fieldAccessFlags[i]);
            }
        }

        len = ci.primitiveConstantInitValues != null ? ci.primitiveConstantInitValues.length
                : 0;
        writeChar(len);
        if (len > 0) {
            Object primitiveConstantInitValues[] =
                    ci.primitiveConstantInitValues;
            for (i = 0; i < len; i++) {
                Object pc = primitiveConstantInitValues[i];
                if (pc != null) {
                    if (pc instanceof String) {
                        writeByte((byte)1);
                        writeStringRef((String) pc);
                    } else if (pc instanceof Integer) {
                        writeByte((byte)2);
                        writeInt(((Integer) pc).intValue());
                    } else if (pc instanceof Long) {
                        writeByte((byte)3);
                        writeLong(((Long) pc).longValue());
                    } else if (pc instanceof Float) {
                        writeByte((byte)4);
                        writeFloat(((Float) pc).floatValue());
                    } else if (pc instanceof Double) {
                        writeByte((byte)5);
                        writeDouble(((Double) pc).doubleValue());
                    }
                } else {
                    writeByte((byte)0);
                }
            }
        }

        len = ci.methodNames != null ? ci.methodNames.length : 0;
        writeChar(len);
        if (len > 0) {
            String methodNames[] = ci.methodNames;
            for (i = 0; i < len; i++) {
                writeStringRef(methodNames[i]);
            }
            String methodSignatures[] = ci.methodSignatures;
            for (i = 0; i < len; i++) {
                writeStringRef(methodSignatures[i]);
            }
            char methodAccessFlags[] = ci.methodAccessFlags;
            for (i = 0; i < len; i++) {
                writeChar(methodAccessFlags[i]);
            }
        }

        len = ci.checkedExceptions != null ? ci.checkedExceptions.length : 0;
        writeChar(len);
        if (len > 0) {
            String checkedExceptions[][] = ci.checkedExceptions;
            for (i = 0; i < len; i++) {
                int lenl = checkedExceptions[i] != null ? checkedExceptions[i].length
                        : 0;
                writeChar(lenl);
                if (lenl > 0) {
                    for (j = 0; j < lenl; j++) {
                        writeStringRef(checkedExceptions[i][j]);
                    }
                }
            }
        }

        len = ci.nestedClasses != null ? ci.nestedClasses.length : 0;
        writeChar(len);
        if (len > 0) {
            String nestedClasses[] = ci.nestedClasses;
            for (i = 0; i < len; i++) {
                writeStringRef(nestedClasses[i]);
            }
        }
    }

    private void writeString(String s) {
        byte sb[] = s.getBytes();
        int len = sb.length;
        if (curStringBufPos + len > curStringBufWatermark) {
            // May need to adapt stringBufInc
            if (len >= stringBufInc) {
                stringBufInc = (stringBufInc + len) * 2;
            } else {
                stringBufInc = (stringBufInc * 5) / 4;  // Still increase a little - observations show that otherwise we usually get here 20 more times
            }
            byte newStringBuf[] = new byte[stringBuf.length + stringBufInc];
            System.arraycopy(stringBuf, 0, newStringBuf, 0, curStringBufPos);
            stringBuf = newStringBuf;
            curStringBufWatermark = stringBuf.length - 20;
        }
        stringBuf[curStringBufPos++] = (byte) ((len >> 8) & 255);
        stringBuf[curStringBufPos++] = (byte) (len & 255);
        System.arraycopy(sb, 0, stringBuf, curStringBufPos, len);
        curStringBufPos += len;
    }

    private void writeStringRef(String s) {
        int stringRef = stringHashTable.get(s);
        if (stringRef == -1) {
            stringHashTable.add(s, stringCount);
            stringRef = stringCount;
            writeString(s);
            stringCount++;
        }
        writeInt(stringRef);
    }

    /** Maps Strings to integer numbers (their positions in String table) */
    static class StringHashTable {

        String keys[];
        int values[];
        int size, nOfElements, watermark;

        StringHashTable(int size) {
            size = makeLikePrimeNumber(size);
            this.size = size;
            keys = new String[size];
            values = new int[size];
            nOfElements = 0;
            watermark = size * 3 / 4;
        }

        final int get(String key) {
            int pos = (key.hashCode() & 0x7FFFFFFF) % size;

            while (keys[pos] != null && !keys[pos].equals(key)) {
                pos = (pos + 3) % size; // Relies on the fact that size % 3 != 0
            }
            if (key.equals(keys[pos])) {
                return values[pos];
            } else {
                return -1;
            }
        }

        final void add(String key, int value) {
            if (nOfElements > watermark) {
                rehash();
            }

            int pos = (key.hashCode() & 0x7FFFFFFF) % size;
            while (keys[pos] != null) {
                pos = (pos + 3) % size;  // Relies on the fact that size % 3 != 0
            }
            keys[pos] = key;
            values[pos] = value;
            nOfElements++;
        }

        private final void rehash() {
            String oldKeys[] = keys;
            int oldValues[] = values;
            int oldSize = size;
            size = makeLikePrimeNumber(size * 3 / 2);
            keys = new String[size];
            values = new int[size];
            nOfElements = 0;
            watermark = size * 3 / 4;

            for (int i = 0; i < oldSize; i++) {
                if (oldKeys[i] != null) {
                    add(oldKeys[i], oldValues[i]);
                }
            }
        }

        private final int makeLikePrimeNumber(int no) {
            no = (no / 2) * 2 + 1;  // Make it an odd number
            // Find the nearest "approximately prime" number
            boolean prime = false;
            do {
                no += 2;
                prime =
                        (no % 3 != 0 && no % 5 != 0 && no % 7 != 0 && no % 11 != 0 &&
                        no % 13 != 0 && no % 17 != 0 && no % 19 != 0 && no % 23 != 0 &&
                        no % 29 != 0 && no % 31 != 0 && no % 37 != 0 && no % 41 != 0);
            } while (!prime);
            return no;
        }
    }
}
