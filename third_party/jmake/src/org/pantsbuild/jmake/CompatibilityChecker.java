/* Copyright (c) 2002-2008 Sun Microsystems, Inc. All rights reserved
 *
 * This program is distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */
package org.pantsbuild.jmake;

import java.lang.reflect.Modifier;
import java.util.List;
import java.util.Set;

/**
 * This class implements checking of source compatibility of classes and supporting operations
 *
 * @author Misha Dmitriev
 *  12 March 2004
 */
public class CompatibilityChecker {

    private PCDManager pcdm;
    private RefClassFinder rf;
    ClassInfo oldClassInfo = null;
    ClassInfo newClassInfo = null;
    private boolean versionsCompatible;
    private boolean publicConstantChanged;

    public CompatibilityChecker(PCDManager pcdm, boolean failOnDependentJar, boolean noWarnOnDependentJar) {
        this.pcdm = pcdm;
        publicConstantChanged = false;
        rf = new RefClassFinder(pcdm, failOnDependentJar, noWarnOnDependentJar);
    }

    /**
     * Compares the two class versions for the given PCDEntry. Returns true if all changes are source
     * compatible, and false otherwise.
     */
    public boolean compareClassVersions(PCDEntry entry) {
        // I once had the following optimization here with the comment "No sense to make any further checks if
        // everything is recompiled anyway", but now I believe it's wrong. For each class that was found changed
        // we need to know whether the new version is compatible with the old or not, since this may determine
        // whether the new version of this class is promoted into the pdb or not (see PCDManager.updateClassInfoInPCD()).
        // So, all changed classes should be checked just to correctly determine version compatibility.
        // if (publicConstantChanged) return false;

        oldClassInfo = pcdm.getClassInfoForPCDEntry(ClassInfo.VER_OLD, entry);
        newClassInfo = pcdm.getClassInfoForPCDEntry(ClassInfo.VER_NEW, entry);

        rf.initialize(oldClassInfo.name, entry.javaFileFullPath.endsWith(".jar"));
        versionsCompatible = true;

        checkAccessFlags();
        checkSuperclasses();
        checkImplementedInterfaces();
        checkFields();
        checkMethodsAndConstructors();

        return versionsCompatible;
    }

    /** Find all dependent classes for a deleted class. */
    public void checkDeletedClass(PCDEntry entry) {
        oldClassInfo = entry.oldClassInfo;
        rf.initialize(oldClassInfo.name, entry.javaFileFullPath.endsWith(".jar"));
        rf.findReferencingClassesForDeletedClass(oldClassInfo);
        // It may happen that the only reference to deleted class X is via "X.class" construct
        String packageToLookIn =
                oldClassInfo.isPublic() ? null : oldClassInfo.packageName;
        rf.findClassesDeclaringField(("class$" + oldClassInfo.name).intern(), "java/lang/Class", true, packageToLookIn);
        checkForFinalFields();
    }

    /** Returns the names of classes affected by source incompatible changes to the new version of the checked class. */
    public String[] getAffectedClasses() {
        return rf.getAffectedClassNames();
    }

    /** All of the following methods return true if no source incompatible changes found, and false otherwise */
    private void checkAccessFlags() {
        char oldClassFlags = oldClassInfo.accessFlags;
        char newClassFlags = newClassInfo.accessFlags;
        if (oldClassFlags == newClassFlags) {
            return;
        }

        if (!Modifier.isFinal(oldClassFlags) && Modifier.isFinal(newClassFlags)) {
            versionsCompatible = false;
            rf.findDirectSubclasses(oldClassInfo);
        }

        if (!Modifier.isAbstract(oldClassFlags) && Modifier.isAbstract(newClassFlags)) {
            versionsCompatible = false;
            rf.findReferencingClasses0(oldClassInfo);
        }

        // Now to accessibility modifiers checking...
        if (Modifier.isPublic(newClassFlags)) {
            return;
        }

        if (Modifier.isProtected(newClassFlags)) {
            if (Modifier.isPublic(oldClassFlags)) {
                versionsCompatible = false;
                rf.findDiffPackageAndNotSubReferencingClasses1(oldClassInfo);
            }
        } else if (Modifier.isPrivate(newClassFlags)) {
            if (!Modifier.isPrivate(oldClassFlags)) {
                versionsCompatible = false;
            } else {
                return;  // private -> private, nothing more to check
            }
            if (Modifier.isPublic(oldClassFlags)) {
                rf.findReferencingClasses1(oldClassInfo);
            } else if (Modifier.isProtected(oldClassFlags)) {
                rf.findThisPackageOrSubReferencingClasses1(oldClassInfo);
            } else {
                rf.findThisPackageReferencingClasses1(oldClassInfo);
            }
        } else {  // newClassFlags has default access, since public has already been excluded
            if (Modifier.isPublic(oldClassFlags)) {
                versionsCompatible = false;
                rf.findDiffPackageReferencingClasses1(oldClassInfo);
            } else if (Modifier.isProtected(oldClassFlags)) {
                versionsCompatible = false;
                rf.findDiffPackageAndSubReferencingClasses1(oldClassInfo);
            }
        }
    }

