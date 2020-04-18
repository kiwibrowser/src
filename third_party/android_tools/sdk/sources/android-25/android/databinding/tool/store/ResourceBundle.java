/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.store;

import android.databinding.tool.processing.ErrorMessages;
import android.databinding.tool.processing.Scope;
import android.databinding.tool.processing.ScopedException;
import android.databinding.tool.processing.scopes.FileScopeProvider;
import android.databinding.tool.processing.scopes.LocationScopeProvider;
import android.databinding.tool.util.L;
import android.databinding.tool.util.ParserHelper;
import android.databinding.tool.util.Preconditions;

import java.io.File;
import java.io.InputStream;
import java.io.Serializable;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Marshaller;
import javax.xml.bind.Unmarshaller;
import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlElementWrapper;
import javax.xml.bind.annotation.XmlRootElement;

/**
 * This is a serializable class that can keep the result of parsing layout files.
 */
public class ResourceBundle implements Serializable {
    private static final String[] ANDROID_VIEW_PACKAGE_VIEWS = new String[]
            {"View", "ViewGroup", "ViewStub", "TextureView", "SurfaceView"};
    private String mAppPackage;

    private HashMap<String, List<LayoutFileBundle>> mLayoutBundles
            = new HashMap<String, List<LayoutFileBundle>>();

    private List<File> mRemovedFiles = new ArrayList<File>();

    public ResourceBundle(String appPackage) {
        mAppPackage = appPackage;
    }

    public void addLayoutBundle(LayoutFileBundle bundle) {
        if (bundle.mFileName == null) {
            L.e("File bundle must have a name. %s does not have one.", bundle);
            return;
        }
        if (!mLayoutBundles.containsKey(bundle.mFileName)) {
            mLayoutBundles.put(bundle.mFileName, new ArrayList<LayoutFileBundle>());
        }
        final List<LayoutFileBundle> bundles = mLayoutBundles.get(bundle.mFileName);
        for (LayoutFileBundle existing : bundles) {
            if (existing.equals(bundle)) {
                L.d("skipping layout bundle %s because it already exists.", bundle);
                return;
            }
        }
        L.d("adding bundle %s", bundle);
        bundles.add(bundle);
    }

    public HashMap<String, List<LayoutFileBundle>> getLayoutBundles() {
        return mLayoutBundles;
    }

    public String getAppPackage() {
        return mAppPackage;
    }

