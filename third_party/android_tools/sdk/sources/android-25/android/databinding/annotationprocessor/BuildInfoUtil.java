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

package android.databinding.annotationprocessor;

import android.databinding.BindingBuildInfo;
import android.databinding.tool.processing.ScopedException;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;

import java.lang.annotation.Annotation;

import javax.annotation.processing.RoundEnvironment;
import javax.lang.model.element.Element;

public class BuildInfoUtil {
    private static BindingBuildInfo sCached;
    public static BindingBuildInfo load(RoundEnvironment roundEnvironment) {
        if (sCached == null) {
            sCached = extractNotNull(roundEnvironment, BindingBuildInfo.class);
            if (sCached != null) {
                L.setDebugLog(sCached.enableDebugLogs());
                ScopedException.encodeOutput(sCached.printEncodedError());
            }
        }
        return sCached;
    }

    private static <T extends Annotation> T extractNotNull(RoundEnvironment roundEnv,
            Class<T> annotationClass) {
        T result = null;
        for (Element element : roundEnv.getElementsAnnotatedWith(annotationClass)) {
            final T info = element.getAnnotation(annotationClass);
            if (info == null) {
                continue; // It gets confused between BindingAppInfo and BinderBundle
            }
            Preconditions.check(result == null, "Should have only one %s",
                    annotationClass.getCanonicalName());
            result = info;
        }
        return result;
    }
}