    private void checkSuperclasses() {
        List<String> oldSuperNames = oldClassInfo.getAllSuperclassNames();
        List<String> newSuperNames = newClassInfo.getAllSuperclassNames();

        int oldNamesSizeMinusOne = oldSuperNames.size() - 1;
        for (int i = 0; i <= oldNamesSizeMinusOne; i++) {
            String oldSuperName = oldSuperNames.get(i);
            if (!newSuperNames.contains(oldSuperName)) {
                versionsCompatible = false;
                ClassInfo missingSuperClass =
                        pcdm.getClassInfoForName(ClassInfo.VER_OLD, oldSuperName);
                if (missingSuperClass == null) { // This class is not in project
                    missingSuperClass =
                            ClassPath.getClassInfoForName(oldSuperName, pcdm);
                    if (missingSuperClass == null) {
                        missingSuperClass = new ClassInfo(oldSuperName, pcdm);
                    }
                }
                rf.findReferencingClasses2(missingSuperClass, oldClassInfo);
            }
        }

        // Now check if the class is an exception, and its kind has changed from unchecked to checked
        if (oldClassInfo.isInterface() || oldSuperNames.size() == 0) {
            return;
        }
        if (!(oldSuperNames.contains("java/lang/RuntimeException") || oldSuperNames.contains("java/lang/Error"))) {
            return;
        }
        if (!(newSuperNames.contains("java/lang/RuntimeException") || newSuperNames.contains("java/lang/Error"))) {
            if (!newSuperNames.contains("java/lang/Throwable")) {
                return;
            }
            // Ok, exception kind has changed from unchecked to checked.
            versionsCompatible = false;
            rf.findReferencingClasses0(oldClassInfo);
            rf.findRefsToMethodsThrowingException(oldClassInfo);
        }
    }

    private void checkImplementedInterfaces() {
        Set<String> oldIntfNames = oldClassInfo.getAllImplementedIntfNames();
        Set<String> newIntfNames = newClassInfo.getAllImplementedIntfNames();

        for (String oldIntfName : oldIntfNames) {
            if (!newIntfNames.contains(oldIntfName)) {
                versionsCompatible = false;
                ClassInfo missingSuperInterface =
                        pcdm.getClassInfoForName(ClassInfo.VER_OLD, oldIntfName);
                if (missingSuperInterface == null) { // This class is not in project
                    missingSuperInterface =
                            ClassPath.getClassInfoForName(oldIntfName, pcdm);
                    if (missingSuperInterface == null) {
                        missingSuperInterface = new ClassInfo(oldIntfName, pcdm);
                    }
                }
                rf.findReferencingClasses2(missingSuperInterface, oldClassInfo);
            }
        }

        // Check if the class is abstract, and an interface has been added to its list of implemented interfaces
        if (newClassInfo.isAbstract()) {
            for (String newIntfName : newIntfNames) {
                if (!oldIntfNames.contains(newIntfName)) {
                    versionsCompatible = false;
                    rf.findConcreteSubclasses(oldClassInfo);
                    break;
                }
            }
        }
    }

