/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.lang.reflect.Modifier;
import java.util.LinkedHashSet;
import java.util.Set;

/**
 * This class implements finding classes referencing other classes and members in various ways.
 *
 * @author Misha Dmitriev
 * 12 March 2004
 */
public class RefClassFinder {

    private boolean failOnDependentJar;   // If true, will fail if a dependency of a sourceless class
    // (coming from a .jar) on a "normal" class is detected
    private boolean noWarnOnDependentJar; // If true, not even a warning will be issued in the above case.
    private String checkedClassName;
    private PCDManager pcdm;
    private Set<String> affectedClassNames;
    private boolean checkedClassIsFromJar;

    /** An instance of RefClassFinder is created once per session, passing it the global options that do not change */
    public RefClassFinder(PCDManager pcdm, boolean failOnDependentJar, boolean noWarnOnDependentJar) {
        this.pcdm = pcdm;
        this.failOnDependentJar = failOnDependentJar;
        this.noWarnOnDependentJar = noWarnOnDependentJar;
    }

    /** This method is called every time we are going to check a new class */
    public void initialize(String checkedClassName, boolean checkedClassIsFromJar) {
        this.checkedClassName = checkedClassName;
        this.checkedClassIsFromJar = checkedClassIsFromJar;
        affectedClassNames = new LinkedHashSet<String>();
    }

    /**
     * Returns the names of project classes that were found potentially affec
     * by the changes to the checked class.
     */
    public String[] getAffectedClassNames() {
        int size = affectedClassNames.size();
        if (size == 0) {
            return null;
        } else {
            String[] ret = new String[size];
            int i = 0;
            for (String className : affectedClassNames) {
                ret[i++] = className;
            }
            return ret;
        }
    }

    /**
     * Find all project classes that can access field fieldNo of class fieldClassInfo.
     * Used if a compile-time constant is changed.
     */
    public void findAllProjectClasses(ClassInfo fieldClassInfo, int fieldNo) {
      for (PCDEntry pcde : pcdm.entries()) {
            if (pcde.checkResult == PCDEntry.CV_DELETED) {
                continue;
            }
            if (pcde.javaFileFullPath.endsWith(".jar")) {
                continue;
            }
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (clientInfo == null) {
                continue;  // New class
            }
            if (memberAccessibleFrom(fieldClassInfo, fieldNo, clientInfo, true)) {
                addToAffectedClassNames(clientInfo.name);
            }
        }
    }

    /**
     * Find all project classes that reference class with the given name
     * (but not its array class) directly from the constantpool.
     */
    public void findReferencingClasses0(ClassInfo classInfo) {
        findReferencingClasses(classInfo, 0, false, null);
    }


    /* In the following "find...ReferencingClasses1" methods, "referencing C" means
     * "referencing C or its array class directly from the constant pool, as a type of a data
     * field, as a type in a method signature or a thrown exception, as a directly implemented
     * interface or a direct superclass".
     */
    /** Used for deleted classes. */
    public void findReferencingClassesForDeletedClass(ClassInfo classInfo) {
        String packageName = classInfo.packageName;
        boolean isPublic = classInfo.isPublic();
        boolean isInterface = classInfo.isInterface();
        for (PCDEntry pcde : pcdm.entries()) {
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (clientInfo == null) {
                continue;  // New class
            }
            if (!isPublic && packageName.equals(clientInfo.packageName)) {
                continue;
            }
            if (clientInfo.referencesClass(classInfo.name, isInterface, 1)) {
                addToAffectedClassNames(clientInfo.name);
            }
        }

    }

    /**
     * For the given class p.C, find each project class X referencing C, that is not a member of
     * package p and is not a direct or indirect subclass of C's directly enclosing class.
     * (public -&gt; protected transformation)
     */
    public void findDiffPackageAndNotSubReferencingClasses1(ClassInfo classInfo) {
        String packageName = classInfo.packageName;
        String directlyEnclosingClass = classInfo.directlyEnclosingClass;
        for (PCDEntry pcde : pcdm.entries()) {
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (clientInfo == null) {
                continue;  // New class
            }
            if (packageName.equals(clientInfo.packageName) ||
                    clientInfo.isSubclassOf(directlyEnclosingClass, false)) {
                continue;
            }
            if (clientInfo.referencesClass(classInfo.name, classInfo.isInterface(), 1)) {
                addToAffectedClassNames(clientInfo.name);
            }
        }
    }

