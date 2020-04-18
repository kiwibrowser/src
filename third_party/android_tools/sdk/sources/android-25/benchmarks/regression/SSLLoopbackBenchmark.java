/*
 * Copyright 2013 The Android Open Source Project
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

import javax.net.ssl.SSLSocket;
import libcore.java.security.TestKeyStore;
import libcore.javax.net.ssl.TestSSLContext;
import libcore.javax.net.ssl.TestSSLSocketPair;

public class SSLLoopbackBenchmark {

    public void time(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            TestSSLContext context = TestSSLContext.create(
                    TestKeyStore.getClient(), TestKeyStore.getServer());
            SSLSocket[] sockets = TestSSLSocketPair.connect(context, null, null);
            context.close();
            sockets[0].close();
            sockets[1].close();
        }
    }
}