    private void checkFields() {
        String oFNames[] = oldClassInfo.fieldNames;
        String oFSignatures[] = oldClassInfo.fieldSignatures;
        char oFFlags[] = oldClassInfo.fieldAccessFlags;
        String nFNames[] = newClassInfo.fieldNames;
        String nFSignatures[] = newClassInfo.fieldSignatures;
        char nFFlags[] = newClassInfo.fieldAccessFlags;
        int oFLen = oFNames != null ? oFNames.length : 0;
        int nFLen = nFNames != null ? nFNames.length : 0;

        int oFMod, nFMod;
        String oFName, oFSig, nFName;
        int i, j, k, endIdx;
        int nonMatchingNewFields = nFLen;

        for (i = 0; i < oFLen; i++) {
            oFMod = oFFlags[i];
            if (Modifier.isPrivate(oFMod)) {
                continue;  // Changes to private fields don't affect compatibility
            }
            oFName = oFNames[i];
            oFSig = oFSignatures[i];
            boolean found = false;

            // Look for the same field in the new version considering name and type
            endIdx = nFLen - 1;
            k = i < nFLen ? i : endIdx;
            for (j = 0; j < nFLen; j++) {
                if (oFName.equals(nFNames[k]) &&
                        oFSig.equals(nFSignatures[k])) {
                    found = true;
                    break;
                }
                if (k < endIdx) {
                    k++;
                } else {
                    k = 0;
                }
            }

            if (found) {
                nonMatchingNewFields--;
                nFMod = nFFlags[k];
                checkFieldModifiers(oFMod, nFMod, i, k);
                if (publicConstantChanged) {
                    return;
                }
            } else { // Matching field not found
                if (Modifier.isStatic(oFMod) && Modifier.isFinal(oFMod) &&
                        oldClassInfo.primitiveConstantInitValues != null &&
                        oldClassInfo.primitiveConstantInitValues[i] != null) {
                    // Compile-time constant deleted
                    versionsCompatible = false;
                    rf.findAllProjectClasses(oldClassInfo, i);
                    if (Modifier.isPublic(oFMod)) {
                        publicConstantChanged = true;
                        return;
                    }
                } else {
                    versionsCompatible = false;
                    rf.findReferencingClassesForField(oldClassInfo, i);
                }
            }
        }

        if (nonMatchingNewFields > 0) { // There are some fields declared in the new version which don't exist in the old one
            // Look for fields hiding same-named fields in superclasses
            for (i = 0; i < nFLen; i++) {
                nFName = nFNames[i];

                boolean found = false;
                for (j = 0; j < oFLen; j++) {
                    if (nFName.equals(oFNames[j])) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    continue;  // nFName is not an added field
                }
                String superName = oldClassInfo.superName;
                ClassInfo superInfo;
                while (superName != null) {
                    superInfo =
                            pcdm.getClassInfoForName(ClassInfo.VER_OLD, superName);
                    if (superInfo == null) {
                        break;
                    }
                    String[] superOFNames = superInfo.fieldNames;
                    int superOFNamesLen = superOFNames != null ? superOFNames.length
                            : 0;
                    for (j = 0; j < superOFNamesLen; j++) {
                        if (nFName == superOFNames[j]) {
                            versionsCompatible = false;
                            rf.findReferencingClassesForField(superInfo, j);
                        }
                    }
                    superName = superInfo.superName;
                }
            }
        }
    }