    public void validateMultiResLayouts() {
        for (List<LayoutFileBundle> layoutFileBundles : mLayoutBundles.values()) {
            for (LayoutFileBundle layoutFileBundle : layoutFileBundles) {
                List<BindingTargetBundle> unboundIncludes = new ArrayList<BindingTargetBundle>();
                for (BindingTargetBundle target : layoutFileBundle.getBindingTargetBundles()) {
                    if (target.isBinder()) {
                        List<LayoutFileBundle> boundTo =
                                mLayoutBundles.get(target.getIncludedLayout());
                        if (boundTo == null || boundTo.isEmpty()) {
                            L.d("There is no binding for %s, reverting to plain layout",
                                    target.getIncludedLayout());
                            if (target.getId() == null) {
                                unboundIncludes.add(target);
                            } else {
                                target.setIncludedLayout(null);
                                target.setInterfaceType("android.view.View");
                                target.mViewName = "android.view.View";
                            }
                        } else {
                            String binding = boundTo.get(0).getFullBindingClass();
                            target.setInterfaceType(binding);
                        }
                    }
                }
                layoutFileBundle.getBindingTargetBundles().removeAll(unboundIncludes);
            }
        }

        for (Map.Entry<String, List<LayoutFileBundle>> bundles : mLayoutBundles.entrySet()) {
            if (bundles.getValue().size() < 2) {
                continue;
            }

            // validate all ids are in correct view types
            // and all variables have the same name
            for (LayoutFileBundle bundle : bundles.getValue()) {
                bundle.mHasVariations = true;
            }
            String bindingClass = validateAndGetSharedClassName(bundles.getValue());
            Map<String, NameTypeLocation> variableTypes = validateAndMergeNameTypeLocations(
                    bundles.getValue(), ErrorMessages.MULTI_CONFIG_VARIABLE_TYPE_MISMATCH,
                    new ValidateAndFilterCallback() {
                        @Override
                        public List<? extends NameTypeLocation> get(LayoutFileBundle bundle) {
                            return bundle.mVariables;
                        }
                    });

            Map<String, NameTypeLocation> importTypes = validateAndMergeNameTypeLocations(
                    bundles.getValue(), ErrorMessages.MULTI_CONFIG_IMPORT_TYPE_MISMATCH,
                    new ValidateAndFilterCallback() {
                        @Override
                        public List<NameTypeLocation> get(LayoutFileBundle bundle) {
                            return bundle.mImports;
                        }
                    });

            for (LayoutFileBundle bundle : bundles.getValue()) {
                // now add missing ones to each to ensure they can be referenced
                L.d("checking for missing variables in %s / %s", bundle.mFileName,
                        bundle.mConfigName);
                for (Map.Entry<String, NameTypeLocation> variable : variableTypes.entrySet()) {
                    if (!NameTypeLocation.contains(bundle.mVariables, variable.getKey())) {
                        NameTypeLocation orig = variable.getValue();
                        bundle.addVariable(orig.name, orig.type, orig.location, false);
                        L.d("adding missing variable %s to %s / %s", variable.getKey(),
                                bundle.mFileName, bundle.mConfigName);
                    }
                }
                for (Map.Entry<String, NameTypeLocation> userImport : importTypes.entrySet()) {
                    if (!NameTypeLocation.contains(bundle.mImports, userImport.getKey())) {
                        bundle.mImports.add(userImport.getValue());
                        L.d("adding missing import %s to %s / %s", userImport.getKey(),
                                bundle.mFileName, bundle.mConfigName);
                    }
                }
            }

            Set<String> includeBindingIds = new HashSet<String>();
            Set<String> viewBindingIds = new HashSet<String>();
            Map<String, String> viewTypes = new HashMap<String, String>();
            Map<String, String> includes = new HashMap<String, String>();
            L.d("validating ids for %s", bundles.getKey());
            Set<String> conflictingIds = new HashSet<String>();
            for (LayoutFileBundle bundle : bundles.getValue()) {
                try {
                    Scope.enter(bundle);
                    for (BindingTargetBundle target : bundle.mBindingTargetBundles) {
                        try {
                            Scope.enter(target);
                            L.d("checking %s %s %s", target.getId(), target.getFullClassName(),
                                    target.isBinder());
                            if (target.mId != null) {
                                if (target.isBinder()) {
                                    if (viewBindingIds.contains(target.mId)) {
                                        L.d("%s is conflicting", target.mId);
                                        conflictingIds.add(target.mId);
                                        continue;
                                    }
                                    includeBindingIds.add(target.mId);
                                } else {
                                    if (includeBindingIds.contains(target.mId)) {
                                        L.d("%s is conflicting", target.mId);
                                        conflictingIds.add(target.mId);
                                        continue;
                                    }
                                    viewBindingIds.add(target.mId);
                                }
                                String existingType = viewTypes.get(target.mId);
                                if (existingType == null) {
                                    L.d("assigning %s as %s", target.getId(),
                                            target.getFullClassName());
                                            viewTypes.put(target.mId, target.getFullClassName());
                                    if (target.isBinder()) {
                                        includes.put(target.mId, target.getIncludedLayout());
                                    }
                                } else if (!existingType.equals(target.getFullClassName())) {
                                    if (target.isBinder()) {
                                        L.d("overriding %s as base binder", target.getId());
                                        viewTypes.put(target.mId,
                                                "android.databinding.ViewDataBinding");
                                        includes.put(target.mId, target.getIncludedLayout());
                                    } else {
                                        L.d("overriding %s as base view", target.getId());
                                        viewTypes.put(target.mId, "android.view.View");
                                    }
                                }
                            }
                        } catch (ScopedException ex) {
                            Scope.defer(ex);
                        } finally {
                            Scope.exit();
                        }
                    }
                } finally {
                    Scope.exit();
                }
            }

            if (!conflictingIds.isEmpty()) {
                for (LayoutFileBundle bundle : bundles.getValue()) {
                    for (BindingTargetBundle target : bundle.mBindingTargetBundles) {
                        if (conflictingIds.contains(target.mId)) {
                            Scope.registerError(String.format(
                                            ErrorMessages.MULTI_CONFIG_ID_USED_AS_IMPORT,
                                            target.mId), bundle, target);
                        }
                    }
                }
            }

            for (LayoutFileBundle bundle : bundles.getValue()) {
                try {
                    Scope.enter(bundle);
                    for (Map.Entry<String, String> viewType : viewTypes.entrySet()) {
                        BindingTargetBundle target = bundle.getBindingTargetById(viewType.getKey());
                        if (target == null) {
                            String include = includes.get(viewType.getKey());
                            if (include == null) {
                                bundle.createBindingTarget(viewType.getKey(), viewType.getValue(),
                                        false, null, null, null);
                            } else {
                                BindingTargetBundle bindingTargetBundle = bundle
                                        .createBindingTarget(
                                                viewType.getKey(), null, false, null, null, null);
                                bindingTargetBundle
                                        .setIncludedLayout(includes.get(viewType.getKey()));
                                bindingTargetBundle.setInterfaceType(viewType.getValue());
                            }
                        } else {
                            L.d("setting interface type on %s (%s) as %s", target.mId,
                                    target.getFullClassName(), viewType.getValue());
                            target.setInterfaceType(viewType.getValue());
                        }
                    }
                } catch (ScopedException ex) {
                    Scope.defer(ex);
                } finally {
                    Scope.exit();
                }
            }
        }
        // assign class names to each
        for (Map.Entry<String, List<LayoutFileBundle>> entry : mLayoutBundles.entrySet()) {
            for (LayoutFileBundle bundle : entry.getValue()) {
                final String configName;
                if (bundle.hasVariations()) {
                    // append configuration specifiers.
                    final String parentFileName = bundle.mDirectory;
                    L.d("parent file for %s is %s", bundle.getFileName(), parentFileName);
                    if ("layout".equals(parentFileName)) {
                        configName = "";
                    } else {
                        configName = ParserHelper.toClassName(parentFileName.substring("layout-".length()));
                    }
                } else {
                    configName = "";
                }
                bundle.mConfigName = configName;
            }
        }
    }

