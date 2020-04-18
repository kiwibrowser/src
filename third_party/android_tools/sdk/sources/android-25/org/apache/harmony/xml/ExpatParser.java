/*
 * Copyright (C) 2007 The Android Open Source Project
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

package org.apache.harmony.xml;

import java.io.IOException;
import java.io.InputStream;
import java.io.Reader;
import java.net.URI;
import java.net.URL;
import java.net.URLConnection;
import libcore.io.IoUtils;
import org.xml.sax.Attributes;
import org.xml.sax.ContentHandler;
import org.xml.sax.DTDHandler;
import org.xml.sax.EntityResolver;
import org.xml.sax.InputSource;
import org.xml.sax.Locator;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;
import org.xml.sax.ext.LexicalHandler;

/**
 * Adapts SAX API to the Expat native XML parser. Not intended for reuse
 * across documents.
 *
 * @see org.apache.harmony.xml.ExpatReader
 */
class ExpatParser {

    private static final int BUFFER_SIZE = 8096; // in bytes

    /** Pointer to XML_Parser instance. */
    private long pointer;

    private boolean inStartElement = false;
    private int attributeCount = -1;
    private long attributePointer = 0;

    private final Locator locator = new ExpatLocator();

    private final ExpatReader xmlReader;

    private final String publicId;
    private final String systemId;

    private final String encoding;

    private final ExpatAttributes attributes = new CurrentAttributes();

    private static final String OUTSIDE_START_ELEMENT
            = "Attributes can only be used within the scope of startElement().";

    /** We default to UTF-8 when the user doesn't specify an encoding. */
    private static final String DEFAULT_ENCODING = "UTF-8";

    /** Encoding used for Java chars, used to parse Readers and Strings */
    /*package*/ static final String CHARACTER_ENCODING = "UTF-16";

    /** Timeout for HTTP connections (in ms) */
    private static final int TIMEOUT = 20 * 1000;

    /**
     * Constructs a new parser with the specified encoding.
     */
    /*package*/ ExpatParser(String encoding, ExpatReader xmlReader,
            boolean processNamespaces, String publicId, String systemId) {
        this.publicId = publicId;
        this.systemId = systemId;

        this.xmlReader = xmlReader;

        /*
         * TODO: Let Expat try to guess the encoding instead of defaulting.
         * Unfortunately, I don't know how to tell which encoding Expat picked,
         * so I won't know how to encode "<externalEntity>" below. The solution
         * I think is to fix Expat to not require the "<externalEntity>"
         * workaround.
         */
        this.encoding = encoding == null ? DEFAULT_ENCODING : encoding;
        this.pointer = initialize(
            this.encoding,
            processNamespaces
        );
    }

    /**
     * Used by {@link EntityParser}.
     */
    private ExpatParser(String encoding, ExpatReader xmlReader, long pointer,
            String publicId, String systemId) {
        this.encoding = encoding;
        this.xmlReader = xmlReader;
        this.pointer = pointer;
        this.systemId = systemId;
        this.publicId = publicId;
    }

    /**
     * Initializes native resources.
     *
     * @return the pointer to the native parser
     */
    private native long initialize(String encoding, boolean namespacesEnabled);

    /**
     * Called at the start of an element.
     *
     * @param uri namespace URI of element or "" if namespace processing is
     *  disabled
     * @param localName local name of element or "" if namespace processing is
     *  disabled
     * @param qName qualified name or "" if namespace processing is enabled
     * @param attributePointer pointer to native attribute char*--we keep
     *  a separate pointer so we can detach it from the parser instance
     * @param attributeCount number of attributes
     */
    /*package*/ void startElement(String uri, String localName, String qName,
            long attributePointer, int attributeCount) throws SAXException {
        ContentHandler contentHandler = xmlReader.contentHandler;
        if (contentHandler == null) {
            return;
        }

        try {
            inStartElement = true;
            this.attributePointer = attributePointer;
            this.attributeCount = attributeCount;

            contentHandler.startElement(
                    uri, localName, qName, this.attributes);
        } finally {
            inStartElement = false;
            this.attributeCount = -1;
            this.attributePointer = 0;
        }
    }

    /*package*/ void endElement(String uri, String localName, String qName)
            throws SAXException {
        ContentHandler contentHandler = xmlReader.contentHandler;
        if (contentHandler != null) {
            contentHandler.endElement(uri, localName, qName);
        }
    }

