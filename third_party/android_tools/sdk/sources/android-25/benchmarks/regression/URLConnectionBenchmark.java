/*
 * Copyright (C) 2010 The Android Open Source Project
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

package benchmarks.regression;

import com.google.caliper.Param;
import com.google.mockwebserver.Dispatcher;
import com.google.mockwebserver.MockResponse;
import com.google.mockwebserver.MockWebServer;
import com.google.mockwebserver.RecordedRequest;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

public final class URLConnectionBenchmark {

    @Param({"0", "1024", "1048576"}) private int bodySize;
    @Param({"2048"}) private int chunkSize;
    @Param({"1024"}) private int readBufferSize;
    @Param private ResponseHeaders responseHeaders;
    @Param private TransferEncoding transferEncoding;
    private byte[] readBuffer;

    private MockWebServer server;
    private URL url;

    private static class SingleResponseDispatcher extends Dispatcher {
        private MockResponse response;
        SingleResponseDispatcher(MockResponse response) {
            this.response = response;
        }
        @Override public MockResponse dispatch(RecordedRequest request) {
            return response;
        }
    };

    protected void setUp() throws Exception {
        readBuffer = new byte[readBufferSize];
        server = new MockWebServer();

        MockResponse response = new MockResponse();
        responseHeaders.apply(response);
        transferEncoding.setBody(response, bodySize, chunkSize);

        // keep serving the same response for all iterations
        server.setDispatcher(new SingleResponseDispatcher(response));
        server.play();

        url = server.getUrl("/");
        get(); // ensure the server has started its threads, etc.
    }

    protected void tearDown() throws Exception {
        server.shutdown();
    }

    public int timeGet(int reps) throws IOException {
        int totalBytesRead = 0;
        for (int i = 0; i < reps; i++) {
            totalBytesRead += get();
        }
        return totalBytesRead;
    }

    private int get() throws IOException {
        int totalBytesRead = 0;
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        // URLConnection connection = url.openConnection();
        InputStream in = connection.getInputStream();
        int count;
        while ((count = in.read(readBuffer)) != -1) {
            totalBytesRead += count;
        }
        return totalBytesRead;
    }

    enum TransferEncoding {
        FIXED_LENGTH,
        CHUNKED;

        void setBody(MockResponse response, int bodySize, int chunkSize) throws IOException {
            if (this == TransferEncoding.FIXED_LENGTH) {
                response.setBody(new byte[bodySize]);
            } else if (this == TransferEncoding.CHUNKED) {
                response.setChunkedBody(new byte[bodySize], chunkSize);
            }
        }
    }

    enum ResponseHeaders {
        MINIMAL,
        TYPICAL;

        void apply(MockResponse response) {
            if (this == TYPICAL) {
                /* from http://api.twitter.com/1/statuses/public_timeline.json */
                response.addHeader("Date: Wed, 30 Jun 2010 17:57:39 GMT");
                response.addHeader("Server: hi");
                response.addHeader("X-RateLimit-Remaining: 0");
                response.addHeader("X-Runtime: 0.01637");
                response.addHeader("Content-Type: application/json; charset=utf-8");
                response.addHeader("X-RateLimit-Class: api_whitelisted");
                response.addHeader("Cache-Control: no-cache, max-age=300");
                response.addHeader("X-RateLimit-Reset: 1277920980");
                response.addHeader("Set-Cookie: _twitter_sess=BAh7EDoOcmV0dXJuX3RvIjZodHRwOi8vZGV2L"
                        + "nR3aXR0ZXIuY29tL3BhZ2Vz%250AL3NpZ25faW5fd2l0aF90d2l0dGVyOgxjc3JmX2lkIiUw"
                        + "ODFhNGY2NTM5NjRm%250ANjY1N2M2NzcwNWI0MDlmZGZjZjoVaW5fbmV3X3VzZXJfZmxvdzA"
                        + "6EXRyYW5z%250AX3Byb21wdDAiKXNob3dfZGlzY292ZXJhYmlsaXR5X2Zvcl9qZXNzZXdpbH"
                        + "Nv%250AbjA6E3Nob3dfaGVscF9saW5rMDoTcGFzc3dvcmRfdG9rZW4iLWUyYjlhNmM3%250A"
                        + "MWJiNzI3NWNlZDI1NDY3MGMzZWNmMTE0MjI4N2EyNGE6D2NyZWF0ZWRfYXRs%250AKwhiM%2"
                        + "52F6JKQE6CXVzZXJpA8tE3iIKZmxhc2hJQzonQWN0aW9uQ29udHJvbGxl%250Acjo6Rmxhc2"
                        + "g6OkZsYXNoSGFzaHsABjoKQHVzZWR7ADoHaWQiJWZmMTNhM2Qx%250AZTU1YTkzMmYyMWM0M"
                        + "GNhZjU4NDVjMTQz--11250628c85830219438eb7eba96a541a9af4098; domain=.twitt"
                        + "er.com; path=/");
                response.addHeader("Expires: Wed, 30 Jun 2010 18:02:39 GMT");
                response.addHeader("Vary: Accept-Encoding");
            }
        }
    }
}
