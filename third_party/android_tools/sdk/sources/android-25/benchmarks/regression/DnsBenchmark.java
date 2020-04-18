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

package benchmarks.regression;

import java.net.InetAddress;
import java.net.UnknownHostException;

public class DnsBenchmark {
    public void timeDns(int reps) throws Exception {
        String[] hosts = new String[] {
            "www.amazon.com",
            "z-ecx.images-amazon.com",
            "g-ecx.images-amazon.com",
            "ecx.images-amazon.com",
            "ad.doubleclick.com",
            "bpx.a9.com",
            "d3dtik4dz1nej0.cloudfront.net",
            "uac.advertising.com",
            "servedby.advertising.com",
            "view.atdmt.com",
            "rmd.atdmt.com",
            "spe.atdmt.com",
            "www.google.com",
            "www.cnn.com",
            "bad.host.mtv.corp.google.com",
        };
        for (int i = 0; i < reps; ++i) {
            try {
                InetAddress.getByName(hosts[i % hosts.length]);
            } catch (UnknownHostException ex) {
            }
        }
    }
}