    /*package*/ void text(char[] text, int length) throws SAXException {
        ContentHandler contentHandler = xmlReader.contentHandler;
        if (contentHandler != null) {
            contentHandler.characters(text, 0, length);
        }
    }

    /*package*/ void comment(char[] text, int length) throws SAXException {
        LexicalHandler lexicalHandler = xmlReader.lexicalHandler;
        if (lexicalHandler != null) {
            lexicalHandler.comment(text, 0, length);
        }
    }

    /*package*/ void startCdata() throws SAXException {
        LexicalHandler lexicalHandler = xmlReader.lexicalHandler;
        if (lexicalHandler != null) {
            lexicalHandler.startCDATA();
        }
    }

    /*package*/ void endCdata() throws SAXException {
        LexicalHandler lexicalHandler = xmlReader.lexicalHandler;
        if (lexicalHandler != null) {
            lexicalHandler.endCDATA();
        }
    }

    /*package*/ void startNamespace(String prefix, String uri)
            throws SAXException {
        ContentHandler contentHandler = xmlReader.contentHandler;
        if (contentHandler != null) {
            contentHandler.startPrefixMapping(prefix, uri);
        }
    }

    /*package*/ void endNamespace(String prefix) throws SAXException {
        ContentHandler contentHandler = xmlReader.contentHandler;
        if (contentHandler != null) {
            contentHandler.endPrefixMapping(prefix);
        }
    }

    /*package*/ void startDtd(String name, String publicId, String systemId)
            throws SAXException {
        LexicalHandler lexicalHandler = xmlReader.lexicalHandler;
        if (lexicalHandler != null) {
            lexicalHandler.startDTD(name, publicId, systemId);
        }
    }

    /*package*/ void endDtd() throws SAXException {
        LexicalHandler lexicalHandler = xmlReader.lexicalHandler;
        if (lexicalHandler != null) {
            lexicalHandler.endDTD();
        }
    }

    /*package*/ void processingInstruction(String target, String data)
            throws SAXException {
        ContentHandler contentHandler = xmlReader.contentHandler;
        if (contentHandler != null) {
            contentHandler.processingInstruction(target, data);
        }
    }

    /*package*/ void notationDecl(String name, String publicId, String systemId) throws SAXException {
        DTDHandler dtdHandler = xmlReader.dtdHandler;
        if (dtdHandler != null) {
            dtdHandler.notationDecl(name, publicId, systemId);
        }
    }

    /*package*/ void unparsedEntityDecl(String name, String publicId, String systemId, String notationName) throws SAXException {
        DTDHandler dtdHandler = xmlReader.dtdHandler;
        if (dtdHandler != null) {
            dtdHandler.unparsedEntityDecl(name, publicId, systemId, notationName);
        }
    }

    /**
     * Handles an external entity.
     *
     * @param context to be passed back to Expat when we parse the entity
     * @param publicId the publicId of the entity
     * @param systemId the systemId of the entity
     */
    /*package*/ void handleExternalEntity(String context, String publicId,
            String systemId) throws SAXException, IOException {
        EntityResolver entityResolver = xmlReader.entityResolver;
        if (entityResolver == null) {
            return;
        }

        /*
         * The spec. is terribly under-specified here. It says that if the
         * systemId is a URL, we should try to resolve it, but it doesn't
         * specify how to tell whether or not the systemId is a URL let alone
         * how to resolve it.
         *
         * Other implementations do various insane things. We try to keep it
         * simple: if the systemId parses as a URI and it's relative, we try to
         * resolve it against the parent document's systemId. If anything goes
         * wrong, we go with the original systemId. If crazybob had designed
         * the API, he would have left all resolving to the EntityResolver.
         */
        if (this.systemId != null) {
            try {
                URI systemUri = new URI(systemId);
                if (!systemUri.isAbsolute() && !systemUri.isOpaque()) {
                    // It could be relative (or it may not be a URI at all!)
                    URI baseUri = new URI(this.systemId);
                    systemUri = baseUri.resolve(systemUri);

                    // Replace systemId w/ resolved URI
                    systemId = systemUri.toString();
                }
            } catch (Exception e) {
                System.logI("Could not resolve '" + systemId + "' relative to"
                        + " '" + this.systemId + "' at " + locator, e);
            }
        }

        InputSource inputSource = entityResolver.resolveEntity(
                publicId, systemId);
        if (inputSource == null) {
            /*
             * The spec. actually says that we should try to treat systemId
             * as a URL and download and parse its contents here, but an
             * entity resolver can easily accomplish the same by returning
             * new InputSource(systemId).
             *
             * Downloading external entities by default would result in several
             * unwanted DTD downloads, not to mention pose a security risk
             * when parsing untrusted XML -- see for example
             * http://archive.cert.uni-stuttgart.de/bugtraq/2002/10/msg00421.html --
             * so we just do nothing instead. This also enables the user to
             * opt out of entity parsing when using
             * {@link org.xml.sax.helpers.DefaultHandler}, something that
             * wouldn't be possible otherwise.
             */
            return;
        }

        String encoding = pickEncoding(inputSource);
        long pointer = createEntityParser(this.pointer, context);
        try {
            EntityParser entityParser = new EntityParser(encoding, xmlReader,
                    pointer, inputSource.getPublicId(),
                    inputSource.getSystemId());

            parseExternalEntity(entityParser, inputSource);
        } finally {
            releaseParser(pointer);
        }
    }

