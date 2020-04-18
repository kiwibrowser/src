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

package android.databinding.tool;

import android.databinding.tool.expr.Dependency;
import android.databinding.tool.expr.Expr;
import android.databinding.tool.expr.ExprModel;
import android.databinding.tool.expr.IdentifierExpr;
import android.databinding.tool.processing.Scope;
import android.databinding.tool.processing.scopes.FileScopeProvider;
import android.databinding.tool.store.Location;
import android.databinding.tool.store.ResourceBundle;
import android.databinding.tool.store.ResourceBundle.BindingTargetBundle;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;
import android.databinding.tool.writer.LayoutBinderWriter;
import android.databinding.tool.writer.LayoutBinderWriterKt;

import org.antlr.v4.runtime.misc.Nullable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

/**
 * Keeps all information about the bindings per layout file
 */
public class LayoutBinder implements FileScopeProvider {
    private static final Comparator<BindingTarget> COMPARE_FIELD_NAME = new Comparator<BindingTarget>() {
        @Override
        public int compare(BindingTarget first, BindingTarget second) {
            final String fieldName1 = LayoutBinderWriterKt.getFieldName(first);
            final String fieldName2 = LayoutBinderWriterKt.getFieldName(second);
            return fieldName1.compareTo(fieldName2);
        }
    };

    /*
    * val pkg: String, val projectPackage: String, val baseClassName: String,
        val layoutName:String, val lb: LayoutExprBinding*/
    private final ExprModel mExprModel;
    private final ExpressionParser mExpressionParser;
    private final List<BindingTarget> mBindingTargets;
    private final List<BindingTarget> mSortedBindingTargets;
    private String mModulePackage;
    private final HashMap<String, String> mUserDefinedVariables = new HashMap<String, String>();

    private LayoutBinderWriter mWriter;
    private ResourceBundle.LayoutFileBundle mBundle;
    private static final String[] sJavaLangClasses = {
            "Deprecated",
            "Override",
            "SafeVarargs",
            "SuppressWarnings",
            "Appendable",
            "AutoCloseable",
            "CharSequence",
            "Cloneable",
            "Comparable",
            "Iterable",
            "Readable",
            "Runnable",
            "Thread.UncaughtExceptionHandler",
            "Boolean",
            "Byte",
            "Character",
            "Character.Subset",
            "Character.UnicodeBlock",
            "Class",
            "ClassLoader",
            "Compiler",
            "Double",
            "Enum",
            "Float",
            "InheritableThreadLocal",
            "Integer",
            "Long",
            "Math",
            "Number",
            "Object",
            "Package",
            "Process",
            "ProcessBuilder",
            "Runtime",
            "RuntimePermission",
            "SecurityManager",
            "Short",
            "StackTraceElement",
            "StrictMath",
            "String",
            "StringBuffer",
            "StringBuilder",
            "System",
            "Thread",
            "ThreadGroup",
            "ThreadLocal",
            "Throwable",
            "Void",
            "Thread.State",
            "ArithmeticException",
            "ArrayIndexOutOfBoundsException",
            "ArrayStoreException",
            "ClassCastException",
            "ClassNotFoundException",
            "CloneNotSupportedException",
            "EnumConstantNotPresentException",
            "Exception",
            "IllegalAccessException",
            "IllegalArgumentException",
            "IllegalMonitorStateException",
            "IllegalStateException",
            "IllegalThreadStateException",
            "IndexOutOfBoundsException",
            "InstantiationException",
            "InterruptedException",
            "NegativeArraySizeException",
            "NoSuchFieldException",
            "NoSuchMethodException",
            "NullPointerException",
            "NumberFormatException",
            "ReflectiveOperationException",
            "RuntimeException",
            "SecurityException",
            "StringIndexOutOfBoundsException",
            "TypeNotPresentException",
            "UnsupportedOperationException",
            "AbstractMethodError",
            "AssertionError",
            "ClassCircularityError",
            "ClassFormatError",
            "Error",
            "ExceptionInInitializerError",
            "IllegalAccessError",
            "IncompatibleClassChangeError",
            "InstantiationError",
            "InternalError",
            "LinkageError",
            "NoClassDefFoundError",
            "NoSuchFieldError",
            "NoSuchMethodError",
            "OutOfMemoryError",
            "StackOverflowError",
            "ThreadDeath",
            "UnknownError",
            "UnsatisfiedLinkError",
            "UnsupportedClassVersionError",
            "VerifyError",
            "VirtualMachineError",
    };

