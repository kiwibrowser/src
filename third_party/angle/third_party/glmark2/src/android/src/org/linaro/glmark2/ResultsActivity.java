/*
 * Copyright © 2012 Linaro Limited
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Alexandros Frantzis
 */
package org.linaro.glmark2;

import java.io.*;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class ResultsActivity extends Activity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_results);
        TextView tv = (TextView) findViewById(R.id.results);

        File f = new File(getFilesDir(), "last_run.log");

        try {
            /* Read log from file */
            BufferedReader reader = new BufferedReader(new FileReader(f));
            String line = null;
            StringBuffer sb = new StringBuffer();

            while ((line = reader.readLine()) != null) {
                sb.append(line);
                sb.append('\n');
            }

            reader.close();
            tv.setText(sb.toString());
        }
        catch (Exception ex) {
        }
    }
}

