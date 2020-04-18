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
package android.databinding.tool.reflection;

import android.databinding.tool.reflection.Callable.Type;
import android.databinding.tool.util.L;
import android.databinding.tool.util.StringUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static android.databinding.tool.reflection.Callable.CAN_BE_INVALIDATED;
import static android.databinding.tool.reflection.Callable.DYNAMIC;
import static android.databinding.tool.reflection.Callable.STATIC;

public abstract class ModelClass {
    public abstract String toJavaCode();

    /**
     * @return whether this ModelClass represents an array.
     */
    public abstract boolean isArray();

    /**
     * For arrays, lists, and maps, this returns the contained value. For other types, null
     * is returned.
     *
     * @return The component type for arrays, the value type for maps, and the element type
     * for lists.
     */
    public abstract ModelClass getComponentType();

    /**
     * @return Whether or not this ModelClass can be treated as a List. This means
     * it is a java.util.List, or one of the Sparse*Array classes.
     */
    public boolean isList() {
        for (ModelClass listType : ModelAnalyzer.getInstance().getListTypes()) {
            if (listType != null) {
                if (listType.isAssignableFrom(this)) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @return whether or not this ModelClass can be considered a Map or not.
     */
    public boolean isMap()  {
        return ModelAnalyzer.getInstance().getMapType().isAssignableFrom(erasure());
    }

    /**
     * @return whether or not this ModelClass is a java.lang.String.
     */
    public boolean isString() {
        return ModelAnalyzer.getInstance().getStringType().equals(this);
    }

    /**
     * @return whether or not this ModelClass represents a Reference type.
     */
    public abstract boolean isNullable();

    /**
     * @return whether or not this ModelClass represents a primitive type.
     */
    public abstract boolean isPrimitive();

    /**
     * @return whether or not this ModelClass represents a Java boolean
     */
    public abstract boolean isBoolean();

    /**
     * @return whether or not this ModelClass represents a Java char
     */
    public abstract boolean isChar();

    /**
     * @return whether or not this ModelClass represents a Java byte
     */
    public abstract boolean isByte();

    /**
     * @return whether or not this ModelClass represents a Java short
     */
    public abstract boolean isShort();

    /**
     * @return whether or not this ModelClass represents a Java int
     */
    public abstract boolean isInt();

    /**
     * @return whether or not this ModelClass represents a Java long
     */
    public abstract boolean isLong();

    /**
     * @return whether or not this ModelClass represents a Java float
     */
    public abstract boolean isFloat();

    /**
     * @return whether or not this ModelClass represents a Java double
     */
    public abstract boolean isDouble();

    /**
     * @return whether or not this has type parameters
     */
    public abstract boolean isGeneric();

    /**
     * @return a list of Generic type paramters for the class. For example, if the class
     * is List&lt;T>, then the return value will be a list containing T. null is returned
     * if this is not a generic type
     */
    public abstract List<ModelClass> getTypeArguments();

    /**
     * @return whether this is a type variable. For example, in List&lt;T>, T is a type variable.
     * However, List&lt;String>, String is not a type variable.
     */
    public abstract boolean isTypeVar();

    /**
     * @return whether this is a wildcard type argument or not.
     */
    public abstract boolean isWildcard();

    /**
     * @return whether or not this ModelClass is java.lang.Object and not a primitive or subclass.
     */
    public boolean isObject() {
        return ModelAnalyzer.getInstance().getObjectType().equals(this);
    }

    /**
     * @return whether or not this ModelClass is an interface
     */
    public abstract boolean isInterface();

    /**
     * @return whether or not his is a ViewDataBinding subclass.
     */
    public boolean isViewDataBinding() {
        return ModelAnalyzer.getInstance().getViewDataBindingType().isAssignableFrom(this);
    }

    /**
     * @return whether or not this ModelClass type extends ViewStub.
     */
    public boolean extendsViewStub() {
        return ModelAnalyzer.getInstance().getViewStubType().isAssignableFrom(this);
    }

    /**
     * @return whether or not this is an Observable type such as ObservableMap, ObservableList,
     * or Observable.
     */
    public boolean isObservable() {
        ModelAnalyzer modelAnalyzer = ModelAnalyzer.getInstance();
        return modelAnalyzer.getObservableType().isAssignableFrom(this) ||
                modelAnalyzer.getObservableListType().isAssignableFrom(this) ||
                modelAnalyzer.getObservableMapType().isAssignableFrom(this);

    }

    /**
     * @return whether or not this is an ObservableField, or any of the primitive versions
     * such as ObservableBoolean and ObservableInt
     */
    public boolean isObservableField() {
        ModelClass erasure = erasure();
        for (ModelClass observableField : ModelAnalyzer.getInstance().getObservableFieldTypes()) {
            if (observableField.isAssignableFrom(erasure)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @return whether or not this ModelClass represents a void
     */
    public abstract boolean isVoid();

    /**
     * When this is a boxed type, such as Integer, this will return the unboxed value,
     * such as int. If this is not a boxed type, this is returned.
     *
     * @return The unboxed type of the class that this ModelClass represents or this if it isn't a
     * boxed type.
     */
    public abstract ModelClass unbox();

    /**
     * When this is a primitive type, such as boolean, this will return the boxed value,
     * such as Boolean. If this is not a primitive type, this is returned.
     *
     * @return The boxed type of the class that this ModelClass represents or this if it isn't a
     * primitive type.
     */
    public abstract ModelClass box();

    /**
     * Returns whether or not the type associated with <code>that</code> can be assigned to
     * the type associated with this ModelClass. If this and that only require boxing or unboxing
     * then true is returned.
     *
     * @param that the ModelClass to compare.
     * @return true if <code>that</code> requires only boxing or if <code>that</code> is an
     * implementation of or subclass of <code>this</code>.
     */
    public abstract boolean isAssignableFrom(ModelClass that);

    /**
     * Returns an array containing all public methods on the type represented by this ModelClass
     * with the name <code>name</code> and can take the passed-in types as arguments. This will
     * also work if the arguments match VarArgs parameter.
     *
     * @param name The name of the method to find.
     * @param args The types that the method should accept.
     * @param staticOnly Whether only static methods should be returned or both instance methods
     *                 and static methods are valid.
     *
     * @return An array containing all public methods with the name <code>name</code> and taking
     * <code>args</code> parameters.
     */
    public ModelMethod[] getMethods(String name, List<ModelClass> args, boolean staticOnly) {
        ModelMethod[] methods = getDeclaredMethods();
        ArrayList<ModelMethod> matching = new ArrayList<ModelMethod>();
        for (ModelMethod method : methods) {
            if (method.isPublic() && (!staticOnly || method.isStatic()) &&
                    name.equals(method.getName()) && method.acceptsArguments(args)) {
                matching.add(method);
            }
        }
        return matching.toArray(new ModelMethod[matching.size()]);
    }

    /**
     * Returns all public instance methods with the given name and number of parameters.
     *
     * @param name The name of the method to find.
     * @param numParameters The number of parameters that the method should take
     * @return An array containing all public methods with the given name and number of parameters.
     */
    public ModelMethod[] getMethods(String name, int numParameters) {
        ModelMethod[] methods = getDeclaredMethods();
        ArrayList<ModelMethod> matching = new ArrayList<ModelMethod>();
        for (ModelMethod method : methods) {
            if (method.isPublic() && !method.isStatic() &&
                    name.equals(method.getName()) &&
                    method.getParameterTypes().length == numParameters) {
                matching.add(method);
            }
        }
        return matching.toArray(new ModelMethod[matching.size()]);
    }

    /**
     * Returns the public method with the name <code>name</code> with the parameters that
     * best match args. <code>staticOnly</code> governs whether a static or instance method
     * will be returned. If no matching method was found, null is returned.
     *
     * @param name The method name to find
     * @param args The arguments that the method should accept
     * @param staticOnly true if the returned method must be static or false if it does not
     *                     matter.
     */
    public ModelMethod getMethod(String name, List<ModelClass> args, boolean staticOnly) {
        ModelMethod[] methods = getMethods(name, args, staticOnly);
        L.d("looking methods for %s. static only ? %s . method count: %d", name, staticOnly,
                methods.length);
        for (ModelMethod method : methods) {
            L.d("method: %s, %s", method.getName(), method.isStatic());
        }
        if (methods.length == 0) {
            return null;
        }
        ModelMethod bestMethod = methods[0];
        for (int i = 1; i < methods.length; i++) {
            if (methods[i].isBetterArgMatchThan(bestMethod, args)) {
                bestMethod = methods[i];
            }
        }
        return bestMethod;
    }

    /**
     * If this represents a class, the super class that it extends is returned. If this
     * represents an interface, the interface that this extends is returned.
     * <code>null</code> is returned if this is not a class or interface, such as an int, or
     * if it is java.lang.Object or an interface that does not extend any other type.
     *
     * @return The class or interface that this ModelClass extends or null.
     */
    public abstract ModelClass getSuperclass();

    /**
     * @return A String representation of the class or interface that this represents, not
     * including any type arguments.
     */
    public String getCanonicalName() {
        return erasure().toJavaCode();
    }

    /**
     * @return The class or interface name of this type or the primitive type if it isn't a
     * reference type.
     */
    public String getSimpleName() {
        final String canonicalName = getCanonicalName();
        final int dotIndex = canonicalName.lastIndexOf('.');
        if (dotIndex >= 0) {
            return canonicalName.substring(dotIndex + 1);
        }
        return canonicalName;
    }

    /**
     * Returns this class type without any generic type arguments.
     * @return this class type without any generic type arguments.
     */
    public abstract ModelClass erasure();

    /**
     * Since when this class is available. Important for Binding expressions so that we don't
     * call non-existing APIs when setting UI.
     *
     * @return The SDK_INT where this method was added. If it is not a framework method, should
     * return 1.
     */
    public int getMinApi() {
        return SdkUtil.getMinApi(this);
    }

    /**
     * Returns the JNI description of the method which can be used to lookup it in SDK.
     * @see TypeUtil
     */
    public abstract String getJniDescription();

    /**
     * Returns a list of all abstract methods in the type.
     */
    public List<ModelMethod> getAbstractMethods() {
        ArrayList<ModelMethod> abstractMethods = new ArrayList<ModelMethod>();
        ModelMethod[] methods = getDeclaredMethods();
        for (ModelMethod method : methods) {
            if (method.isAbstract()) {
                abstractMethods.add(method);
            }
        }
        return abstractMethods;
    }

    /**
     * Returns the getter method or field that the name refers to.
     * @param name The name of the field or the body of the method name -- can be name(),
     *             getName(), or isName().
     * @param staticOnly Whether this should look for static methods and fields or instance
     *                     versions
     * @return the getter method or field that the name refers to or null if none can be found.
     */
    public Callable findGetterOrField(String name, boolean staticOnly) {
        if ("length".equals(name) && isArray()) {
            return new Callable(Type.FIELD, name, null,
                    ModelAnalyzer.getInstance().loadPrimitive("int"), 0, 0);
        }
        String capitalized = StringUtils.capitalize(name);
        String[] methodNames = {
                "get" + capitalized,
                "is" + capitalized,
                name
        };
        for (String methodName : methodNames) {
            ModelMethod[] methods = getMethods(methodName, new ArrayList<ModelClass>(), staticOnly);
            for (ModelMethod method : methods) {
                if (method.isPublic() && (!staticOnly || method.isStatic()) &&
                        !method.getReturnType(Arrays.asList(method.getParameterTypes())).isVoid()) {
                    int flags = DYNAMIC;
                    if (method.isStatic()) {
                        flags |= STATIC;
                    }
                    if (method.isBindable()) {
                        flags |= CAN_BE_INVALIDATED;
                    } else {
                        // if method is not bindable, look for a backing field
                        final ModelField backingField = getField(name, true, method.isStatic());
                        L.d("backing field for method %s is %s", method.getName(),
                                backingField == null ? "NOT FOUND" : backingField.getName());
                        if (backingField != null && backingField.isBindable()) {
                            flags |= CAN_BE_INVALIDATED;
                        }
                    }
                    final ModelMethod setterMethod = findSetter(method, name);
                    final String setterName = setterMethod == null ? null : setterMethod.getName();
                    final Callable result = new Callable(Callable.Type.METHOD, methodName,
                            setterName, method.getReturnType(null), method.getParameterTypes().length,
                            flags);
                    return result;
                }
            }
        }

        // could not find a method. Look for a public field
        ModelField publicField = null;
        if (staticOnly) {
            publicField = getField(name, false, true);
        } else {
            // first check non-static
            publicField = getField(name, false, false);
            if (publicField == null) {
                // check for static
                publicField = getField(name, false, true);
            }
        }
        if (publicField == null) {
            return null;
        }
        ModelClass fieldType = publicField.getFieldType();
        int flags = 0;
        String setterFieldName = name;
        if (publicField.isStatic()) {
            flags |= STATIC;
        }
        if (!publicField.isFinal()) {
            setterFieldName = null;
            flags |= DYNAMIC;
        }
        if (publicField.isBindable()) {
            flags |= CAN_BE_INVALIDATED;
        }
        return new Callable(Callable.Type.FIELD, name, setterFieldName, fieldType, 0, flags);
    }

    public ModelMethod findInstanceGetter(String name) {
        String capitalized = StringUtils.capitalize(name);
        String[] methodNames = {
                "get" + capitalized,
                "is" + capitalized,
                name
        };
        for (String methodName : methodNames) {
            ModelMethod[] methods = getMethods(methodName, new ArrayList<ModelClass>(), false);
            for (ModelMethod method : methods) {
                if (method.isPublic() && !method.isStatic() &&
                        !method.getReturnType(Arrays.asList(method.getParameterTypes())).isVoid()) {
                    return method;
                }
            }
        }
        return null;
    }

    private ModelField getField(String name, boolean allowPrivate, boolean isStatic) {
        ModelField[] fields = getDeclaredFields();
        for (ModelField field : fields) {
            boolean nameMatch = name.equals(field.getName()) ||
                    name.equals(stripFieldName(field.getName()));
            if (nameMatch && field.isStatic() == isStatic &&
                    (allowPrivate || field.isPublic())) {
                return field;
            }
        }
        return null;
    }

    private ModelMethod findSetter(ModelMethod getter, String originalName) {
        final String capitalized = StringUtils.capitalize(originalName);
        final String[] possibleNames;
        if (originalName.equals(getter.getName())) {
            possibleNames = new String[] { originalName, "set" + capitalized };
        } else if (getter.getName().startsWith("is")){
            possibleNames = new String[] { "set" + capitalized, "setIs" + capitalized };
        } else {
            possibleNames = new String[] { "set" + capitalized };
        }
        for (String name : possibleNames) {
            List<ModelMethod> methods = findMethods(name, getter.isStatic());
            if (methods != null) {
                ModelClass param = getter.getReturnType(null);
                for (ModelMethod method : methods) {
                    ModelClass[] parameterTypes = method.getParameterTypes();
                    if (parameterTypes != null && parameterTypes.length == 1 &&
                            parameterTypes[0].equals(param) &&
                            method.isStatic() == getter.isStatic()) {
                        return method;
                    }
                }
            }
        }
        return null;
    }

    /**
     * Finds public methods that matches the given name exactly. These may be resolved into
     * listener methods during Expr.resolveListeners.
     */
    public List<ModelMethod> findMethods(String name, boolean staticOnly) {
        ModelMethod[] methods = getDeclaredMethods();
        ArrayList<ModelMethod> matching = new ArrayList<ModelMethod>();
        for (ModelMethod method : methods) {
            if (method.getName().equals(name) && (!staticOnly || method.isStatic()) &&
                    method.isPublic()) {
                matching.add(method);
            }
        }
        if (matching.isEmpty()) {
            return null;
        }
        return matching;
    }

    public boolean isIncomplete() {
        if (isTypeVar() || isWildcard()) {
            return true;
        }
        List<ModelClass> typeArgs = getTypeArguments();
        if (typeArgs != null) {
            for (ModelClass typeArg : typeArgs) {
                if (typeArg.isIncomplete()) {
                    return true;
                }
            }
        }
        return false;
    }

    protected abstract ModelField[] getDeclaredFields();

    protected abstract ModelMethod[] getDeclaredMethods();

    private static String stripFieldName(String fieldName) {
        // TODO: Make this configurable through IntelliJ
        if (fieldName.length() > 2) {
            final char start = fieldName.charAt(2);
            if (fieldName.startsWith("m_") && Character.isJavaIdentifierStart(start)) {
                return Character.toLowerCase(start) + fieldName.substring(3);
            }
        }
        if (fieldName.length() > 1) {
            final char start = fieldName.charAt(1);
            final char fieldIdentifier = fieldName.charAt(0);
            final boolean strip;
            if (fieldIdentifier == '_') {
                strip = true;
            } else if (fieldIdentifier == 'm' && Character.isJavaIdentifierStart(start) &&
                    !Character.isLowerCase(start)) {
                strip = true;
            } else {
                strip = false; // not mUppercase format
            }
            if (strip) {
                return Character.toLowerCase(start) + fieldName.substring(2);
            }
        }
        return fieldName;
    }
}
