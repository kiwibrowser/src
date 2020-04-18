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
 * This class is a persistent container for the Project Class Directory, that can
 * read and write itself from/to disk.
 *
 * @author Misha Dmitriev
 *  12 November 2001
 */
public class PCDContainer {

    /** The data structure (currently {@link LinkedHashMap}) for PCD, that maps class name to
    record containing information about the class */
    Map<String,PCDEntry> pcd;
    String storeName;
    boolean textFormat;

    private PCDContainer(Map<String,PCDEntry> pcd, String storeName, boolean textFormat) {
        this.storeName = storeName;
        this.pcd = pcd;
        this.textFormat = textFormat;
    }

    public static PCDContainer load(String storeName, boolean textFormat) {
        if (storeName == null) {
            storeName = Main.DEFAULT_STORE_NAME;
        }
        File storeFile = Utils.checkFileForName(storeName);
        if (storeFile != null) {
            Utils.printInfoMessageNoEOL("Opening project database...  ");
            Map<String,PCDEntry> pcd;
            if (textFormat) {
                pcd = new TextProjectDatabaseReader().readProjectDatabaseFromFile(storeFile);
            } else {
                pcd = new BinaryProjectDatabaseReader().readProjectDatabaseFromFile(storeFile);
            }
            PCDContainer pcdc = new PCDContainer(pcd, storeName, textFormat);
            Utils.printInfoMessage("Done.");
            return pcdc;
        }
        return new PCDContainer(null, storeName, textFormat);
    }

    public void save() {
        Utils.printInfoMessageNoEOL("Writing project database...  ");
        File outfile = new File(storeName);
        if (textFormat) {
            new TextProjectDatabaseWriter().writeProjectDatabaseToFile(outfile, pcd);
        } else {
            new BinaryProjectDatabaseWriter().writeProjectDatabaseToFile(outfile, pcd);
        }
        Utils.printInfoMessage("Done.");
    }
}
