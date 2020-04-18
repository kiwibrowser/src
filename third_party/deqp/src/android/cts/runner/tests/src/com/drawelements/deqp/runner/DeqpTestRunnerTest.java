/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.drawelements.deqp.runner;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.IShellOutputReceiver;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.build.IFolderBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IRuntimeHintProvider;
import com.android.tradefed.util.AbiUtils;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunInterruptedException;

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.easymock.IAnswer;
import org.easymock.IMocksControl;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringReader;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * Unit tests for {@link DeqpTestRunner}.
 */
public class DeqpTestRunnerTest extends TestCase {
    private static final String NAME = "dEQP-GLES3";
    private static final IAbi ABI = new Abi("armeabi-v7a", "32");
    private static final String CASE_LIST_FILE_NAME = "/sdcard/dEQP-TestCaseList.txt";
    private static final String LOG_FILE_NAME = "/sdcard/TestLog.qpa";
    private static final String INSTRUMENTATION_NAME =
            "com.drawelements.deqp/com.drawelements.deqp.testercore.DeqpInstrumentation";
    private static final String QUERY_INSTRUMENTATION_NAME =
            "com.drawelements.deqp/com.drawelements.deqp.platformutil.DeqpPlatformCapabilityQueryInstrumentation";
    private static final String DEQP_ONDEVICE_APK = "com.drawelements.deqp.apk";
    private static final String DEQP_ONDEVICE_PKG = "com.drawelements.deqp";
    private static final String ONLY_LANDSCAPE_FEATURES =
            "feature:"+DeqpTestRunner.FEATURE_LANDSCAPE;
    private static final String ALL_FEATURES =
            ONLY_LANDSCAPE_FEATURES + "\nfeature:"+DeqpTestRunner.FEATURE_PORTRAIT;
    private static List<Map<String,String>> DEFAULT_INSTANCE_ARGS;

    static {
        DEFAULT_INSTANCE_ARGS = new ArrayList<>(1);
        DEFAULT_INSTANCE_ARGS.add(new HashMap<String,String>());
        DEFAULT_INSTANCE_ARGS.iterator().next().put("glconfig", "rgba8888d24s8");
        DEFAULT_INSTANCE_ARGS.iterator().next().put("rotation", "unspecified");
        DEFAULT_INSTANCE_ARGS.iterator().next().put("surfacetype", "window");
    }

    private File mTestsDir = null;

    public static class BuildHelperMock extends CompatibilityBuildHelper {
        private File mTestsDir = null;
        public BuildHelperMock(IFolderBuildInfo buildInfo, File testsDir) {
            super(buildInfo);
            mTestsDir = testsDir;
        }
        @Override
        public File getTestsDir() throws FileNotFoundException {
            return mTestsDir;
        }
    }


    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTestsDir = FileUtil.createTempDir("deqp-test-cases");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void tearDown() throws Exception {
        FileUtil.recursiveDelete(mTestsDir);
        super.tearDown();
    }

    private static DeqpTestRunner buildGlesTestRunner(int majorVersion,
                                                      int minorVersion,
                                                      Collection<TestIdentifier> tests,
                                                      File testsDir) throws ConfigurationException, FileNotFoundException {
        StringWriter testlist = new StringWriter();
        for (TestIdentifier test : tests) {
            testlist.write(test.getClassName() + "." + test.getTestName() + "\n");
        }
        return buildGlesTestRunner(majorVersion, minorVersion, testlist.toString(), testsDir);
    }

    private static CompatibilityBuildHelper getMockBuildHelper(File testsDir) {
        IFolderBuildInfo mockIFolderBuildInfo = EasyMock.createMock(IFolderBuildInfo.class);
        EasyMock.replay(mockIFolderBuildInfo);
        return new BuildHelperMock(mockIFolderBuildInfo, testsDir);
    }

    private static DeqpTestRunner buildGlesTestRunner(int majorVersion,
                                                      int minorVersion,
                                                      String testlist,
                                                      File testsDir) throws ConfigurationException, FileNotFoundException {
        DeqpTestRunner runner = new DeqpTestRunner();
        OptionSetter setter = new OptionSetter(runner);

        String deqpPackage = "dEQP-GLES" + Integer.toString(majorVersion)
                + (minorVersion > 0 ? Integer.toString(minorVersion) : "");

        setter.setOptionValue("deqp-package", deqpPackage);
        setter.setOptionValue("deqp-gl-config-name", "rgba8888d24s8");
        setter.setOptionValue("deqp-caselist-file", "dummyfile.txt");
        setter.setOptionValue("deqp-screen-rotation", "unspecified");
        setter.setOptionValue("deqp-surface-type", "window");

        runner.setCaselistReader(new StringReader(testlist));
        runner.setAbi(ABI);
        runner.setBuildHelper(getMockBuildHelper(testsDir));

        return runner;
    }

    private static String getTestId(DeqpTestRunner runner) {
        return AbiUtils.createId(ABI.getName(), runner.getPackageName());
    }

