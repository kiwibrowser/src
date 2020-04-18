/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.io.Serializable;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/**
 * A reflection of a class, in the form that allows fast checks and information obtaining.
 *
 * @author Misha Dmitriev
 *  5 April 2004
 */
@SuppressWarnings("serial")
public class ClassInfo implements Serializable {

    public static final int VER_OLD = 0;  // Old version
    public static final int VER_NEW = 1;  // New version
    public static final int NO_VERSIONS = 2;  // Non-project class, no change tracking
    private transient PCDManager pcdm;
    transient int verCode;                          // Version code for this ClassInfo - one of the above.
    String name = null;
    transient String packageName;                   // Package name; restored when database is reloaded
    int javacTargetRelease = Utils.JAVAC_TARGET_RELEASE_OLDEST; // Can have values from Utils.JAVAC_TARGET_RELEASE_xxx
    String cpoolRefsToClasses[];          // Directly referenced class names trimmed of array and 'L' prefixes and ';' suffixes
    boolean isRefClassArray[];            // Indicates if a directly referenced class is actually an array class
    // In all signatures we replace the 'L' and ';' symbols that enclose non-primitive type names with '@' and '#' respectively,
    // so that class names inside signatures can be located fast and unambiguously.
    String cpoolRefsToFieldClasses[];     // Defining classes of referenced fields, trimmed of enclosing 'L' and ';' symbols
    String cpoolRefsToFieldNames[];       // Names of referenced fields
    String cpoolRefsToFieldSignatures[];  // Signatures of referenced fields
    String cpoolRefsToMethodClasses[];    // Defining classes of referenced methods, trimmed of enclosing 'L' and ';' symbols
    String cpoolRefsToMethodNames[];      // Names of referenced methods
    String cpoolRefsToMethodSignatures[]; // Signatures of referenced methods
    char accessFlags;                     // isInterface flag included
    boolean isNonMemberNestedClass = false; // True if this is a non-member nested class
    String superName;
    String interfaces[];
    String fieldNames[];
    String fieldSignatures[];
    char fieldAccessFlags[];
    Object primitiveConstantInitValues[];
    String methodNames[];
    String methodSignatures[];
    char methodAccessFlags[];
    String checkedExceptions[][];
    transient ClassInfo directSubclasses[];       // Direct subclasses. Created lazily and not preserved on disk.
    transient String directlyEnclosingClass;      // Directly enclosing class name; restored when database is reloaded
    transient String topLevelEnclosingClass;      // Top-level enclosing class name; restored when database is reloaded
    String nestedClasses[];             // Names of all nested classes. Don't make transient - it's used to check
    // if nested classes for this class were added/deleted in new version
    transient char nestedClassAccessFlags[];      // No need to store this information permanently
    transient boolean nestedClassNonMember[];     // Ditto

    /** Creates new ClassInfo out of a class file. The last parameter is needed only to produce sensible error reports.*/
    public ClassInfo(byte[] classFileBytes, int verCode, PCDManager pcdm, String classFileFullPath) {
        this.pcdm = pcdm;
        this.verCode = verCode;
        pcdm.getClassFileReader().readClassFile(classFileBytes, this, classFileFullPath, true);
        packageName = Utils.getPackageName(name);
        directlyEnclosingClass =
                Utils.getDirectlyEnclosingClass(name, this.javacTargetRelease);
        topLevelEnclosingClass = Utils.getTopLevelEnclosingClass(name);
    }

    /**
     * Create a "lightweight" ClassInfo, that contains just the class name, super name, interfaces, flags and verCode.
     * Used for non-project classes, that don't change themselves, for which we are only interested in type hierarchy structure.
     */
    public ClassInfo(byte[] classFileBytes, PCDManager pcdm, String classFileFullPath) {
        this.pcdm = pcdm;
        this.verCode = NO_VERSIONS;
        pcdm.getClassFileReader().readClassFile(classFileBytes, this, classFileFullPath, false);
        packageName = Utils.getPackageName(name);
        directlyEnclosingClass =
                Utils.getDirectlyEnclosingClass(name, this.javacTargetRelease);
        topLevelEnclosingClass = Utils.getTopLevelEnclosingClass(name);
    }

