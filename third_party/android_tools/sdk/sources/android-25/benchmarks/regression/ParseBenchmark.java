/*
 * Copyright (C) 2011 Google Inc.
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

package benchmarks.regression;

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Param;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StringReader;
import java.io.StringWriter;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.SAXParserFactory;
import org.json.JSONArray;
import org.json.JSONObject;
import org.xml.sax.InputSource;
import org.xml.sax.helpers.DefaultHandler;
import org.xmlpull.v1.XmlPullParser;

/**
 * Measure throughput of various parsers.
 *
 * <p>This benchmark requires that ParseBenchmarkData.zip is on the classpath.
 * That file contains Twitter feed data, which is representative of what
 * applications will be parsing.
 */
public final class ParseBenchmark {

    @Param Document document;
    @Param Api api;

    private enum Document {
        TWEETS,
        READER_SHORT,
        READER_LONG
    }

    private enum Api {
        ANDROID_STREAM("json") {
            @Override Parser newParser() {
                return new AndroidStreamParser();
            }
        },
        ORG_JSON("json") {
            @Override Parser newParser() {
                return new OrgJsonParser();
            }
        },
        XML_PULL("xml") {
            @Override Parser newParser() {
                return new GeneralXmlPullParser();
            }
        },
        XML_DOM("xml") {
            @Override Parser newParser() {
                return new XmlDomParser();
            }
        },
        XML_SAX("xml") {
            @Override Parser newParser() {
                return new XmlSaxParser();
            }
        };

        final String extension;

        private Api(String extension) {
            this.extension = extension;
        }

        abstract Parser newParser();
    }

    private String text;
    private Parser parser;

    @BeforeExperiment
    protected void setUp() throws Exception {
        text = resourceToString("/" + document.name() + "." + api.extension);
        parser = api.newParser();
    }

    public void timeParse(int reps) throws Exception {
        for (int i = 0; i < reps; i++) {
            parser.parse(text);
        }
    }

    private static String resourceToString(String path) throws Exception {
        InputStream in = ParseBenchmark.class.getResourceAsStream(path);
        if (in == null) {
            throw new IllegalArgumentException("No such file: " + path);
        }

        Reader reader = new InputStreamReader(in, "UTF-8");
        char[] buffer = new char[8192];
        StringWriter writer = new StringWriter();
        int count;
        while ((count = reader.read(buffer)) != -1) {
            writer.write(buffer, 0, count);
        }
        reader.close();
        return writer.toString();
    }

    interface Parser {
        void parse(String data) throws Exception;
    }

    private static class AndroidStreamParser implements Parser {
        @Override public void parse(String data) throws Exception {
            android.util.JsonReader jsonReader
                    = new android.util.JsonReader(new StringReader(data));
            readToken(jsonReader);
            jsonReader.close();
        }

        public void readObject(android.util.JsonReader reader) throws IOException {
            reader.beginObject();
            while (reader.hasNext()) {
                reader.nextName();
                readToken(reader);
            }
            reader.endObject();
        }

        public void readArray(android.util.JsonReader reader) throws IOException {
            reader.beginArray();
            while (reader.hasNext()) {
                readToken(reader);
            }
            reader.endArray();
        }

        private void readToken(android.util.JsonReader reader) throws IOException {
            switch (reader.peek()) {
            case BEGIN_ARRAY:
                readArray(reader);
                break;
            case BEGIN_OBJECT:
                readObject(reader);
                break;
            case BOOLEAN:
                reader.nextBoolean();
                break;
            case NULL:
                reader.nextNull();
                break;
            case NUMBER:
                reader.nextLong();
                break;
            case STRING:
                reader.nextString();
                break;
            default:
                throw new IllegalArgumentException("Unexpected token" + reader.peek());
            }
        }
    }

    private static class OrgJsonParser implements Parser {
        @Override public void parse(String data) throws Exception {
            if (data.startsWith("[")) {
                new JSONArray(data);
            } else if (data.startsWith("{")) {
                new JSONObject(data);
            } else {
                throw new IllegalArgumentException();
            }
        }
    }

    private static class GeneralXmlPullParser implements Parser {
        @Override public void parse(String data) throws Exception {
            XmlPullParser xmlParser = android.util.Xml.newPullParser();
            xmlParser.setInput(new StringReader(data));
            xmlParser.nextTag();
            while (xmlParser.next() != XmlPullParser.END_DOCUMENT) {
                xmlParser.getName();
                xmlParser.getText();
            }
        }
    }

    private static class XmlDomParser implements Parser {
        @Override public void parse(String data) throws Exception {
            DocumentBuilderFactory.newInstance().newDocumentBuilder()
                    .parse(new InputSource(new StringReader(data)));
        }
    }

    private static class XmlSaxParser implements Parser {
        @Override public void parse(String data) throws Exception {
            SAXParserFactory.newInstance().newSAXParser().parse(
                    new InputSource(new StringReader(data)), new DefaultHandler());
        }
    }
}
