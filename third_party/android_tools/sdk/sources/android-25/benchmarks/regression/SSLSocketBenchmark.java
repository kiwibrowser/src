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

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Param;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.net.URL;
import javax.net.SocketFactory;
import javax.net.ssl.SSLContext;

public class SSLSocketBenchmark {

    private static final int BUFFER_SIZE = 8192;

    final byte[] buffer = new byte[BUFFER_SIZE];

    @Param private WebSite webSite;

    public enum WebSite {
        DOCS("https://docs.google.com"),
        MAIL("https://mail.google.com"),
        SITES("https://sites.google.com"),
        WWW("https://www.google.com");
        final InetAddress host;
        final int port;
        final byte[] request;
        WebSite(String uri) {
            try {
                URL url = new URL(uri);

                this.host = InetAddress.getByName(url.getHost());

                int p = url.getPort();
                String portString;
                if (p == -1) {
                    this.port = 443;
                    portString = "";
                } else {
                    this.port = p;
                    portString = ":" + port;
                }

                this.request = ("GET " + uri + " HTTP/1.0\r\n"
                                + "Host: " + host + portString + "\r\n"
                                +"\r\n").getBytes();

            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }

    private SocketFactory sf;

    @BeforeExperiment
    protected void setUp() throws Exception {
        SSLContext sslContext = SSLContext.getInstance("SSL");
        sslContext.init(null, null, null);
        this.sf = sslContext.getSocketFactory();
    }

    public void time(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            Socket s = sf.createSocket(webSite.host, webSite.port);
            OutputStream out = s.getOutputStream();
            out.write(webSite.request);
            InputStream in = s.getInputStream();
            while (true) {
                int n = in.read(buffer);
                if (n == -1) {
                    break;
                }
            }
            in.close();
        }
    }
}