    /**
     * Receives a list of bundles which are representations of the same layout file in different
     * configurations.
     * @param bundles
     * @return The map for variables and their types
     */
    private Map<String, NameTypeLocation> validateAndMergeNameTypeLocations(
            List<LayoutFileBundle> bundles, String errorMessage,
            ValidateAndFilterCallback callback) {
        Map<String, NameTypeLocation> result = new HashMap<String, NameTypeLocation>();
        Set<String> mismatched = new HashSet<String>();
        for (LayoutFileBundle bundle : bundles) {
            for (NameTypeLocation item : callback.get(bundle)) {
                NameTypeLocation existing = result.get(item.name);
                if (existing != null && !existing.type.equals(item.type)) {
                    mismatched.add(item.name);
                    continue;
                }
                result.put(item.name, item);
            }
        }
        if (mismatched.isEmpty()) {
            return result;
        }
        // create exceptions. We could get more clever and find the outlier but for now, listing
        // each file w/ locations seems enough
        for (String mismatch : mismatched) {
            for (LayoutFileBundle bundle : bundles) {
                NameTypeLocation found = null;
                for (NameTypeLocation item : callback.get(bundle)) {
                    if (mismatch.equals(item.name)) {
                        found = item;
                        break;
                    }
                }
                if (found == null) {
                    // variable is not defined in this layout, continue
                    continue;
                }
                Scope.registerError(String.format(
                                errorMessage, found.name, found.type,
                                bundle.mDirectory + "/" + bundle.getFileName()), bundle,
                        found.location.createScope());
            }
        }
        return result;
    }

