/* Copyright (c) 2002-2013 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.ObjectInputStream;
import java.io.UnsupportedEncodingException;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


/**
 * This class creates the internal representation of the project database from a text buffer.
 *
 * The Pants build tool manipulates this data in various ways, and it's easiest for it
 * to do so by parsing text files directly.  This brings JMake into line with Zinc (the
 * Scala incremental compiler) and allows Pants to handle both uniformly.
 *
 * @author  Benjy Weinberger
 * 13 January 2013
 */
public class TextProjectDatabaseReader {
    public Map<String,PCDEntry> readProjectDatabaseFromFile(File infile) {
        try {
            BufferedReader in =
                new BufferedReader(new InputStreamReader(new FileInputStream(infile), "UTF-8"));
            try {
                return readProjectDatabase(in);
            } finally {
                in.close();
            }
        } catch (FileNotFoundException e) {
            throw new PrivateException(e);
        } catch (UnsupportedEncodingException e) {
            throw new PrivateException(e);
        } catch (IOException e) {
            throw new PrivateException(e);
        }
    }

    public Map<String,PCDEntry> readProjectDatabase(BufferedReader in) {
        Map<String,PCDEntry> pcd;
        try {
            String line = in.readLine();
            if (!"pcd entries:".equals(line))
                throw error("Expected: 'pcd entries:', got: " + line);
            line = in.readLine();
            Matcher m = Pattern.compile("^(\\d+) items$").matcher(line);
            if (!m.matches())
                throw error("Expected: '<n> items', got: " + line);
            int numEntries = Integer.parseInt(m.group(1));
            pcd = new LinkedHashMap<String, PCDEntry>(numEntries);
            for (int i = 0; i < numEntries; i++) {
                line = in.readLine();
                if (line == null)
                    throw error("Unexpected EOF");
                String[] parts = line.split("\t");
                if (parts.length != 5) {
                    throw error("Invalid line: " + line);
                }
                String className = parts[0];
                String javaFullFilePath = parts[1];
                long oldClassFileLastModified = Long.parseLong(parts[2]);
                long oldClassFileFingerprint = Long.parseLong(parts[3]);
                ClassInfo ci = classInfoFromBase64(parts[4]);
                PCDEntry entry = new PCDEntry(className, javaFullFilePath, oldClassFileLastModified,
                    oldClassFileFingerprint, ci);
                pcd.put(entry.className, entry);
            }
            // We're done: We have detailed dep information in the PCD entries, so we don't
            // need to read the dep information lines from the file.
        } catch (IOException e) {
            throw new PrivateException(e);
        }
        return pcd;
    }

    private PrivateException error(String msg) {
        return new PrivateException(new IllegalArgumentException(msg));
    }

    private ClassInfo classInfoFromBase64(String s) {
        try {
            byte[] bytes = Base64.decode(s.toCharArray());
            ObjectInputStream ois = new ObjectInputStream(new ByteArrayInputStream(bytes));
            ClassInfo ret = (ClassInfo)ois.readObject();
            ret.initializeImmediateTransientFields();
            return ret;
        } catch (IOException e) {
            throw new PrivateException(e);
        } catch (ClassNotFoundException e) {
            throw new PrivateException(e);
        }
    }
}
