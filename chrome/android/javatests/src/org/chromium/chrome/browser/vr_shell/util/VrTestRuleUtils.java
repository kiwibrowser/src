// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.util;

import org.junit.Assert;
import org.junit.rules.RuleChain;
import org.junit.rules.TestRule;

import org.chromium.base.test.params.ParameterSet;
import org.chromium.chrome.browser.vr_shell.rules.ChromeTabbedActivityVrTestRule;
import org.chromium.chrome.browser.vr_shell.rules.CustomTabActivityVrTestRule;
import org.chromium.chrome.browser.vr_shell.rules.VrActivityRestriction.SupportedActivity;
import org.chromium.chrome.browser.vr_shell.rules.VrActivityRestrictionRule;
import org.chromium.chrome.browser.vr_shell.rules.VrTestRule;
import org.chromium.chrome.browser.vr_shell.rules.WebappActivityVrTestRule;

import java.util.ArrayList;
import java.util.concurrent.Callable;

/**
 * Utility class for interacting with VR-specific Rules, particularly VrActivityRestrictionRule.
 */
public class VrTestRuleUtils {
    /**
     * Creates the list of VrTestRules that are currently supported for use in test
     * parameterization.
     */
    public static ArrayList<ParameterSet> generateDefaultVrTestRuleParameters() {
        ArrayList<ParameterSet> parameters = new ArrayList<ParameterSet>();
        parameters.add(new ParameterSet()
                               .value(new Callable<ChromeTabbedActivityVrTestRule>() {
                                   @Override
                                   public ChromeTabbedActivityVrTestRule call() {
                                       return new ChromeTabbedActivityVrTestRule();
                                   }
                               })
                               .name("ChromeTabbedActivity"));

        parameters.add(new ParameterSet()
                               .value(new Callable<CustomTabActivityVrTestRule>() {
                                   @Override
                                   public CustomTabActivityVrTestRule call() {
                                       return new CustomTabActivityVrTestRule();
                                   }
                               })
                               .name("CustomTabActivity"));

        parameters.add(new ParameterSet()
                               .value(new Callable<WebappActivityVrTestRule>() {
                                   @Override
                                   public WebappActivityVrTestRule call() {
                                       return new WebappActivityVrTestRule();
                                   }
                               })
                               .name("WebappActivity"));

        return parameters;
    }

    /**
     * Creates a RuleChain that applies the VrActivityRestrictionRule before the given VrTestRule.
     */
    public static RuleChain wrapRuleInVrActivityRestrictionRule(TestRule rule) {
        Assert.assertTrue("Given rule is a VrTestRule", rule instanceof VrTestRule);
        return RuleChain
                .outerRule(new VrActivityRestrictionRule(((VrTestRule) rule).getRestriction()))
                .around(rule);
    }

    /**
     * Converts VrActivityRestriction.SupportedActivity enum to strings
     */
    public static String supportedActivityToString(SupportedActivity activity) {
        switch (activity) {
            case CTA:
                return "ChromeTabbedActivity";
            case CCT:
                return "CustomTabActivity";
            case WAA:
                return "WebappActivity";
            case ALL:
                return "AllActivities";
            default:
                return "UnknownActivity";
        }
    }
}