    /**
     * Test version of OpenGL ES.
     */
    private void testGlesVersion(int requiredMajorVersion, int requiredMinorVersion, int majorVersion, int minorVersion) throws Exception {
        final TestIdentifier testId = new TestIdentifier("dEQP-GLES"
                + Integer.toString(requiredMajorVersion) + Integer.toString(requiredMinorVersion)
                + ".info", "version");

        final String testPath = "dEQP-GLES"
                + Integer.toString(requiredMajorVersion) + Integer.toString(requiredMinorVersion)
                +".info.version";

        final String testTrie = "{dEQP-GLES"
                + Integer.toString(requiredMajorVersion) + Integer.toString(requiredMinorVersion)
                + "{info{version}}}";

        final String resultCode = "Pass";

        /* MultiLineReceiver expects "\r\n" line ending. */
        final String output = "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=2014.x\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseId\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=0xcafebabe\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=targetName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=android\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=" + testPath + "\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=" + resultCode + "\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Detail" + resultCode + "\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_CODE: 0\r\n";

        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        IDevice mockIDevice = EasyMock.createMock(IDevice.class);
        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();
        tests.add(testId);

        DeqpTestRunner deqpTest = buildGlesTestRunner(requiredMajorVersion, requiredMinorVersion, tests, mTestsDir);

        int version = (majorVersion << 16) | minorVersion;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
            .andReturn(Integer.toString(version)).atLeastOnce();

        if (majorVersion > requiredMajorVersion
                || (majorVersion == requiredMajorVersion && minorVersion >= requiredMinorVersion)) {

            EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG)))
                    .andReturn("").once();
            EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                    EasyMock.eq(true),
                    EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName()))))
                    .andReturn(null).once();

            expectRenderConfigQuery(mockDevice, requiredMajorVersion,
                    requiredMinorVersion);

            String commandLine = String.format(
                    "--deqp-caselist-file=%s --deqp-gl-config-name=rgba8888d24s8 "
                    + "--deqp-screen-rotation=unspecified "
                    + "--deqp-surface-type=window "
                    + "--deqp-log-images=disable "
                    + "--deqp-watchdog=enable",
                    CASE_LIST_FILE_NAME);

            runInstrumentationLineAndAnswer(mockDevice, mockIDevice, testTrie, commandLine,
                    output);

            EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG)))
                    .andReturn("").once();
        }

        mockListener.testRunStarted(getTestId(deqpTest), 1);
        EasyMock.expectLastCall().once();

        mockListener.testStarted(EasyMock.eq(testId));
        EasyMock.expectLastCall().once();

        mockListener.testEnded(EasyMock.eq(testId), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        EasyMock.replay(mockDevice, mockIDevice);
        EasyMock.replay(mockListener);

        deqpTest.setDevice(mockDevice);
        deqpTest.run(mockListener);

        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice, mockIDevice);
    }

    private void expectRenderConfigQuery(ITestDevice mockDevice, int majorVersion,
            int minorVersion) throws Exception {
        expectRenderConfigQuery(mockDevice,
                String.format("--deqp-gl-config-name=rgba8888d24s8 "
                + "--deqp-screen-rotation=unspecified "
                + "--deqp-surface-type=window "
                + "--deqp-gl-major-version=%d "
                + "--deqp-gl-minor-version=%d", majorVersion, minorVersion));
    }

    private void expectRenderConfigQuery(ITestDevice mockDevice, String commandLine)
            throws Exception {
        expectRenderConfigQueryAndReturn(mockDevice, commandLine, "Yes");
    }

    private void expectRenderConfigQueryAndReturn(ITestDevice mockDevice, String commandLine,
            String output) throws Exception {
        final String queryOutput = "INSTRUMENTATION_RESULT: Supported=" + output + "\r\n"
                + "INSTRUMENTATION_CODE: 0\r\n";
        final String command = String.format(
                "am instrument %s -w -e deqpQueryType renderConfigSupported -e deqpCmdLine "
                    + "\"%s\" %s",
                AbiUtils.createAbiFlag(ABI.getName()), commandLine,
                QUERY_INSTRUMENTATION_NAME);

        mockDevice.executeShellCommand(EasyMock.eq(command),
                EasyMock.<IShellOutputReceiver>notNull());

        EasyMock.expectLastCall().andAnswer(new IAnswer<Object>() {
            @Override
            public Object answer() {
                IShellOutputReceiver receiver
                        = (IShellOutputReceiver)EasyMock.getCurrentArguments()[1];

                receiver.addOutput(queryOutput.getBytes(), 0, queryOutput.length());
                receiver.flush();

                return null;
            }
        });
    }

    /**
     * Test that result code produces correctly pass or fail.
     */
    private void testResultCode(final String resultCode, boolean pass) throws Exception {
        final TestIdentifier testId = new TestIdentifier("dEQP-GLES3.info", "version");
        final String testPath = "dEQP-GLES3.info.version";
        final String testTrie = "{dEQP-GLES3{info{version}}}";

        /* MultiLineReceiver expects "\r\n" line ending. */
        final String output = "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=2014.x\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseId\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=0xcafebabe\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=targetName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=android\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=" + testPath + "\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=" + resultCode + "\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Detail" + resultCode + "\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_CODE: 0\r\n";

        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        IDevice mockIDevice = EasyMock.createMock(IDevice.class);

        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();
        tests.add(testId);

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);

        int version = 3 << 16;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                .once();

        EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true), EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName()))))
                .andReturn(null).once();

        expectRenderConfigQuery(mockDevice, 3, 0);

        String commandLine = String.format(
                "--deqp-caselist-file=%s --deqp-gl-config-name=rgba8888d24s8 "
                + "--deqp-screen-rotation=unspecified "
                + "--deqp-surface-type=window "
                + "--deqp-log-images=disable "
                + "--deqp-watchdog=enable",
                CASE_LIST_FILE_NAME);

        runInstrumentationLineAndAnswer(mockDevice, mockIDevice, testTrie, commandLine, output);

        mockListener.testRunStarted(getTestId(deqpTest), 1);
        EasyMock.expectLastCall().once();

        mockListener.testStarted(EasyMock.eq(testId));
        EasyMock.expectLastCall().once();

        if (!pass) {
            mockListener.testFailed(testId,
                    "=== with config {glformat=rgba8888d24s8,rotation=unspecified,surfacetype=window,required=false} ===\n"
                    + resultCode + ": Detail" + resultCode);

            EasyMock.expectLastCall().once();
        }

        mockListener.testEnded(EasyMock.eq(testId), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                .once();

        EasyMock.replay(mockDevice, mockIDevice);
        EasyMock.replay(mockListener);

        deqpTest.setDevice(mockDevice);
        deqpTest.run(mockListener);

        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice, mockIDevice);
    }

    /**
     * Test running multiple test cases.
     */
    public void testRun_multipleTests() throws Exception {
        /* MultiLineReceiver expects "\r\n" line ending. */
        final String output = "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=2014.x\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseId\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=0xcafebabe\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=targetName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=android\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.vendor\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.renderer\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.version\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.shading_language_version\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.extensions\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.render_target\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_CODE: 0\r\n";

        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.info", "vendor"),
                new TestIdentifier("dEQP-GLES3.info", "renderer"),
                new TestIdentifier("dEQP-GLES3.info", "version"),
                new TestIdentifier("dEQP-GLES3.info", "shading_language_version"),
                new TestIdentifier("dEQP-GLES3.info", "extensions"),
                new TestIdentifier("dEQP-GLES3.info", "render_target")
        };

        final String[] testPaths = {
                "dEQP-GLES3.info.vendor",
                "dEQP-GLES3.info.renderer",
                "dEQP-GLES3.info.version",
                "dEQP-GLES3.info.shading_language_version",
                "dEQP-GLES3.info.extensions",
                "dEQP-GLES3.info.render_target"
        };

        final String testTrie
                = "{dEQP-GLES3{info{vendor,renderer,version,shading_language_version,extensions,render_target}}}";

        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        IDevice mockIDevice = EasyMock.createMock(IDevice.class);

        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();

        for (TestIdentifier id : testIds) {
            tests.add(id);
        }

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);

        int version = 3 << 16;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                .once();
        EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true), EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName()))))
                .andReturn(null).once();

        expectRenderConfigQuery(mockDevice, 3, 0);

        String commandLine = String.format(
                "--deqp-caselist-file=%s --deqp-gl-config-name=rgba8888d24s8 "
                + "--deqp-screen-rotation=unspecified "
                + "--deqp-surface-type=window "
                + "--deqp-log-images=disable "
                + "--deqp-watchdog=enable",
                CASE_LIST_FILE_NAME);

        runInstrumentationLineAndAnswer(mockDevice, mockIDevice, testTrie, commandLine, output);

        mockListener.testRunStarted(getTestId(deqpTest), testPaths.length);
        EasyMock.expectLastCall().once();

        for (int i = 0; i < testPaths.length; i++) {
            mockListener.testStarted(EasyMock.eq(testIds[i]));
            EasyMock.expectLastCall().once();

            mockListener.testEnded(EasyMock.eq(testIds[i]),
                    EasyMock.<Map<String, String>>notNull());

            EasyMock.expectLastCall().once();
        }

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                .once();

        EasyMock.replay(mockDevice, mockIDevice);
        EasyMock.replay(mockListener);

        deqpTest.setDevice(mockDevice);
        deqpTest.run(mockListener);

        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice, mockIDevice);
    }

    static private String buildTestProcessOutput(List<TestIdentifier> tests) {
        /* MultiLineReceiver expects "\r\n" line ending. */
        final String outputHeader = "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=2014.x\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseId\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=0xcafebabe\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=targetName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=android\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n";

        final String outputEnd = "INSTRUMENTATION_STATUS: dEQP-EventType=EndSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_CODE: 0\r\n";

        StringWriter output = new StringWriter();
        output.write(outputHeader);
        for (TestIdentifier test : tests) {
            output.write("INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n");
            output.write("INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=");
            output.write(test.getClassName());
            output.write(".");
            output.write(test.getTestName());
            output.write("\r\n");
            output.write("INSTRUMENTATION_STATUS_CODE: 0\r\n");
            output.write("INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n");
            output.write("INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n");
            output.write("INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n");
            output.write("INSTRUMENTATION_STATUS_CODE: 0\r\n");
            output.write("INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n");
            output.write("INSTRUMENTATION_STATUS_CODE: 0\r\n");
        }
        output.write(outputEnd);
        return output.toString();
    }

    private void testFiltering(DeqpTestRunner deqpTest,
                               String expectedTrie,
                               List<TestIdentifier> expectedTests) throws Exception {
        int version = 3 << 16;
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        boolean thereAreTests = !expectedTests.isEmpty();
        if (thereAreTests)
        {
            // only expect to install/uninstall packages if there are any tests
            EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                .once();
            EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true), EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName()))))
                .andReturn(null).once();
        }

        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        mockListener.testRunStarted(getTestId(deqpTest), expectedTests.size());
        EasyMock.expectLastCall().once();

        IDevice mockIDevice = EasyMock.createMock(IDevice.class);
        if (thereAreTests)
        {
            expectRenderConfigQuery(mockDevice, 3, 0);

            String testOut = buildTestProcessOutput(expectedTests);
            runInstrumentationLineAndAnswer(mockDevice, mockIDevice, testOut);

            for (int i = 0; i < expectedTests.size(); i++) {
                mockListener.testStarted(EasyMock.eq(expectedTests.get(i)));
                EasyMock.expectLastCall().once();

                mockListener.testEnded(EasyMock.eq(expectedTests.get(i)),
                                       EasyMock.<Map<String, String>>notNull());

                EasyMock.expectLastCall().once();
            }
        }

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        if (thereAreTests)
        {
            // package will only be installed if there are tests to run
            EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                .once();
        }

        EasyMock.replay(mockDevice, mockIDevice);
        EasyMock.replay(mockListener);

        deqpTest.setDevice(mockDevice);
        deqpTest.run(mockListener);

        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice, mockIDevice);
    }

    public void testRun_trivialIncludeFilter() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.missing", "no"),
                new TestIdentifier("dEQP-GLES3.missing", "nope"),
                new TestIdentifier("dEQP-GLES3.missing", "donotwant"),
                new TestIdentifier("dEQP-GLES3.pick_me", "yes"),
                new TestIdentifier("dEQP-GLES3.pick_me", "ok"),
                new TestIdentifier("dEQP-GLES3.pick_me", "accepted"),
        };

        List<TestIdentifier> allTests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) {
            allTests.add(id);
        }

        List<TestIdentifier> activeTests = new ArrayList<TestIdentifier>();
        activeTests.add(testIds[3]);
        activeTests.add(testIds[4]);
        activeTests.add(testIds[5]);

        String expectedTrie = "{dEQP-GLES3{pick_me{yes,ok,accepted}}}";

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, allTests, mTestsDir);
        deqpTest.addIncludeFilter("dEQP-GLES3.pick_me#*");
        testFiltering(deqpTest, expectedTrie, activeTests);
    }

    public void testRun_trivialExcludeFilter() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.missing", "no"),
                new TestIdentifier("dEQP-GLES3.missing", "nope"),
                new TestIdentifier("dEQP-GLES3.missing", "donotwant"),
                new TestIdentifier("dEQP-GLES3.pick_me", "yes"),
                new TestIdentifier("dEQP-GLES3.pick_me", "ok"),
                new TestIdentifier("dEQP-GLES3.pick_me", "accepted"),
        };

        List<TestIdentifier> allTests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) {
            allTests.add(id);
        }

        List<TestIdentifier> activeTests = new ArrayList<TestIdentifier>();
        activeTests.add(testIds[3]);
        activeTests.add(testIds[4]);
        activeTests.add(testIds[5]);

        String expectedTrie = "{dEQP-GLES3{pick_me{yes,ok,accepted}}}";

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, allTests, mTestsDir);
        deqpTest.addExcludeFilter("dEQP-GLES3.missing#*");
        testFiltering(deqpTest, expectedTrie, activeTests);
    }

    public void testRun_includeAndExcludeFilter() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.group1", "foo"),
                new TestIdentifier("dEQP-GLES3.group1", "nope"),
                new TestIdentifier("dEQP-GLES3.group1", "donotwant"),
                new TestIdentifier("dEQP-GLES3.group2", "foo"),
                new TestIdentifier("dEQP-GLES3.group2", "yes"),
                new TestIdentifier("dEQP-GLES3.group2", "thoushallnotpass"),
        };

        List<TestIdentifier> allTests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) {
            allTests.add(id);
        }

        List<TestIdentifier> activeTests = new ArrayList<TestIdentifier>();
        activeTests.add(testIds[4]);

        String expectedTrie = "{dEQP-GLES3{group2{yes}}}";

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, allTests, mTestsDir);

        Set<String> includes = new HashSet<>();
        includes.add("dEQP-GLES3.group2#*");
        deqpTest.addAllIncludeFilters(includes);

        Set<String> excludes = new HashSet<>();
        excludes.add("*foo");
        excludes.add("*thoushallnotpass");
        deqpTest.addAllExcludeFilters(excludes);
        testFiltering(deqpTest, expectedTrie, activeTests);
    }

    public void testRun_includeAll() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.group1", "mememe"),
                new TestIdentifier("dEQP-GLES3.group1", "yeah"),
                new TestIdentifier("dEQP-GLES3.group1", "takeitall"),
                new TestIdentifier("dEQP-GLES3.group2", "jeba"),
                new TestIdentifier("dEQP-GLES3.group2", "yes"),
                new TestIdentifier("dEQP-GLES3.group2", "granted"),
        };

        List<TestIdentifier> allTests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) {
            allTests.add(id);
        }

        String expectedTrie = "{dEQP-GLES3{group1{mememe,yeah,takeitall},group2{jeba,yes,granted}}}";

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, allTests, mTestsDir);
        deqpTest.addIncludeFilter("*");
        testFiltering(deqpTest, expectedTrie, allTests);
    }

    public void testRun_excludeAll() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.group1", "no"),
                new TestIdentifier("dEQP-GLES3.group1", "nope"),
                new TestIdentifier("dEQP-GLES3.group1", "nottoday"),
                new TestIdentifier("dEQP-GLES3.group2", "banned"),
                new TestIdentifier("dEQP-GLES3.group2", "notrecognized"),
                new TestIdentifier("dEQP-GLES3.group2", "-2"),
        };

        List<TestIdentifier> allTests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) {
            allTests.add(id);
        }

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, allTests, mTestsDir);
        deqpTest.addExcludeFilter("*");
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        mockListener.testRunStarted(getTestId(deqpTest), 0);
        EasyMock.expectLastCall().once();
        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        EasyMock.replay(mockListener);
        deqpTest.run(mockListener);
        EasyMock.verify(mockListener);
    }

    /**
     * Test running a unexecutable test.
     */
    public void testRun_unexecutableTests() throws Exception {
        final String instrumentationAnswerNoExecs =
                "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=2014.x\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseId\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=0xcafebabe\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=targetName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=android\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_CODE: 0\r\n";

        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.missing", "no"),
                new TestIdentifier("dEQP-GLES3.missing", "nope"),
                new TestIdentifier("dEQP-GLES3.missing", "donotwant"),
        };

        final String[] testPaths = {
                "dEQP-GLES3.missing.no",
                "dEQP-GLES3.missing.nope",
                "dEQP-GLES3.missing.donotwant",
        };

        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        IDevice mockIDevice = EasyMock.createMock(IDevice.class);

        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();

        for (TestIdentifier id : testIds) {
            tests.add(id);
        }

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);

        int version = 3 << 16;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                .once();
        EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true), EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName()))))
                .andReturn(null).once();

        expectRenderConfigQuery(mockDevice, 3, 0);

        String commandLine = String.format(
                "--deqp-caselist-file=%s --deqp-gl-config-name=rgba8888d24s8 "
                + "--deqp-screen-rotation=unspecified "
                + "--deqp-surface-type=window "
                + "--deqp-log-images=disable "
                + "--deqp-watchdog=enable",
                CASE_LIST_FILE_NAME);

        // first try
        runInstrumentationLineAndAnswer(mockDevice, mockIDevice,
                "{dEQP-GLES3{missing{no,nope,donotwant}}}", commandLine, instrumentationAnswerNoExecs);

        // splitting begins
        runInstrumentationLineAndAnswer(mockDevice, mockIDevice,
                "{dEQP-GLES3{missing{no}}}", commandLine, instrumentationAnswerNoExecs);
        runInstrumentationLineAndAnswer(mockDevice, mockIDevice,
                "{dEQP-GLES3{missing{nope,donotwant}}}", commandLine, instrumentationAnswerNoExecs);
        runInstrumentationLineAndAnswer(mockDevice, mockIDevice,
                "{dEQP-GLES3{missing{nope}}}", commandLine, instrumentationAnswerNoExecs);
        runInstrumentationLineAndAnswer(mockDevice, mockIDevice,
                "{dEQP-GLES3{missing{donotwant}}}", commandLine, instrumentationAnswerNoExecs);

        mockListener.testRunStarted(getTestId(deqpTest), testPaths.length);
        EasyMock.expectLastCall().once();

        for (int i = 0; i < testPaths.length; i++) {
            mockListener.testStarted(EasyMock.eq(testIds[i]));
            EasyMock.expectLastCall().once();

            mockListener.testFailed(EasyMock.eq(testIds[i]),
                    EasyMock.eq("=== with config {glformat=rgba8888d24s8,rotation=unspecified,surfacetype=window,required=false} ===\n"
                    + "Abort: Test cannot be executed"));
            EasyMock.expectLastCall().once();

            mockListener.testEnded(EasyMock.eq(testIds[i]),
                    EasyMock.<Map<String, String>>notNull());
            EasyMock.expectLastCall().once();
        }

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                .once();

        EasyMock.replay(mockDevice, mockIDevice);
        EasyMock.replay(mockListener);

        deqpTest.setDevice(mockDevice);
        deqpTest.run(mockListener);

        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice, mockIDevice);
    }

    /**
     * Test that test are left unexecuted if pm list query fails
     */
    public void testRun_queryPmListFailure()
            throws Exception {
        final TestIdentifier testId = new TestIdentifier("dEQP-GLES3.orientation", "test");

        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();
        tests.add(testId);

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);
        OptionSetter setter = new OptionSetter(deqpTest);
        // Note: If the rotation is the default unspecified, features are not queried at all
        setter.setOptionValue("deqp-screen-rotation", "90");

        deqpTest.setDevice(mockDevice);

        int version = 3 << 16;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        EasyMock.expect(mockDevice.executeShellCommand("pm list features"))
                .andReturn("not a valid format");

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).
            andReturn("").once();

        EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true),
                EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName())))).andReturn(null)
                .once();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG)))
                .andReturn("").once();

        mockListener.testRunStarted(getTestId(deqpTest), 1);
        EasyMock.expectLastCall().once();

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        EasyMock.replay(mockDevice);
        EasyMock.replay(mockListener);
        deqpTest.run(mockListener);
        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice);
    }

    /**
     * Test that test are left unexecuted if renderablity query fails
     */
    public void testRun_queryRenderabilityFailure()
            throws Exception {
        final TestIdentifier testId = new TestIdentifier("dEQP-GLES3.orientation", "test");

        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);

        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();
        tests.add(testId);

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);

        deqpTest.setDevice(mockDevice);

        int version = 3 << 16;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).
            andReturn("").once();

        EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true),
                EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName())))).andReturn(null)
                .once();

        expectRenderConfigQueryAndReturn(mockDevice,
                "--deqp-gl-config-name=rgba8888d24s8 "
                + "--deqp-screen-rotation=unspecified "
                + "--deqp-surface-type=window "
                + "--deqp-gl-major-version=3 "
                + "--deqp-gl-minor-version=0", "Maybe?");

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG)))
                .andReturn("").once();

        mockListener.testRunStarted(getTestId(deqpTest), 1);
        EasyMock.expectLastCall().once();

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        EasyMock.replay(mockDevice);
        EasyMock.replay(mockListener);
        deqpTest.run(mockListener);
        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice);
    }

    /**
     * Test that orientation is supplied to runner correctly
     */
    private void testOrientation(final String rotation, final String featureString)
            throws Exception {
        final TestIdentifier testId = new TestIdentifier("dEQP-GLES3.orientation", "test");
        final String testPath = "dEQP-GLES3.orientation.test";
        final String testTrie = "{dEQP-GLES3{orientation{test}}}";
        final String output = "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=2014.x\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseId\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=0xcafebabe\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=targetName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=android\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=" + testPath + "\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_CODE: 0\r\n";

        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        IDevice mockIDevice = EasyMock.createMock(IDevice.class);

        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();
        tests.add(testId);

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);
        OptionSetter setter = new OptionSetter(deqpTest);
        setter.setOptionValue("deqp-screen-rotation", rotation);

        deqpTest.setDevice(mockDevice);

        int version = 3 << 16;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        if (!rotation.equals(BatchRunConfiguration.ROTATION_UNSPECIFIED)) {
            EasyMock.expect(mockDevice.executeShellCommand("pm list features"))
                    .andReturn(featureString);
        }

        final boolean isPortraitOrientation =
                rotation.equals(BatchRunConfiguration.ROTATION_PORTRAIT) ||
                rotation.equals(BatchRunConfiguration.ROTATION_REVERSE_PORTRAIT);
        final boolean isLandscapeOrientation =
                rotation.equals(BatchRunConfiguration.ROTATION_LANDSCAPE) ||
                rotation.equals(BatchRunConfiguration.ROTATION_REVERSE_LANDSCAPE);
        final boolean executable =
                rotation.equals(BatchRunConfiguration.ROTATION_UNSPECIFIED) ||
                (isPortraitOrientation &&
                featureString.contains(DeqpTestRunner.FEATURE_PORTRAIT)) ||
                (isLandscapeOrientation &&
                featureString.contains(DeqpTestRunner.FEATURE_LANDSCAPE));

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).
            andReturn("").once();

        EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true),
                EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName())))).andReturn(null)
                .once();

        if (executable) {
            expectRenderConfigQuery(mockDevice, String.format(
                    "--deqp-gl-config-name=rgba8888d24s8 --deqp-screen-rotation=%s "
                    + "--deqp-surface-type=window --deqp-gl-major-version=3 "
                    + "--deqp-gl-minor-version=0", rotation));

            String commandLine = String.format(
                    "--deqp-caselist-file=%s --deqp-gl-config-name=rgba8888d24s8 "
                    + "--deqp-screen-rotation=%s "
                    + "--deqp-surface-type=window "
                    + "--deqp-log-images=disable "
                    + "--deqp-watchdog=enable",
                    CASE_LIST_FILE_NAME, rotation);

            runInstrumentationLineAndAnswer(mockDevice, mockIDevice, testTrie, commandLine,
                    output);
        }

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG)))
                .andReturn("").once();

        mockListener.testRunStarted(getTestId(deqpTest), 1);
        EasyMock.expectLastCall().once();

        mockListener.testStarted(EasyMock.eq(testId));
        EasyMock.expectLastCall().once();

        mockListener.testEnded(EasyMock.eq(testId), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        EasyMock.replay(mockDevice, mockIDevice);
        EasyMock.replay(mockListener);
        deqpTest.run(mockListener);
        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice, mockIDevice);
    }

    /**
     * Test OpeGL ES3 tests on device with OpenGL ES2.
     */
    public void testRun_require30DeviceVersion20() throws Exception {
        testGlesVersion(3, 0, 2, 0);
    }

    /**
     * Test OpeGL ES3.1 tests on device with OpenGL ES2.
     */
    public void testRun_require31DeviceVersion20() throws Exception {
        testGlesVersion(3, 1, 2, 0);
    }

    /**
     * Test OpeGL ES3 tests on device with OpenGL ES3.
     */
    public void testRun_require30DeviceVersion30() throws Exception {
        testGlesVersion(3, 0, 3, 0);
    }

    /**
     * Test OpeGL ES3.1 tests on device with OpenGL ES3.
     */
    public void testRun_require31DeviceVersion30() throws Exception {
        testGlesVersion(3, 1, 3, 0);
    }

    /**
     * Test OpeGL ES3 tests on device with OpenGL ES3.1.
     */
    public void testRun_require30DeviceVersion31() throws Exception {
        testGlesVersion(3, 0, 3, 1);
    }

    /**
     * Test OpeGL ES3.1 tests on device with OpenGL ES3.1.
     */
    public void testRun_require31DeviceVersion31() throws Exception {
        testGlesVersion(3, 1, 3, 1);
    }

    /**
     * Test dEQP Pass result code.
     */
    public void testRun_resultPass() throws Exception {
        testResultCode("Pass", true);
    }

    /**
     * Test dEQP Fail result code.
     */
    public void testRun_resultFail() throws Exception {
        testResultCode("Fail", false);
    }

    /**
     * Test dEQP NotSupported result code.
     */
    public void testRun_resultNotSupported() throws Exception {
        testResultCode("NotSupported", true);
    }

    /**
     * Test dEQP QualityWarning result code.
     */
    public void testRun_resultQualityWarning() throws Exception {
        testResultCode("QualityWarning", true);
    }

    /**
     * Test dEQP CompatibilityWarning result code.
     */
    public void testRun_resultCompatibilityWarning() throws Exception {
        testResultCode("CompatibilityWarning", true);
    }

    /**
     * Test dEQP ResourceError result code.
     */
    public void testRun_resultResourceError() throws Exception {
        testResultCode("ResourceError", false);
    }

    /**
     * Test dEQP InternalError result code.
     */
    public void testRun_resultInternalError() throws Exception {
        testResultCode("InternalError", false);
    }

    /**
     * Test dEQP Crash result code.
     */
    public void testRun_resultCrash() throws Exception {
        testResultCode("Crash", false);
    }

    /**
     * Test dEQP Timeout result code.
     */
    public void testRun_resultTimeout() throws Exception {
        testResultCode("Timeout", false);
    }
    /**
     * Test dEQP Orientation
     */
    public void testRun_orientationLandscape() throws Exception {
        testOrientation("90", ALL_FEATURES);
    }

    /**
     * Test dEQP Orientation
     */
    public void testRun_orientationPortrait() throws Exception {
        testOrientation("0", ALL_FEATURES);
    }

    /**
     * Test dEQP Orientation
     */
    public void testRun_orientationReverseLandscape() throws Exception {
        testOrientation("270", ALL_FEATURES);
    }

    /**
     * Test dEQP Orientation
     */
    public void testRun_orientationReversePortrait() throws Exception {
        testOrientation("180", ALL_FEATURES);
    }

    /**
     * Test dEQP Orientation
     */
    public void testRun_orientationUnspecified() throws Exception {
        testOrientation("unspecified", ALL_FEATURES);
    }

    /**
     * Test dEQP Orientation with limited features
     */
    public void testRun_orientationUnspecifiedLimitedFeatures() throws Exception {
        testOrientation("unspecified", ONLY_LANDSCAPE_FEATURES);
    }

    /**
     * Test dEQP Orientation with limited features
     */
    public void testRun_orientationLandscapeLimitedFeatures() throws Exception {
        testOrientation("90", ONLY_LANDSCAPE_FEATURES);
    }

    /**
     * Test dEQP Orientation with limited features
     */
    public void testRun_orientationPortraitLimitedFeatures() throws Exception {
        testOrientation("0", ONLY_LANDSCAPE_FEATURES);
    }

    /**
     * Test dEQP unsupported pixel format
     */
    public void testRun_unsupportedPixelFormat() throws Exception {
        final String pixelFormat = "rgba5658d16m4";
        final TestIdentifier testId = new TestIdentifier("dEQP-GLES3.pixelformat", "test");

        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);

        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();
        tests.add(testId);

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);
        OptionSetter setter = new OptionSetter(deqpTest);
        setter.setOptionValue("deqp-gl-config-name", pixelFormat);

        deqpTest.setDevice(mockDevice);

        int version = 3 << 16;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).
            andReturn("").once();

        EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true),
                EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName())))).andReturn(null)
                .once();

        expectRenderConfigQueryAndReturn(mockDevice, String.format(
                "--deqp-gl-config-name=%s --deqp-screen-rotation=unspecified "
                + "--deqp-surface-type=window "
                + "--deqp-gl-major-version=3 "
                + "--deqp-gl-minor-version=0", pixelFormat), "No");

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG)))
                .andReturn("").once();

        mockListener.testRunStarted(getTestId(deqpTest), 1);
        EasyMock.expectLastCall().once();

        mockListener.testStarted(EasyMock.eq(testId));
        EasyMock.expectLastCall().once();

        mockListener.testEnded(EasyMock.eq(testId), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        EasyMock.replay(mockDevice);
        EasyMock.replay(mockListener);
        deqpTest.run(mockListener);
        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice);
    }

    public static interface RecoverableTestDevice extends ITestDevice {
        public void recoverDevice() throws DeviceNotAvailableException;
    }

    private static enum RecoveryEvent {
        PROGRESS,
        FAIL_CONNECTION_REFUSED,
        FAIL_LINK_KILLED,
    }

    private void runRecoveryWithPattern(DeqpTestRunner.Recovery recovery, RecoveryEvent[] events)
            throws DeviceNotAvailableException {
        for (RecoveryEvent event : events) {
            switch (event) {
                case PROGRESS:
                    recovery.onExecutionProgressed();
                    break;
                case FAIL_CONNECTION_REFUSED:
                    recovery.recoverConnectionRefused();
                    break;
                case FAIL_LINK_KILLED:
                    recovery.recoverComLinkKilled();
                    break;
            }
        }
    }

    private void setRecoveryExpectationWait(DeqpTestRunner.ISleepProvider mockSleepProvider) {
        mockSleepProvider.sleep(EasyMock.gt(0));
        EasyMock.expectLastCall().once();
    }

    private void setRecoveryExpectationKillProcess(RecoverableTestDevice mockDevice,
            DeqpTestRunner.ISleepProvider mockSleepProvider) throws DeviceNotAvailableException {
        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.contains("ps"))).
                andReturn("root 1234 com.drawelement.deqp").once();

        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.eq("kill -9 1234"))).
                andReturn("").once();

        // Recovery checks if kill failed
        mockSleepProvider.sleep(EasyMock.gt(0));
        EasyMock.expectLastCall().once();
        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.contains("ps"))).
                andReturn("").once();
    }

    private void setRecoveryExpectationRecovery(RecoverableTestDevice mockDevice)
            throws DeviceNotAvailableException {
        mockDevice.recoverDevice();
        EasyMock.expectLastCall().once();
    }

    private void setRecoveryExpectationReboot(RecoverableTestDevice mockDevice)
            throws DeviceNotAvailableException {
        mockDevice.reboot();
        EasyMock.expectLastCall().once();
    }

    private int setRecoveryExpectationOfAConnFailure(RecoverableTestDevice mockDevice,
            DeqpTestRunner.ISleepProvider mockSleepProvider, int numConsecutiveErrors)
            throws DeviceNotAvailableException {
        switch (numConsecutiveErrors) {
            case 0:
            case 1:
                setRecoveryExpectationRecovery(mockDevice);
                return 2;
            case 2:
                setRecoveryExpectationReboot(mockDevice);
                return 3;
            default:
                return 4;
        }
    }

    private int setRecoveryExpectationOfAComKilled(RecoverableTestDevice mockDevice,
            DeqpTestRunner.ISleepProvider mockSleepProvider, int numConsecutiveErrors)
            throws DeviceNotAvailableException {
        switch (numConsecutiveErrors) {
            case 0:
                setRecoveryExpectationWait(mockSleepProvider);
                setRecoveryExpectationKillProcess(mockDevice, mockSleepProvider);
                return 1;
            case 1:
                setRecoveryExpectationRecovery(mockDevice);
                setRecoveryExpectationKillProcess(mockDevice, mockSleepProvider);
                return 2;
            case 2:
                setRecoveryExpectationReboot(mockDevice);
                return 3;
            default:
                return 4;
        }
    }

    private void setRecoveryExpectationsOfAPattern(RecoverableTestDevice mockDevice,
            DeqpTestRunner.ISleepProvider mockSleepProvider, RecoveryEvent[] events)
            throws DeviceNotAvailableException {
        int numConsecutiveErrors = 0;
        for (RecoveryEvent event : events) {
            switch (event) {
                case PROGRESS:
                    numConsecutiveErrors = 0;
                    break;
                case FAIL_CONNECTION_REFUSED:
                    numConsecutiveErrors = setRecoveryExpectationOfAConnFailure(mockDevice,
                            mockSleepProvider, numConsecutiveErrors);
                    break;
                case FAIL_LINK_KILLED:
                    numConsecutiveErrors = setRecoveryExpectationOfAComKilled(mockDevice,
                            mockSleepProvider, numConsecutiveErrors);
                    break;
            }
        }
    }

    /**
     * Test dEQP runner recovery state machine.
     */
    private void testRecoveryWithPattern(boolean expectSuccess, RecoveryEvent...pattern)
            throws Exception {
        DeqpTestRunner.Recovery recovery = new DeqpTestRunner.Recovery();
        IMocksControl orderedControl = EasyMock.createStrictControl();
        RecoverableTestDevice mockDevice = orderedControl.createMock(RecoverableTestDevice.class);
        EasyMock.expect(mockDevice.getSerialNumber()).andStubReturn("SERIAL");
        DeqpTestRunner.ISleepProvider mockSleepProvider =
                orderedControl.createMock(DeqpTestRunner.ISleepProvider.class);

        setRecoveryExpectationsOfAPattern(mockDevice, mockSleepProvider, pattern);

        orderedControl.replay();

        recovery.setDevice(mockDevice);
        recovery.setSleepProvider(mockSleepProvider);
        try {
            runRecoveryWithPattern(recovery, pattern);
            if (!expectSuccess) {
                fail("Expected DeviceNotAvailableException");
            }
        } catch (DeviceNotAvailableException ex) {
            if (expectSuccess) {
                fail("Did not expect DeviceNotAvailableException");
            }
        }

        orderedControl.verify();
    }

    // basic patterns

    public void testRecovery_NoEvents() throws Exception {
        testRecoveryWithPattern(true);
    }

    public void testRecovery_AllOk() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.PROGRESS, RecoveryEvent.PROGRESS);
    }

    // conn fail patterns

    public void testRecovery_OneConnectionFailureBegin() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.FAIL_CONNECTION_REFUSED,
                RecoveryEvent.PROGRESS);
    }

    public void testRecovery_TwoConnectionFailuresBegin() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.FAIL_CONNECTION_REFUSED,
                RecoveryEvent.FAIL_CONNECTION_REFUSED, RecoveryEvent.PROGRESS);
    }

    public void testRecovery_ThreeConnectionFailuresBegin() throws Exception {
        testRecoveryWithPattern(false, RecoveryEvent.FAIL_CONNECTION_REFUSED,
                RecoveryEvent.FAIL_CONNECTION_REFUSED, RecoveryEvent.FAIL_CONNECTION_REFUSED);
    }

    public void testRecovery_OneConnectionFailureMid() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.PROGRESS,
                RecoveryEvent.FAIL_CONNECTION_REFUSED, RecoveryEvent.PROGRESS);
    }

    public void testRecovery_TwoConnectionFailuresMid() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.PROGRESS,
                RecoveryEvent.FAIL_CONNECTION_REFUSED, RecoveryEvent.FAIL_CONNECTION_REFUSED,
                RecoveryEvent.PROGRESS);
    }

    public void testRecovery_ThreeConnectionFailuresMid() throws Exception {
        testRecoveryWithPattern(false, RecoveryEvent.PROGRESS,
                RecoveryEvent.FAIL_CONNECTION_REFUSED, RecoveryEvent.FAIL_CONNECTION_REFUSED,
                RecoveryEvent.FAIL_CONNECTION_REFUSED);
    }

    // link fail patterns

    public void testRecovery_OneLinkFailureBegin() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.PROGRESS);
    }

    public void testRecovery_TwoLinkFailuresBegin() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.FAIL_LINK_KILLED, RecoveryEvent.PROGRESS);
    }

    public void testRecovery_ThreeLinkFailuresBegin() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.FAIL_LINK_KILLED, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.PROGRESS);
    }

    public void testRecovery_FourLinkFailuresBegin() throws Exception {
        testRecoveryWithPattern(false, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.FAIL_LINK_KILLED, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.FAIL_LINK_KILLED);
    }

    public void testRecovery_OneLinkFailureMid() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.PROGRESS,
                RecoveryEvent.FAIL_LINK_KILLED, RecoveryEvent.PROGRESS);
    }

    public void testRecovery_TwoLinkFailuresMid() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.PROGRESS,
                RecoveryEvent.FAIL_LINK_KILLED, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.PROGRESS);
    }

    public void testRecovery_ThreeLinkFailuresMid() throws Exception {
        testRecoveryWithPattern(true, RecoveryEvent.PROGRESS,
                RecoveryEvent.FAIL_LINK_KILLED, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.FAIL_LINK_KILLED, RecoveryEvent.PROGRESS);
    }

    public void testRecovery_FourLinkFailuresMid() throws Exception {
        testRecoveryWithPattern(false, RecoveryEvent.PROGRESS, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.FAIL_LINK_KILLED, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.FAIL_LINK_KILLED);
    }

    // mixed patterns

    public void testRecovery_MixedFailuresProgressBetween() throws Exception {
        testRecoveryWithPattern(true,
                RecoveryEvent.PROGRESS, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.PROGRESS, RecoveryEvent.FAIL_CONNECTION_REFUSED,
                RecoveryEvent.PROGRESS, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.PROGRESS, RecoveryEvent.FAIL_CONNECTION_REFUSED,
                RecoveryEvent.PROGRESS);
    }

    public void testRecovery_MixedFailuresNoProgressBetween() throws Exception {
        testRecoveryWithPattern(true,
                RecoveryEvent.PROGRESS, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.FAIL_CONNECTION_REFUSED, RecoveryEvent.FAIL_LINK_KILLED,
                RecoveryEvent.PROGRESS);
    }

    /**
     * Test recovery if process cannot be killed
     */
    public void testRecovery_unkillableProcess () throws Exception {
        DeqpTestRunner.Recovery recovery = new DeqpTestRunner.Recovery();
        IMocksControl orderedControl = EasyMock.createStrictControl();
        RecoverableTestDevice mockDevice = orderedControl.createMock(RecoverableTestDevice.class);
        DeqpTestRunner.ISleepProvider mockSleepProvider =
                orderedControl.createMock(DeqpTestRunner.ISleepProvider.class);

        // recovery attempts to kill the process after a timeout
        mockSleepProvider.sleep(EasyMock.gt(0));
        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.contains("ps"))).
                andReturn("root 1234 com.drawelement.deqp").once();
        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.eq("kill -9 1234"))).
                andReturn("").once();

        // Recovery checks if kill failed
        mockSleepProvider.sleep(EasyMock.gt(0));
        EasyMock.expectLastCall().once();
        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.contains("ps"))).
                andReturn("root 1234 com.drawelement.deqp").once();

        // Recovery resets the connection
        mockDevice.recoverDevice();
        EasyMock.expectLastCall().once();

        // and attempts to kill the process again
        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.contains("ps"))).
                andReturn("root 1234 com.drawelement.deqp").once();
        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.eq("kill -9 1234"))).
                andReturn("").once();

        // Recovery checks if kill failed
        mockSleepProvider.sleep(EasyMock.gt(0));
        EasyMock.expectLastCall().once();
        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.contains("ps"))).
                andReturn("root 1234 com.drawelement.deqp").once();

        // recovery reboots the device
        mockDevice.reboot();
        EasyMock.expectLastCall().once();

        orderedControl.replay();
        recovery.setDevice(mockDevice);
        recovery.setSleepProvider(mockSleepProvider);
        recovery.recoverComLinkKilled();
        orderedControl.verify();
    }

    /**
     * Test external interruption before batch run.
     */
    public void testInterrupt_killBeforeBatch() throws Exception {
        final TestIdentifier testId = new TestIdentifier("dEQP-GLES3.interrupt", "test");

        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();
        tests.add(testId);

        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        IDevice mockIDevice = EasyMock.createMock(IDevice.class);
        IRunUtil mockRunUtil = EasyMock.createMock(IRunUtil.class);

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);

        deqpTest.setDevice(mockDevice);
        deqpTest.setRunUtil(mockRunUtil);

        int version = 3 << 16;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).
            andReturn("").once();

        EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true),
                EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName())))).andReturn(null)
                .once();

        expectRenderConfigQuery(mockDevice,
                "--deqp-gl-config-name=rgba8888d24s8 --deqp-screen-rotation=unspecified "
                + "--deqp-surface-type=window --deqp-gl-major-version=3 "
                + "--deqp-gl-minor-version=0");

        mockRunUtil.sleep(0);
        EasyMock.expectLastCall().andThrow(new RunInterruptedException());

        mockListener.testRunStarted(getTestId(deqpTest), 1);
        EasyMock.expectLastCall().once();
        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.anyObject());
        EasyMock.expectLastCall().once();

        EasyMock.replay(mockDevice, mockIDevice);
        EasyMock.replay(mockListener);
        EasyMock.replay(mockRunUtil);
        try {
            deqpTest.run(mockListener);
            fail("expected RunInterruptedException");
        } catch (RunInterruptedException ex) {
            // expected
        }
        EasyMock.verify(mockRunUtil);
        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice, mockIDevice);
    }

    private void runShardedTest(TestIdentifier[] testIds,
            ArrayList<ArrayList<TestIdentifier>> testsForShard) throws Exception {
        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) tests.add(id);

        DeqpTestRunner runner = buildGlesTestRunner(3, 0, tests, mTestsDir);
        ArrayList<IRemoteTest> shards = (ArrayList<IRemoteTest>)runner.split();

        for (int shardIndex = 0; shardIndex < shards.size(); shardIndex++) {
            DeqpTestRunner shard = (DeqpTestRunner)shards.get(shardIndex);
            shard.setBuildHelper(getMockBuildHelper(mTestsDir));

            ArrayList<TestIdentifier> shardTests = testsForShard.get(shardIndex);

            ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
            ITestInvocationListener mockListener
                    = EasyMock.createStrictMock(ITestInvocationListener.class);
            IDevice mockIDevice = EasyMock.createMock(IDevice.class);
            int version = 3 << 16;
            EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                    .andReturn(Integer.toString(version)).atLeastOnce();

            EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                    .once();
            EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                    EasyMock.eq(true), EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName()))))
                    .andReturn(null).once();

            mockListener.testRunStarted(getTestId(shard), shardTests.size());
            EasyMock.expectLastCall().once();

            expectRenderConfigQuery(mockDevice, 3, 0);

            String testOut = buildTestProcessOutput(shardTests);
            // NOTE: This assumes that there won't be multiple batches per shard!
            runInstrumentationLineAndAnswer(mockDevice, mockIDevice, testOut);

            for (int i = 0; i < shardTests.size(); i++) {
                mockListener.testStarted(EasyMock.eq(shardTests.get(i)));
                EasyMock.expectLastCall().once();

                mockListener.testEnded(EasyMock.eq(shardTests.get(i)),
                                       EasyMock.<Map<String, String>>notNull());

                EasyMock.expectLastCall().once();
            }

            mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
            EasyMock.expectLastCall().once();

            EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                    .once();

            EasyMock.replay(mockDevice, mockIDevice);
            EasyMock.replay(mockListener);

            shard.setDevice(mockDevice);
            shard.run(mockListener);

            EasyMock.verify(mockListener);
            EasyMock.verify(mockDevice, mockIDevice);
        }
    }

    public void testSharding_smallTrivial() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.info", "vendor"),
                new TestIdentifier("dEQP-GLES3.info", "renderer"),
                new TestIdentifier("dEQP-GLES3.info", "version"),
                new TestIdentifier("dEQP-GLES3.info", "shading_language_version"),
                new TestIdentifier("dEQP-GLES3.info", "extensions"),
                new TestIdentifier("dEQP-GLES3.info", "render_target")
        };
        ArrayList<ArrayList<TestIdentifier>> shardedTests = new ArrayList<>();
        ArrayList<TestIdentifier> shardOne = new ArrayList<>();
        for (int i = 0; i < testIds.length; i++) {
            shardOne.add(testIds[i]);
        }
        shardedTests.add(shardOne);
        runShardedTest(testIds, shardedTests);
    }

    public void testSharding_twoShards() throws Exception {
        final int TEST_COUNT = 1237;
        final int SHARD_SIZE = 1000;

        ArrayList<TestIdentifier> testIds = new ArrayList<>(TEST_COUNT);
        for (int i = 0; i < TEST_COUNT; i++) {
            testIds.add(new TestIdentifier("dEQP-GLES3.funny.group", String.valueOf(i)));
        }

        ArrayList<ArrayList<TestIdentifier>> shardedTests = new ArrayList<>();
        ArrayList<TestIdentifier> shard = new ArrayList<>();
        for (int i = 0; i < testIds.size(); i++) {
            if (i == SHARD_SIZE) {
                shardedTests.add(shard);
                shard = new ArrayList<>();
            }
            shard.add(testIds.get(i));
        }
        shardedTests.add(shard);
        runShardedTest(testIds.toArray(new TestIdentifier[testIds.size()]), shardedTests);
    }

    public void testSharding_empty() throws Exception {
        DeqpTestRunner runner = buildGlesTestRunner(3, 0, new ArrayList<TestIdentifier>(), mTestsDir);
        ArrayList<IRemoteTest> shards = (ArrayList<IRemoteTest>)runner.split();
        // Returns null when cannot be sharded.
        assertNull(shards);
    }

    /**
     * Test external interruption in testFailed().
     */
    public void testInterrupt_killReportTestFailed() throws Exception {
        final TestIdentifier testId = new TestIdentifier("dEQP-GLES3.interrupt", "test");
        final String testPath = "dEQP-GLES3.interrupt.test";
        final String testTrie = "{dEQP-GLES3{interrupt{test}}}";
        final String output = "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=2014.x\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseId\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=0xcafebabe\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=targetName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=android\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=" + testPath + "\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Fail\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Fail\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_CODE: 0\r\n";

        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();
        tests.add(testId);

        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        IDevice mockIDevice = EasyMock.createMock(IDevice.class);
        IRunUtil mockRunUtil = EasyMock.createMock(IRunUtil.class);

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);

        deqpTest.setDevice(mockDevice);
        deqpTest.setRunUtil(mockRunUtil);

        int version = 3 << 16;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).
            andReturn("").once();

        EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true),
                EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName())))).andReturn(null)
                .once();

        expectRenderConfigQuery(mockDevice,
                "--deqp-gl-config-name=rgba8888d24s8 --deqp-screen-rotation=unspecified "
                + "--deqp-surface-type=window --deqp-gl-major-version=3 "
                + "--deqp-gl-minor-version=0");

        mockRunUtil.sleep(0);
        EasyMock.expectLastCall().once();

        String commandLine = String.format(
                "--deqp-caselist-file=%s --deqp-gl-config-name=rgba8888d24s8 "
                + "--deqp-screen-rotation=unspecified "
                + "--deqp-surface-type=window "
                + "--deqp-log-images=disable "
                + "--deqp-watchdog=enable",
                CASE_LIST_FILE_NAME);

        runInstrumentationLineAndAnswer(mockDevice, mockIDevice, testTrie, commandLine,
                output);

        mockListener.testRunStarted(getTestId(deqpTest), 1);
        EasyMock.expectLastCall().once();

        mockListener.testStarted(EasyMock.eq(testId));
        EasyMock.expectLastCall().once();

        mockListener.testFailed(EasyMock.eq(testId), EasyMock.<String>notNull());
        EasyMock.expectLastCall().andThrow(new RunInterruptedException());

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.anyObject());
        EasyMock.expectLastCall().once();
        EasyMock.replay(mockDevice, mockIDevice);
        EasyMock.replay(mockListener);
        EasyMock.replay(mockRunUtil);
        try {
            deqpTest.run(mockListener);
            fail("expected RunInterruptedException");
        } catch (RunInterruptedException ex) {
            // expected
        }
        EasyMock.verify(mockRunUtil);
        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice, mockIDevice);
    }

    public void testRuntimeHint_optionSet() throws Exception {
        /* MultiLineReceiver expects "\r\n" line ending. */
        final String output = "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=2014.x\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=releaseId\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=0xcafebabe\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Name=targetName\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=SessionInfo\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-SessionInfo-Value=android\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.vendor\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.renderer\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.version\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.shading_language_version\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.extensions\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=BeginTestCase\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-BeginTestCase-TestCasePath=dEQP-GLES3.info.render_target\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Code=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-TestCaseResult-Details=Pass\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=TestCaseResult\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndTestCase\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_STATUS: dEQP-EventType=EndSession\r\n"
                + "INSTRUMENTATION_STATUS_CODE: 0\r\n"
                + "INSTRUMENTATION_CODE: 0\r\n";

        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.info", "vendor"),
                new TestIdentifier("dEQP-GLES3.info", "renderer"),
                new TestIdentifier("dEQP-GLES3.info", "version"),
                new TestIdentifier("dEQP-GLES3.info", "shading_language_version"),
                new TestIdentifier("dEQP-GLES3.info", "extensions"),
                new TestIdentifier("dEQP-GLES3.info", "render_target")
        };

        final String[] testPaths = {
                "dEQP-GLES3.info.vendor",
                "dEQP-GLES3.info.renderer",
                "dEQP-GLES3.info.version",
                "dEQP-GLES3.info.shading_language_version",
                "dEQP-GLES3.info.extensions",
                "dEQP-GLES3.info.render_target"
        };

        final String testTrie
                = "{dEQP-GLES3{info{vendor,renderer,version,shading_language_version,extensions,render_target}}}";

        ITestDevice mockDevice = EasyMock.createMock(ITestDevice.class);
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        IDevice mockIDevice = EasyMock.createMock(IDevice.class);

        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();

        for (TestIdentifier id : testIds) {
            tests.add(id);
        }

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);
        OptionSetter setter = new OptionSetter(deqpTest);
        final long runtimeMs = 123456;
        setter.setOptionValue("runtime-hint", String.valueOf(runtimeMs));
        assertEquals("Wrong expected runtime - option not passed cleanly", runtimeMs, deqpTest.getRuntimeHint());

        // Try running the tests as well. The unit tests do not set the hint be default,
        // so that case is covered.

        int version = 3 << 16;
        EasyMock.expect(mockDevice.getProperty("ro.opengles.version"))
                .andReturn(Integer.toString(version)).atLeastOnce();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                .once();
        EasyMock.expect(mockDevice.installPackage(EasyMock.<File>anyObject(),
                EasyMock.eq(true), EasyMock.eq(AbiUtils.createAbiFlag(ABI.getName()))))
                .andReturn(null).once();

        expectRenderConfigQuery(mockDevice, 3, 0);

        String commandLine = String.format(
                "--deqp-caselist-file=%s --deqp-gl-config-name=rgba8888d24s8 "
                + "--deqp-screen-rotation=unspecified "
                + "--deqp-surface-type=window "
                + "--deqp-log-images=disable "
                + "--deqp-watchdog=enable",
                CASE_LIST_FILE_NAME);

        runInstrumentationLineAndAnswer(mockDevice, mockIDevice, testTrie, commandLine, output);

        mockListener.testRunStarted(getTestId(deqpTest), testPaths.length);
        EasyMock.expectLastCall().once();

        for (int i = 0; i < testPaths.length; i++) {
            mockListener.testStarted(EasyMock.eq(testIds[i]));
            EasyMock.expectLastCall().once();

            mockListener.testEnded(EasyMock.eq(testIds[i]),
                    EasyMock.<Map<String, String>>notNull());

            EasyMock.expectLastCall().once();
        }

        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.<Map<String, String>>notNull());
        EasyMock.expectLastCall().once();

        EasyMock.expect(mockDevice.uninstallPackage(EasyMock.eq(DEQP_ONDEVICE_PKG))).andReturn("")
                .once();

        EasyMock.replay(mockDevice, mockIDevice);
        EasyMock.replay(mockListener);

        deqpTest.setDevice(mockDevice);
        deqpTest.run(mockListener);

        EasyMock.verify(mockListener);
        EasyMock.verify(mockDevice, mockIDevice);
    }

    public void testRuntimeHint_optionSetSharded() throws Exception {
        final int TEST_COUNT = 1237;
        final int SHARD_SIZE = 1000;

        ArrayList<TestIdentifier> testIds = new ArrayList<>(TEST_COUNT);
        for (int i = 0; i < TEST_COUNT; i++) {
            testIds.add(new TestIdentifier("dEQP-GLES3.funny.group", String.valueOf(i)));
        }

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, testIds, mTestsDir);
        OptionSetter setter = new OptionSetter(deqpTest);
        final long fullRuntimeMs = testIds.size()*100;
        setter.setOptionValue("runtime-hint", String.valueOf(fullRuntimeMs));

        ArrayList<IRemoteTest> shards = (ArrayList<IRemoteTest>)deqpTest.split();
        assertEquals("First shard's time not proportional to test count",
                 (fullRuntimeMs*SHARD_SIZE)/TEST_COUNT,
                 ((IRuntimeHintProvider)shards.get(0)).getRuntimeHint());
        assertEquals("Second shard's time not proportional to test count",
                 (fullRuntimeMs*(TEST_COUNT-SHARD_SIZE))/TEST_COUNT,
                 ((IRuntimeHintProvider)shards.get(1)).getRuntimeHint());
    }

    /**
     * Test that strict shardable is able to split deterministically the set of tests.
     */
    public void testGetTestShard() throws Exception {
        final int TEST_COUNT = 2237;
        final int SHARD_COUNT = 4;

        ArrayList<TestIdentifier> testIds = new ArrayList<>(TEST_COUNT);
        for (int i = 0; i < TEST_COUNT; i++) {
            testIds.add(new TestIdentifier("dEQP-GLES3.funny.group", String.valueOf(i)));
        }

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, testIds, mTestsDir);
        OptionSetter setter = new OptionSetter(deqpTest);
        final long fullRuntimeMs = testIds.size()*100;
        setter.setOptionValue("runtime-hint", String.valueOf(fullRuntimeMs));

        DeqpTestRunner shard1 = (DeqpTestRunner)deqpTest.getTestShard(SHARD_COUNT, 0);
        assertEquals(559, shard1.getTestInstance().size());
        int j = 0;
        // Ensure numbers, and that order is stable
        for (TestIdentifier t : shard1.getTestInstance().keySet()) {
            assertEquals(String.format("dEQP-GLES3.funny.group#%s", j),
                    String.format("%s#%s", t.getClassName(), t.getTestName()));
            j++;
        }
        DeqpTestRunner shard2 = (DeqpTestRunner)deqpTest.getTestShard(SHARD_COUNT, 1);
        assertEquals(559, shard2.getTestInstance().size());
        for (TestIdentifier t : shard2.getTestInstance().keySet()) {
            assertEquals(String.format("dEQP-GLES3.funny.group#%s", j),
                    String.format("%s#%s", t.getClassName(), t.getTestName()));
            j++;
        }
        DeqpTestRunner shard3 = (DeqpTestRunner)deqpTest.getTestShard(SHARD_COUNT, 2);
        assertEquals(559, shard3.getTestInstance().size());
        for (TestIdentifier t : shard3.getTestInstance().keySet()) {
            assertEquals(String.format("dEQP-GLES3.funny.group#%s", j),
                    String.format("%s#%s", t.getClassName(), t.getTestName()));
            j++;
        }
        DeqpTestRunner shard4 = (DeqpTestRunner)deqpTest.getTestShard(SHARD_COUNT, 3);
        assertEquals(560, shard4.getTestInstance().size());
        for (TestIdentifier t : shard4.getTestInstance().keySet()) {
            assertEquals(String.format("dEQP-GLES3.funny.group#%s", j),
                    String.format("%s#%s", t.getClassName(), t.getTestName()));
            j++;
        }
        assertEquals(TEST_COUNT, j);
    }

    /**
     * Test that strict shardable is creating an empty shard of the runner when too many shards
     * are requested.
     */
    public void testGetTestShard_tooManyShardRequested() throws Exception {
        final int TEST_COUNT = 2;
        final int SHARD_COUNT = 3;

        ArrayList<TestIdentifier> testIds = new ArrayList<>(TEST_COUNT);
        for (int i = 0; i < TEST_COUNT; i++) {
            testIds.add(new TestIdentifier("dEQP-GLES3.funny.group", String.valueOf(i)));
        }
        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, testIds, mTestsDir);
        OptionSetter setter = new OptionSetter(deqpTest);
        final long fullRuntimeMs = testIds.size()*100;
        setter.setOptionValue("runtime-hint", String.valueOf(fullRuntimeMs));
        DeqpTestRunner shard1 = (DeqpTestRunner)deqpTest.getTestShard(SHARD_COUNT, 0);
        assertEquals(1, shard1.getTestInstance().size());
        int j = 0;
        // Ensure numbers, and that order is stable
        for (TestIdentifier t : shard1.getTestInstance().keySet()) {
            assertEquals(String.format("dEQP-GLES3.funny.group#%s", j),
                    String.format("%s#%s", t.getClassName(), t.getTestName()));
            j++;
        }
        DeqpTestRunner shard2 = (DeqpTestRunner)deqpTest.getTestShard(SHARD_COUNT, 1);
        assertEquals(1, shard2.getTestInstance().size());
        for (TestIdentifier t : shard2.getTestInstance().keySet()) {
            assertEquals(String.format("dEQP-GLES3.funny.group#%s", j),
                    String.format("%s#%s", t.getClassName(), t.getTestName()));
            j++;
        }
        DeqpTestRunner shard3 = (DeqpTestRunner)deqpTest.getTestShard(SHARD_COUNT, 2);
        assertTrue(shard3.getTestInstance().isEmpty());
        assertEquals(TEST_COUNT, j);
        ITestInvocationListener mockListener
                = EasyMock.createStrictMock(ITestInvocationListener.class);
        mockListener.testRunStarted(EasyMock.anyObject(), EasyMock.eq(0));
        mockListener.testRunEnded(EasyMock.anyLong(), EasyMock.anyObject());
        EasyMock.replay(mockListener);
        shard3.run(mockListener);
        EasyMock.verify(mockListener);
    }

    public void testRuntimeHint_optionNotSet() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.info", "vendor"),
                new TestIdentifier("dEQP-GLES3.info", "renderer"),
                new TestIdentifier("dEQP-GLES3.info", "version"),
                new TestIdentifier("dEQP-GLES3.info", "shading_language_version"),
                new TestIdentifier("dEQP-GLES3.info", "extensions"),
                new TestIdentifier("dEQP-GLES3.info", "render_target")
        };
        Collection<TestIdentifier> tests = new ArrayList<TestIdentifier>();

        for (TestIdentifier id : testIds) {
            tests.add(id);
        }

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, tests, mTestsDir);

        long runtime = deqpTest.getRuntimeHint();
        assertTrue("Runtime for tests must be positive", runtime > 0);
        assertTrue("Runtime for tests must be reasonable", runtime < (1000 * 10)); // Must be done in 10s
    }


    private void runInstrumentationLineAndAnswer(ITestDevice mockDevice, IDevice mockIDevice,
            final String output) throws Exception {
        String cmd = String.format(
            "--deqp-caselist-file=%s --deqp-gl-config-name=rgba8888d24s8 "
            + "--deqp-screen-rotation=unspecified "
            + "--deqp-surface-type=window "
            + "--deqp-log-images=disable "
            + "--deqp-watchdog=enable",
            CASE_LIST_FILE_NAME);
        runInstrumentationLineAndAnswer(mockDevice, mockIDevice, null, cmd, output);
    }

    private void runInstrumentationLineAndAnswer(ITestDevice mockDevice, IDevice mockIDevice,
            final String testTrie, final String cmd, final String output) throws Exception {
        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.eq("rm " + CASE_LIST_FILE_NAME)))
                .andReturn("").once();

        EasyMock.expect(mockDevice.executeShellCommand(EasyMock.eq("rm " + LOG_FILE_NAME)))
                .andReturn("").once();

        if (testTrie == null) {
            mockDevice.pushString((String)EasyMock.anyObject(), EasyMock.eq(CASE_LIST_FILE_NAME));
        }
        else {
            mockDevice.pushString(testTrie + "\n", CASE_LIST_FILE_NAME);
        }
        EasyMock.expectLastCall().andReturn(true).once();

        String command = String.format(
                "am instrument %s -w -e deqpLogFileName \"%s\" -e deqpCmdLine \"%s\" "
                    + "-e deqpLogData \"%s\" %s",
                AbiUtils.createAbiFlag(ABI.getName()), LOG_FILE_NAME, cmd, false,
                INSTRUMENTATION_NAME);

        EasyMock.expect(mockDevice.getIDevice()).andReturn(mockIDevice);
        mockIDevice.executeShellCommand(EasyMock.eq(command),
                EasyMock.<IShellOutputReceiver>notNull(), EasyMock.anyLong(),
                EasyMock.isA(TimeUnit.class));

        EasyMock.expectLastCall().andAnswer(new IAnswer<Object>() {
            @Override
            public Object answer() {
                IShellOutputReceiver receiver
                        = (IShellOutputReceiver)EasyMock.getCurrentArguments()[1];

                receiver.addOutput(output.getBytes(), 0, output.length());
                receiver.flush();

                return null;
            }
        });
    }

    static private void writeStringsToFile(File target, Set<String> strings) throws IOException {
        try (PrintWriter out = new PrintWriter(new FileWriter(target))) {
            out.print(String.join(System.lineSeparator(), strings));
            out.println();
        }
    }

    private void addFilterFileForOption(DeqpTestRunner test, Set<String> filters, String option)
            throws IOException, ConfigurationException {
        String filterFile = option + ".txt";
        writeStringsToFile(new File(mTestsDir, filterFile), filters);
        OptionSetter setter = new OptionSetter(test);
        setter.setOptionValue(option, filterFile);
    }

    public void testIncludeFilterFile() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.missing", "no"),
                new TestIdentifier("dEQP-GLES3.missing", "nope"),
                new TestIdentifier("dEQP-GLES3.missing", "donotwant"),
                new TestIdentifier("dEQP-GLES3.pick_me", "yes"),
                new TestIdentifier("dEQP-GLES3.pick_me", "ok"),
                new TestIdentifier("dEQP-GLES3.pick_me", "accepted"),
        };

        List<TestIdentifier> allTests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) {
            allTests.add(id);
        }

        List<TestIdentifier> activeTests = new ArrayList<TestIdentifier>();
        activeTests.add(testIds[3]);
        activeTests.add(testIds[4]);
        activeTests.add(testIds[5]);

        String expectedTrie = "{dEQP-GLES3{pick_me{yes,ok,accepted}}}";

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, allTests, mTestsDir);
        Set<String> includes = new HashSet<>();
        includes.add("dEQP-GLES3.pick_me#*");
        addFilterFileForOption(deqpTest, includes, "include-filter-file");
        testFiltering(deqpTest, expectedTrie, activeTests);
    }

    public void testMissingIncludeFilterFile() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.pick_me", "yes"),
                new TestIdentifier("dEQP-GLES3.pick_me", "ok"),
                new TestIdentifier("dEQP-GLES3.pick_me", "accepted"),
        };

        List<TestIdentifier> allTests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) {
            allTests.add(id);
        }

        String expectedTrie = "{dEQP-GLES3{pick_me{yes,ok,accepted}}}";

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, allTests, mTestsDir);
        OptionSetter setter = new OptionSetter(deqpTest);
        setter.setOptionValue("include-filter-file", "not-a-file.txt");
        try {
            testFiltering(deqpTest, expectedTrie, allTests);
            fail("Test execution should have aborted with exception.");
        } catch (RuntimeException e) {
        }
    }

    public void testExcludeFilterFile() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.missing", "no"),
                new TestIdentifier("dEQP-GLES3.missing", "nope"),
                new TestIdentifier("dEQP-GLES3.missing", "donotwant"),
                new TestIdentifier("dEQP-GLES3.pick_me", "yes"),
                new TestIdentifier("dEQP-GLES3.pick_me", "ok"),
                new TestIdentifier("dEQP-GLES3.pick_me", "accepted"),
        };

        List<TestIdentifier> allTests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) {
            allTests.add(id);
        }

        List<TestIdentifier> activeTests = new ArrayList<TestIdentifier>();
        activeTests.add(testIds[3]);
        activeTests.add(testIds[4]);
        activeTests.add(testIds[5]);

        String expectedTrie = "{dEQP-GLES3{pick_me{yes,ok,accepted}}}";

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, allTests, mTestsDir);
        Set<String> excludes = new HashSet<>();
        excludes.add("dEQP-GLES3.missing#*");
        addFilterFileForOption(deqpTest, excludes, "exclude-filter-file");
        testFiltering(deqpTest, expectedTrie, activeTests);
    }

    public void testFilterComboWithFiles() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.group1", "footah"),
                new TestIdentifier("dEQP-GLES3.group1", "foo"),
                new TestIdentifier("dEQP-GLES3.group1", "nope"),
                new TestIdentifier("dEQP-GLES3.group1", "nonotwant"),
                new TestIdentifier("dEQP-GLES3.group2", "foo"),
                new TestIdentifier("dEQP-GLES3.group2", "yes"),
                new TestIdentifier("dEQP-GLES3.group2", "thoushallnotpass"),
        };

        List<TestIdentifier> allTests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) {
            allTests.add(id);
        }

        List<TestIdentifier> activeTests = new ArrayList<TestIdentifier>();
        activeTests.add(testIds[0]);
        activeTests.add(testIds[5]);

        String expectedTrie = "{dEQP-GLES3{group1{footah}group2{yes}}}";

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, allTests, mTestsDir);

        Set<String> includes = new HashSet<>();
        includes.add("dEQP-GLES3.group2#*");
        deqpTest.addAllIncludeFilters(includes);

        Set<String> fileIncludes = new HashSet<>();
        fileIncludes.add("dEQP-GLES3.group1#no*");
        fileIncludes.add("dEQP-GLES3.group1#foo*");
        addFilterFileForOption(deqpTest, fileIncludes, "include-filter-file");

        Set<String> fileExcludes = new HashSet<>();
        fileExcludes.add("*foo");
        fileExcludes.add("*thoushallnotpass");
        addFilterFileForOption(deqpTest, fileExcludes, "exclude-filter-file");

        deqpTest.addExcludeFilter("dEQP-GLES3.group1#no*");

        testFiltering(deqpTest, expectedTrie, activeTests);
    }

    public void testDotToHashConversionInFilters() throws Exception {
        final TestIdentifier[] testIds = {
                new TestIdentifier("dEQP-GLES3.missing", "no"),
                new TestIdentifier("dEQP-GLES3.pick_me", "donotwant"),
                new TestIdentifier("dEQP-GLES3.pick_me", "yes")
        };

        List<TestIdentifier> allTests = new ArrayList<TestIdentifier>();
        for (TestIdentifier id : testIds) {
            allTests.add(id);
        }

        List<TestIdentifier> activeTests = new ArrayList<TestIdentifier>();
        activeTests.add(testIds[2]);

        String expectedTrie = "{dEQP-GLES3{pick_me{yes}}}";

        DeqpTestRunner deqpTest = buildGlesTestRunner(3, 0, allTests, mTestsDir);
        deqpTest.addIncludeFilter("dEQP-GLES3.pick_me.yes");
        testFiltering(deqpTest, expectedTrie, activeTests);
    }
}