    /**
     * Picks an encoding for an external entity. Defaults to UTF-8.
     */
    private String pickEncoding(InputSource inputSource) {
        Reader reader = inputSource.getCharacterStream();
        if (reader != null) {
            return CHARACTER_ENCODING;
        }

        String encoding = inputSource.getEncoding();
        return encoding == null ? DEFAULT_ENCODING : encoding;
    }

    /**
     * Parses the the external entity provided by the input source.
     */
    private void parseExternalEntity(ExpatParser entityParser,
            InputSource inputSource) throws IOException, SAXException {
        /*
         * Expat complains if the external entity isn't wrapped with a root
         * element so we add one and ignore it later on during parsing.
         */

        // Try the character stream.
        Reader reader = inputSource.getCharacterStream();
        if (reader != null) {
            try {
                entityParser.append("<externalEntity>");
                entityParser.parseFragment(reader);
                entityParser.append("</externalEntity>");
            } finally {
                IoUtils.closeQuietly(reader);
            }
            return;
        }

        // Try the byte stream.
        InputStream in = inputSource.getByteStream();
        if (in != null) {
            try {
                entityParser.append("<externalEntity>"
                        .getBytes(entityParser.encoding));
                entityParser.parseFragment(in);
                entityParser.append("</externalEntity>"
                        .getBytes(entityParser.encoding));
            } finally {
                IoUtils.closeQuietly(in);
            }
            return;
        }

        // Make sure we use the user-provided systemId.
        String systemId = inputSource.getSystemId();
        if (systemId == null) {
            // TODO: We could just try our systemId here.
            throw new ParseException("No input specified.", locator);
        }

        // Try the system id.
        in = openUrl(systemId);
        try {
            entityParser.append("<externalEntity>"
                    .getBytes(entityParser.encoding));
            entityParser.parseFragment(in);
            entityParser.append("</externalEntity>"
                    .getBytes(entityParser.encoding));
        } finally {
            IoUtils.closeQuietly(in);
        }
    }

    /**
     * Creates a native entity parser.
     *
     * @param parentPointer pointer to parent Expat parser
     * @param context passed to {@link #handleExternalEntity}
     * @return pointer to native parser
     */
    private static native long createEntityParser(long parentPointer, String context);

    /**
     * Appends part of an XML document. This parser will parse the given XML to
     * the extent possible and dispatch to the appropriate methods.
     *
     * @param xml a whole or partial snippet of XML
     * @throws SAXException if an error occurs during parsing
     */
    /*package*/ void append(String xml) throws SAXException {
        try {
            appendString(this.pointer, xml, false);
        } catch (ExpatException e) {
            throw new ParseException(e.getMessage(), this.locator);
        }
    }

    private native void appendString(long pointer, String xml, boolean isFinal)
            throws SAXException, ExpatException;

    /**
     * Appends part of an XML document. This parser will parse the given XML to
     * the extent possible and dispatch to the appropriate methods.
     *
     * @param xml a whole or partial snippet of XML
     * @param offset into the char[]
     * @param length of characters to use
     * @throws SAXException if an error occurs during parsing
     */
    /*package*/ void append(char[] xml, int offset, int length)
            throws SAXException {
        try {
            appendChars(this.pointer, xml, offset, length);
        } catch (ExpatException e) {
            throw new ParseException(e.getMessage(), this.locator);
        }
    }

    private native void appendChars(long pointer, char[] xml, int offset,
            int length) throws SAXException, ExpatException;

