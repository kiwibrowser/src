/* Copyright (c) 2002-2013 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;
import java.io.Writer;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;


/**
 * This class implements writing a text stream representing a project database.
 *
 * @see TextProjectDatabaseReader for details.
 *
 * @author  Benjy Weinberger
 * 13 January 2013
 */
public class TextProjectDatabaseWriter {
    private static Set<String> primitives = new LinkedHashSet<String>(
        Arrays.asList("boolean", "byte", "char", "double", "float", "int", "long", "short",
                      "Z", "B", "C", "D", "F", "I", "J", "S"));

    private ByteArrayOutputStream baos = new ByteArrayOutputStream();  // Reusable temp buffer.

    public void writeProjectDatabaseToFile(File outfile, Map<String, PCDEntry> pcd) {
        try {
            Writer out = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(outfile), "UTF-8"));
            try {
                writeProjectDatabase(out, pcd);
            } finally {
                out.close();
            }
        } catch (FileNotFoundException e) {
            throw new PrivateException(e);
        } catch (UnsupportedEncodingException e) {
            throw new PrivateException(e);
        } catch (IOException e) {
            throw new PrivateException(e);
        }
    }

	public void writeProjectDatabase(Writer out, Map<String,PCDEntry> pcd) {
        try {
            out.write("pcd entries:\n");
            out.write(Integer.toString(pcd.size()));
            out.write(" items\n");
            Map<String, Set<String>> depsBySource = new LinkedHashMap<String, Set<String>>();
            for (PCDEntry entry : pcd.values()) {
                writePCDEntry(out, entry);
                Set<String> deps = depsBySource.get(entry.javaFileFullPath);
                if (deps == null) {
                    deps = new LinkedHashSet<String>();
                    depsBySource.put(entry.javaFileFullPath, deps);
                }
                addDepsFromClassInfo(deps, entry.oldClassInfo);
            }
            // Write out dependency information. Note that we don't need to read this back to recreate
            // the PCD. We write it out here just as a convenience, so that external readers of the PDB
            // file don't have to grok our internal ClassInfo structures.
            out.write("dependencies:\n");
            out.write(Integer.toString(depsBySource.size()));
            out.write(" items\n");
            for (Map.Entry<String, Set<String>> item : depsBySource.entrySet()) {
                out.write(item.getKey());
                for (String s : item.getValue()) {
                    out.write('\t');
                    out.write(s);
                }
                out.write('\n');
            }
        } catch (IOException e) {
            throw new PrivateException(e);
        }
	}

    private void addDepsFromClassInfo(Set<String> deps, ClassInfo ci) {
        for (String s : ci.cpoolRefsToClasses) {
            int i = 0;
            int j = s.length();

            // Fix some inconsistencies in how we represent types internally:
            // Despite the comment on ci.cpoolRefsToClasses, class names may be
            // representing in it with '['s and with '@', '#' instead of 'L', ';'.
            while (s.charAt(i) == '[') i++;
            if (s.charAt(i) == '@') i++;
            if (s.endsWith("#")) j--;
            int k = s.indexOf('$');

            // Take the outer class, on references to nested classes.
            if (k != -1) j = k;
            if (i > 0 || j < s.length())
                s = s.substring(i, j);

            // We don't need to record deps on primitive types, or arrays of them.
            if (!primitives.contains(s))
                deps.add(s);
        }
    }

	private void writePCDEntry(Writer out, PCDEntry entry) {
        try {
            out.write(entry.className);
            out.write('\t');
            out.write(entry.javaFileFullPath);
            out.write('\t');
            out.write(Long.toString(entry.oldClassFileLastModified));
            out.write('\t');
            out.write(Long.toString(entry.oldClassFileFingerprint));
            out.write('\t');
            out.write(classInfoToBase64(entry.oldClassInfo));
            out.write('\n');
        } catch (IOException e) {
            throw new PrivateException(e);
        }
	}

	private char[] classInfoToBase64(ClassInfo ci) {
        baos.reset();
        try {
            ObjectOutputStream oos = new ObjectOutputStream(baos);
            oos.writeObject(ci);
            oos.close();
        } catch (IOException e) {
            throw new PrivateException(e);
        }
        return Base64.encode(baos.toByteArray());
	}
}