    /** It is already known that old field is not private */
    private void checkFieldModifiers(int oFMod, int nFMod, int oldFieldIdx, int newFieldIdx) {
        if (oFMod == nFMod) {
            if (Modifier.isFinal(oFMod) &&
                    (!ClassInfo.constFieldInitValuesEqual(oldClassInfo, oldFieldIdx, newClassInfo, newFieldIdx))) {
                versionsCompatible = false;
                rf.findAllProjectClasses(oldClassInfo, oldFieldIdx);
                if (Modifier.isPublic(oFMod)) {
                    publicConstantChanged = true;  // Means we will have to recompile ALL project classes
                }
                return;
            }
        }

        // These tests are ordered such that if a previous test succeeds, there is no need to do further tests, since that
        // former test will cause more classes to be checked than any of the further tests. That is why it is possible to
        // check properties that are in fact independent (e.g. accessibility vs. static/non-static) together. But this
        // optimization only works since all kinds of tests result in the same kind of find..ReferencingClassesForField()
        // outcome. For methods this is not true, and so there we have to check independent properties separately.
        if (Modifier.isStatic(oFMod) && Modifier.isFinal(oFMod) && // oFMod is known to be non-private
                (!Modifier.isFinal(nFMod) || !ClassInfo.constFieldInitValuesEqual(oldClassInfo, oldFieldIdx, newClassInfo, newFieldIdx))) {
            versionsCompatible = false;
            rf.findAllProjectClasses(oldClassInfo, oldFieldIdx);
            if (Modifier.isPublic(oFMod)) {
                publicConstantChanged = true;
            }
        } else if (Modifier.isPrivate(nFMod) || // oFMod is known to be non-private
                (!Modifier.isFinal(oFMod) && Modifier.isFinal(nFMod)) ||
                (Modifier.isStatic(oFMod) != Modifier.isStatic(nFMod)) ||
                (Modifier.isVolatile(oFMod) != Modifier.isVolatile(nFMod))) {
            versionsCompatible = false;
            rf.findReferencingClassesForField(oldClassInfo, oldFieldIdx);
        } else if (Modifier.isPublic(oFMod) && Modifier.isProtected(nFMod)) {
            versionsCompatible = false;
            rf.findDiffPackageReferencingClassesForField(oldClassInfo, oldFieldIdx);
        } else if ((Modifier.isPublic(oFMod) || Modifier.isProtected(oFMod)) &&
                (!(Modifier.isPublic(nFMod) || Modifier.isProtected(nFMod) || Modifier.isPrivate(nFMod)))) {
            versionsCompatible = false;
            if (Modifier.isPublic(oFMod)) {
                rf.findDiffPackageReferencingClassesForField(oldClassInfo, oldFieldIdx);
            } else {
                rf.findDiffPackageAndSubReferencingClassesForField(oldClassInfo, oldFieldIdx);
            }
        }
    }

    private void checkForFinalFields() {
        char oFFlags[] = oldClassInfo.fieldAccessFlags;
        int oFLen = oldClassInfo.fieldNames != null ? oldClassInfo.fieldNames.length
                : 0;
        int oFMod;

        for (int i = 0; i < oFLen; i++) {
            oFMod = oFFlags[i];
            if (Modifier.isPrivate(oFMod)) {
                continue;  // Changes to private fields don't affect compatibility
            }
            if (Modifier.isStatic(oFMod) && Modifier.isFinal(oFMod)) {
                rf.findAllProjectClasses(oldClassInfo, i);
                if (Modifier.isPublic(oFMod)) {
                    publicConstantChanged = true;
                    return;
                }
            }
        }
    }