    /** Even more lightweight variant - created for a deleted non-project class, to enable minimum possible checks. */
    public ClassInfo(String name, PCDManager pcdm) {
        this.pcdm = pcdm;
        this.verCode = NO_VERSIONS;
        this.name = name;
        packageName = Utils.getPackageName(name);
        directlyEnclosingClass = Utils.getDirectlyEnclosingClass(name, 0);
        topLevelEnclosingClass = Utils.getTopLevelEnclosingClass(name);
    }

    public ClassInfo() {
    }

    /** Initialize transient data that can be initialized immediately after this ClassInfo is read from the project database */
    public void initializeImmediateTransientFields() {
        verCode = VER_OLD;

        packageName = Utils.getPackageName(name);

        directlyEnclosingClass =
                Utils.getDirectlyEnclosingClass(name, this.javacTargetRelease);
        topLevelEnclosingClass = Utils.getTopLevelEnclosingClass(name);
    }

    /**
     * Called to restore the pointer to the current PCDManager after this ClassInfo is brought back
     * from the store.
     */
    public void restorePCDM(PCDManager pcdm) {
        this.pcdm = pcdm;
    }

    public boolean isInterface() {
        return Modifier.isInterface(accessFlags);
    }

    public boolean isAbstract() {
        return Modifier.isAbstract(accessFlags);
    }

    public boolean isPublic() {
        return Modifier.isPublic(accessFlags);
    }

    /**
     * Returns the names of the superclasses of the given class (transitively), that belong
     * to the same project, plus those of the superclasses that can be found on the class path
     * supplied to jmake, and on the boot class path.
     */
    public List<String> getAllSuperclassNames() {
        List<String> res = new ArrayList<String>();
        String superName = this.superName;
        while (superName != null && !"java/lang/Object".equals(superName)) {
            res.add(superName);
            ClassInfo classInfo = pcdm.getClassInfoForName(verCode, superName);
            if (classInfo == null) { // Class not in project (or deleted?). Try to find it and further superclasses in non-project classes
                ClassPath.getSuperclasses(superName, res, pcdm);
                break;
            }
            superName = classInfo.superName;
        }
        return res;
    }

    /**
     * Returns the set of names of the interfaces transitively implemented by the given
     * class, that belong to the same project.
     */
    public Set<String> getAllImplementedIntfNames() {
        Set<String> res = new LinkedHashSet<String>();
        addImplementedInterfaceNames(false, res);
        return res;
    }

    /** Add to the given set the names of direct/all interfaces implemented by the given class. */
    private void addImplementedInterfaceNames(boolean directOnly,
            Set<String> intfSet) {
        if (interfaces != null) {
            for (int i = 0; i < interfaces.length; i++) {
                String superIntfName = interfaces[i];
                intfSet.add(superIntfName);
                if (directOnly) {
                    continue;
                }
                ClassInfo superIntfInfo =
                        pcdm.getClassInfoForName(verCode, superIntfName);
                if (superIntfInfo == null) {  // Class not in project
                    ClassPath.addAllImplementedInterfaceNames(superIntfName, intfSet, pcdm);
                } else {
                    superIntfInfo.addImplementedInterfaceNames(false, intfSet);
                }
            }
        }

        if (directOnly || superName == null ||
                "java/lang/Object".equals(superName)) {
            return;
        }
        ClassInfo superInfo = pcdm.getClassInfoForName(verCode, superName);
        if (superInfo == null) {  // Class not in project
            ClassPath.addAllImplementedInterfaceNames(superName, intfSet, pcdm);
        } else {
            superInfo.addImplementedInterfaceNames(false, intfSet);
        }
    }

    /** Returns the array of all direct subclasses of this class (array of zero length if there are none). */
    public ClassInfo[] getDirectSubclasses() {
        if (directSubclasses != null) {
            return directSubclasses;
        }

        List<ClassInfo> listRes = new ArrayList<ClassInfo>();

        for (PCDEntry entry : pcdm.entries()) {
            ClassInfo classInfo = pcdm.getClassInfoForPCDEntry(verCode, entry);
            if (classInfo == null) {
                continue;  // New or deleted class, depending on verCode
            }
            if (classInfo.superName.equals(name)) {
                listRes.add(classInfo);
            }
        }

        directSubclasses = listRes.toArray(new ClassInfo[listRes.size()]);
        return directSubclasses;
    }

