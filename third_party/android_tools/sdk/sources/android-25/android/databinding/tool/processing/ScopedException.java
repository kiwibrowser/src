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


import com.google.common.base.Joiner;
import com.google.common.base.Splitter;
import com.google.common.base.Strings;

import android.databinding.tool.store.Location;
import android.databinding.tool.util.L;
import android.databinding.tool.util.StringUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * An exception that contains scope information.
 */
public class ScopedException extends RuntimeException {
    public static final String ERROR_LOG_PREFIX = "****/ data binding error ****";
    public static final String ERROR_LOG_SUFFIX = "****\\ data binding error ****";
    public static final String MSG_KEY = "msg:";
    public static final String LOCATION_KEY = "loc:";
    public static final String FILE_KEY = "file:";
    private static boolean sEncodeOutput = false;
    private ScopedErrorReport mScopedErrorReport;
    private String mScopeLog;

    public ScopedException(String message, Object... args) {
        super(message == null ? "unknown data binding exception" :
                args.length == 0 ? message : String.format(message, args));
        mScopedErrorReport = Scope.createReport();
        mScopeLog = L.isDebugEnabled() ? Scope.produceScopeLog() : null;
    }

    ScopedException(String message, ScopedErrorReport scopedErrorReport) {
        super(message);
        mScopedErrorReport = scopedErrorReport;
    }

    public String getBareMessage() {
        return super.getMessage();
    }

    @Override
    public String getMessage() {
        return sEncodeOutput ? createEncodedMessage() : createHumanReadableMessage();
    }

    private String createHumanReadableMessage() {
        ScopedErrorReport scopedError = getScopedErrorReport();
        StringBuilder sb = new StringBuilder();
        sb.append(super.getMessage()).append("\n")
                .append("file://").append(scopedError.getFilePath());
        if (scopedError.getLocations() != null && scopedError.getLocations().size() > 0) {
            sb.append(" Line:");
            sb.append(scopedError.getLocations().get(0).startLine);
        }
        sb.append("\n");
        return sb.toString();
    }

    private String createEncodedMessage() {
        ScopedErrorReport scopedError = getScopedErrorReport();
        StringBuilder sb = new StringBuilder();
        sb.append(ERROR_LOG_PREFIX)
                .append(MSG_KEY).append(super.getMessage()).append("\n")
                .append(FILE_KEY).append(scopedError.getFilePath()).append("\n");
        if (scopedError.getLocations() != null) {
            for (Location location : scopedError.getLocations()) {
                sb.append(LOCATION_KEY).append(location.toUserReadableString()).append("\n");
            }
        }
        sb.append(ERROR_LOG_SUFFIX);
        return Joiner.on(' ').join(Splitter.on(StringUtils.LINE_SEPARATOR).split(sb));
    }

    public ScopedErrorReport getScopedErrorReport() {
        return mScopedErrorReport;
    }

    public boolean isValid() {
        return mScopedErrorReport.isValid();
    }

    public static ScopedException createFromOutput(String output) {
        String message = "";
        String file = "";
        List<Location> locations = new ArrayList<Location>();
        int msgStart = output.indexOf(MSG_KEY);
        if (msgStart < 0) {
            message = output;
        } else {
            int fileStart = output.indexOf(FILE_KEY, msgStart + MSG_KEY.length());
            if (fileStart < 0) {
                message = output;
            } else {
                message = output.substring(msgStart + MSG_KEY.length(), fileStart);
                int locStart = output.indexOf(LOCATION_KEY, fileStart + FILE_KEY.length());
                if (locStart < 0) {
                    file = output.substring(fileStart + FILE_KEY.length());
                } else {
                    file = output.substring(fileStart + FILE_KEY.length(), locStart);
                    int nextLoc = 0;
                    while(nextLoc >= 0) {
                        nextLoc = output.indexOf(LOCATION_KEY, locStart + LOCATION_KEY.length());
                        Location loc;
                        if (nextLoc < 0) {
                            loc = Location.fromUserReadableString(
                                    output.substring(locStart + LOCATION_KEY.length()));
                        } else {
                            loc = Location.fromUserReadableString(
                                    output.substring(locStart + LOCATION_KEY.length(), nextLoc));
                        }
                        if (loc != null && loc.isValid()) {
                            locations.add(loc);
                        }
                        locStart = nextLoc;
                    }
                }
            }
        }
        return new ScopedException(message.trim(),
                new ScopedErrorReport(Strings.isNullOrEmpty(file) ? null : file.trim(), locations));
    }

    public static List<ScopedException> extractErrors(String output) {
        List<ScopedException> errors = new ArrayList<ScopedException>();
        int index = output.indexOf(ERROR_LOG_PREFIX);
        final int limit = output.length();
        while (index >= 0 && index < limit) {
            int end = output.indexOf(ERROR_LOG_SUFFIX, index + ERROR_LOG_PREFIX.length());
            if (end == -1) {
                errors.add(createFromOutput(output.substring(index + ERROR_LOG_PREFIX.length())));
                break;
            } else {
                errors.add(createFromOutput(output.substring(index + ERROR_LOG_PREFIX.length(),
                        end)));
                index = output.indexOf(ERROR_LOG_PREFIX, end + ERROR_LOG_SUFFIX.length());
            }
        }
        return errors;
    }

    public static void encodeOutput(boolean encodeOutput) {
        sEncodeOutput = encodeOutput;
    }

    public static boolean issEncodeOutput() {
        return sEncodeOutput;
    }
}
