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
import java.io.StringReader;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import org.xml.sax.InputSource;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserFactory;

// http://code.google.com/p/android/issues/detail?id=18102
public final class XmlEntitiesBenchmark {
  
  @Param({"10", "100", "1000"}) int length;
  @Param({"0", "0.5", "1.0"}) float entityFraction;

  private XmlPullParserFactory xmlPullParserFactory;
  private DocumentBuilderFactory documentBuilderFactory;

  /** a string like {@code <doc>&amp;&amp;++</doc>}. */
  private String xml;

  @BeforeExperiment
  protected void setUp() throws Exception {
    xmlPullParserFactory = XmlPullParserFactory.newInstance();
    documentBuilderFactory = DocumentBuilderFactory.newInstance();

    StringBuilder xmlBuilder = new StringBuilder();
    xmlBuilder.append("<doc>");
    for (int i = 0; i < (length * entityFraction); i++) {
      xmlBuilder.append("&amp;");
    }
    while (xmlBuilder.length() < length) {
      xmlBuilder.append("+");
    }
    xmlBuilder.append("</doc>");
    xml = xmlBuilder.toString();
  }
  
  public void timeXmlParser(int reps) throws Exception {
    for (int i = 0; i < reps; i++) {
      XmlPullParser parser = xmlPullParserFactory.newPullParser();
      parser.setInput(new StringReader(xml));
      while (parser.next() != XmlPullParser.END_DOCUMENT) {
      }
    }
  }
  
  public void timeDocumentBuilder(int reps) throws Exception {
    for (int i = 0; i < reps; i++) {
      DocumentBuilder documentBuilder = documentBuilderFactory.newDocumentBuilder();
      documentBuilder.parse(new InputSource(new StringReader(xml)));
    }
  }
}
