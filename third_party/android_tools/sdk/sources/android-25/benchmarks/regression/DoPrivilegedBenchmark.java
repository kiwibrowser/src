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

import java.security.AccessController;
import java.security.PrivilegedAction;

public class DoPrivilegedBenchmark {
    public void timeDirect(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            String lineSeparator = System.getProperty("line.separator");
        }
    }
    
    public void timeFastAndSlow(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            String lineSeparator;
            if (System.getSecurityManager() == null) {
                lineSeparator = System.getProperty("line.separator");
            } else {
                lineSeparator = AccessController.doPrivileged(new PrivilegedAction<String>() {
                    public String run() {
                        return System.getProperty("line.separator");
                    }
                });
            }
        }
    }
    
    public void timeNewAction(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            String lineSeparator = AccessController.doPrivileged(new PrivilegedAction<String>() {
                public String run() {
                    return System.getProperty("line.separator");
                }
            });
        }
    }
    
    public void timeReusedAction(int reps) throws Exception {
        final PrivilegedAction<String> action = new ReusableAction("line.separator");
        for (int i = 0; i < reps; ++i) {
            String lineSeparator = AccessController.doPrivileged(action);
        }
    }
    
    private static final class ReusableAction implements PrivilegedAction<String> {
        private final String propertyName;
        
        public ReusableAction(String propertyName) {
            this.propertyName = propertyName;
        }
        
        public String run() {
            return System.getProperty(propertyName);
        }
    }
}
