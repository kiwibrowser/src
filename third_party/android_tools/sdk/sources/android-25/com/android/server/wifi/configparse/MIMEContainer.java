package com.android.server.wifi.configparse;

import android.util.Log;

import com.android.server.wifi.hotspot2.Utils;

import java.io.IOException;
import java.io.LineNumberReader;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

public class MIMEContainer {
    private static final String Type = "Content-Type";
    private static final String Encoding = "Content-Transfer-Encoding";

    private static final String Boundary = "boundary=";
    private static final String CharsetTag = "charset=";

    private final boolean mLast;
    private final List<MIMEContainer> mMimeContainers;
    private final String mText;

    private final boolean mMixed;
    private final boolean mBase64;
    private final Charset mCharset;
    private final String mContentType;

    /**
     * Parse nested MIME content
     * @param in A reader to read MIME data from; Note that the charset should be ISO-8859-1 to
     *           ensure transparent octet to character mapping. This is because the content will
     *           be re-encoded using the correct charset once it is discovered.
     * @param boundary A boundary string for the MIME section that this container is in.
     *                 Pass null for the top level object.
     * @throws java.io.IOException
     */
    public MIMEContainer(LineNumberReader in, String boundary) throws IOException {
        Map<String,List<String>> headers = parseHeader(in);

        List<String> type = headers.get(Type);
        if (type == null || type.isEmpty()) {
            throw new IOException("Missing " + Type + " @ " + in.getLineNumber());
        }

        boolean multiPart = false;
        boolean mixed = false;
        String subBoundary = null;
        Charset charset = StandardCharsets.ISO_8859_1;

        mContentType = type.get(0);

        if (mContentType.startsWith("multipart/")) {
            multiPart = true;

            for (String attribute : type) {
                if (attribute.startsWith(Boundary)) {
                    subBoundary = Utils.unquote(attribute.substring(Boundary.length()));
                }
            }

            if (mContentType.endsWith("/mixed")) {
                mixed = true;
            }
        }
        else if (mContentType.startsWith("text/")) {
            for (String attribute : type) {
                if (attribute.startsWith(CharsetTag)) {
                    charset = Charset.forName(attribute.substring(CharsetTag.length()));
                }
            }
        }

        mMixed = mixed;
        mCharset = charset;

        if (multiPart && subBoundary != null) {
            for (;;) {
                String line = in.readLine();
                if (line == null) {
                    throw new IOException("Unexpected EOF before first boundary @ " +
                            in.getLineNumber());
                }
                if (line.startsWith("--") && line.length() == subBoundary.length() + 2 &&
                        line.regionMatches(2, subBoundary, 0, subBoundary.length())) {
                    break;
                }
            }

            mMimeContainers = new ArrayList<>();
            for (;;) {
                MIMEContainer container = new MIMEContainer(in, subBoundary);
                mMimeContainers.add(container);
                if (container.isLast()) {
                    break;
                }
            }
        }
        else {
            mMimeContainers = null;
        }

        List<String> encoding = headers.get(Encoding);
        boolean quoted = false;
        boolean base64 = false;
        if (encoding != null) {
            for (String text : encoding) {
                if (text.equalsIgnoreCase("quoted-printable")) {
                    quoted = true;
                    break;
                }
                else if (text.equalsIgnoreCase("base64")) {
                    base64 = true;
                    break;
                }
            }
        }
        mBase64 = base64;

        Log.d(Utils.hs2LogTag(getClass()),
                String.format("%s MIME container, boundary '%s', type '%s', encoding %s",
                multiPart ? "multipart" : "plain", boundary, mContentType, encoding));

        AtomicBoolean eof = new AtomicBoolean();
        mText = recode(getBody(in, boundary, quoted, eof), charset);
        mLast = eof.get();
    }

    public List<MIMEContainer> getMimeContainers() {
        return mMimeContainers;
    }

    public String getText() {
        return mText;
    }

    public boolean isMixed() {
        return mMixed;
    }

    public boolean isBase64() {
        return mBase64;
    }

    public String getContentType() {
        return mContentType;
    }

    private boolean isLast() {
        return mLast;
    }

    private void toString(StringBuilder sb, int nesting) {
        char[] indent = new char[nesting*4];
        Arrays.fill(indent, ' ');
        if (mBase64) {
            sb.append("base64, type ").append(mContentType).append('\n');
        }
        else if (mMimeContainers != null) {
            sb.append(indent).append("multipart/").append((mMixed ? "mixed" : "other" )).append('\n');
        }
        else {
            sb.append(indent).append(
                    String.format("%s, type %s",
                            mCharset,
                            mContentType)
            ).append('\n');
        }

        if (mMimeContainers != null) {
            for (MIMEContainer mimeContainer : mMimeContainers) {
                mimeContainer.toString(sb, nesting + 1);
            }
        }
        sb.append(indent).append("Text: ");
        if (mText.length() < 100000) {
            sb.append("'").append(mText).append("'\n");
        }
        else {
            sb.append(mText.length()).append(" chars\n");
        }
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        toString(sb, 0);
        return sb.toString();
    }

