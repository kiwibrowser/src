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

import android.databinding.BindingAdapter;
import android.databinding.BindingBuildInfo;
import android.databinding.BindingConversion;
import android.databinding.BindingMethod;
import android.databinding.BindingMethods;
import android.databinding.InverseBindingAdapter;
import android.databinding.InverseBindingMethod;
import android.databinding.InverseBindingMethods;
import android.databinding.Untaggable;
import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.store.SetterStore;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;

import java.io.IOException;
import java.util.HashSet;
import java.util.List;

import javax.annotation.processing.ProcessingEnvironment;
import javax.annotation.processing.RoundEnvironment;
import javax.lang.model.element.Element;
import javax.lang.model.element.ElementKind;
import javax.lang.model.element.ExecutableElement;
import javax.lang.model.element.Modifier;
import javax.lang.model.element.TypeElement;
import javax.lang.model.element.VariableElement;
import javax.lang.model.type.MirroredTypeException;
import javax.lang.model.type.TypeKind;
import javax.lang.model.type.TypeMirror;
import javax.lang.model.util.Elements;
import javax.lang.model.util.Types;

public class ProcessMethodAdapters extends ProcessDataBinding.ProcessingStep {
    private final static String INVERSE_BINDING_EVENT_ATTR_SUFFIX = "AttrChanged";

    public ProcessMethodAdapters() {
    }

    @Override
    public boolean onHandleStep(RoundEnvironment roundEnv,
            ProcessingEnvironment processingEnvironment, BindingBuildInfo buildInfo) {
        L.d("processing adapters");
        final ModelAnalyzer modelAnalyzer = ModelAnalyzer.getInstance();
        Preconditions.checkNotNull(modelAnalyzer, "Model analyzer should be"
                + " initialized first");
        SetterStore store = SetterStore.get(modelAnalyzer);
        clearIncrementalClasses(roundEnv, store);

        addBindingAdapters(roundEnv, processingEnvironment, store);
        addRenamed(roundEnv, store);
        addConversions(roundEnv, store);
        addUntaggable(roundEnv, store);
        addInverseAdapters(roundEnv, processingEnvironment, store);
        addInverseMethods(roundEnv, store);

        try {
            store.write(buildInfo.modulePackage(), processingEnvironment);
        } catch (IOException e) {
            L.e(e, "Could not write BindingAdapter intermediate file.");
        }
        return true;
    }

    @Override
    public void onProcessingOver(RoundEnvironment roundEnvironment,
            ProcessingEnvironment processingEnvironment, BindingBuildInfo buildInfo) {

    }

    private void addBindingAdapters(RoundEnvironment roundEnv, ProcessingEnvironment
            processingEnv, SetterStore store) {
        for (Element element : AnnotationUtil
                .getElementsAnnotatedWith(roundEnv, BindingAdapter.class)) {
            if (element.getKind() != ElementKind.METHOD ||
                    !element.getModifiers().contains(Modifier.PUBLIC)) {
                L.e(element, "@BindingAdapter on invalid element: %s", element);
                continue;
            }
            BindingAdapter bindingAdapter = element.getAnnotation(BindingAdapter.class);

            ExecutableElement executableElement = (ExecutableElement) element;
            List<? extends VariableElement> parameters = executableElement.getParameters();
            if (bindingAdapter.value().length == 0) {
                L.e(element, "@BindingAdapter requires at least one attribute. %s",
                        element);
                continue;
            }

            final boolean takesComponent = takesComponent(executableElement, processingEnv);
            final int startIndex = 1 + (takesComponent ? 1 : 0);
            final int numAttributes = bindingAdapter.value().length;
            final int numAdditionalArgs = parameters.size() - startIndex;
            if (numAdditionalArgs == (2 * numAttributes)) {
                // This BindingAdapter takes old and new values. Make sure they are properly ordered
                Types typeUtils = processingEnv.getTypeUtils();
                boolean hasParameterError = false;
                for (int i = startIndex; i < numAttributes + startIndex; i++) {
                    if (!typeUtils.isSameType(parameters.get(i).asType(),
                            parameters.get(i + numAttributes).asType())) {
                        L.e(executableElement, "BindingAdapter %s: old values should be followed " +
                                "by new values. Parameter %d must be the same type as parameter " +
                                "%d.", executableElement, i + 1, i + numAttributes + 1);
                        hasParameterError = true;
                        break;
                    }
                }
                if (hasParameterError) {
                    continue;
                }
            } else if (numAdditionalArgs != numAttributes) {
                L.e(element, "@BindingAdapter %s has %d attributes and %d value " +
                        "parameters. There should be %d or %d value parameters.",
                        executableElement, numAttributes, numAdditionalArgs, numAttributes,
                        numAttributes * 2);
                continue;
            }
            warnAttributeNamespaces(element, bindingAdapter.value());
            try {
                if (numAttributes == 1) {
                    final String attribute = bindingAdapter.value()[0];
                    store.addBindingAdapter(processingEnv, attribute, executableElement,
                            takesComponent);
                } else {
                    store.addBindingAdapter(processingEnv, bindingAdapter.value(),
                            executableElement, takesComponent, bindingAdapter.requireAll());
                }
            } catch (IllegalArgumentException e) {
                L.e(element, "@BindingAdapter for duplicate View and parameter type: %s", element);
            }
        }
    }