    /**
     * Appends part of an XML document. This parser will parse the given XML to
     * the extent possible and dispatch to the appropriate methods.
     *
     * @param xml a whole or partial snippet of XML
     * @throws SAXException if an error occurs during parsing
     */
    /*package*/ void append(byte[] xml) throws SAXException {
        append(xml, 0, xml.length);
    }

    /**
     * Appends part of an XML document. This parser will parse the given XML to
     * the extent possible and dispatch to the appropriate methods.
     *
     * @param xml a whole or partial snippet of XML
     * @param offset into the byte[]
     * @param length of bytes to use
     * @throws SAXException if an error occurs during parsing
     */
    /*package*/ void append(byte[] xml, int offset, int length)
            throws SAXException {
        try {
            appendBytes(this.pointer, xml, offset, length);
        } catch (ExpatException e) {
            throw new ParseException(e.getMessage(), this.locator);
        }
    }

    private native void appendBytes(long pointer, byte[] xml, int offset,
            int length) throws SAXException, ExpatException;

    /**
     * Parses an XML document from the given input stream.
     */
    /*package*/ void parseDocument(InputStream in) throws IOException,
            SAXException {
        startDocument();
        parseFragment(in);
        finish();
        endDocument();
    }

    /**
     * Parses an XML Document from the given reader.
     */
    /*package*/ void parseDocument(Reader in) throws IOException, SAXException {
        startDocument();
        parseFragment(in);
        finish();
        endDocument();
    }

    /**
     * Parses XML from the given Reader.
     */
    private void parseFragment(Reader in) throws IOException, SAXException {
        char[] buffer = new char[BUFFER_SIZE / 2];
        int length;
        while ((length = in.read(buffer)) != -1) {
            try {
                appendChars(this.pointer, buffer, 0, length);
            } catch (ExpatException e) {
                throw new ParseException(e.getMessage(), locator);
            }
        }
    }

    /**
     * Parses XML from the given input stream.
     */
    private void parseFragment(InputStream in)
            throws IOException, SAXException {
        byte[] buffer = new byte[BUFFER_SIZE];
        int length;
        while ((length = in.read(buffer)) != -1) {
            try {
                appendBytes(this.pointer, buffer, 0, length);
            } catch (ExpatException e) {
                throw new ParseException(e.getMessage(), this.locator);
            }
        }
    }

    private void startDocument() throws SAXException {
        ContentHandler contentHandler = xmlReader.contentHandler;
        if (contentHandler != null) {
            contentHandler.setDocumentLocator(this.locator);
            contentHandler.startDocument();
        }
    }

    private void endDocument() throws SAXException {
        ContentHandler contentHandler;
        contentHandler = xmlReader.contentHandler;
        if (contentHandler != null) {
            contentHandler.endDocument();
        }
    }

    /**
     * Indicate that we're finished parsing.
     *
     * @throws SAXException if the xml is incomplete
     */
    /*package*/ void finish() throws SAXException {
        try {
            appendString(this.pointer, "", true);
        } catch (ExpatException e) {
            throw new ParseException(e.getMessage(), this.locator);
        }
    }

    @Override protected synchronized void finalize() throws Throwable {
        try {
            if (this.pointer != 0) {
                release(this.pointer);
                this.pointer = 0;
            }
        } finally {
            super.finalize();
        }
    }

    /**
     * Releases all native objects.
     */
    private native void release(long pointer);

    /**
     * Releases native parser only.
     */
    private static native void releaseParser(long pointer);

    /**
     * Initialize static resources.
     */
    private static native void staticInitialize(String emptyString);

    static {
        staticInitialize("");
    }

    /**
     * Gets the current line number within the XML file.
     */
    private int line() {
        return line(this.pointer);
    }

    private static native int line(long pointer);

    /**
     * Gets the current column number within the XML file.
     */
    private int column() {
        return column(this.pointer);
    }

    private static native int column(long pointer);

    /**
     * Clones the current attributes so they can be used outside of
     * startElement().
     */
    /*package*/ Attributes cloneAttributes() {
        if (!inStartElement) {
            throw new IllegalStateException(OUTSIDE_START_ELEMENT);
        }

        if (attributeCount == 0) {
            return ClonedAttributes.EMPTY;
        }

        long clonePointer
                = cloneAttributes(this.attributePointer, this.attributeCount);
        return new ClonedAttributes(pointer, clonePointer, attributeCount);
    }