    /**
     * For class p.C, find each project class X referencing C, whose top level enclosing
     * class is different from that of C.
     * (public -&gt; private transformation)
     */
    public void findReferencingClasses1(ClassInfo classInfo) {
        String topLevelEnclosingClass = classInfo.topLevelEnclosingClass;
        for (PCDEntry pcde : pcdm.entries()) {
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (clientInfo == null) {
                continue;  // New class
            }
            if (topLevelEnclosingClass.equals(clientInfo.topLevelEnclosingClass)) {
                continue;
            }
            if (clientInfo.referencesClass(classInfo.name, classInfo.isInterface(), 1)) {
                addToAffectedClassNames(clientInfo.name);
            }
        }
    }

    /**
     * For class p.C, find each project class X referencing C, whose direct or indirect superclass
     * is C's directly enclosing class, or which is a member of package p, whose top level enclosing
     * class is different from that of C.
     * (protected -&gt; private transformation)
     */
    public void findThisPackageOrSubReferencingClasses1(ClassInfo classInfo) {
        String directlyEnclosingClass = classInfo.directlyEnclosingClass;
        String topLevelEnclosingClass = classInfo.topLevelEnclosingClass;
        String packageName = classInfo.packageName;
      for (PCDEntry entry : pcdm.entries()) {
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, entry);
            if (clientInfo == null) {
                continue;  // New class
            }
            if ((!clientInfo.packageName.equals(packageName)) &&
                    !clientInfo.isSubclassOf(directlyEnclosingClass, false)) {
                continue;
            }
            if (clientInfo.topLevelEnclosingClass.equals(topLevelEnclosingClass)) {
                continue;
            }
            if (clientInfo.referencesClass(classInfo.name, classInfo.isInterface(), 1)) {
                addToAffectedClassNames(clientInfo.name);
            }
        }
    }

    /**
     * For class p.C, find each project class X referencing C, which is a member of package p and whose
     * top level enclosing class is different from that of C.
     * (default -&gt; private transformation)
     */
    public void findThisPackageReferencingClasses1(ClassInfo classInfo) {
        String topLevelEnclosingClass = classInfo.topLevelEnclosingClass;
        String packageName = classInfo.packageName;
        for (PCDEntry entry : pcdm.entries()) {
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD,
                        entry);
            if (clientInfo == null) {
                continue;  // New class
            }
            if (!clientInfo.packageName.equals(packageName)) {
                continue;
            }
            if (topLevelEnclosingClass.equals(clientInfo.topLevelEnclosingClass)) {
                continue;
            }
            if (clientInfo.referencesClass(classInfo.name, classInfo.isInterface(), 1)) {
                addToAffectedClassNames(clientInfo.name);
            }
        }
    }

    /**
     * For class p.C, find each project class X referencing C, which is not a member of package p.
     * (public -&gt; default transformation)
     */
    public void findDiffPackageReferencingClasses1(ClassInfo classInfo) {
        String packageName = classInfo.packageName;
        for (PCDEntry pcde : pcdm.entries()) {
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (clientInfo == null) {
                continue;  // New class
            }
            if (clientInfo.packageName.equals(packageName)) {
                continue;
            }
            if (clientInfo.referencesClass(classInfo.name, classInfo.isInterface(), 1)) {
                addToAffectedClassNames(clientInfo.name);
            }
        }
    }

    /**
     * For class p.C, find each project class X referencing C, which is not a member of package p and
     * whose direct or indirect superclass is C's directly enclosing class.
     * (protected -&gt; default transformation)
     */
    public void findDiffPackageAndSubReferencingClasses1(ClassInfo classInfo) {
        String packageName = classInfo.packageName;
        String directlyEnclosingClass = classInfo.directlyEnclosingClass;
      for (PCDEntry pcde : pcdm.entries()) {
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (clientInfo == null) {
                continue;  // New class
            }
            if (clientInfo.packageName.equals(packageName)) {
                continue;
            }
            if (!clientInfo.isSubclassOf(directlyEnclosingClass, false)) {
                continue;
            }
            if (clientInfo.referencesClass(classInfo.name, classInfo.isInterface(), 1)) {
                addToAffectedClassNames(clientInfo.name);
            }
        }
    }

    /**
     * Find all project classes that reference both of the classes with the
     * given names (or array classes of one or both) directly or indirectly from the
     * constantpool, as a type of a data field, as a type in a method signature or a
     * thrown exception, as a directly/indirectly implemented interface or a
     * direct/indirect superclass.
     */
    public void findReferencingClasses2(ClassInfo classInfo1, ClassInfo classInfo2) {
        Set<String> refClazz1 = new LinkedHashSet<String>();
        findReferencingClasses(classInfo1, 2, false, refClazz1);
        Set<String> refClazz2 = new LinkedHashSet<String>();
        findReferencingClasses(classInfo2, 2, false, refClazz2);

        for (String className1 : refClazz1) {
            if (refClazz2.contains(className1)) {
                addToAffectedClassNames(className1);
            }
        }
    }

    /** Find all project classes which are direct subclasses of the given class */
    public void findDirectSubclasses(ClassInfo classInfo) {
        for (ClassInfo subclassInfo : classInfo.getDirectSubclasses()) {
            addToAffectedClassNames(subclassInfo.name);
        }
    }

    /**
     * Find all non-abstract project classes that implement the given interface or any of its
     * subclasses directly, and all non-abstract classes that are direct descendants of abstract
     * classes that implement the given interface directly or indirectly. Class C implements
     * interface I indirectly, if C or some superclass of C directly implements I or some sublcass of I.
     */
    public void findDirectlyAndOtherwiseImplementingConcreteClasses(ClassInfo intfInfo) {
        for (PCDEntry entry : pcdm.entries()) {
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, entry);
            if (clientInfo == null) {
                continue;  // New class
            }
            if (clientInfo.isInterface()) {
                continue;
            }
            if (clientInfo.isAbstract()) {
                if (clientInfo.implementsInterfaceDirectlyOrIndirectly(intfInfo.name)) {
                    findAllNearestConcreteSubclasses(clientInfo);
                }
            } else {
                if (clientInfo.implementsIntfOrSubintfDirectly(intfInfo.name)) {
                    addToAffectedClassNames(clientInfo.name);
                }
            }
        }
    }

    private void findAllNearestConcreteSubclasses(ClassInfo classInfo) {
        for (ClassInfo subclassInfo : classInfo.getDirectSubclasses()) {
            if (subclassInfo.isAbstract()) {
                findAllNearestConcreteSubclasses(subclassInfo);
            } else {
                addToAffectedClassNames(subclassInfo.name);
            }
        }
    }

    /**
     * Find all interfaces and abstract classes that implement the given interface and declare or inherit
     * a method with the given name. For those that overload this method, find referencing classes.
     */
    public void findAbstractSubtypesWithSameNameMethod(ClassInfo intfInfo, String mName, final String mSig) {
      for (PCDEntry pcde : pcdm.entries()) {
            ClassInfo ci =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (ci == null) {
                continue;  // New class or not in project
            }
            if (!(ci.isInterface() || ci.isAbstract())) {
                continue;
            }
            if (ci.implementsInterfaceDirectlyOrIndirectly(intfInfo.name)) {
                addToAffectedClassNames(ci.name);
                // Check if the new method overloads an existing (declared or inherited) method. Overloading test is rough -
                // we just check if the number of parameters is the same.
                ci.findExistingSameNameMethods(mName, true, true, new ClassInfo.MethodHandler() {

                    void handleMethod(ClassInfo classInfo, int otherMethodIdx) {
                        String otherMSig =
                                classInfo.methodSignatures[otherMethodIdx];
                        if ( (!mSig.equals(otherMSig)) &&
                                Utils.sameParamNumber(mSig, otherMSig)) {
                            findReferencingClassesForMethod(classInfo, otherMethodIdx);
                        }
                    }
                });
            }
        }
    }

    /** Find all project classes that reference the given field. */
    public void findReferencingClassesForField(ClassInfo classInfo, int fieldNo) {
        findReferencingClassesForMember(classInfo, fieldNo, true, false, false);
    }

    /**
     * Find all project classes that reference the given field and which are in
     * different packages.
     */
    public void findDiffPackageReferencingClassesForField(ClassInfo classInfo, int fieldNo) {
        findReferencingClassesForMember(classInfo, fieldNo, true, true, false);
    }

    /**
     * Find all project classes that reference the given field, which are in different
     * packages and are direct or indirect subclasses of the member's declaring class
     * (protected -&gt; default transformation).
     */
    public void findDiffPackageAndSubReferencingClassesForField(ClassInfo classInfo, int fieldNo) {
        findReferencingClassesForMember(classInfo, fieldNo, true, true, true);
    }

    /** Find all project classes that reference the given method. */
    public void findReferencingClassesForMethod(ClassInfo classInfo, int methodNo) {
        findReferencingClassesForMember(classInfo, methodNo, false, false, false);
    }

    /**
     * Find all project classes that reference the given method and which are in
     * different packages.
     */
    public void findDiffPackageReferencingClassesForMethod(ClassInfo classInfo, int methodNo) {
        findReferencingClassesForMember(classInfo, methodNo, false, true, false);
    }

    /**
     * Find all project classes that reference the given method, which are in different
     * packages and are direct or indirect subclasses of the member's declaring class
     * (protected -&gt; default transformation)
     */
    public void findDiffPackageAndSubReferencingClassesForMethod(ClassInfo classInfo, int methodNo) {
        findReferencingClassesForMember(classInfo, methodNo, false, true, true);
    }

    /**
     * Find all project classes that re-implement the given method and that are
     * direct/indirect subclasses of this method's declaring class. If some subclass C
     * re-implements the given method, we don't have to search C's subclasses further.
     */
    public void findSubclassesReimplementingMethod(ClassInfo classInfo, int methodNo) {
        findSubclassesReimplementingMethod(classInfo, classInfo, methodNo);
    }

    private void findSubclassesReimplementingMethod(ClassInfo targetClass, ClassInfo methodDeclaringClass, int methodNo) {
        for (ClassInfo subclass : targetClass.getDirectSubclasses()) {
            if (subclass.declaresMethod(methodDeclaringClass, methodNo)) {
                addToAffectedClassNames(subclass.name);
            } else {
                findSubclassesReimplementingMethod(subclass, methodDeclaringClass, methodNo);
            }
        }
    }

    /**
     * For a given class C, find all concrete direct subclasses, and all direct concrente subclasses of C's direct
     * or indirect abstract subclasses.
     */
    public void findConcreteSubclasses(ClassInfo targetClass) {
        for (ClassInfo subclass : targetClass.getDirectSubclasses()) {
            if (subclass.isAbstract()) {
                findConcreteSubclasses(subclass);
            } else {
                addToAffectedClassNames(subclass.name);
            }
        }
    }

    /**
     * Find any concrete subclasses of targetClass that don't override or inherit a concrete implementation
     * of the given method.
     */
    public void findConcreteSubclassesNotOverridingAbstractMethod(ClassInfo targetClass, ClassInfo methodDeclaringClass, int methodNo) {
        for (ClassInfo subclass : targetClass.getDirectSubclasses()) {
            int pos =
                    subclass.getDeclaredMethodPos(methodDeclaringClass, methodNo);
            if (pos == -1) { // This method is not overridden in this class
                if (!subclass.isAbstract()) {
                    addToAffectedClassNames(subclass.name);
                } else {
                    findConcreteSubclassesNotOverridingAbstractMethod(subclass, methodDeclaringClass, methodNo);
                }
            } else { // A chance that this method is declared abstract once again...
                if (Modifier.isAbstract(subclass.methodAccessFlags[pos])) {
                    findConcreteSubclassesNotOverridingAbstractMethod(subclass, methodDeclaringClass, methodNo);
                }
            }
        }
    }

    /** Find all project classes that reference any method that throws the given exception. */
    public void findRefsToMethodsThrowingException(ClassInfo excClassInfo) {
            for (PCDEntry pcde : pcdm.entries()) {
            ClassInfo classInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (classInfo == null) {
                continue;  // New class
            }
            int methodIdx = -1;
            do {
                methodIdx =
                        classInfo.hasMethodThrowingException(excClassInfo, methodIdx + 1);
                if (methodIdx != -1) {
                    findReferencingClassesForMethod(classInfo, methodIdx);
                }
            } while (methodIdx != -1);
        }
    }

    /**
     * Find all project classes declaring a static field with the given name. Currently used only to look up
     * classes referencing given class X via the "X.class" construct.
     */
    public void findClassesDeclaringField(String name, String signature, boolean isStatic, String packageToLookIn) {
      for (PCDEntry pcde : pcdm.entries()) {
            ClassInfo classInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (classInfo == null) {
                continue;  // New class
            }
            if (packageToLookIn != null &&
                    !classInfo.packageName.equals(packageToLookIn)) {
                continue;
            }
            if (classInfo.declaresField(name, signature, isStatic)) {
                addToAffectedClassNames(classInfo.name);
            }
        }
    }

    public void addToAffectedClassNames(String className) {
        String res = pcdm.classAlreadyRecompiledOrUncompileable(className);
        if (res == null) {
            affectedClassNames.add(className);
        } else if (!"".equals(res)) {  // The dependent class comes from a .jar.
            if (checkedClassIsFromJar || noWarnOnDependentJar) {
                return;
            }
            String message = "Class " + className + " is affected by a change to " + checkedClassName + ", but can't be recompiled, " +
                    "since it is located in archive " + res;
            if (failOnDependentJar) {
                throw new PrivateException(new PublicExceptions.JarDependsOnSourceException(message));
            } else {
                Utils.printWarningMessage("Warning: " + message);
            }
        }
    }

    /**
     * Find all project classes that reference the class with the given name.
     * The second parameter controls the "thoroughness degree", and its value is passed to ClassInfo.referencesClass()
     * method (see the comment to it). The fromDiffPackages parameter defines whether all such classes
     * or only classes from different packages are required.
     */
    private void findReferencingClasses(ClassInfo classInfo,
            int thorDegree, boolean fromDiffPackages,
            Set<String> ret) {
        String packageName = classInfo.packageName;
        for (PCDEntry pcde : pcdm.entries()) {
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (clientInfo == null) {
                continue;  // New class
            }
            if (fromDiffPackages && packageName.equals(clientInfo.packageName)) {
                continue;
            }
            // If thorDegree == 2, i.e. indirect references from the constantpool (e.g. a reference to a method which
            // has classInfo as one of its formal parameter types) are taken into account, then we should check all of
            // the classes, whether classInfo is directly accessible from them or not.
            if (thorDegree != 2 && (!classAccessibleFrom(classInfo, clientInfo))) {
                continue;
            }

            if (clientInfo.referencesClass(classInfo.name, classInfo.isInterface(), thorDegree)) {
                if (ret == null) {
                    addToAffectedClassNames(clientInfo.name);
                } else {
                    ret.add(clientInfo.name);
                }
            }
        }
    }

    /**
     * Find all project classes that reference the given member. If fromDiffPackages
     * is true, then only classes that do not belong to the package of the member's
     * declaring class should be returned. If onlySubclasses is true, then only
     * classes that are subclasses of member's declaring class should be returned.
     */
    private void findReferencingClassesForMember(ClassInfo declaringClassInfo, int memberNo,
            boolean isField,
            boolean fromDiffPackages, boolean onlySubclasses) {
        String declaringClassName = declaringClassInfo.name;
        String declaringClassPackage = declaringClassInfo.packageName;
        for (PCDEntry pcde : pcdm.entries()) {
            ClassInfo clientInfo =
                    pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, pcde);
            if (clientInfo == null) {
                continue;  // New class
            }
            String className = clientInfo.name;
            if (className.equals(declaringClassName)) {
                continue;
            }
            if (!memberAccessibleFrom(declaringClassInfo, memberNo, clientInfo, isField)) {
                continue;
            }
            if (fromDiffPackages &&
                    declaringClassPackage.equals(clientInfo.packageName)) {
                continue;
            }
            if (onlySubclasses && !clientInfo.isSubclassOf(declaringClassName, false)) {
                continue;
            }

            if (isField) {
                if (clientInfo.referencesField(declaringClassInfo, memberNo)) {
                    addToAffectedClassNames(clientInfo.name);
                }
            } else {
                if (clientInfo.referencesMethod(declaringClassInfo, memberNo)) {
                    addToAffectedClassNames(clientInfo.name);
                }
            }
        }
    }

    /** Checks if class classInfo is accessible from class clientClassInfo. */
    private boolean classAccessibleFrom(ClassInfo classInfo, ClassInfo clientClassInfo) {
        char classFlags = classInfo.accessFlags;
        String classPackage = classInfo.packageName;
        String clientClassPackage = clientClassInfo.packageName;

        if (Modifier.isPublic(classFlags)) {
            return true;
        } else if (Modifier.isProtected(classFlags)) {
            if (classPackage.equals(clientClassPackage) ||
                    clientClassInfo.isSubclassOf(classInfo.directlyEnclosingClass, false)) {
                return true;
            }
        } else if (Modifier.isPrivate(classFlags)) {
            if (classInfo.topLevelEnclosingClass.equals(clientClassInfo.topLevelEnclosingClass)) {
                return true;
            }
        } else {
            if (classPackage.equals(clientClassPackage)) {
                return true;
            }
        }

        return false;
    }

    /**
     * Checks if member memberNo (which is a field if isField == true, and method otherwise) of class memberClassInfo is
     * accessible from class clientClassInfo.
     */
    private boolean memberAccessibleFrom(ClassInfo memberClassInfo,
            int memberNo, ClassInfo clientClassInfo, boolean isField) {
        char memberClassFlags = memberClassInfo.accessFlags;
        char memberFlags = isField ? memberClassInfo.fieldAccessFlags[memberNo]
                : memberClassInfo.methodAccessFlags[memberNo];
        String memberClassPackage = memberClassInfo.packageName;
        String clientClassPackage = clientClassInfo.packageName;

        if (Modifier.isPublic(memberClassFlags)) {
            if (Modifier.isPublic(memberFlags)) {
                return true;
            } else if (Modifier.isProtected(memberFlags) &&
                    (memberClassPackage.equals(clientClassPackage) ||
                    clientClassInfo.isSubclassOf(memberClassInfo.name, false))) {
                return true;
            } else if (Modifier.isPrivate(memberFlags)) {
                if (memberClassInfo.topLevelEnclosingClass.equals(
                        clientClassInfo.topLevelEnclosingClass)) {
                    return true;
                }
            } else if (memberClassPackage.equals(clientClassPackage)) {
                return true;
            }
        } else if (Modifier.isProtected(memberClassFlags)) {
            if (!(memberClassPackage.equals(clientClassPackage) ||
                    clientClassInfo.isSubclassOf(memberClassInfo.directlyEnclosingClass, false))) {
                return true;
            }
            if (Modifier.isPublic(memberFlags) ||
                    Modifier.isProtected(memberFlags)) {
                return true;
            } else if (Modifier.isPrivate(memberFlags)) {
                if (memberClassInfo.topLevelEnclosingClass.equals(
                        clientClassInfo.topLevelEnclosingClass)) {
                    return true;
                }
            } else {
                if (memberClassPackage.equals(clientClassPackage)) {
                    return true;
                }
            }
        } else if (Modifier.isPrivate(memberClassFlags)) {
            if (memberClassInfo.topLevelEnclosingClass.equals(
                    clientClassInfo.topLevelEnclosingClass)) {
                return true;
            }
        } else {  // memberClassInfo is package-private
            if (!memberClassPackage.equals(clientClassPackage)) {
                return false;
            }
            if (Modifier.isPublic(memberFlags) || Modifier.isProtected(memberFlags)) {
                return true;
            } else if (Modifier.isPrivate(memberFlags)) {
                if (memberClassInfo.topLevelEnclosingClass.equals(
                        clientClassInfo.topLevelEnclosingClass)) {
                    return true;
                }
            } else {
                return true;
            }
        }

        return false;
    }
}