    /** Check if the initial values for the given primitive constatnts in two classes are the same. */
    public static boolean constFieldInitValuesEqual(ClassInfo oldClassInfo, int oldFieldNo,
            ClassInfo newClassInfo, int newFieldNo) {
        Object oldInitValue = oldClassInfo.primitiveConstantInitValues == null ? null
                : oldClassInfo.primitiveConstantInitValues[oldFieldNo];
        Object newInitValue = newClassInfo.primitiveConstantInitValues == null ? null
                : newClassInfo.primitiveConstantInitValues[newFieldNo];
        if (oldInitValue == newInitValue) {
            return true;
        }
        if (oldInitValue == null || newInitValue == null) {
            return false;
        }

        if (oldInitValue instanceof Integer) {
            if (((Integer) oldInitValue).intValue() == ((Integer) newInitValue).intValue()) {
                return true;
            } else {
                return false;
            }
        } else if (oldInitValue instanceof String) {
            if ( ((String) oldInitValue).equals((String) newInitValue) ) {
                return true;
            } else {
                return false;
            }
        } else if (oldInitValue instanceof Long) {
            if (((Long) oldInitValue).longValue() == ((Long) newInitValue).longValue()) {
                return true;
            } else {
                return false;
            }
        } else if (oldInitValue instanceof Float) {
            if (((Float) oldInitValue).floatValue() == ((Float) newInitValue).floatValue()) {
                return true;
            } else {
                return false;
            }
        } else if (oldInitValue instanceof Double) {
            if (((Double) oldInitValue).doubleValue() == ((Double) newInitValue).doubleValue()) {
                return true;
            } else {
                return false;
            }
        }

        return true;
    }

    public boolean implementsInterfaceDirectly(String intfName) {
        if (interfaces == null) {
            return false;
        }
        for (int i = 0; i < interfaces.length; i++) {
            if (intfName.equals(interfaces[i])) {
                return true;
            }
        }
        return false;
    }

