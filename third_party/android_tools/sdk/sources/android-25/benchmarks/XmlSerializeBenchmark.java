/*
 * Copyright (C) 2015 Google Inc.
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
import java.io.CharArrayWriter;
import java.lang.reflect.Constructor;
import java.util.Random;
import org.xmlpull.v1.XmlSerializer;


public class XmlSerializeBenchmark {

    @Param( {"0.99 0.7 0.7 0.7 0.7 0.7",
            "0.999 0.3 0.3 0.95 0.9 0.9"})
    String datasetAsString;

    @Param( { "854328", "312547"} )
    int seed;

    double[] dataset;
    private Constructor<? extends XmlSerializer> kxmlConstructor;
    private Constructor<? extends XmlSerializer> fastConstructor;

    private void serializeRandomXml(Constructor<? extends XmlSerializer> ctor, long seed)
            throws Exception {
        double contChance = dataset[0];
        double levelUpChance = dataset[1];
        double levelDownChance = dataset[2];
        double attributeChance = dataset[3];
        double writeChance1 = dataset[4];
        double writeChance2 = dataset[5];

        XmlSerializer serializer = (XmlSerializer) ctor.newInstance();

        CharArrayWriter w = new CharArrayWriter();
        serializer.setOutput(w);
        int level = 0;
        Random r = new Random(seed);
        char[] toWrite = {'a','b','c','d','s','z'};
        serializer.startDocument("UTF-8", true);
        while(r.nextDouble() < contChance) {
            while(level > 0 && r.nextDouble() < levelUpChance) {
                serializer.endTag("aaaaaa", "bbbbbb");
                level--;
            }
            while(r.nextDouble() < levelDownChance) {
                serializer.startTag("aaaaaa", "bbbbbb");
                level++;
            }
            serializer.startTag("aaaaaa", "bbbbbb");
            level++;
            while(r.nextDouble() < attributeChance) {
                serializer.attribute("aaaaaa", "cccccc", "dddddd");
            }
            serializer.endTag("aaaaaa", "bbbbbb");
            level--;
            while(r.nextDouble() < writeChance1)
                serializer.text(toWrite, 0, 5);
            while(r.nextDouble() < writeChance2)
                serializer.text("Textxtsxtxtxt ");
        }
        serializer.endDocument();
    }

    @SuppressWarnings("unchecked")
    @BeforeExperiment
    protected void setUp() throws Exception {
        kxmlConstructor = (Constructor) Class.forName("org.kxml2.io.KXmlSerializer")
                .getConstructor();
        fastConstructor = (Constructor) Class.forName("com.android.internal.util.FastXmlSerializer")
                .getConstructor();
        String[] splitted = datasetAsString.split(" ");
        dataset = new double[splitted.length];
        for (int i = 0; i < splitted.length; i++) {
            dataset[i] = Double.valueOf(splitted[i]);
        }
    }

    private void internalTimeSerializer(Constructor<? extends XmlSerializer> ctor, int reps)
            throws Exception {
        for (int i = 0; i < reps; i++) {
            serializeRandomXml(ctor, seed);
        }
    }

    public void timeKxml(int reps) throws Exception {
        internalTimeSerializer(kxmlConstructor, reps);
    }

    public void timeFast(int reps) throws Exception {
        internalTimeSerializer(fastConstructor, reps);
    }
}
