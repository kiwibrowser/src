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

import com.google.common.base.Joiner;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.IOUtils;

import android.databinding.BindingBuildInfo;
import android.databinding.tool.CompilerChef;
import android.databinding.tool.LayoutXmlProcessor;
import android.databinding.tool.reflection.SdkUtil;
import android.databinding.tool.store.ResourceBundle;
import android.databinding.tool.util.GenerationalClassUtil;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;
import android.databinding.tool.util.StringUtils;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.processing.ProcessingEnvironment;
import javax.annotation.processing.RoundEnvironment;
import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Unmarshaller;

public class ProcessExpressions extends ProcessDataBinding.ProcessingStep {
    public ProcessExpressions() {
    }

    @Override
    public boolean onHandleStep(RoundEnvironment roundEnvironment,
            ProcessingEnvironment processingEnvironment, BindingBuildInfo buildInfo)
            throws JAXBException {
        ResourceBundle resourceBundle;
        SdkUtil.initialize(buildInfo.minSdk(), new File(buildInfo.sdkRoot()));
        resourceBundle = new ResourceBundle(buildInfo.modulePackage());
        List<IntermediateV2> intermediateList = loadDependencyIntermediates();
        for (Intermediate intermediate : intermediateList) {
            try {
                intermediate.appendTo(resourceBundle);
            } catch (Throwable throwable) {
                L.e(throwable, "unable to prepare resource bundle");
            }
        }

        IntermediateV2 mine = createIntermediateFromLayouts(buildInfo.layoutInfoDir(),
                intermediateList);
        if (mine != null) {
            mine.updateOverridden(resourceBundle);
            intermediateList.add(mine);
            saveIntermediate(processingEnvironment, buildInfo, mine);
            mine.appendTo(resourceBundle);
        }
        // generate them here so that bindable parser can read
        try {
            writeResourceBundle(resourceBundle, buildInfo.isLibrary(), buildInfo.minSdk(),
                    buildInfo.exportClassListTo());
        } catch (Throwable t) {
            L.e(t, "cannot generate view binders");
        }
        return true;
    }

    private List<IntermediateV2> loadDependencyIntermediates() {
        final List<Intermediate> original = GenerationalClassUtil.loadObjects(
                GenerationalClassUtil.ExtensionFilter.LAYOUT);
        final List<IntermediateV2> upgraded = new ArrayList<IntermediateV2>(original.size());
        for (Intermediate intermediate : original) {
            final Intermediate updatedIntermediate = intermediate.upgrade();
            Preconditions.check(updatedIntermediate instanceof IntermediateV2, "Incompatible data"
                    + " binding dependency. Please update your dependencies or recompile them with"
                    + " application module's data binding version.");
            //noinspection ConstantConditions
            upgraded.add((IntermediateV2) updatedIntermediate);
        }
        return upgraded;
    }

    private void saveIntermediate(ProcessingEnvironment processingEnvironment,
            BindingBuildInfo buildInfo, IntermediateV2 intermediate) {
        GenerationalClassUtil.writeIntermediateFile(processingEnvironment,
                buildInfo.modulePackage(), buildInfo.modulePackage() +
                        GenerationalClassUtil.ExtensionFilter.LAYOUT.getExtension(),
                intermediate);
    }

    @Override
    public void onProcessingOver(RoundEnvironment roundEnvironment,
            ProcessingEnvironment processingEnvironment, BindingBuildInfo buildInfo) {
    }

    private IntermediateV2 createIntermediateFromLayouts(String layoutInfoFolderPath,
            List<IntermediateV2> intermediateList) {
        final Set<String> excludeList = new HashSet<String>();
        for (IntermediateV2 lib : intermediateList) {
            excludeList.addAll(lib.mLayoutInfoMap.keySet());
        }
        final File layoutInfoFolder = new File(layoutInfoFolderPath);
        if (!layoutInfoFolder.isDirectory()) {
            L.d("layout info folder does not exist, skipping for %s", layoutInfoFolderPath);
            return null;
        }
        IntermediateV2 result = new IntermediateV2();
        for (File layoutFile : layoutInfoFolder.listFiles(new FilenameFilter() {
            @Override
            public boolean accept(File dir, String name) {
                return name.endsWith(".xml") && !excludeList.contains(name);
            }
        })) {
            try {
                result.addEntry(layoutFile.getName(), FileUtils.readFileToString(layoutFile));
            } catch (IOException e) {
                L.e(e, "cannot load layout file information. Try a clean build");
            }
        }
        return result;
    }

