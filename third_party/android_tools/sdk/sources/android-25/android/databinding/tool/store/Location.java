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

package android.databinding.tool.store;

import org.antlr.v4.runtime.ParserRuleContext;
import org.antlr.v4.runtime.Token;

import android.databinding.tool.processing.scopes.LocationScopeProvider;
import android.databinding.tool.util.StringUtils;

import java.util.Collections;
import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlElement;

/**
 * Identifies the range of a code block inside a file or a string.
 * Note that, unlike antlr4 tokens, the line positions start from 0 (to be compatible with Studio).
 * <p>
 * Both start and end line/column indices are inclusive.
 */
@XmlAccessorType(XmlAccessType.NONE)
public class Location {
    public static final int NaN = -1;
    @XmlAttribute(name = "startLine")
    public int startLine;
    @XmlAttribute(name = "startOffset")
    public int startOffset;
    @XmlAttribute(name = "endLine")
    public int endLine;
    @XmlAttribute(name = "endOffset")
    public int endOffset;
    @XmlElement(name = "parentLocation")
    public Location parentLocation;

    // for XML unmarshalling
    public Location() {
        startOffset = endOffset = startLine = endLine = NaN;
    }

    public Location(Location other) {
        startOffset = other.startOffset;
        endOffset = other.endOffset;
        startLine = other.startLine;
        endLine = other.endLine;
    }

    public Location(Token start, Token end) {
        if (start == null) {
            startLine = startOffset = NaN;
        } else {
            startLine = start.getLine() - 1; //token lines start from 1
            startOffset = start.getCharPositionInLine();
        }

        if (end == null) {
            endLine = endOffset = NaN;
        } else {
            endLine = end.getLine() - 1; // token lines start from 1
            String endText = end.getText();
            int lastLineStart = endText.lastIndexOf(StringUtils.LINE_SEPARATOR);
            String lastLine = lastLineStart < 0 ? endText : endText.substring(lastLineStart + 1);
            endOffset = end.getCharPositionInLine() + lastLine.length() - 1;//end is inclusive
        }
    }

    public Location(ParserRuleContext context) {
        this(context == null ? null : context.getStart(),
                context == null ? null : context.getStop());
    }

    public Location(int startLine, int startOffset, int endLine, int endOffset) {
        this.startOffset = startOffset;
        this.startLine = startLine;
        this.endLine = endLine;
        this.endOffset = endOffset;
    }

    @Override
    public String toString() {
        return "Location{" +
                "startLine=" + startLine +
                ", startOffset=" + startOffset +
                ", endLine=" + endLine +
                ", endOffset=" + endOffset +
                ", parentLocation=" + parentLocation +
                '}';
    }

    public void setParentLocation(Location parentLocation) {
        this.parentLocation = parentLocation;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        Location location = (Location) o;

        if (endLine != location.endLine) {
            return false;
        }
        if (endOffset != location.endOffset) {
            return false;
        }
        if (startLine != location.startLine) {
            return false;
        }
        if (startOffset != location.startOffset) {
            return false;
        }
        return !(parentLocation != null ? !parentLocation.equals(location.parentLocation)
                : location.parentLocation != null);

    }

    @Override
    public int hashCode() {
        int result = startLine;
        result = 31 * result + startOffset;
        result = 31 * result + endLine;
        result = 31 * result + endOffset;
        return result;
    }

    public boolean isValid() {
        return startLine != NaN && endLine != NaN && startOffset != NaN && endOffset != NaN;
    }

    public boolean contains(Location other) {
        if (startLine > other.startLine) {
            return false;
        }
        if (startLine == other.startLine && startOffset > other.startOffset) {
            return false;
        }
        if (endLine < other.endLine) {
            return false;
        }
        return !(endLine == other.endLine && endOffset < other.endOffset);
    }

    private Location getValidParentAbsoluteLocation() {
        if (parentLocation == null) {
            return null;
        }
        if (parentLocation.isValid()) {
            return parentLocation.toAbsoluteLocation();
        }
        return parentLocation.getValidParentAbsoluteLocation();
    }

    public Location toAbsoluteLocation() {
        Location absoluteParent = getValidParentAbsoluteLocation();
        if (absoluteParent == null) {
            return this;
        }
        Location copy = new Location(this);
        boolean sameLine = copy.startLine == copy.endLine;
        if (copy.startLine == 0) {
            copy.startOffset += absoluteParent.startOffset;
        }
        if (sameLine) {
            copy.endOffset += absoluteParent.startOffset;
        }

        copy.startLine += absoluteParent.startLine;
        copy.endLine += absoluteParent.startLine;
        return copy;
    }

    public String toUserReadableString() {
        return startLine + ":" + startOffset + " - " + endLine + ":" + endOffset;
    }

    public static Location fromUserReadableString(String str) {
        int glue = str.indexOf('-');
        if (glue == -1) {
            return new Location();
        }
        String start = str.substring(0, glue);
        String end = str.substring(glue + 1);
        int[] point = new int[]{-1, -1};
        Location location = new Location();
        parsePoint(start, point);
        location.startLine = point[0];
        location.startOffset = point[1];
        point[0] = point[1] = -1;
        parsePoint(end, point);
        location.endLine = point[0];
        location.endOffset = point[1];
        return location;
    }

    private static boolean parsePoint(String content, int[] into) {
        int index = content.indexOf(':');
        if (index == -1) {
            return false;
        }
        into[0] = Integer.parseInt(content.substring(0, index).trim());
        into[1] = Integer.parseInt(content.substring(index + 1).trim());
        return true;
    }

    public LocationScopeProvider createScope() {
        return new LocationScopeProvider() {
            @Override
            public List<Location> provideScopeLocation() {
                return Collections.singletonList(Location.this);
            }
        };
    }
}