    /**
     * Receives a list of bundles which are representations of the same layout file in different
     * configurations.
     * @param bundles
     * @return The shared class name for these bundles
     */
    private String validateAndGetSharedClassName(List<LayoutFileBundle> bundles) {
        String sharedClassName = null;
        boolean hasMismatch = false;
        for (LayoutFileBundle bundle : bundles) {
            bundle.mHasVariations = true;
            String fullBindingClass = bundle.getFullBindingClass();
            if (sharedClassName == null) {
                sharedClassName = fullBindingClass;
            } else if (!sharedClassName.equals(fullBindingClass)) {
                hasMismatch = true;
                break;
            }
        }
        if (!hasMismatch) {
            return sharedClassName;
        }
        // generate proper exceptions for each
        for (LayoutFileBundle bundle : bundles) {
            Scope.registerError(String.format(ErrorMessages.MULTI_CONFIG_LAYOUT_CLASS_NAME_MISMATCH,
                    bundle.getFullBindingClass(), bundle.mDirectory + "/" + bundle.getFileName()),
                    bundle, bundle.getClassNameLocationProvider());
        }
        return sharedClassName;
    }

    public void addRemovedFile(File file) {
        mRemovedFiles.add(file);
    }

    public List<File> getRemovedFiles() {
        return mRemovedFiles;
    }

    @XmlAccessorType(XmlAccessType.NONE)
    @XmlRootElement(name="Layout")
    public static class LayoutFileBundle implements Serializable, FileScopeProvider {
        @XmlAttribute(name="layout", required = true)
        public String mFileName;
        @XmlAttribute(name="modulePackage", required = true)
        public String mModulePackage;
        @XmlAttribute(name="absoluteFilePath", required = true)
        public String mAbsoluteFilePath;
        private String mConfigName;

        // The binding class as given by the user
        @XmlAttribute(name="bindingClass", required = false)
        public String mBindingClass;

        // The location of the name of the generated class, optional
        @XmlElement(name = "ClassNameLocation", required = false)
        private Location mClassNameLocation;
        // The full package and class name as determined from mBindingClass and mModulePackage
        private String mFullBindingClass;

        // The simple binding class name as determined from mBindingClass and mModulePackage
        private String mBindingClassName;

        // The package of the binding class as determined from mBindingClass and mModulePackage
        private String mBindingPackage;

        @XmlAttribute(name="directory", required = true)
        public String mDirectory;
        public boolean mHasVariations;

        @XmlElement(name="Variables")
        public List<VariableDeclaration> mVariables = new ArrayList<VariableDeclaration>();

        @XmlElement(name="Imports")
        public List<NameTypeLocation> mImports = new ArrayList<NameTypeLocation>();

        @XmlElementWrapper(name="Targets")
        @XmlElement(name="Target")
        public List<BindingTargetBundle> mBindingTargetBundles = new ArrayList<BindingTargetBundle>();

        @XmlAttribute(name="isMerge", required = true)
        private boolean mIsMerge;

        private LocationScopeProvider mClassNameLocationProvider;

        // for XML binding
        public LayoutFileBundle() {
        }

        /**
         * Updates configuration fields from the given bundle but does not change variables,
         * binding expressions etc.
         */
        public void inheritConfigurationFrom(LayoutFileBundle other) {
            mFileName = other.mFileName;
            mModulePackage = other.mModulePackage;
            mBindingClass = other.mBindingClass;
            mFullBindingClass = other.mFullBindingClass;
            mBindingClassName = other.mBindingClassName;
            mBindingPackage = other.mBindingPackage;
            mHasVariations = other.mHasVariations;
            mIsMerge = other.mIsMerge;
        }

        public LayoutFileBundle(File file, String fileName, String directory,
                String modulePackage, boolean isMerge) {
            mFileName = fileName;
            mDirectory = directory;
            mModulePackage = modulePackage;
            mIsMerge = isMerge;
            mAbsoluteFilePath = file.getAbsolutePath();
        }

        public LocationScopeProvider getClassNameLocationProvider() {
            if (mClassNameLocationProvider == null && mClassNameLocation != null
                    && mClassNameLocation.isValid()) {
                mClassNameLocationProvider = mClassNameLocation.createScope();
            }
            return mClassNameLocationProvider;
        }