    private static native long cloneAttributes(long pointer, int attributeCount);

    /**
     * Used for cloned attributes.
     */
    private static class ClonedAttributes extends ExpatAttributes {

        private static final Attributes EMPTY = new ClonedAttributes(0, 0, 0);

        private final long parserPointer;
        private long pointer;
        private final int length;

        /**
         * Constructs a Java wrapper for native attributes.
         *
         * @param parserPointer pointer to the parse, can be 0 if length is 0.
         * @param pointer pointer to the attributes array, can be 0 if the
         *  length is 0.
         * @param length number of attributes
         */
        private ClonedAttributes(long parserPointer, long pointer, int length) {
            this.parserPointer = parserPointer;
            this.pointer = pointer;
            this.length = length;
        }

        @Override
        public long getParserPointer() {
            return this.parserPointer;
        }

        @Override
        public long getPointer() {
            return pointer;
        }

        @Override
        public int getLength() {
            return length;
        }

        @Override protected synchronized void finalize() throws Throwable {
            try {
                if (pointer != 0) {
                    freeAttributes(pointer);
                    pointer = 0;
                }
            } finally {
                super.finalize();
            }
        }
    }

    private class ExpatLocator implements Locator {

        public String getPublicId() {
            return publicId;
        }

        public String getSystemId() {
            return systemId;
        }

        public int getLineNumber() {
            return line();
        }

        public int getColumnNumber() {
            return column();
        }

        @Override
        public String toString() {
            return "Locator[publicId: " + publicId + ", systemId: " + systemId
                + ", line: " + getLineNumber()
                + ", column: " + getColumnNumber() + "]";
        }
    }

    /**
     * Attributes that are only valid during startElement().
     */
    private class CurrentAttributes extends ExpatAttributes {

        @Override
        public long getParserPointer() {
            return pointer;
        }

        @Override
        public long getPointer() {
            if (!inStartElement) {
                throw new IllegalStateException(OUTSIDE_START_ELEMENT);
            }
            return attributePointer;
        }

        @Override
        public int getLength() {
            if (!inStartElement) {
                throw new IllegalStateException(OUTSIDE_START_ELEMENT);
            }
            return attributeCount;
        }
    }

    /**
     * Includes line and column in the message.
     */
    private static class ParseException extends SAXParseException {

        private ParseException(String message, Locator locator) {
            super(makeMessage(message, locator), locator);
        }

        private static String makeMessage(String message, Locator locator) {
            return makeMessage(message, locator.getLineNumber(),
                    locator.getColumnNumber());
        }

        private static String makeMessage(
                String message, int line, int column) {
            return "At line " + line + ", column "
                    + column + ": " + message;
        }
    }

    /**
     * Opens an InputStream for the given URL.
     */
    /*package*/ static InputStream openUrl(String url) throws IOException {
        try {
            URLConnection urlConnection = new URL(url).openConnection();
            urlConnection.setConnectTimeout(TIMEOUT);
            urlConnection.setReadTimeout(TIMEOUT);
            urlConnection.setDoInput(true);
            urlConnection.setDoOutput(false);
            return urlConnection.getInputStream();
        } catch (Exception e) {
            IOException ioe = new IOException("Couldn't open " + url);
            ioe.initCause(e);
            throw ioe;
        }
    }

    /**
     * Parses an external entity.
     */
    private static class EntityParser extends ExpatParser {

        private int depth = 0;

        private EntityParser(String encoding, ExpatReader xmlReader,
                long pointer, String publicId, String systemId) {
            super(encoding, xmlReader, pointer, publicId, systemId);
        }

        @Override
        void startElement(String uri, String localName, String qName,
                long attributePointer, int attributeCount) throws SAXException {
            /*
             * Skip topmost element generated by our workaround in
             * {@link #handleExternalEntity}.
             */
            if (depth++ > 0) {
                super.startElement(uri, localName, qName, attributePointer,
                        attributeCount);
            }
        }

        @Override
        void endElement(String uri, String localName, String qName)
                throws SAXException {
            if (--depth > 0) {
                super.endElement(uri, localName, qName);
            }
        }

        @Override
        @SuppressWarnings("FinalizeDoesntCallSuperFinalize")
        protected synchronized void finalize() throws Throwable {
            /*
             * Don't release our native resources. We do so explicitly in
             * {@link #handleExternalEntity} and we don't want to release the
             * parsing context--our parent is using it.
             */
        }
    }
}