    private void checkMethodsAndConstructors() {
        String oMNames[] = oldClassInfo.methodNames;
        String oMSignatures[] = oldClassInfo.methodSignatures;
        char oMFlags[] = oldClassInfo.methodAccessFlags;
        String nMNames[] = newClassInfo.methodNames;
        String nMSignatures[] = newClassInfo.methodSignatures;
        char nMFlags[] = newClassInfo.methodAccessFlags;
        int oMLen = oMNames != null ? oMNames.length : 0;
        int nMLen = nMNames != null ? nMNames.length : 0;

        int oMMod, nMMod;
        String oMName, oMSig, nMName, nMSig;
        int i, j, k, endIdx;
        int nonMatchingNewMethods = nMLen;

        for (i = 0; i < oMLen; i++) {
            oMMod = oMFlags[i];
            if (Modifier.isPrivate(oMMod)) {
                continue;  // Changes to private methods don't affect compatibility
            }
            oMName = oMNames[i];
            oMSig = oMSignatures[i];
            boolean found = false;

            // Look for the same method in the new version considering name and signature
            endIdx = nMLen - 1;
            k = i < nMLen ? i : endIdx;
            for (j = 0; j < nMLen; j++) {
                if (oMName == nMNames[k] && oMSig == nMSignatures[k]) {
                    found = true;
                    break;
                }
                if (k < endIdx) {
                    k++;
                } else {
                    k = 0;
                }
            }

            if (found) {
                nonMatchingNewMethods--;
                nMMod = nMFlags[k];
                if (oMMod != nMMod) {
                    checkMethodModifiers(oMMod, nMMod, i);
                }

                // Check if the new method throws more exceptions than the old one
                if (newClassInfo.checkedExceptions != null && newClassInfo.checkedExceptions[k] != null) {
                    if (oldClassInfo.checkedExceptions == null) {
                        versionsCompatible = false;
                        rf.findReferencingClassesForMethod(oldClassInfo, i);
                    } else if (oldClassInfo.checkedExceptions[i] == null) {
                        versionsCompatible = false;
                        rf.findReferencingClassesForMethod(oldClassInfo, i);
                    } else {
                        String oldExceptions[] =
                                oldClassInfo.checkedExceptions[i];
                        String newExceptions[] =
                                newClassInfo.checkedExceptions[k];
                        for (int ei = 0; ei < newExceptions.length; ei++) {
                            String newEx = newExceptions[ei];
                            found = false;
                            for (int ej = 0; ej < oldExceptions.length; ej++) {
                                if (newEx.equals(oldExceptions[ej])) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                versionsCompatible = false;
                                rf.findReferencingClassesForMethod(oldClassInfo, i);
                                break;
                            }
                        }
                    }
                }
            } else { // Matching method not found
                versionsCompatible = false;
                rf.findReferencingClassesForMethod(oldClassInfo, i);
                // Deleting a concrete method from an abstract class is a special case
                if (oldClassInfo.isAbstract() && !Modifier.isAbstract(oMMod)) {
                    rf.findConcreteSubclassesNotOverridingAbstractMethod(oldClassInfo, oldClassInfo, i);
                }
            }
        }

        if (nonMatchingNewMethods > 0) {  // There are some methods/constructors declared in the new version which don't exist in the old one
            if (!oldClassInfo.isInterface()) {
                for (i = 0; i < nMLen; i++) {
                    nMMod = nMFlags[i];
                    if (Modifier.isPrivate(nMMod)) {
                        continue;
                    }
                    String newMName = nMNames[i];
                    final String newMSig = nMSignatures[i];
                    final boolean isStatic = Modifier.isStatic(nMMod);

                    boolean found = false;
                    for (j = 0; j < oMLen; j++) {
                        if (newMName.equals(oMNames[j]) &&
                                newMSig.equals(oMSignatures[j])) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        continue;  // nMName is not an added method
                    }
                    // Check if the new method is a static one that hides an inherited static method
                    // Check if the new method overloads an existing (declared or inherited) method. Overloading test is rough -
                    // we just check if the number of parameters is the same. Note that if a new constructor has been added, it
                    // can be treated in the same way, except that we shouldn't look up "same name methods" for it in superclasses.
                    oldClassInfo.findExistingSameNameMethods(newMName,
                            !newMName.equals("<init>"), false,
                            new ClassInfo.MethodHandler() {

                        void handleMethod(ClassInfo classInfo, int methodIdx) {
                            String otherMSig =
                                    classInfo.methodSignatures[methodIdx];
                            if ((newMSig.equals(otherMSig) && isStatic &&
                                    classInfo != oldClassInfo) ||
                                    (newMSig != otherMSig &&
                                    Utils.sameParamNumber(newMSig, otherMSig))) {
                                versionsCompatible = false;
                                rf.findReferencingClassesForMethod(classInfo, methodIdx);
                            }
                        }
                    });

                    if (Modifier.isAbstract(nMMod)) {
                        // An abstract method added to the class. Find any concrete subclasses that don't override
                        // or inherit a concrete implementation of this method.
                        versionsCompatible = false;
                        rf.findConcreteSubclassesNotOverridingAbstractMethod(oldClassInfo, newClassInfo, i);
                    }
                    // Check if there is a method with the same name in some subclass, such that it now overrides
                    // or overloads the added method.
                    if (subclassesDeclareSameNameMethod(oldClassInfo, newMName)) {
                        versionsCompatible = false;
                    }
                }
            } else {  // We are checking an interface.
                for (i = 0; i < nMLen; i++) {
                    String newMName = nMNames[i];
                    final String newMSig = nMSignatures[i];

                    boolean found = false;
                    for (j = 0; j < oMLen; j++) {
                        if (newMName == oMNames[j] && newMSig == oMSignatures[j]) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        versionsCompatible = false;

                        // Check if the new method overloads an existing (declared or inherited) method. Overloading test is rough -
                        // we just check if the number of parameters is the same.
                        oldClassInfo.findExistingSameNameMethods(newMName, true, true, new ClassInfo.MethodHandler() {

                            void handleMethod(ClassInfo classInfo, int methodIdx) {
                                String otherMSig =
                                        classInfo.methodSignatures[methodIdx];
                                if (newMSig != otherMSig &&
                                        Utils.sameParamNumber(newMSig, otherMSig)) {
                                    rf.findReferencingClassesForMethod(classInfo, methodIdx);
                                }
                            }
                        });

                        rf.findDirectlyAndOtherwiseImplementingConcreteClasses(oldClassInfo);
                        rf.findAbstractSubtypesWithSameNameMethod(oldClassInfo, newMName, newMSig);
                        break;
                    }
                }
            }
        }
    }

