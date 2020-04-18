/*
 * Copyright (C) 2015 The Android Open Source Project
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
package android.databinding.tool.writer;

import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.store.SetterStore;

import java.util.List;
import java.util.Map;

public class ComponentWriter {
    private static final String INDENT = "    ";

    public ComponentWriter() {
    }

    public String createComponent() {
        final StringBuilder builder = new StringBuilder();
        builder.append("package android.databinding;\n\n");
        builder.append("public interface DataBindingComponent {\n");
        final SetterStore setterStore = SetterStore.get(ModelAnalyzer.getInstance());
        Map<String, List<String>> bindingAdapters = setterStore.getComponentBindingAdapters();
        for (final String simpleName : bindingAdapters.keySet()) {
            final List<String> classes = bindingAdapters.get(simpleName);
            if (classes.size() > 1) {
                int index = 1;
                for (String className : classes) {
                    addGetter(builder, simpleName, className, index++);
                }
            } else {
                addGetter(builder, simpleName, classes.iterator().next(), 0);
            }
        }
        builder.append("}\n");
        return builder.toString();
    }

    private static void addGetter(StringBuilder builder, String simpleName, String className,
            int index) {
        builder.append(INDENT)
                .append(className)
                .append(" get")
                .append(simpleName);
        if (index > 0) {
            builder.append(index);
        }
        builder.append("();\n");
    }
}