        public void addVariable(String name, String type, Location location, boolean declared) {
            Preconditions.check(!NameTypeLocation.contains(mVariables, name),
                    "Cannot use same variable name twice. %s in %s", name, location);
            mVariables.add(new VariableDeclaration(name, type, location, declared));
        }

        public void addImport(String alias, String type, Location location) {
            Preconditions.check(!NameTypeLocation.contains(mImports, alias),
                    "Cannot import same alias twice. %s in %s", alias, location);
            mImports.add(new NameTypeLocation(alias, type, location));
        }

        public BindingTargetBundle createBindingTarget(String id, String viewName,
                boolean used, String tag, String originalTag, Location location) {
            BindingTargetBundle target = new BindingTargetBundle(id, viewName, used, tag,
                    originalTag, location);
            mBindingTargetBundles.add(target);
            return target;
        }

        public boolean isEmpty() {
            return mVariables.isEmpty() && mImports.isEmpty() && mBindingTargetBundles.isEmpty();
        }

        public BindingTargetBundle getBindingTargetById(String key) {
            for (BindingTargetBundle target : mBindingTargetBundles) {
                if (key.equals(target.mId)) {
                    return target;
                }
            }
            return null;
        }

        public String getFileName() {
            return mFileName;
        }

        public String getConfigName() {
            return mConfigName;
        }

        public String getDirectory() {
            return mDirectory;
        }

        public boolean hasVariations() {
            return mHasVariations;
        }

        public List<VariableDeclaration> getVariables() {
            return mVariables;
        }

        public List<NameTypeLocation> getImports() {
            return mImports;
        }

        public boolean isMerge() {
            return mIsMerge;
        }

        public String getBindingClassName() {
            if (mBindingClassName == null) {
                String fullClass = getFullBindingClass();
                int dotIndex = fullClass.lastIndexOf('.');
                mBindingClassName = fullClass.substring(dotIndex + 1);
            }
            return mBindingClassName;
        }

        public void setBindingClass(String bindingClass, Location location) {
            mBindingClass = bindingClass;
            mClassNameLocation = location;
        }

        public String getBindingClassPackage() {
            if (mBindingPackage == null) {
                String fullClass = getFullBindingClass();
                int dotIndex = fullClass.lastIndexOf('.');
                mBindingPackage = fullClass.substring(0, dotIndex);
            }
            return mBindingPackage;
        }

        private String getFullBindingClass() {
            if (mFullBindingClass == null) {
                if (mBindingClass == null) {
                    mFullBindingClass = getModulePackage() + ".databinding." +
                            ParserHelper.toClassName(getFileName()) + "Binding";
                } else if (mBindingClass.startsWith(".")) {
                    mFullBindingClass = getModulePackage() + mBindingClass;
                } else if (mBindingClass.indexOf('.') < 0) {
                    mFullBindingClass = getModulePackage() + ".databinding." + mBindingClass;
                } else {
                    mFullBindingClass = mBindingClass;
                }
            }
            return mFullBindingClass;
        }

        public List<BindingTargetBundle> getBindingTargetBundles() {
            return mBindingTargetBundles;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (o == null || getClass() != o.getClass()) {
                return false;
            }

            LayoutFileBundle bundle = (LayoutFileBundle) o;

            if (mConfigName != null ? !mConfigName.equals(bundle.mConfigName)
                    : bundle.mConfigName != null) {
                return false;
            }
            if (mDirectory != null ? !mDirectory.equals(bundle.mDirectory)
                    : bundle.mDirectory != null) {
                return false;
            }
            return !(mFileName != null ? !mFileName.equals(bundle.mFileName)
                    : bundle.mFileName != null);

        }

        @Override
        public int hashCode() {
            int result = mFileName != null ? mFileName.hashCode() : 0;
            result = 31 * result + (mConfigName != null ? mConfigName.hashCode() : 0);
            result = 31 * result + (mDirectory != null ? mDirectory.hashCode() : 0);
            return result;
        }

