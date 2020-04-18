/*
 * Copyright (C) 2015 The Android Open Source Project
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

import java.security.Provider;
import java.security.Security;
import javax.crypto.Cipher;

public class ProviderBenchmark {
    public void timeStableProviders(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            Cipher c = Cipher.getInstance("RSA");
        }
    }

    public void timeWithNewProvider(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            Security.addProvider(new MockProvider());
            try {
                Cipher c = Cipher.getInstance("RSA");
            } finally {
                Security.removeProvider("Mock");
            }
        }
    }

    private static class MockProvider extends Provider {
        public MockProvider() {
            super("Mock", 1.0, "Mock me!");
        }
    }
}