    private static Map<String,List<String>> parseHeader(LineNumberReader in) throws IOException {

        StringBuilder value = null;
        String header = null;

        Map<String,List<String>> headers = new HashMap<>();

        for (;;) {
            String line = in.readLine();
            if ( line == null ) {
                throw new IOException("Missing body @ " + in.getLineNumber());
            }
            else if (line.length() == 0) {
                break;
            }

            if (line.charAt(0) <= ' ') {
                if (value == null) {
                    throw new IOException("Illegal blank prefix in header line '" + line + "' @ " + in.getLineNumber());
                }
                value.append(' ').append(line.trim());
                continue;
            }

            int nameEnd = line.indexOf(':');
            if (nameEnd < 0) {
                throw new IOException("Bad header line: '" + line + "' @ " + in.getLineNumber());
            }

            if (header != null) {
                String[] values = value.toString().split(";");
                List<String> valueList = new ArrayList<>(values.length);
                for (String segment : values) {
                    valueList.add(segment.trim());
                }
                headers.put(header, valueList);
                //System.out.println("Header '" + header + "' = " + valueList);
            }

            header = line.substring(0, nameEnd);
            value = new StringBuilder();
            value.append(line.substring(nameEnd+1).trim());
        }

        if (header != null) {
            String[] values = value.toString().split(";");
            List<String> valueList = new ArrayList<>(values.length);
            for (String segment : values) {
                valueList.add(segment.trim());
            }
            headers.put(header, valueList);
            //System.out.println("Header '" + header + "' = " + valueList);
        }

        return headers;
    }

    private static String getBody(LineNumberReader in, String boundary, boolean quoted, AtomicBoolean eof)
            throws IOException {

        StringBuilder text = new StringBuilder();
        for (;;) {
            String line = in.readLine();
            if (line == null) {
                if (boundary != null) {
                    throw new IOException("Unexpected EOF file in body @ " + in.getLineNumber());
                }
                else {
                    return text.toString();
                }
            }
            Boolean end = boundaryCheck(line, boundary);
            if (end != null) {
                eof.set(end);
                //System.out.println("Boundary " + boundary + ": " + end);
                return text.toString();
            }

            if (quoted) {
                if (line.endsWith("=")) {
                    text.append(unescape(line.substring(line.length() - 1), in.getLineNumber()));
                }
                else {
                    text.append(unescape(line, in.getLineNumber()));
                }
            }
            else {
                text.append(line);
            }
        }
    }

    private static String recode(String s, Charset charset) {
        if (charset.equals(StandardCharsets.ISO_8859_1) || charset.equals(StandardCharsets.US_ASCII)) {
            return s;
        }

        byte[] octets = s.getBytes(StandardCharsets.ISO_8859_1);
        return new String(octets, charset);
    }

    private static Boolean boundaryCheck(String line, String boundary) {
        if (line.startsWith("--") && line.regionMatches(2, boundary, 0, boundary.length())) {
            if (line.length() == boundary.length() + 2) {
                return Boolean.FALSE;
            }
            else if (line.length() == boundary.length() + 4 && line.endsWith("--") ) {
                return Boolean.TRUE;
            }
        }
        return null;
    }

    private static String unescape(String text, int line) throws IOException {
        StringBuilder sb = new StringBuilder();
        for (int n = 0; n < text.length(); n++) {
            char ch = text.charAt(n);
            if (ch > 127) {
                throw new IOException("Bad codepoint " + (int)ch + " in quoted printable @ " + line);
            }
            if (ch == '=' && n < text.length() - 2) {
                int h1 = fromStrictHex(text.charAt(n+1));
                int h2 = fromStrictHex(text.charAt(n+2));
                if (h1 >= 0 && h2 >= 0) {
                    sb.append((char)((h1 << 4) | h2));
                    n += 2;
                }
                else {
                    sb.append(ch);
                }
            }
            else {
                sb.append(ch);
            }
        }
        return sb.toString();
    }

    private static int fromStrictHex(char ch) {
        if (ch >= '0' && ch <= '9') {
            return ch - '0';
        }
        else if (ch >= 'A' && ch <= 'F') {
            return ch - 'A' + 10;
        }
        else {
            return -1;
        }
    }
}