        @Override
        public String toString() {
            return "LayoutFileBundle{" +
                    "mHasVariations=" + mHasVariations +
                    ", mDirectory='" + mDirectory + '\'' +
                    ", mConfigName='" + mConfigName + '\'' +
                    ", mModulePackage='" + mModulePackage + '\'' +
                    ", mFileName='" + mFileName + '\'' +
                    '}';
        }

        public String getModulePackage() {
            return mModulePackage;
        }

        public String getAbsoluteFilePath() {
            return mAbsoluteFilePath;
        }

        @Override
        public String provideScopeFilePath() {
            return mAbsoluteFilePath;
        }

        private static Marshaller sMarshaller;
        private static Unmarshaller sUmarshaller;

        public String toXML() throws JAXBException {
            StringWriter writer = new StringWriter();
            getMarshaller().marshal(this, writer);
            return writer.getBuffer().toString();
        }

        public static LayoutFileBundle fromXML(InputStream inputStream) throws JAXBException {
            return (LayoutFileBundle) getUnmarshaller().unmarshal(inputStream);
        }

        private static Marshaller getMarshaller() throws JAXBException {
            if (sMarshaller == null) {
                JAXBContext context = JAXBContext
                        .newInstance(ResourceBundle.LayoutFileBundle.class);
                sMarshaller = context.createMarshaller();
            }
            return sMarshaller;
        }

        private static Unmarshaller getUnmarshaller() throws JAXBException {
            if (sUmarshaller == null) {
                JAXBContext context = JAXBContext
                        .newInstance(ResourceBundle.LayoutFileBundle.class);
                sUmarshaller = context.createUnmarshaller();
            }
            return sUmarshaller;
        }
    }

    @XmlAccessorType(XmlAccessType.NONE)
    public static class NameTypeLocation {
        @XmlAttribute(name="type", required = true)
        public String type;

        @XmlAttribute(name="name", required = true)
        public String name;

        @XmlElement(name="location", required = false)
        public Location location;

        public NameTypeLocation() {
        }

        public NameTypeLocation(String name, String type, Location location) {
            this.type = type;
            this.name = name;
            this.location = location;
        }

        @Override
        public String toString() {
            return "{" +
                    "type='" + type + '\'' +
                    ", name='" + name + '\'' +
                    ", location=" + location +
                    '}';
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (o == null || getClass() != o.getClass()) {
                return false;
            }

            NameTypeLocation that = (NameTypeLocation) o;

            if (location != null ? !location.equals(that.location) : that.location != null) {
                return false;
            }
            if (!name.equals(that.name)) {
                return false;
            }
            return type.equals(that.type);

        }

        @Override
        public int hashCode() {
            int result = type.hashCode();
            result = 31 * result + name.hashCode();
            result = 31 * result + (location != null ? location.hashCode() : 0);
            return result;
        }

        public static boolean contains(List<? extends NameTypeLocation> list, String name) {
            for (NameTypeLocation ntl : list) {
                if (name.equals(ntl.name)) {
                    return true;
                }
            }
            return false;
        }
    }

    @XmlAccessorType(XmlAccessType.NONE)
    public static class VariableDeclaration extends NameTypeLocation {
        @XmlAttribute(name="declared", required = false)
        public boolean declared;

        public VariableDeclaration() {

        }

        public VariableDeclaration(String name, String type, Location location, boolean declared) {
            super(name, type, location);
            this.declared = declared;
        }
    }

    public static class MarshalledMapType {
        public List<NameTypeLocation> entries;
    }

    @XmlAccessorType(XmlAccessType.NONE)
    public static class BindingTargetBundle implements Serializable, LocationScopeProvider {
        // public for XML serialization

        @XmlAttribute(name="id")
        public String mId;
        @XmlAttribute(name="tag", required = true)
        public String mTag;
        @XmlAttribute(name="originalTag")
        public String mOriginalTag;
        @XmlAttribute(name="view", required = false)
        public String mViewName;
        private String mFullClassName;
        public boolean mUsed = true;
        @XmlElementWrapper(name="Expressions")
        @XmlElement(name="Expression")
        public List<BindingBundle> mBindingBundleList = new ArrayList<BindingBundle>();
        @XmlAttribute(name="include")
        public String mIncludedLayout;
        @XmlElement(name="location")
        public Location mLocation;
        private String mInterfaceType;