    private static boolean takesComponent(ExecutableElement executableElement,
            ProcessingEnvironment processingEnvironment) {
        List<? extends VariableElement> parameters = executableElement.getParameters();
        Elements elementUtils = processingEnvironment.getElementUtils();
        TypeMirror viewElement = elementUtils.getTypeElement("android.view.View").asType();
        if (parameters.size() < 2) {
            return false; // Validation will fail in the caller
        }
        TypeMirror parameter1 = parameters.get(0).asType();
        Types typeUtils = processingEnvironment.getTypeUtils();
        if (parameter1.getKind() == TypeKind.DECLARED &&
                typeUtils.isAssignable(parameter1, viewElement)) {
            return false; // first parameter is a View
        }
        if (parameters.size() < 3) {
            TypeMirror viewStubProxy = elementUtils.
                    getTypeElement("android.databinding.ViewStubProxy").asType();
            if (!typeUtils.isAssignable(parameter1, viewStubProxy)) {
                L.e(executableElement, "@BindingAdapter %s is applied to a method that has two " +
                        "parameters, the first must be a View type", executableElement);
            }
            return false;
        }
        TypeMirror parameter2 = parameters.get(1).asType();
        if (typeUtils.isAssignable(parameter2, viewElement)) {
            return true; // second parameter is a View
        }
        L.e(executableElement, "@BindingAdapter %s is applied to a method that doesn't take a " +
                "View subclass as the first or second parameter. When a BindingAdapter uses a " +
                "DataBindingComponent, the component parameter is first and the View " +
                "parameter is second, otherwise the View parameter is first.",
                executableElement);
        return false;
    }

    private static void warnAttributeNamespace(Element element, String attribute) {
        if (attribute.contains(":") && !attribute.startsWith("android:")) {
            L.w(element, "Application namespace for attribute %s will be ignored.", attribute);
        }
    }

    private static void warnAttributeNamespaces(Element element, String[] attributes) {
        for (String attribute : attributes) {
            warnAttributeNamespace(element, attribute);
        }
    }

    private void addRenamed(RoundEnvironment roundEnv, SetterStore store) {
        for (Element element : AnnotationUtil
                .getElementsAnnotatedWith(roundEnv, BindingMethods.class)) {
            BindingMethods bindingMethods = element.getAnnotation(BindingMethods.class);

            for (BindingMethod bindingMethod : bindingMethods.value()) {
                final String attribute = bindingMethod.attribute();
                final String method = bindingMethod.method();
                warnAttributeNamespace(element, attribute);
                String type;
                try {
                    type = bindingMethod.type().getCanonicalName();
                } catch (MirroredTypeException e) {
                    type = e.getTypeMirror().toString();
                }
                store.addRenamedMethod(attribute, type, method, (TypeElement) element);
            }
        }
    }

    private void addConversions(RoundEnvironment roundEnv, SetterStore store) {
        for (Element element : AnnotationUtil
                .getElementsAnnotatedWith(roundEnv, BindingConversion.class)) {
            if (element.getKind() != ElementKind.METHOD ||
                    !element.getModifiers().contains(Modifier.STATIC) ||
                    !element.getModifiers().contains(Modifier.PUBLIC)) {
                L.e(element, "@BindingConversion is only allowed on public static methods %s",
                        element);
                continue;
            }

            ExecutableElement executableElement = (ExecutableElement) element;
            if (executableElement.getParameters().size() != 1) {
                L.e(element, "@BindingConversion method should have one parameter %s", element);
                continue;
            }
            if (executableElement.getReturnType().getKind() == TypeKind.VOID) {
                L.e(element, "@BindingConversion method must return a value %s", element);
                continue;
            }
            store.addConversionMethod(executableElement);
        }
    }

