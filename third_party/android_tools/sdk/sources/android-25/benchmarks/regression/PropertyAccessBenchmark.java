/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.google.caliper.BeforeExperiment;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

public final class PropertyAccessBenchmark {
    private View view = new View();
    private Method setX;
    private GeneratedProperty generatedSetter = new GeneratedSetter();
    private GeneratedProperty generatedField = new GeneratedField();
    private Field x;
    private Object[] argsBox = new Object[1];

    @BeforeExperiment
    protected void setUp() throws Exception {
        setX = View.class.getDeclaredMethod("setX", float.class);
        x = View.class.getDeclaredField("x");
    }

    public void timeDirectSetter(int reps) {
        for (int i = 0; i < reps; i++) {
            view.setX(0.1f);
        }
    }

    public void timeDirectFieldSet(int reps) {
        for (int i = 0; i < reps; i++) {
            view.x = 0.1f;
        }
    }

    public void timeDirectSetterAndBoxing(int reps) {
        for (int i = 0; i < reps; i++) {
            Float value = 0.1f;
            view.setX(value);
        }
    }

    public void timeDirectFieldSetAndBoxing(int reps) {
        for (int i = 0; i < reps; i++) {
            Float value = 0.1f;
            view.x = value;
        }
    }

    public void timeReflectionSetterAndTwoBoxes(int reps) throws Exception {
        for (int i = 0; i < reps; i++) {
            setX.invoke(view, 0.1f);
        }
    }

    public void timeReflectionSetterAndOneBox(int reps) throws Exception {
        for (int i = 0; i < reps; i++) {
            argsBox[0] = 0.1f;
            setX.invoke(view, argsBox);
        }
    }

    public void timeReflectionFieldSet(int reps) throws Exception {
        for (int i = 0; i < reps; i++) {
            x.setFloat(view, 0.1f);
        }
    }

    public void timeGeneratedSetter(int reps) throws Exception {
        for (int i = 0; i < reps; i++) {
            generatedSetter.setFloat(view, 0.1f);
        }
    }

    public void timeGeneratedFieldSet(int reps) throws Exception {
        for (int i = 0; i < reps; i++) {
            generatedField.setFloat(view, 0.1f);
        }
    }

    static class View {
        float x;

        public void setX(float x) {
            this.x = x;
        }
    }

    static interface GeneratedProperty {
        void setFloat(View v, float f);
    }

    static class GeneratedSetter implements GeneratedProperty {
        public void setFloat(View v, float f) {
            v.setX(f);
        }
    }

    static class GeneratedField implements GeneratedProperty {
        public void setFloat(View v, float f) {
            v.x = f;
        }
    }
}