    private void writeResourceBundle(ResourceBundle resourceBundle, boolean forLibraryModule,
            final int minSdk, String exportClassNamesTo)
            throws JAXBException {
        final CompilerChef compilerChef = CompilerChef.createChef(resourceBundle, getWriter());
        compilerChef.sealModels();
        compilerChef.writeComponent();
        if (compilerChef.hasAnythingToGenerate()) {
            compilerChef.writeViewBinderInterfaces(forLibraryModule);
            if (!forLibraryModule) {
                compilerChef.writeViewBinders(minSdk);
            }
        }
        if (forLibraryModule && exportClassNamesTo == null) {
            L.e("When compiling a library module, build info must include exportClassListTo path");
        }
        if (forLibraryModule) {
            Set<String> classNames = compilerChef.getWrittenClassNames();
            String out = Joiner.on(StringUtils.LINE_SEPARATOR).join(classNames);
            L.d("Writing list of classes to %s . \nList:%s", exportClassNamesTo, out);
            try {
                //noinspection ConstantConditions
                FileUtils.write(new File(exportClassNamesTo), out);
            } catch (IOException e) {
                L.e(e, "Cannot create list of written classes");
            }
        }
        mCallback.onChefReady(compilerChef, forLibraryModule, minSdk);
    }

    public interface Intermediate extends Serializable {

        Intermediate upgrade();

        void appendTo(ResourceBundle resourceBundle) throws Throwable;
    }

    public static class IntermediateV1 implements Intermediate {

        transient Unmarshaller mUnmarshaller;

        // name to xml content map
        Map<String, String> mLayoutInfoMap = new HashMap<String, String>();

        @Override
        public Intermediate upgrade() {
            final IntermediateV2 updated = new IntermediateV2();
            updated.mLayoutInfoMap = mLayoutInfoMap;
            updated.mUnmarshaller = mUnmarshaller;
            return updated;
        }

        @Override
        public void appendTo(ResourceBundle resourceBundle) throws JAXBException {
            if (mUnmarshaller == null) {
                JAXBContext context = JAXBContext
                        .newInstance(ResourceBundle.LayoutFileBundle.class);
                mUnmarshaller = context.createUnmarshaller();
            }
            for (String content : mLayoutInfoMap.values()) {
                final InputStream is = IOUtils.toInputStream(content);
                try {
                    final ResourceBundle.LayoutFileBundle bundle
                            = (ResourceBundle.LayoutFileBundle) mUnmarshaller.unmarshal(is);
                    resourceBundle.addLayoutBundle(bundle);
                    L.d("loaded layout info file %s", bundle);
                } finally {
                    IOUtils.closeQuietly(is);
                }
            }
        }

        public void addEntry(String name, String contents) {
            mLayoutInfoMap.put(name, contents);
        }

        // keeping the method to match deserialized structure
        @SuppressWarnings("unused")
        public void removeOverridden(List<Intermediate> existing) {
        }
    }

    public static class IntermediateV2 extends IntermediateV1 {
        // specify so that we can define updates ourselves.
        private static final long serialVersionUID = 2L;
        @Override
        public void appendTo(ResourceBundle resourceBundle) throws JAXBException {
            for (Map.Entry<String, String> entry : mLayoutInfoMap.entrySet()) {
                final InputStream is = IOUtils.toInputStream(entry.getValue());
                try {
                    final ResourceBundle.LayoutFileBundle bundle = ResourceBundle.LayoutFileBundle
                            .fromXML(is);
                    resourceBundle.addLayoutBundle(bundle);
                    L.d("loaded layout info file %s", bundle);
                } finally {
                    IOUtils.closeQuietly(is);
                }
            }
        }

        /**
         * if a layout is overridden from a module (which happens when layout is auto-generated),
         * we need to update its contents from the class that overrides it.
         * This must be done before this bundle is saved, otherwise, it will not be recognized
         * when it is used in another project.
         */
        public void updateOverridden(ResourceBundle bundle) throws JAXBException {
            // When a layout is copied from inherited module, it is eleminated while reading
            // info files. (createIntermediateFromLayouts).
            // Build process may also duplicate some files at compile time. This is where
            // we detect those copies and force inherit their module and classname information.
            final HashMap<String, List<ResourceBundle.LayoutFileBundle>> bundles = bundle
                    .getLayoutBundles();
            for (Map.Entry<String, String> info : mLayoutInfoMap.entrySet()) {
                String key = LayoutXmlProcessor.exportLayoutNameFromInfoFileName(info.getKey());
                final List<ResourceBundle.LayoutFileBundle> existingList = bundles.get(key);
                if (existingList != null && !existingList.isEmpty()) {
                    ResourceBundle.LayoutFileBundle myBundle = ResourceBundle.LayoutFileBundle
                            .fromXML(IOUtils.toInputStream(info.getValue()));
                    final ResourceBundle.LayoutFileBundle inheritFrom = existingList.get(0);
                    myBundle.inheritConfigurationFrom(inheritFrom);
                    L.d("inheriting data for %s (%s) from %s", info.getKey(), key, inheritFrom);
                    mLayoutInfoMap.put(info.getKey(), myBundle.toXML());
                }
            }
        }
    }
}