    public LayoutBinder(ResourceBundle.LayoutFileBundle layoutBundle) {
        try {
            Scope.enter(this);
            mExprModel = new ExprModel();
            mExpressionParser = new ExpressionParser(mExprModel);
            mBindingTargets = new ArrayList<BindingTarget>();
            mBundle = layoutBundle;
            mModulePackage = layoutBundle.getModulePackage();
            HashSet<String> names = new HashSet<String>();
            // copy over data.
            for (ResourceBundle.VariableDeclaration variable : mBundle.getVariables()) {
                addVariable(variable.name, variable.type, variable.location, variable.declared);
                names.add(variable.name);
            }

            for (ResourceBundle.NameTypeLocation userImport : mBundle.getImports()) {
                mExprModel.addImport(userImport.name, userImport.type, userImport.location);
                names.add(userImport.name);
            }
            if (!names.contains("context")) {
                mExprModel.builtInVariable("context", "android.content.Context",
                        "getRoot().getContext()");
                names.add("context");
            }
            for (String javaLangClass : sJavaLangClasses) {
                mExprModel.addImport(javaLangClass, "java.lang." + javaLangClass, null);
            }
            // First resolve all the View fields
            // Ensure there are no conflicts with variable names
            for (BindingTargetBundle targetBundle : mBundle.getBindingTargetBundles()) {
                try {
                    Scope.enter(targetBundle);
                    final BindingTarget bindingTarget = createBindingTarget(targetBundle);
                    if (bindingTarget.getId() != null) {
                        final String fieldName = LayoutBinderWriterKt.
                                getReadableName(bindingTarget);
                        if (names.contains(fieldName)) {
                            L.w("View field %s collides with a variable or import", fieldName);
                        } else {
                            names.add(fieldName);
                            mExprModel.viewFieldExpr(bindingTarget);
                        }
                    }
                } finally {
                    Scope.exit();
                }
            }

            for (BindingTarget bindingTarget : mBindingTargets) {
                try {
                    Scope.enter(bindingTarget.mBundle);
                    for (BindingTargetBundle.BindingBundle bindingBundle : bindingTarget.mBundle
                            .getBindingBundleList()) {
                        try {
                            Scope.enter(bindingBundle.getValueLocation());
                            bindingTarget.addBinding(bindingBundle.getName(),
                                    parse(bindingBundle.getExpr(), bindingBundle.isTwoWay(),
                                            bindingBundle.getValueLocation()));
                        } finally {
                            Scope.exit();
                        }
                    }
                    bindingTarget.resolveTwoWayExpressions();
                    bindingTarget.resolveMultiSetters();
                    bindingTarget.resolveListeners();
                } finally {
                    Scope.exit();
                }
            }
            mSortedBindingTargets = new ArrayList<BindingTarget>(mBindingTargets);
            Collections.sort(mSortedBindingTargets, COMPARE_FIELD_NAME);
        } finally {
            Scope.exit();
        }
    }

    public void resolveWhichExpressionsAreUsed() {
        List<Expr> used = new ArrayList<Expr>();
        for (BindingTarget target : mBindingTargets) {
            for (Binding binding : target.getBindings()) {
                binding.getExpr().setIsUsed(true);
                used.add(binding.getExpr());
            }
        }
        while (!used.isEmpty()) {
            Expr e = used.remove(used.size() - 1);
            for (Dependency dep : e.getDependencies()) {
                if (!dep.getOther().isUsed()) {
                    used.add(dep.getOther());
                    dep.getOther().setIsUsed(true);
                }
            }
        }
    }

    public IdentifierExpr addVariable(String name, String type, Location location,
            boolean declared) {
        Preconditions.check(!mUserDefinedVariables.containsKey(name),
                "%s has already been defined as %s", name, type);
        final IdentifierExpr id = mExprModel.identifier(name);
        id.setUserDefinedType(type);
        id.enableDirectInvalidation();
        if (location != null) {
            id.addLocation(location);
        }
        mUserDefinedVariables.put(name, type);
        if (declared) {
            id.setDeclared();
        }
        return id;
    }

    public HashMap<String, String> getUserDefinedVariables() {
        return mUserDefinedVariables;
    }

    public BindingTarget createBindingTarget(ResourceBundle.BindingTargetBundle targetBundle) {
        final BindingTarget target = new BindingTarget(targetBundle);
        mBindingTargets.add(target);
        target.setModel(mExprModel);
        return target;
    }

    public Expr parse(String input, boolean isTwoWay, @Nullable Location locationInFile) {
        final Expr parsed = mExpressionParser.parse(input, locationInFile);
        parsed.setBindingExpression(true);
        parsed.setTwoWay(isTwoWay);
        return parsed;
    }

    public List<BindingTarget> getBindingTargets() {
        return mBindingTargets;
    }

    public List<BindingTarget> getSortedTargets() {
        return mSortedBindingTargets;
    }

    public boolean isEmpty() {
        return mExprModel.size() == 0;
    }

    public ExprModel getModel() {
        return mExprModel;
    }

    private void ensureWriter() {
        if (mWriter == null) {
            mWriter = new LayoutBinderWriter(this);
        }
    }

    public void sealModel() {
        mExprModel.seal();
    }

    public String writeViewBinderBaseClass(boolean forLibrary) {
        ensureWriter();
        return mWriter.writeBaseClass(forLibrary);
    }

    public String writeViewBinder(int minSdk) {
        ensureWriter();
        Preconditions.checkNotNull(getPackage(), "package cannot be null");
        Preconditions.checkNotNull(getClassName(), "base class name cannot be null");
        return mWriter.write(minSdk);
    }

    public String getPackage() {
        return mBundle.getBindingClassPackage();
    }

    public boolean isMerge() {
        return mBundle.isMerge();
    }

    public String getModulePackage() {
        return mModulePackage;
    }

    public String getLayoutname() {
        return mBundle.getFileName();
    }

    public String getImplementationName() {
        if (hasVariations()) {
            return mBundle.getBindingClassName() + mBundle.getConfigName() + "Impl";
        } else {
            return mBundle.getBindingClassName();
        }
    }
    
    public String getClassName() {
        return mBundle.getBindingClassName();
    }

    public String getTag() {
        return mBundle.getDirectory() + "/" + mBundle.getFileName();
    }

    public boolean hasVariations() {
        return mBundle.hasVariations();
    }

    @Override
    public String provideScopeFilePath() {
        return mBundle.getAbsoluteFilePath();
    }
}