    private void addInverseAdapters(RoundEnvironment roundEnv,
            ProcessingEnvironment processingEnv, SetterStore store) {
        for (Element element : AnnotationUtil
                .getElementsAnnotatedWith(roundEnv, InverseBindingAdapter.class)) {
            if (!element.getModifiers().contains(Modifier.PUBLIC)) {
                L.e(element, "@InverseBindingAdapter must be associated with a public method");
                continue;
            }
            ExecutableElement executableElement = (ExecutableElement) element;
            if (executableElement.getReturnType().getKind() == TypeKind.VOID) {
                L.e(element, "@InverseBindingAdapter must have a non-void return type");
                continue;
            }
            final InverseBindingAdapter inverseBindingAdapter =
                    executableElement.getAnnotation(InverseBindingAdapter.class);
            final String attribute = inverseBindingAdapter.attribute();
            warnAttributeNamespace(element, attribute);
            final String event = inverseBindingAdapter.event().isEmpty()
                    ? inverseBindingAdapter.attribute() + INVERSE_BINDING_EVENT_ATTR_SUFFIX
                    : inverseBindingAdapter.event();
            warnAttributeNamespace(element, event);
            final boolean takesComponent = takesComponent(executableElement, processingEnv);
            final int expectedArgs = takesComponent ? 2 : 1;
            final int numParameters = executableElement.getParameters().size();
            if (numParameters != expectedArgs) {
                L.e(element, "@InverseBindingAdapter %s takes %s parameters, but %s parameters " +
                        "were expected", element, numParameters, expectedArgs);
                continue;
            }
            try {
                store.addInverseAdapter(processingEnv, attribute, event, executableElement,
                        takesComponent);
            } catch (IllegalArgumentException e) {
                L.e(element, "@InverseBindingAdapter for duplicate View and parameter type: %s",
                        element);
            }
        }
    }

    private void addInverseMethods(RoundEnvironment roundEnv, SetterStore store) {
        for (Element element : AnnotationUtil
                .getElementsAnnotatedWith(roundEnv, InverseBindingMethods.class)) {
            InverseBindingMethods bindingMethods =
                    element.getAnnotation(InverseBindingMethods.class);

            for (InverseBindingMethod bindingMethod : bindingMethods.value()) {
                final String attribute = bindingMethod.attribute();
                final String method = bindingMethod.method();
                final String event = bindingMethod.event().isEmpty()
                        ? bindingMethod.attribute() + INVERSE_BINDING_EVENT_ATTR_SUFFIX
                        : bindingMethod.event();
                warnAttributeNamespace(element, attribute);
                warnAttributeNamespace(element, event);
                String type;
                try {
                    type = bindingMethod.type().getCanonicalName();
                } catch (MirroredTypeException e) {
                    type = e.getTypeMirror().toString();
                }
                store.addInverseMethod(attribute, event, type, method, (TypeElement) element);
            }
        }
    }

    private void addUntaggable(RoundEnvironment roundEnv, SetterStore store) {
        for (Element element : AnnotationUtil.
                getElementsAnnotatedWith(roundEnv, Untaggable.class)) {
            Untaggable untaggable = element.getAnnotation(Untaggable.class);
            store.addUntaggableTypes(untaggable.value(), (TypeElement) element);
        }
    }

    private void clearIncrementalClasses(RoundEnvironment roundEnv, SetterStore store) {
        HashSet<String> classes = new HashSet<String>();

        for (Element element : AnnotationUtil
                .getElementsAnnotatedWith(roundEnv, BindingAdapter.class)) {
            TypeElement containingClass = (TypeElement) element.getEnclosingElement();
            classes.add(containingClass.getQualifiedName().toString());
        }
        for (Element element : AnnotationUtil
                .getElementsAnnotatedWith(roundEnv, BindingMethods.class)) {
            classes.add(((TypeElement) element).getQualifiedName().toString());
        }
        for (Element element : AnnotationUtil
                .getElementsAnnotatedWith(roundEnv, BindingConversion.class)) {
            classes.add(((TypeElement) element.getEnclosingElement()).getQualifiedName().
                    toString());
        }
        for (Element element : AnnotationUtil.
                getElementsAnnotatedWith(roundEnv, Untaggable.class)) {
            classes.add(((TypeElement) element).getQualifiedName().toString());
        }
        store.clear(classes);
    }
}