        // For XML serialization
        public BindingTargetBundle() {}

        public BindingTargetBundle(String id, String viewName, boolean used,
                String tag, String originalTag, Location location) {
            mId = id;
            mViewName = viewName;
            mUsed = used;
            mTag = tag;
            mOriginalTag = originalTag;
            mLocation = location;
        }

        public void addBinding(String name, String expr, boolean isTwoWay, Location location,
                Location valueLocation) {
            mBindingBundleList.add(new BindingBundle(name, expr, isTwoWay, location, valueLocation));
        }

        public void setIncludedLayout(String includedLayout) {
            mIncludedLayout = includedLayout;
        }

        public String getIncludedLayout() {
            return mIncludedLayout;
        }

        public boolean isBinder() {
            return mIncludedLayout != null;
        }

        public void setInterfaceType(String interfaceType) {
            mInterfaceType = interfaceType;
        }

        public void setLocation(Location location) {
            mLocation = location;
        }

        public Location getLocation() {
            return mLocation;
        }

        public String getId() {
            return mId;
        }

        public String getTag() {
            return mTag;
        }

        public String getOriginalTag() {
            return mOriginalTag;
        }

        public String getFullClassName() {
            if (mFullClassName == null) {
                if (isBinder()) {
                    mFullClassName = mInterfaceType;
                } else if (mViewName.indexOf('.') == -1) {
                    if (Arrays.asList(ANDROID_VIEW_PACKAGE_VIEWS).contains(mViewName)) {
                        mFullClassName = "android.view." + mViewName;
                    } else if("WebView".equals(mViewName)) {
                        mFullClassName = "android.webkit." + mViewName;
                    } else {
                        mFullClassName = "android.widget." + mViewName;
                    }
                } else {
                    mFullClassName = mViewName;
                }
            }
            if (mFullClassName == null) {
                L.e("Unexpected full class name = null. view = %s, interface = %s, layout = %s",
                        mViewName, mInterfaceType, mIncludedLayout);
            }
            return mFullClassName;
        }

        public boolean isUsed() {
            return mUsed;
        }

        public List<BindingBundle> getBindingBundleList() {
            return mBindingBundleList;
        }

        public String getInterfaceType() {
            return mInterfaceType;
        }

        @Override
        public List<Location> provideScopeLocation() {
            return mLocation == null ? null : Arrays.asList(mLocation);
        }

        @XmlAccessorType(XmlAccessType.NONE)
        public static class BindingBundle implements Serializable {

            private String mName;
            private String mExpr;
            private Location mLocation;
            private Location mValueLocation;
            private boolean mIsTwoWay;

            public BindingBundle() {}

            public BindingBundle(String name, String expr, boolean isTwoWay, Location location,
                    Location valueLocation) {
                mName = name;
                mExpr = expr;
                mLocation = location;
                mIsTwoWay = isTwoWay;
                mValueLocation = valueLocation;
            }

            @XmlAttribute(name="attribute", required=true)
            public String getName() {
                return mName;
            }

            @XmlAttribute(name="text", required=true)
            public String getExpr() {
                return mExpr;
            }

            public void setName(String name) {
                mName = name;
            }

            public void setExpr(String expr) {
                mExpr = expr;
            }

            public void setTwoWay(boolean isTwoWay) {
                mIsTwoWay = isTwoWay;
            }

            @XmlElement(name="Location")
            public Location getLocation() {
                return mLocation;
            }

            public void setLocation(Location location) {
                mLocation = location;
            }

            @XmlElement(name="ValueLocation")
            public Location getValueLocation() {
                return mValueLocation;
            }

            @XmlElement(name="TwoWay")
            public boolean isTwoWay() {
                return mIsTwoWay;
            }

            public void setValueLocation(Location valueLocation) {
                mValueLocation = valueLocation;
            }
        }
    }

    /**
     * Just an inner callback class to process imports and variables w/ the same code.
     */
    private interface ValidateAndFilterCallback {
        List<? extends NameTypeLocation> get(LayoutFileBundle bundle);
    }
}
