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

/**
 * Compares various kinds of method invocation.
 */
public class MethodInvocationBenchmark {
    interface I {
        void emptyInterface();
    }

    static class C implements I {
        private int field;

        private int getField() {
            return field;
        }

        public int timeInternalGetter(int reps) {
            int result = 0;
            for (int i = 0; i < reps; ++i) {
                result = getField();
            }
            return result;
        }

        public int timeInternalFieldAccess(int reps) {
            int result = 0;
            for (int i = 0; i < reps; ++i) {
                result = field;
            }
            return result;
        }

        public static void emptyStatic() {
        }

        public void emptyVirtual() {
        }

        public void emptyInterface() {
        }
    }

    public void timeInternalGetter(int reps) {
        new C().timeInternalGetter(reps);
    }

    public void timeInternalFieldAccess(int reps) {
        new C().timeInternalFieldAccess(reps);
    }

    // Test an intrinsic.
    public int timeStringLength(int reps) {
        int result = 0;
        for (int i = 0; i < reps; ++i) {
            result = "hello, world!".length();
        }
        return result;
    }

    public void timeEmptyStatic(int reps) {
        C c = new C();
        for (int i = 0; i < reps; ++i) {
            c.emptyStatic();
        }
    }

    public void timeEmptyVirtual(int reps) {
        C c = new C();
        for (int i = 0; i < reps; ++i) {
            c.emptyVirtual();
        }
    }

    public void timeEmptyInterface(int reps) {
        I c = new C();
        for (int i = 0; i < reps; ++i) {
            c.emptyInterface();
        }
    }

    public static class Inner {
        private int i;
        private void privateMethod() { ++i; }
        protected void protectedMethod() { ++i; }
        public void publicMethod() { ++i; }
        void packageMethod() { ++i; }
        final void finalPackageMethod() { ++i; }
    }

    public void timePrivateInnerPublicMethod(int reps) {
        Inner inner = new Inner();
        for (int i = 0; i < reps; ++i) {
            inner.publicMethod();
        }
    }

    public void timePrivateInnerProtectedMethod(int reps) {
        Inner inner = new Inner();
        for (int i = 0; i < reps; ++i) {
            inner.protectedMethod();
        }
    }

    public void timePrivateInnerPrivateMethod(int reps) {
        Inner inner = new Inner();
        for (int i = 0; i < reps; ++i) {
            inner.privateMethod();
        }
    }

    public void timePrivateInnerPackageMethod(int reps) {
        Inner inner = new Inner();
        for (int i = 0; i < reps; ++i) {
            inner.packageMethod();
        }
    }

    public void timePrivateInnerFinalPackageMethod(int reps) {
        Inner inner = new Inner();
        for (int i = 0; i < reps; ++i) {
          inner.finalPackageMethod();
        }
    }
}
