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

import android.databinding.tool.processing.Scope;
import android.databinding.tool.processing.ScopedException;
import android.databinding.tool.store.ResourceBundle;
import android.databinding.tool.util.L;
import android.databinding.tool.writer.ComponentWriter;
import android.databinding.tool.writer.JavaFileWriter;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The main class that handles parsing files and generating classes.
 */
public class DataBinder {
    List<LayoutBinder> mLayoutBinders = new ArrayList<LayoutBinder>();
    private static final String COMPONENT_CLASS = "android.databinding.DataBindingComponent";

    private JavaFileWriter mFileWriter;

    Set<String> writtenClasses = new HashSet<String>();

    public DataBinder(ResourceBundle resourceBundle) {
        L.d("reading resource bundle into data binder");
        for (Map.Entry<String, List<ResourceBundle.LayoutFileBundle>> entry :
                resourceBundle.getLayoutBundles().entrySet()) {
            for (ResourceBundle.LayoutFileBundle bundle : entry.getValue()) {
                try {
                    mLayoutBinders.add(new LayoutBinder(bundle));
                } catch (ScopedException ex) {
                    Scope.defer(ex);
                }
            }
        }
    }
    public List<LayoutBinder> getLayoutBinders() {
        return mLayoutBinders;
    }

    public void sealModels() {
        for (LayoutBinder layoutBinder : mLayoutBinders) {
            layoutBinder.sealModel();
        }
    }

    public void writerBaseClasses(boolean isLibrary) {
        for (LayoutBinder layoutBinder : mLayoutBinders) {
            try {
                Scope.enter(layoutBinder);
                if (isLibrary || layoutBinder.hasVariations()) {
                    String className = layoutBinder.getClassName();
                    String canonicalName = layoutBinder.getPackage() + "." + className;
                    if (writtenClasses.contains(canonicalName)) {
                        continue;
                    }
                    L.d("writing data binder base %s", canonicalName);
                    mFileWriter.writeToFile(canonicalName,
                            layoutBinder.writeViewBinderBaseClass(isLibrary));
                    writtenClasses.add(canonicalName);
                }
            } catch (ScopedException ex){
                Scope.defer(ex);
            } finally {
                Scope.exit();
            }
        }
    }

    public void writeBinders(int minSdk) {
        for (LayoutBinder layoutBinder : mLayoutBinders) {
            try {
                Scope.enter(layoutBinder);
                String className = layoutBinder.getImplementationName();
                String canonicalName = layoutBinder.getPackage() + "." + className;
                L.d("writing data binder %s", canonicalName);
                writtenClasses.add(canonicalName);
                mFileWriter.writeToFile(canonicalName, layoutBinder.writeViewBinder(minSdk));
            } catch (ScopedException ex) {
                Scope.defer(ex);
            } finally {
                Scope.exit();
            }
        }
    }

    public void writeComponent() {
        ComponentWriter componentWriter = new ComponentWriter();

        writtenClasses.add(COMPONENT_CLASS);
        mFileWriter.writeToFile(COMPONENT_CLASS, componentWriter.createComponent());
    }

    public Set<String> getWrittenClassNames() {
        return writtenClasses;
    }

    public void setFileWriter(JavaFileWriter fileWriter) {
        mFileWriter = fileWriter;
    }

    public JavaFileWriter getFileWriter() {
        return mFileWriter;
    }
}
