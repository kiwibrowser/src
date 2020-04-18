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

package android.databinding.tool;


import android.databinding.tool.expr.Expr;
import android.databinding.tool.expr.ExprModel;
import android.databinding.tool.expr.FieldAccessExpr;
import android.databinding.tool.expr.IdentifierExpr;
import android.databinding.tool.expr.StaticIdentifierExpr;
import android.databinding.tool.reflection.Callable;
import android.databinding.tool.reflection.java.JavaAnalyzer;
import android.databinding.tool.reflection.java.JavaClass;

import org.junit.Before;
import org.junit.Test;

import java.util.List;
import java.util.Map;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

public class LayoutBinderTest {
    MockLayoutBinder mLayoutBinder;
    ExprModel mExprModel;
    @Before
    public void setUp() throws Exception {
        mLayoutBinder = new MockLayoutBinder();
        mExprModel = mLayoutBinder.getModel();
        JavaAnalyzer.initForTests();
    }

    @Test
    public void testRegisterId() {
        int originalSize = mExprModel.size();
        mLayoutBinder.addVariable("test", "java.lang.String", null);
        assertEquals(originalSize + 1, mExprModel.size());
        final Map.Entry<String, Expr> entry = findIdentifier("test");
        final Expr value = entry.getValue();
        assertEquals(value.getClass(), IdentifierExpr.class);
        final IdentifierExpr id = (IdentifierExpr) value;
        assertEquals("test", id.getName());
        assertEquals(new JavaClass(String.class), id.getResolvedType());
        assertTrue(id.isDynamic());
    }

    @Test
    public void testRegisterImport() {
        int originalSize = mExprModel.size();
        mExprModel.addImport("test", "java.lang.String", null);
        assertEquals(originalSize + 1, mExprModel.size());
        final Map.Entry<String, Expr> entry = findIdentifier("test");
        final Expr value = entry.getValue();
        assertEquals(value.getClass(), StaticIdentifierExpr.class);
        final IdentifierExpr id = (IdentifierExpr) value;
        assertEquals("test", id.getName());
        assertEquals(new JavaClass(String.class), id.getResolvedType());
        assertFalse(id.isDynamic());
    }

    @Test
    public void testParse() {
        int originalSize = mExprModel.size();
        mLayoutBinder.addVariable("user", "android.databinding.tool2.LayoutBinderTest.TestUser",
                null);
        mLayoutBinder.parse("user.name", false, null);
        mLayoutBinder.parse("user.lastName", false, null);
        assertEquals(originalSize + 3, mExprModel.size());
        final List<Expr> bindingExprs = mExprModel.getBindingExpressions();
        assertEquals(2, bindingExprs.size());
        IdentifierExpr id = mExprModel.identifier("user");
        assertTrue(bindingExprs.get(0) instanceof FieldAccessExpr);
        assertTrue(bindingExprs.get(1) instanceof FieldAccessExpr);
        assertEquals(2, id.getParents().size());
        assertTrue(bindingExprs.get(0).getChildren().contains(id));
        assertTrue(bindingExprs.get(1).getChildren().contains(id));
    }

    @Test
    public void testParseWithMethods() {
        mLayoutBinder.addVariable("user", "android.databinding.tool.LayoutBinderTest.TestUser",
                null);
        mLayoutBinder.parse("user.fullName", false, null);
        Expr item = mExprModel.getBindingExpressions().get(0);
        assertTrue(item instanceof FieldAccessExpr);
        IdentifierExpr id = mExprModel.identifier("user");
        FieldAccessExpr fa = (FieldAccessExpr) item;
        fa.getResolvedType();
        final Callable getter = fa.getGetter();
        assertTrue(getter.type == Callable.Type.METHOD);
        assertSame(id, fa.getChild());
        assertTrue(fa.isDynamic());
    }

    private Map.Entry<String, Expr> findIdentifier(String name) {
        for (Map.Entry<String, Expr> entry : mExprModel.getExprMap().entrySet()) {
            if (entry.getValue() instanceof IdentifierExpr) {
                IdentifierExpr expr = (IdentifierExpr) entry.getValue();
                if (name.equals(expr.getName())) {
                    return entry;
                }
            }
        }
        return null;
    }

    static class TestUser {
        public String name;
        public String lastName;

        public String fullName() {
            return name + " " + lastName;
        }
    }
}
