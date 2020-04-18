/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package benchmarks;

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Param;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.util.Arrays;
import java.util.List;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;
import org.xmlpull.v1.XmlPullParser;

public class XmlParseBenchmark {

    @Param String xmlFile;
    ByteArrayInputStream inputStream;

    static List<String> xmlFileValues = Arrays.asList(
            "/etc/apns-conf.xml",
            "/etc/media_profiles.xml",
            "/etc/permissions/features.xml"
    );

    private SAXParser saxParser;
    private DocumentBuilder documentBuilder;
    private Constructor<? extends XmlPullParser> kxmlConstructor;
    private Constructor<? extends XmlPullParser> expatConstructor;

    @SuppressWarnings("unchecked")
    @BeforeExperiment
    protected void setUp() throws Exception {
        byte[] xmlBytes = getXmlBytes();
        inputStream = new ByteArrayInputStream(xmlBytes);
        inputStream.mark(xmlBytes.length);

        SAXParserFactory saxParserFactory = SAXParserFactory.newInstance();
        saxParser = saxParserFactory.newSAXParser();

        DocumentBuilderFactory builderFactory = DocumentBuilderFactory.newInstance();
        documentBuilder = builderFactory.newDocumentBuilder();

        kxmlConstructor = (Constructor) Class.forName("org.kxml2.io.KXmlParser").getConstructor();
        expatConstructor = (Constructor) Class.forName("org.apache.harmony.xml.ExpatPullParser")
                .getConstructor();
    }

    private byte[] getXmlBytes() throws IOException {
        FileInputStream fileIn = new FileInputStream(xmlFile);
        ByteArrayOutputStream bytesOut = new ByteArrayOutputStream();
        int count;
        byte[] buffer = new byte[1024];
        while ((count = fileIn.read(buffer)) != -1) {
            bytesOut.write(buffer, 0, count);
        }
        fileIn.close();
        return bytesOut.toByteArray();
    }

    public int timeSax(int reps) throws IOException, SAXException {
        int elementCount = 0;
        for (int i = 0; i < reps; i++) {
            inputStream.reset();
            ElementCounterSaxHandler elementCounterSaxHandler = new ElementCounterSaxHandler();
            saxParser.parse(inputStream, elementCounterSaxHandler);
            elementCount += elementCounterSaxHandler.elementCount;
        }
        return elementCount;
    }

    private static class ElementCounterSaxHandler extends DefaultHandler {
        int elementCount = 0;
        @Override public void startElement(String uri, String localName,
                String qName, Attributes attributes) {
            elementCount++;
        }
    }

    public int timeDom(int reps) throws IOException, SAXException {
        int elementCount = 0;
        for (int i = 0; i < reps; i++) {
            inputStream.reset();
            Document document = documentBuilder.parse(inputStream);
            elementCount += countDomElements(document.getDocumentElement());
        }
        return elementCount;
    }

    private int countDomElements(Node node) {
        int result = 0;
        for (; node != null; node = node.getNextSibling()) {
            if (node.getNodeType() == Node.ELEMENT_NODE) {
                result++;
            }
            result += countDomElements(node.getFirstChild());
        }
        return result;
    }

    public int timeExpat(int reps) throws Exception {
        return testXmlPull(expatConstructor, reps);
    }

    public int timeKxml(int reps) throws Exception {
        return testXmlPull(kxmlConstructor, reps);
    }

    private int testXmlPull(Constructor<? extends XmlPullParser> constructor, int reps)
            throws Exception {
        int elementCount = 0;
        for (int i = 0; i < reps; i++) {
            inputStream.reset();
            XmlPullParser xmlPullParser = constructor.newInstance();
            xmlPullParser.setInput(inputStream, "UTF-8");
            int type;
            while ((type = xmlPullParser.next()) != XmlPullParser.END_DOCUMENT) {
                if (type == XmlPullParser.START_TAG) {
                    elementCount++;
                }
            }
        }
        return elementCount;
    }
}
