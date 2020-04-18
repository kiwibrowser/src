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

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

public class ReflectionBenchmark {
    public void timeObject_getClass(int reps) throws Exception {
        C c = new C();
        for (int rep = 0; rep < reps; ++rep) {
            c.getClass();
        }
    }

    public void timeClass_getField(int reps) throws Exception {
        Class<?> klass = C.class;
        for (int rep = 0; rep < reps; ++rep) {
            klass.getField("f");
        }
    }

    public void timeClass_getDeclaredField(int reps) throws Exception {
        Class<?> klass = C.class;
        for (int rep = 0; rep < reps; ++rep) {
            klass.getDeclaredField("f");
        }
    }

    public void timeClass_getConstructor(int reps) throws Exception {
        Class<?> klass = C.class;
        for (int rep = 0; rep < reps; ++rep) {
            klass.getConstructor();
        }
    }

    public void timeClass_newInstance(int reps) throws Exception {
        Class<?> klass = C.class;
        Constructor constructor = klass.getConstructor();
        for (int rep = 0; rep < reps; ++rep) {
            constructor.newInstance();
        }
    }

    public void timeClass_getMethod(int reps) throws Exception {
        Class<?> klass = C.class;
        for (int rep = 0; rep < reps; ++rep) {
            klass.getMethod("m");
        }
    }

    public void timeClass_getDeclaredMethod(int reps) throws Exception {
        Class<?> klass = C.class;
        for (int rep = 0; rep < reps; ++rep) {
            klass.getDeclaredMethod("m");
        }
    }

    public void timeField_setInt(int reps) throws Exception {
        Class<?> klass = C.class;
        Field f = klass.getDeclaredField("f");
        C instance = new C();
        for (int rep = 0; rep < reps; ++rep) {
            f.setInt(instance, 1);
        }
    }

    public void timeField_getInt(int reps) throws Exception {
        Class<?> klass = C.class;
        Field f = klass.getDeclaredField("f");
        C instance = new C();
        for (int rep = 0; rep < reps; ++rep) {
            f.getInt(instance);
        }
    }

    public void timeMethod_invokeV(int reps) throws Exception {
        Class<?> klass = C.class;
        Method m = klass.getDeclaredMethod("m");
        C instance = new C();
        for (int rep = 0; rep < reps; ++rep) {
            m.invoke(instance);
        }
    }

    public void timeMethod_invokeStaticV(int reps) throws Exception {
        Class<?> klass = C.class;
        Method m = klass.getDeclaredMethod("sm");
        for (int rep = 0; rep < reps; ++rep) {
            m.invoke(null);
        }
    }

    public void timeMethod_invokeI(int reps) throws Exception {
        Class<?> klass = C.class;
        Method m = klass.getDeclaredMethod("setField", int.class);
        C instance = new C();
        for (int rep = 0; rep < reps; ++rep) {
            m.invoke(instance, 1);
        }
    }

    public void timeMethod_invokePreBoxedI(int reps) throws Exception {
        Class<?> klass = C.class;
        Method m = klass.getDeclaredMethod("setField", int.class);
        C instance = new C();
        Integer one = Integer.valueOf(1);
        for (int rep = 0; rep < reps; ++rep) {
            m.invoke(instance, one);
        }
    }

    public void timeMethod_invokeStaticI(int reps) throws Exception {
        Class<?> klass = C.class;
        Method m = klass.getDeclaredMethod("setStaticField", int.class);
        for (int rep = 0; rep < reps; ++rep) {
            m.invoke(null, 1);
        }
    }

    public void timeMethod_invokeStaticPreBoxedI(int reps) throws Exception {
        Class<?> klass = C.class;
        Method m = klass.getDeclaredMethod("setStaticField", int.class);
        Integer one = Integer.valueOf(1);
        for (int rep = 0; rep < reps; ++rep) {
            m.invoke(null, one);
        }
    }

    public void timeRegularMethodInvocation(int reps) throws Exception {
        C instance = new C();
        for (int rep = 0; rep < reps; ++rep) {
            instance.setField(1);
        }
    }

    public void timeRegularConstructor(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            new C();
        }
    }

    public void timeClass_classNewInstance(int reps) throws Exception {
        Class<?> klass = C.class;
        for (int rep = 0; rep < reps; ++rep) {
            klass.newInstance();
        }
    }

    public void timeClass_isInstance(int reps) throws Exception {
        D d = new D();
        Class<?> klass = IC.class;
        for (int rep = 0; rep < reps; ++rep) {
            klass.isInstance(d);
        }
    }

    public void timeGetInstanceField(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            // The field here (and in timeGetStaticField) were chosen to be
            // ~75% down the bottom of the alphabetically sorted field list.
            // It's hard to construct a "fair" test case without resorting to
            // a class whose field names are created algorithmically.
            //
            // TODO: Write a test script that generates both the classes we're
            // reflecting on and the test case for each of its fields.
            R.class.getField("mtextAppearanceLargePopupMenu");
        }
    }

    public void timeGetStaticField(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            R.class.getField("weekNumberColor");
        }
    }

    public void timeGetInterfaceStaticField(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            F.class.getField("sf");
        }
    }

    public void timeGetSuperClassField(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            G.class.getField("f");
        }
    }


    public static class C {
        public static int sf = 0;
        public int f = 0;

        public C() {
            // A non-empty constructor so we don't get optimized away.
            f = 1;
        }

        public void m() {
        }

        public static void sm() {
        }

        public void setField(int value) {
            f = value;
        }

        public static void setStaticField(int value) {
            sf = value;
        }
    }

    interface IA {
    }

    interface IB extends IA {
    }

    interface IC extends IB {
        public static final int sf = 0;
    }

    class D implements IC {
    }

    class E extends D {
    }

    class F extends E implements IB {
    }

    class G extends C {
    }
}