    /** Check if this class implements interface I or any subinterface of I directly */
    public boolean implementsIntfOrSubintfDirectly(String intfName) {
        if (interfaces == null) {
            return false;
        }
        for (int i = 0; i < interfaces.length; i++) {
            if (intfName.equals(interfaces[i])) {
                return true;
            }
            // An interface can have multiple superinterfaces, all of which are listed in its "interfaces" array
            // (although in the .java source it "extends" them all).
            ClassInfo superIntfInfo =
                    pcdm.getClassInfoForName(verCode, interfaces[i]);
            if (superIntfInfo == null) {
                continue;  // Class not in project
            }
            if (superIntfInfo.implementsIntfOrSubintfDirectly(intfName)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Class C implements interface I indirectly, if C or some superclass of C directly implements I
     * or some subinterface of I.
     */
    public boolean implementsInterfaceDirectlyOrIndirectly(String intfName) {
        if (interfaces == null) {
            return false;
        }

        if (implementsIntfOrSubintfDirectly(intfName)) {
            return true;
        }

        if (superName != null) {
            ClassInfo superInfo = pcdm.getClassInfoForName(verCode, superName);
            if (superInfo == null) {
                return false;  // Class not in project
            }
            return superInfo.implementsInterfaceDirectlyOrIndirectly(intfName);
        }

        return false;
    }

    /**
     * Returns true if this class declares a field with the same name and type as
     * the field number fieldNo in class classInfo.
     */
    public boolean declaresField(ClassInfo classInfo, int fieldNo) {
        if (fieldNames == null) {
            return false;
        }
        String fieldName = classInfo.fieldNames[fieldNo];
        String fieldSignature = classInfo.fieldSignatures[fieldNo];

        for (int i = 0; i < fieldNames.length; i++) {
            if (fieldName.equals(fieldNames[i]) &&
                    fieldSignature.equals(fieldSignatures[i])) {
                return true;
            }
        }
        return false;
    }

    /** Returns true if this class declares a field with the given name, signature and access */
    public boolean declaresField(String name, String signature, boolean isStatic) {
        if (fieldNames == null) {
            return false;
        }
        signature = ("@" + signature + "#").intern();
        for (int i = 0; i < fieldNames.length; i++) {
            if (name.equals(fieldNames[i]) &&
                    signature.equals(fieldSignatures[i]) &&
                    Modifier.isStatic(fieldAccessFlags[i]) == isStatic) {
                return true;
            }
        }
        return false;
    }

    /**
     * Returns true if this class declares a method with the same name and signature as
     * the method number methodNo in class classInfo.
     */
    public boolean declaresMethod(ClassInfo classInfo, int methodNo) {
        if (methodNames == null) {
            return false;
        }
        String methodName = classInfo.methodNames[methodNo];
        String methodSignature = classInfo.methodSignatures[methodNo];

        for (int i = 0; i < methodNames.length; i++) {
            if (methodName.equals(methodNames[i]) &&
                    methodSignature.equals(methodSignatures[i])) {
                return true;
            }
        }
        return false;
    }

    /**
     * If this class declares a method with the same name and signature as the given method,
     * return its position. Otherwise, return -1.
     */
    public int getDeclaredMethodPos(ClassInfo classInfo, int methodNo) {
        if (methodNames == null) {
            return -1;
        }
        String methodName = classInfo.methodNames[methodNo];
        String methodSignature = classInfo.methodSignatures[methodNo];

        for (int i = 0; i < methodNames.length; i++) {
            if (methodName.equals(methodNames[i]) &&
                    methodSignature.equals(methodSignatures[i])) {
                return i;
            }
        }
        return -1;
    }

    /**
     * Returns a nonnegative number (position in the method array) if this class declares a method with the
     * name methodName, and -1 otherwise.
     */
    public int declaresSameNameMethod(String methodName) {
        if (methodNames == null) {
            return -1;
        }
        for (int j = 0; j < methodNames.length; j++) {
            if (methodName.equals(methodNames[j])) {
                return j;
            }
        }
        return -1;
    }

    /**
     * Check if this class references the given class in different ways, depending on thorDegree parameter.
     * thorDegree = 0: the given class (but not its array class) directly from the constantpool.
     *
     * thorDegree = 1: the given class or its array class directly from the constantpool, as a
     * type of a data field, as a type in a method signature or a thrown exception, as a directly
     * implemented interface or a direct superclass
     *
     * thorDegree = 2: the given class or its array class directly or indirectly from the
     * constantpool, as a type of a data field, as a type in a method signature or a thrown exception,
     * as a directly/indirectly implemented interface or a direct/indirect superclass.
     *
     * isRefTypeInterface indicates whether className is an interface.
     */
    public boolean referencesClass(String className, boolean isRefTypeInterface, int thorDegree) {
        int i;

        if (thorDegree == 0) {
            if (cpoolRefsToClasses == null) {
                return false;
            }
            for (i = 0; i < cpoolRefsToClasses.length; i++) {
                if (!isRefClassArray[i] &&
                        className.equals(cpoolRefsToClasses[i])) {
                    return true;
                }
            }
        } else {
            if (isSubclassOf(className, (thorDegree == 1))) {
                return true;
            }
            if (isRefTypeInterface) {
                if (thorDegree == 1) {
                    if (implementsInterfaceDirectly(className)) {
                        return true;
                    }
                } else {
                    // Check for indirectly implemented interfaces
                    if (implementsInterfaceDirectlyOrIndirectly(className)) {
                        return true;
                    }
                }
            }

            if (cpoolRefsToClasses != null) {
                for (i = 0; i < cpoolRefsToClasses.length; i++) {
                    if (className.equals(cpoolRefsToClasses[i])) {
                        return true;
                    }
                }
            }
            if (thorDegree == 2) {
                // Check for indirect references from the constantpool
                if (cpoolRefsToFieldSignatures != null) {
                    for (i = 0; i < cpoolRefsToFieldSignatures.length; i++) {
                        if (signatureIncludesClassName(cpoolRefsToFieldSignatures[i], className)) {
                            return true;
                        }
                    }
                }
                if (cpoolRefsToMethodNames != null) {
                    for (i = 0; i < cpoolRefsToMethodSignatures.length; i++) {
                        if (signatureIncludesClassName(cpoolRefsToMethodSignatures[i], className)) {
                            return true;
                        }
                    }
                }
            }

            if (fieldSignatures != null) {
                for (i = 0; i < fieldSignatures.length; i++) {
                    if (signatureIncludesClassName(fieldSignatures[i], className)) {
                        return true;
                    }
                }
            }
            if (methodSignatures != null) {
                for (i = 0; i < methodSignatures.length; i++) {
                    if (signatureIncludesClassName(methodSignatures[i], className)) {
                        return true;
                    }
                }
            }
            if (checkedExceptions != null) {
                for (i = 0; i < checkedExceptions.length; i++) {
                    if (checkedExceptions[i] != null) {
                        String excArray[] = checkedExceptions[i];
                        for (int j = 0; j < excArray.length; j++) {
                            if (className.equals(excArray[j])) {
                                return true;
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    private static boolean signatureIncludesClassName(String signature, String className) {
        int stIndex = signature.indexOf(className);
        if (stIndex == -1) {
            return false;
        }
        return ((stIndex != 0 && signature.charAt(stIndex - 1) == '@' && signature.charAt(stIndex + className.length()) == '#') ||
                (stIndex == 0 && signature.length() == className.length()));
    }

    public boolean isSubclassOf(String className, boolean directOnly) {
        if (className.equals(superName)) {
            return true;
        }
        if (directOnly) {
            return false;
        }
        String superName = this.superName;
        while (superName != null) {
            if (className.equals(superName)) {
                return true;
            }
            ClassInfo classInfo = pcdm.getClassInfoForName(verCode, superName);
            if (classInfo == null) {
                break;  // Class not in project
            }
            superName = classInfo.superName;
        }
        return false;
    }

    /**
     * Check if this class references field number fieldNo of class fieldDefClassInfo. Let us call
     * this field C.f. Actual reference contained in the constant pool may be not to C.f itself,
     * but to Csub.f, where Csub is some subclass of C such that neither Csub nor any other class
     * located between C and Csub in the class hierarchy redeclares f. We look up both "real"
     * references C.f and "fake" references such as Csub.f.
     */
    public boolean referencesField(ClassInfo fieldDefClassInfo, int fieldNo) {
        if (cpoolRefsToFieldNames == null) {
            return false;
        }
        String fieldDefClassName = fieldDefClassInfo.name;
        String fieldName = fieldDefClassInfo.fieldNames[fieldNo];
        String fieldSig = fieldDefClassInfo.fieldSignatures[fieldNo];
        for (int i = 0; i < cpoolRefsToFieldNames.length; i++) {
            if (fieldName.equals(cpoolRefsToFieldNames[i]) &&
                    fieldSig.equals(cpoolRefsToFieldSignatures[i]) ) {
                if (fieldDefClassName.equals(cpoolRefsToFieldClasses[i]) ) {
                    return true;  // "real" reference
                } else {  // Check if this is a "fake" reference that resolves to the above "real" reference
                    ClassInfo classInThisCpool =
                            pcdm.getClassInfoForName(verCode, cpoolRefsToFieldClasses[i]);
                    if (classInThisCpool == null) {
                        continue;  // Class not in project
                    }
                    if (!classInThisCpool.isSubclassOf(fieldDefClassInfo.name, false)) {
                        continue;
                    }

                    // Ok, now check that this field is not actually redeclared in fieldDefClassInfo or
                    // somewhere in between it and classInThisCpool
                    boolean redeclared = false;
                    ClassInfo curClass = classInThisCpool;
                    do {
                        if (curClass.declaresField(fieldDefClassInfo, fieldNo)) {
                            redeclared = true;
                            break;
                        }
                        String superName = curClass.superName;
                        curClass = pcdm.getClassInfoForName(verCode, superName);
                        if (curClass == null) {
                            break;
                        }
                    } while (curClass != fieldDefClassInfo);
                    if (!redeclared) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /**
     * Check if this class references method number methodNo of class methodDefClassInfo. Let us
     * call this method C.m. Actual reference contained in the constant pool may be not to C.m
     * itself, but to Csub.m, where Csub is some subclass of C such that neither Csub nor any
     * other class located between C and Csub in the class hierarchy redeclares m. We look up
     * both "real" references C.m and "fake" references such as Csub.m.
     */
    public boolean referencesMethod(ClassInfo methodDefClassInfo, int methodNo) {
        if (cpoolRefsToMethodNames == null) {
            return false;
        }
        String methodDefClassName = methodDefClassInfo.name;
        String methodName = methodDefClassInfo.methodNames[methodNo];
        String methodSig = methodDefClassInfo.methodSignatures[methodNo];
        for (int i = 0; i < cpoolRefsToMethodNames.length; i++) {
            if (methodName.equals(cpoolRefsToMethodNames[i]) &&
                    methodSig.equals(cpoolRefsToMethodSignatures[i])) {
                if (methodDefClassName.equals(cpoolRefsToMethodClasses[i])) {
                    return true;  // "real" reference
                } else {  // Check if this is a "fake" reference that resolves to the above "real" reference
                    // Be careful - class in the cpool may be not a project class (e.g. a core class).
                    ClassInfo classInThisCpool =
                            pcdm.getClassInfoForName(verCode, cpoolRefsToMethodClasses[i]);
                    if (classInThisCpool == null) {
                        continue;  // Class not in project
                    }
                    if (classInThisCpool.isSubclassOf(methodDefClassInfo.name, false)) {
                        // Ok, now check that this method is not actually redeclared in classInThisCpool (which is
                        // lower in the hierarchy) or somewhere in between it and classInThisCpool
                        boolean redeclared = false;
                        ClassInfo curClass = classInThisCpool;
                        do {
                            if (curClass.declaresMethod(methodDefClassInfo, methodNo)) {
                                redeclared = true;
                                break;
                            }
                            String superName = curClass.superName;
                            curClass =
                                    pcdm.getClassInfoForName(verCode, superName);
                            if (curClass == null) {
                                break;
                            }
                        } while (curClass != methodDefClassInfo);
                        if (!redeclared) {
                            return true;
                        }
                    } else if (methodDefClassInfo.isInterface() && classInThisCpool.implementsIntfOrSubintfDirectly(methodDefClassName)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /**
     * If this class has a method that throws the given exception, return its index. Otherwise return -1.
     * The search starts from method with index startMethodIdx.
     */
    public int hasMethodThrowingException(ClassInfo excClassInfo, int startMethodIdx) {
        if (checkedExceptions == null) {
            return -1;
        }
        if (startMethodIdx >= checkedExceptions.length) {
            return -1;
        }
        String excName = excClassInfo.name;
        for (int i = startMethodIdx; i < checkedExceptions.length; i++) {
            if (checkedExceptions[i] == null) {
                continue;
            }
            String[] exc = checkedExceptions[i];
            for (int j = 0; j < exc.length; j++) {
                if (exc[j].equals(excName)) {
                    return i;
                }
            }
        }
        return -1;
    }

    public static abstract class MethodHandler {

        abstract void handleMethod(ClassInfo ci, int methodIdx);
    }

    /**
     * Check this class and all its superclasses (if includeSuperclasses == true) and superinterfaces (if includeInterfaces == true)
     * for a method with the given name. If such a method is found, call h.handleMethod(classInfo, methodIdx).
     */
    public void findExistingSameNameMethods(String methodName, boolean includeSuperclasses, boolean includeInterfaces, MethodHandler h) {
        String className = name;
        ClassInfo classInfo;
        while (className != null) {
            classInfo = pcdm.getClassInfoForName(verCode, className);
            if (classInfo == null) {
                break;  // Class not in project
            }
            String mNames[] = classInfo.methodNames;
            int mNamesLen = mNames != null ? mNames.length : 0;
            for (int i = 0; i < mNamesLen; i++) {
                if (methodName.equals(mNames[i])) {
                    h.handleMethod(classInfo, i);
                }
            }
            if (includeInterfaces && classInfo.interfaces != null) {
                String intfNames[] = classInfo.interfaces;
                for (int i = 0; i < intfNames.length; i++) {
                    ClassInfo superIntfInfo =
                            pcdm.getClassInfoForName(verCode, intfNames[i]);
                    if (superIntfInfo == null) {
                        continue;  // Class not in project
                    }
                    superIntfInfo.findExistingSameNameMethods(methodName, true, includeInterfaces, h);
                }
            }
            if (includeSuperclasses) {
                className = classInfo.superName;
            } else {
                return;
            }
        }
    }

    public static boolean isPrimitiveFieldSig(String fieldSig) {
        return fieldSig.indexOf('@') == -1;
    }

    /**
     * Check if the given signature is of a class type, and that class does not belong to the project.
     * It used to be a check for just a core type name, but sometimes people use JDK sources as e.g. a test
     * case - so better perform a universal (and entirely correct, unlike just a core type name) test here.
     */
    public boolean isNonProjectClassTypeFieldSig(String fieldSig) {
        int stPos = fieldSig.indexOf('@');
        if (stPos == -1) {
            return false;
        }
        int endPos = fieldSig.indexOf('#');
        String className = fieldSig.substring(stPos + 1, endPos);
        return (!pcdm.isProjectClass(verCode, className));
    }

    /** For debugging. */
    public String toString() {
        return name + (verCode == VER_OLD ? " OLD" : " NEW");
    }
}