    private void checkMethodModifiers(int oMMod, int nMMod, int oldMethodIdx) {
        if (Modifier.isPrivate(nMMod)) {
            versionsCompatible = false;
            rf.findReferencingClassesForMethod(oldClassInfo, oldMethodIdx);
        } else if (Modifier.isPublic(oMMod) && Modifier.isProtected(nMMod)) {
            versionsCompatible = false;
            rf.findDiffPackageReferencingClassesForMethod(oldClassInfo, oldMethodIdx);
        } else if ((Modifier.isPublic(oMMod) || Modifier.isProtected(oMMod)) &&
                (!(Modifier.isPublic(nMMod) || Modifier.isProtected(nMMod) || Modifier.isPrivate(nMMod)))) {
            versionsCompatible = false;
            if (Modifier.isPublic(oMMod)) {
                rf.findDiffPackageReferencingClassesForMethod(oldClassInfo, oldMethodIdx);
            } else {
                rf.findDiffPackageAndSubReferencingClassesForMethod(oldClassInfo, oldMethodIdx);
            }
        } else if ((Modifier.isPrivate(oMMod) && !Modifier.isPrivate(nMMod)) ||
                (Modifier.isProtected(oMMod) && Modifier.isPublic(nMMod)) ||
                (!(Modifier.isPublic(oMMod) || Modifier.isProtected(oMMod) || Modifier.isPrivate(oMMod)) &&
                (Modifier.isPublic(nMMod) || Modifier.isProtected(nMMod)))) {
            versionsCompatible = false;
            rf.findSubclassesReimplementingMethod(oldClassInfo, oldMethodIdx);
        }

        if ((!Modifier.isAbstract(oMMod) && Modifier.isAbstract(nMMod)) ||
                (Modifier.isStatic(oMMod) != Modifier.isStatic(nMMod))) {
            versionsCompatible = false;
            rf.findReferencingClassesForMethod(oldClassInfo, oldMethodIdx);
            if (!Modifier.isAbstract(oMMod) && Modifier.isAbstract(nMMod)) {
                rf.findConcreteSubclassesNotOverridingAbstractMethod(oldClassInfo, newClassInfo, oldMethodIdx);
            }
        }
        if (!Modifier.isFinal(oMMod) && Modifier.isFinal(nMMod)) {
            versionsCompatible = false;
            rf.findSubclassesReimplementingMethod(oldClassInfo, oldMethodIdx);
        }
    }

    /**
     * Returns true if any subclass(es), direct or indirect, declare a method with name methodName.
     * For each such occurence, referencing classes are looked up and added to the list of affected classes.
     */
    private boolean subclassesDeclareSameNameMethod(ClassInfo oldClassInfo, String methodName) {
        boolean res = false;
        ClassInfo[] directSubclasses = oldClassInfo.getDirectSubclasses();
        for (int i = 0; i < directSubclasses.length; i++) {
            ClassInfo subclass = directSubclasses[i];
            int methNo = subclass.declaresSameNameMethod(methodName);
            if (methNo >= 0) {
                rf.addToAffectedClassNames(subclass.name);
                rf.findReferencingClassesForMethod(subclass, methNo);
                res = true;
            }
            if (subclassesDeclareSameNameMethod(subclass, methodName)) {
                res = true;
            }
        }
        return res;
    }
}
