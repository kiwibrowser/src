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
import android.databinding.tool.CompilerChef;
import android.databinding.tool.processing.Scope;
import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;
import android.databinding.tool.writer.AnnotationJavaFileWriter;
import android.databinding.tool.writer.BRWriter;
import android.databinding.tool.writer.JavaFileWriter;

import java.util.Arrays;
import java.util.List;
import java.util.Set;

import javax.annotation.processing.AbstractProcessor;
import javax.annotation.processing.ProcessingEnvironment;
import javax.annotation.processing.RoundEnvironment;
import javax.annotation.processing.SupportedAnnotationTypes;
import javax.lang.model.SourceVersion;
import javax.lang.model.element.TypeElement;
import javax.xml.bind.JAXBException;

@SupportedAnnotationTypes({
        "android.databinding.BindingAdapter",
        "android.databinding.Untaggable",
        "android.databinding.BindingMethods",
        "android.databinding.BindingConversion",
        "android.databinding.BindingBuildInfo"}
)
/**
 * Parent annotation processor that dispatches sub steps to ensure execution order.
 * Use initProcessingSteps to add a new step.
 */
public class ProcessDataBinding extends AbstractProcessor {
    private List<ProcessingStep> mProcessingSteps;
    @Override
    public boolean process(Set<? extends TypeElement> annotations, RoundEnvironment roundEnv) {
        if (mProcessingSteps == null) {
            initProcessingSteps();
        }
        final BindingBuildInfo buildInfo = BuildInfoUtil.load(roundEnv);
        if (buildInfo == null) {
            return false;
        }
        boolean done = true;
        for (ProcessingStep step : mProcessingSteps) {
            try {
                done = step.runStep(roundEnv, processingEnv, buildInfo) && done;
            } catch (JAXBException e) {
                L.e(e, "Exception while handling step %s", step);
            }
        }
        if (roundEnv.processingOver()) {
            for (ProcessingStep step : mProcessingSteps) {
                step.onProcessingOver(roundEnv, processingEnv, buildInfo);
            }
        }
        Scope.assertNoError();
        return done;
    }

    @Override
    public SourceVersion getSupportedSourceVersion() {
        return SourceVersion.latest();
    }

    private void initProcessingSteps() {
        final ProcessBindable processBindable = new ProcessBindable();
        mProcessingSteps = Arrays.asList(
                new ProcessMethodAdapters(),
                new ProcessExpressions(),
                processBindable
        );
        Callback dataBinderWriterCallback = new Callback() {
            CompilerChef mChef;
            BRWriter mBRWriter;
            boolean mLibraryProject;
            int mMinSdk;

            @Override
            public void onChefReady(CompilerChef chef, boolean libraryProject, int minSdk) {
                Preconditions.checkNull(mChef, "Cannot set compiler chef twice");
                chef.addBRVariables(processBindable);
                mChef = chef;
                mLibraryProject = libraryProject;
                mMinSdk = minSdk;
                considerWritingMapper();
                mChef.writeDynamicUtil();
            }

            private void considerWritingMapper() {
                if (mLibraryProject || mChef == null || mBRWriter == null) {
                    return;
                }
                mChef.writeDataBinderMapper(mMinSdk, mBRWriter);
            }

            @Override
            public void onBrWriterReady(BRWriter brWriter) {
                Preconditions.checkNull(mBRWriter, "Cannot set br writer twice");
                mBRWriter = brWriter;
                considerWritingMapper();
            }
        };
        AnnotationJavaFileWriter javaFileWriter = new AnnotationJavaFileWriter(processingEnv);
        for (ProcessingStep step : mProcessingSteps) {
            step.mJavaFileWriter = javaFileWriter;
            step.mCallback = dataBinderWriterCallback;
        }
    }

    @Override
    public synchronized void init(ProcessingEnvironment processingEnv) {
        super.init(processingEnv);
        ModelAnalyzer.setProcessingEnvironment(processingEnv);
    }

    /**
     * To ensure execution order and binding build information, we use processing steps.
     */
    public abstract static class ProcessingStep {
        private boolean mDone;
        private JavaFileWriter mJavaFileWriter;
        protected Callback mCallback;

        protected JavaFileWriter getWriter() {
            return mJavaFileWriter;
        }

        private boolean runStep(RoundEnvironment roundEnvironment,
                ProcessingEnvironment processingEnvironment,
                BindingBuildInfo buildInfo) throws JAXBException {
            if (mDone) {
                return true;
            }
            mDone = onHandleStep(roundEnvironment, processingEnvironment, buildInfo);
            return mDone;
        }

        /**
         * Invoked in each annotation processing step.
         *
         * @return True if it is done and should never be invoked again.
         */
        abstract public boolean onHandleStep(RoundEnvironment roundEnvironment,
                ProcessingEnvironment processingEnvironment,
                BindingBuildInfo buildInfo) throws JAXBException;

        /**
         * Invoked when processing is done. A good place to generate the output if the
         * processor requires multiple steps.
         */
        abstract public void onProcessingOver(RoundEnvironment roundEnvironment,
                ProcessingEnvironment processingEnvironment,
                BindingBuildInfo buildInfo);
    }

    interface Callback {
        void onChefReady(CompilerChef chef, boolean libraryProject, int minSdk);
        void onBrWriterReady(BRWriter brWriter);
    }
}
