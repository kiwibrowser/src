/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.processing;

import android.databinding.tool.store.Location;
import android.databinding.tool.util.StringUtils;

import java.util.List;

public class ScopedErrorReport {

    private final String mFilePath;

    private final List<Location> mLocations;

    /**
     * Only created by Scope
     */
    ScopedErrorReport(String filePath, List<Location> locations) {
        mFilePath = filePath;
        mLocations = locations;
    }

    public String getFilePath() {
        return mFilePath;
    }

    public List<Location> getLocations() {
        return mLocations;
    }

    public boolean isValid() {
        return StringUtils.isNotBlank(mFilePath);
    }

    public String toUserReadableString() {
        StringBuilder sb = new StringBuilder();
        if (mFilePath != null) {
            sb.append("File:");
            sb.append(mFilePath);
        }
        if (mLocations != null && mLocations.size() > 0) {
            if (mLocations.size() > 1) {
                sb.append("Locations:");
                for (Location location : mLocations) {
                    sb.append("\n    ").append(location.toUserReadableString());
                }
            } else {
                sb.append("\n    Location: ").append(mLocations.get(0).toUserReadableString());
            }
        }
        return sb.toString();
    }
}
