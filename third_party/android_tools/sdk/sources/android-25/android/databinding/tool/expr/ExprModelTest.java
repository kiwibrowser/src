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

package android.databinding.tool.expr;

import android.databinding.Bindable;
import android.databinding.Observable;
import android.databinding.tool.LayoutBinder;
import android.databinding.tool.MockLayoutBinder;
import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.reflection.ModelClass;
import android.databinding.tool.reflection.java.JavaAnalyzer;
import android.databinding.tool.store.Location;
import android.databinding.tool.util.L;
import android.databinding.tool.writer.KCode;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Collections;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

public class ExprModelTest {

    private static class DummyExpr extends Expr {

        String mKey;

        public DummyExpr(String key, DummyExpr... children) {
            super(children);
            mKey = key;
        }

        @Override
        protected ModelClass resolveType(ModelAnalyzer modelAnalyzer) {
            return modelAnalyzer.findClass(Integer.class);
        }

        @Override
        protected List<Dependency> constructDependencies() {
            return constructDynamicChildrenDependencies();
        }

        @Override
        protected String computeUniqueKey() {
            return mKey + super.computeUniqueKey();
        }

        @Override
        protected KCode generateCode(boolean full) {
            return new KCode();
        }

        @Override
        protected String getInvertibleError() {
            return "DummyExpr cannot be 2-way.";
        }
    }

    ExprModel mExprModel;

    @Rule
    public TestWatcher mTestWatcher = new TestWatcher() {
        @Override
        protected void failed(Throwable e, Description description) {
            if (mExprModel != null && mExprModel.getFlagMapping() != null) {
                final String[] mapping = mExprModel.getFlagMapping();
                for (int i = 0; i < mapping.length; i++) {
                    L.d("flag %d: %s", i, mapping[i]);
                }
            }
        }
    };

    @Before
    public void setUp() throws Exception {
        JavaAnalyzer.initForTests();
        mExprModel = new ExprModel();
    }

    @Test
    public void testAddNormal() {
        final DummyExpr d = new DummyExpr("a");
        assertSame(d, mExprModel.register(d));
        assertSame(d, mExprModel.register(d));
        assertEquals(1, mExprModel.mExprMap.size());
    }

    @Test
    public void testAddDupe1() {
        final DummyExpr d = new DummyExpr("a");
        assertSame(d, mExprModel.register(d));
        assertSame(d, mExprModel.register(new DummyExpr("a")));
        assertEquals(1, mExprModel.mExprMap.size());
    }

    @Test
    public void testAddMultiple() {
        mExprModel.register(new DummyExpr("a"));
        mExprModel.register(new DummyExpr("b"));
        assertEquals(2, mExprModel.mExprMap.size());
    }


    @Test
    public void testAddWithChildren() {
        DummyExpr a = new DummyExpr("a");
        DummyExpr b = new DummyExpr("b");
        DummyExpr c = new DummyExpr("c", a, b);
        mExprModel.register(c);
        DummyExpr a2 = new DummyExpr("a");
        DummyExpr b2 = new DummyExpr("b");
        DummyExpr c2 = new DummyExpr("c", a, b);
        assertEquals(c, mExprModel.register(c2));
    }

    @Test
    public void testShouldRead() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", "java.lang.String", null);
        IdentifierExpr b = lb.addVariable("b", "java.lang.String", null);
        IdentifierExpr c = lb.addVariable("c", "java.lang.String", null);
        lb.parse("a == null ? b : c", false, null);
        mExprModel.comparison("==", a, mExprModel.symbol("null", Object.class));
        lb.getModel().seal();
        List<Expr> shouldRead = getShouldRead();
        // a and a == null
        assertEquals(2, shouldRead.size());
        final List<Expr> readFirst = getReadFirst(shouldRead, null);
        assertEquals(1, readFirst.size());
        final Expr first = readFirst.get(0);
        assertSame(a, first);
        // now , assume we've read this
        final BitSet shouldReadFlags = first.getShouldReadFlags();
        assertNotNull(shouldReadFlags);
    }

    @Test
    public void testReadConstantTernary() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", "java.lang.String", null);
        IdentifierExpr b = lb.addVariable("b", "java.lang.String", null);
        TernaryExpr ternaryExpr = parse(lb, "true ? a : b", TernaryExpr.class);
        mExprModel.seal();
        List<Expr> shouldRead = getShouldRead();
        assertExactMatch(shouldRead, ternaryExpr.getPred());
        List<Expr> first = getReadFirst(shouldRead);
        assertExactMatch(first, ternaryExpr.getPred());
        mExprModel.markBitsRead();
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, a, b, ternaryExpr);
        first = getReadFirst(shouldRead);
        assertExactMatch(first, a, b);
        List<Expr> justRead = new ArrayList<Expr>();
        justRead.add(a);
        justRead.add(b);
        first = filterOut(getReadFirst(shouldRead, justRead), justRead);
        assertExactMatch(first, ternaryExpr);
        assertFalse(mExprModel.markBitsRead());
    }

    @Test
    public void testTernaryWithPlus() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr user = lb
                .addVariable("user", "android.databinding.tool.expr.ExprModelTest.User",
                        null);
        MathExpr parsed = parse(lb, "user.name + \" \" + (user.lastName ?? \"\")", MathExpr.class);
        mExprModel.seal();
        List<Expr> toRead = getShouldRead();
        List<Expr> readNow = getReadFirst(toRead);
        assertEquals(1, readNow.size());
        assertSame(user, readNow.get(0));
        List<Expr> justRead = new ArrayList<Expr>();
        justRead.add(user);
        readNow = filterOut(getReadFirst(toRead, justRead), justRead);
        assertEquals(2, readNow.size()); //user.name && user.lastName
        justRead.addAll(readNow);
        // user.lastname (T, F), user.name + " "
        readNow = filterOut(getReadFirst(toRead, justRead), justRead);
        assertEquals(2, readNow.size()); //user.name && user.lastName
        justRead.addAll(readNow);
        readNow = filterOut(getReadFirst(toRead, justRead), justRead);
        assertEquals(0, readNow.size());
        mExprModel.markBitsRead();

        toRead = getShouldRead();
        assertEquals(2, toRead.size());
        justRead.clear();
        readNow = filterOut(getReadFirst(toRead, justRead), justRead);
        assertEquals(1, readNow.size());
        assertSame(parsed.getRight(), readNow.get(0));
        justRead.addAll(readNow);

        readNow = filterOut(getReadFirst(toRead, justRead), justRead);
        assertEquals(1, readNow.size());
        assertSame(parsed, readNow.get(0));
        justRead.addAll(readNow);

        readNow = filterOut(getReadFirst(toRead, justRead), justRead);
        assertEquals(0, readNow.size());
        mExprModel.markBitsRead();
        assertEquals(0, getShouldRead().size());
    }

    private List<Expr> filterOut(List<Expr> itr, final List<Expr> exclude) {
        List<Expr> result = new ArrayList<Expr>();
        for (Expr expr : itr) {
            if (!exclude.contains(expr)) {
                result.add(expr);
            }
        }
        return result;
    }

    @Test
    public void testTernaryInsideTernary() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr cond1 = lb.addVariable("cond1", "boolean", null);
        IdentifierExpr cond2 = lb.addVariable("cond2", "boolean", null);

        IdentifierExpr a = lb.addVariable("a", "boolean", null);
        IdentifierExpr b = lb.addVariable("b", "boolean", null);
        IdentifierExpr c = lb.addVariable("c", "boolean", null);

        final TernaryExpr ternaryExpr = parse(lb, "cond1 ? cond2 ? a : b : c", TernaryExpr.class);
        final TernaryExpr innerTernary = (TernaryExpr) ternaryExpr.getIfTrue();
        mExprModel.seal();

        List<Expr> toRead = getShouldRead();
        assertEquals(1, toRead.size());
        assertEquals(ternaryExpr.getPred(), toRead.get(0));

        List<Expr> readNow = getReadFirst(toRead);
        assertEquals(1, readNow.size());
        assertEquals(ternaryExpr.getPred(), readNow.get(0));
        int cond1True = ternaryExpr.getRequirementFlagIndex(true);
        int cond1False = ternaryExpr.getRequirementFlagIndex(false);
        // ok, it is read now.
        mExprModel.markBitsRead();

        // now it should read cond2 or c, depending on the flag from first
        toRead = getShouldRead();
        assertEquals(2, toRead.size());
        assertExactMatch(toRead, ternaryExpr.getIfFalse(), innerTernary.getPred());
        assertFlags(ternaryExpr.getIfFalse(), cond1False);
        assertFlags(ternaryExpr.getIfTrue(), cond1True);

        mExprModel.markBitsRead();

        // now it should read a or b, innerTernary, outerTernary
        toRead = getShouldRead();
        assertExactMatch(toRead, innerTernary.getIfTrue(), innerTernary.getIfFalse(), ternaryExpr,
                innerTernary);
        assertFlags(innerTernary.getIfTrue(), innerTernary.getRequirementFlagIndex(true));
        assertFlags(innerTernary.getIfFalse(), innerTernary.getRequirementFlagIndex(false));
        assertFalse(mExprModel.markBitsRead());
    }

    @Test
    public void testRequirementFlags() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", "java.lang.String", null);
        IdentifierExpr b = lb.addVariable("b", "java.lang.String", null);
        IdentifierExpr c = lb.addVariable("c", "java.lang.String", null);
        IdentifierExpr d = lb.addVariable("d", "java.lang.String", null);
        IdentifierExpr e = lb.addVariable("e", "java.lang.String", null);
        final Expr aTernary = lb.parse("a == null ? b == null ? c : d : e", false, null);
        assertTrue(aTernary instanceof TernaryExpr);
        final Expr bTernary = ((TernaryExpr) aTernary).getIfTrue();
        assertTrue(bTernary instanceof TernaryExpr);
        final Expr aIsNull = mExprModel
                .comparison("==", a, mExprModel.symbol("null", Object.class));
        final Expr bIsNull = mExprModel
                .comparison("==", b, mExprModel.symbol("null", Object.class));
        lb.getModel().seal();
        List<Expr> shouldRead = getShouldRead();
        // a and a == null
        assertEquals(2, shouldRead.size());
        assertFalse(a.getShouldReadFlags().isEmpty());
        assertTrue(a.getShouldReadFlags().get(a.getId()));
        assertTrue(b.getShouldReadFlags().isEmpty());
        assertTrue(c.getShouldReadFlags().isEmpty());
        assertTrue(d.getShouldReadFlags().isEmpty());
        assertTrue(e.getShouldReadFlags().isEmpty());

        List<Expr> readFirst = getReadFirst(shouldRead, null);
        assertEquals(1, readFirst.size());
        final Expr first = readFirst.get(0);
        assertSame(a, first);
        assertTrue(mExprModel.markBitsRead());
        for (Expr expr : mExprModel.getPendingExpressions()) {
            assertNull(expr.mShouldReadFlags);
        }
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, e, b, bIsNull);

        assertFlags(e, aTernary.getRequirementFlagIndex(false));

        assertFlags(b, aTernary.getRequirementFlagIndex(true));
        assertFlags(bIsNull, aTernary.getRequirementFlagIndex(true));
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertEquals(4, shouldRead.size());
        assertTrue(shouldRead.contains(c));
        assertTrue(shouldRead.contains(d));
        assertTrue(shouldRead.contains(aTernary));
        assertTrue(shouldRead.contains(bTernary));

        assertTrue(c.getShouldReadFlags().get(bTernary.getRequirementFlagIndex(true)));
        assertEquals(1, c.getShouldReadFlags().cardinality());

        assertTrue(d.getShouldReadFlags().get(bTernary.getRequirementFlagIndex(false)));
        assertEquals(1, d.getShouldReadFlags().cardinality());

        assertTrue(bTernary.getShouldReadFlags().get(aTernary.getRequirementFlagIndex(true)));
        assertEquals(1, bTernary.getShouldReadFlags().cardinality());
        // +1 for invalidate all flag
        assertEquals(6, aTernary.getShouldReadFlags().cardinality());
        for (Expr expr : new Expr[]{a, b, c, d, e}) {
            assertTrue(aTernary.getShouldReadFlags().get(expr.getId()));
        }

        readFirst = getReadFirst(shouldRead);
        assertEquals(2, readFirst.size());
        assertTrue(readFirst.contains(c));
        assertTrue(readFirst.contains(d));
        assertFalse(mExprModel.markBitsRead());
    }

    @Test
    public void testPostConditionalDependencies() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();

        IdentifierExpr u1 = lb.addVariable("u1", User.class.getCanonicalName(), null);
        IdentifierExpr u2 = lb.addVariable("u2", User.class.getCanonicalName(), null);
        IdentifierExpr a = lb.addVariable("a", int.class.getCanonicalName(), null);
        IdentifierExpr b = lb.addVariable("b", int.class.getCanonicalName(), null);
        IdentifierExpr c = lb.addVariable("c", int.class.getCanonicalName(), null);
        IdentifierExpr d = lb.addVariable("d", int.class.getCanonicalName(), null);
        IdentifierExpr e = lb.addVariable("e", int.class.getCanonicalName(), null);
        TernaryExpr abTernary = parse(lb, "a > b ? u1.name : u2.name", TernaryExpr.class);
        TernaryExpr bcTernary = parse(lb, "b > c ? u1.getCond(d) ? u1.lastName : u2.lastName : `xx`"
                + " + u2.getCond(e) ", TernaryExpr.class);
        Expr abCmp = abTernary.getPred();
        Expr bcCmp = bcTernary.getPred();
        Expr u1GetCondD = ((TernaryExpr) bcTernary.getIfTrue()).getPred();
        final MathExpr xxPlusU2getCondE = (MathExpr) bcTernary.getIfFalse();
        Expr u2GetCondE = xxPlusU2getCondE.getRight();
        Expr u1Name = abTernary.getIfTrue();
        Expr u2Name = abTernary.getIfFalse();
        Expr u1LastName = ((TernaryExpr) bcTernary.getIfTrue()).getIfTrue();
        Expr u2LastName = ((TernaryExpr) bcTernary.getIfTrue()).getIfFalse();

        mExprModel.seal();
        List<Expr> shouldRead = getShouldRead();

        assertExactMatch(shouldRead, a, b, c, abCmp, bcCmp);

        List<Expr> firstRead = getReadFirst(shouldRead);

        assertExactMatch(firstRead, a, b, c);

        assertFlags(a, a, b, u1, u2, u1Name, u2Name);
        assertFlags(b, a, b, u1, u2, u1Name, u2Name, c, d, u1LastName, u2LastName, e);
        assertFlags(c, b, c, u1, d, u1LastName, u2LastName, e);
        assertFlags(abCmp, a, b, u1, u2, u1Name, u2Name);
        assertFlags(bcCmp, b, c, u1, d, u1LastName, u2LastName, e);

        assertTrue(mExprModel.markBitsRead());

        shouldRead = getShouldRead();
        Expr[] batch = {d, e, u1, u2, u1GetCondD, u2GetCondE, xxPlusU2getCondE, abTernary,
                abTernary.getIfTrue(), abTernary.getIfFalse()};
        assertExactMatch(shouldRead, batch);
        firstRead = getReadFirst(shouldRead);
        assertExactMatch(firstRead, d, e, u1, u2);

        assertFlags(d, bcTernary.getRequirementFlagIndex(true));
        assertFlags(e, bcTernary.getRequirementFlagIndex(false));
        assertFlags(u1, bcTernary.getRequirementFlagIndex(true),
                abTernary.getRequirementFlagIndex(true));
        assertFlags(u2, bcTernary.getRequirementFlagIndex(false),
                abTernary.getRequirementFlagIndex(false));

        assertFlags(u1GetCondD, bcTernary.getRequirementFlagIndex(true));
        assertFlags(u2GetCondE, bcTernary.getRequirementFlagIndex(false));
        assertFlags(xxPlusU2getCondE, bcTernary.getRequirementFlagIndex(false));
        assertFlags(abTernary, a, b, u1, u2, u1Name, u2Name);
        assertFlags(abTernary.getIfTrue(), abTernary.getRequirementFlagIndex(true));
        assertFlags(abTernary.getIfFalse(), abTernary.getRequirementFlagIndex(false));

        assertTrue(mExprModel.markBitsRead());

        shouldRead = getShouldRead();
        // FIXME: there is no real case to read u1 anymore because if b>c was not true,
        // u1.getCond(d) will never be set. Right now, we don't have mechanism to figure this out
        // and also it does not affect correctness (just an unnecessary if stmt)
        assertExactMatch(shouldRead, u1, u2, u1LastName, u2LastName, bcTernary.getIfTrue(), bcTernary);
        firstRead = getReadFirst(shouldRead);
        assertExactMatch(firstRead, u1, u2);
        assertFlags(u1, bcTernary.getIfTrue().getRequirementFlagIndex(true));
        assertFlags(u2, bcTernary.getIfTrue().getRequirementFlagIndex(false));
        assertFlags(u1LastName, bcTernary.getIfTrue().getRequirementFlagIndex(true));
        assertFlags(u2LastName, bcTernary.getIfTrue().getRequirementFlagIndex(false));

        assertFlags(bcTernary.getIfTrue(), bcTernary.getRequirementFlagIndex(true));
        assertFlags(bcTernary, b, c, u1, u2, d, u1LastName, u2LastName, e);

        assertFalse(mExprModel.markBitsRead());
    }

    @Test
    public void testCircularDependency() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", int.class.getCanonicalName(),
                null);
        IdentifierExpr b = lb.addVariable("b", int.class.getCanonicalName(),
                null);
        final TernaryExpr abTernary = parse(lb, "a > 3 ? a : b", TernaryExpr.class);
        mExprModel.seal();
        List<Expr> shouldRead = getShouldRead();
        assertExactMatch(shouldRead, a, abTernary.getPred());
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, b, abTernary);
        assertFalse(mExprModel.markBitsRead());
    }

    @Test
    public void testNestedCircularDependency() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", int.class.getCanonicalName(),
                null);
        IdentifierExpr b = lb.addVariable("b", int.class.getCanonicalName(),
                null);
        IdentifierExpr c = lb.addVariable("c", int.class.getCanonicalName(),
                null);
        final TernaryExpr a3Ternary = parse(lb, "a > 3 ? c > 4 ? a : b : c", TernaryExpr.class);
        final TernaryExpr c4Ternary = (TernaryExpr) a3Ternary.getIfTrue();
        mExprModel.seal();
        List<Expr> shouldRead = getShouldRead();
        assertExactMatch(shouldRead, a, a3Ternary.getPred());
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, c, c4Ternary.getPred());
        assertFlags(c, a3Ternary.getRequirementFlagIndex(true),
                a3Ternary.getRequirementFlagIndex(false));
        assertFlags(c4Ternary.getPred(), a3Ternary.getRequirementFlagIndex(true));
    }

    @Test
    public void testInterExprDependency() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr u = lb.addVariable("u", User.class.getCanonicalName(),
                null);
        final Expr uComment = parse(lb, "u.comment", FieldAccessExpr.class);
        final TernaryExpr uTernary = parse(lb, "u.getUseComment ? u.comment : `xx`", TernaryExpr.class);
        mExprModel.seal();
        assertTrue(uTernary.getPred().canBeInvalidated());
        List<Expr> shouldRead = getShouldRead();
        assertExactMatch(shouldRead, u, uComment, uTernary.getPred());
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, uComment, uTernary);
    }

    @Test
    public void testInterExprCircularDependency() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", int.class.getCanonicalName(),
                null);
        IdentifierExpr b = lb.addVariable("b", int.class.getCanonicalName(),
                null);
        final TernaryExpr abTernary = parse(lb, "a > 3 ? a : b", TernaryExpr.class);
        final TernaryExpr abTernary2 = parse(lb, "b > 3 ? b : a", TernaryExpr.class);
        mExprModel.seal();
        List<Expr> shouldRead = getShouldRead();
        assertExactMatch(shouldRead, a, b, abTernary.getPred(), abTernary2.getPred());
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, abTernary, abTernary2);
    }

    @Test
    public void testInterExprCircularDependency2() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", boolean.class.getCanonicalName(),
                null);
        IdentifierExpr b = lb.addVariable("b", boolean.class.getCanonicalName(),
                null);
        final TernaryExpr abTernary = parse(lb, "a ? b : true", TernaryExpr.class);
        final TernaryExpr baTernary = parse(lb, "b ? a : false", TernaryExpr.class);
        mExprModel.seal();
        List<Expr> shouldRead = getShouldRead();
        assertExactMatch(shouldRead, a, b);
        assertFlags(a, a, b);
        assertFlags(b, a, b);
        List<Expr> readFirst = getReadFirst(shouldRead);
        assertExactMatch(readFirst, a, b);
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, abTernary, baTernary);
        readFirst = getReadFirst(shouldRead);
        assertExactMatch(readFirst, abTernary, baTernary);
        assertFalse(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertEquals(0, shouldRead.size());
    }

    @Test
    public void testInterExprCircularDependency3() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", boolean.class.getCanonicalName(),
                null);
        IdentifierExpr b = lb.addVariable("b", boolean.class.getCanonicalName(),
                null);
        IdentifierExpr c = lb.addVariable("c", boolean.class.getCanonicalName(),
                null);
        final TernaryExpr abTernary = parse(lb, "a ? b : c", TernaryExpr.class);
        final TernaryExpr abTernary2 = parse(lb, "b ? a : c", TernaryExpr.class);
        mExprModel.seal();
        List<Expr> shouldRead = getShouldRead();
        assertExactMatch(shouldRead, a, b);
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        // read a and b again, this time for their dependencies and also the rest since everything
        // is ready to be read
        assertExactMatch(shouldRead, c, abTernary, abTernary2);
        mExprModel.markBitsRead();
        shouldRead = getShouldRead();
        assertEquals(0, shouldRead.size());
    }

    @Test
    public void testInterExprCircularDependency4() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", boolean.class.getCanonicalName(),
                null);
        IdentifierExpr b = lb.addVariable("b", boolean.class.getCanonicalName(),
                null);
        IdentifierExpr c = lb.addVariable("c", boolean.class.getCanonicalName(),
                null);
        IdentifierExpr d = lb.addVariable("d", boolean.class.getCanonicalName(),
                null);
        final TernaryExpr cTernary = parse(lb, "c ? (a ? d : false) : false", TernaryExpr.class);
        final TernaryExpr abTernary = parse(lb, "a ? b : true", TernaryExpr.class);
        final TernaryExpr baTernary = parse(lb, "b ? a : false", TernaryExpr.class);
        mExprModel.seal();
        List<Expr> shouldRead = getShouldRead();
        // check if a,b or c should be read. these are easily calculated from binding expressions'
        // invalidation
        assertExactMatch(shouldRead, c, a, b);

        List<Expr> justRead = new ArrayList<Expr>();
        List<Expr> readFirst = getReadFirst(shouldRead);
        assertExactMatch(readFirst, c, a, b);
        Collections.addAll(justRead, a, b, c);
        assertEquals(0, filterOut(getReadFirst(shouldRead, justRead), justRead).size());
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        // if a and b are not invalid, a won't be read in the first step. But if c's expression
        // is invalid and c == true, a must be read. Depending on a, d might be read as well.
        // don't need to read b anymore because `a ? b : true` and `b ? a : false` has the same
        // invalidation flags.
        assertExactMatch(shouldRead, a, abTernary, baTernary);
        justRead.clear();

        readFirst = getReadFirst(shouldRead);
        // first must read `a`.
        assertExactMatch(readFirst, a);
        Collections.addAll(justRead, a);

        readFirst = filterOut(getReadFirst(shouldRead, justRead), justRead);
        assertExactMatch(readFirst, abTernary, baTernary);
        Collections.addAll(justRead, abTernary, baTernary);

        readFirst = filterOut(getReadFirst(shouldRead, justRead), justRead);
        assertEquals(0, filterOut(getReadFirst(shouldRead, justRead), justRead).size());
        assertTrue(mExprModel.markBitsRead());

        shouldRead = getShouldRead();
        // now we can read adf ternary and c ternary
        justRead.clear();
        assertExactMatch(shouldRead, d, cTernary.getIfTrue(), cTernary);
        readFirst = getReadFirst(shouldRead);
        assertExactMatch(readFirst, d);
        Collections.addAll(justRead, d);
        readFirst = filterOut(getReadFirst(shouldRead, justRead), justRead);
        assertExactMatch(readFirst, cTernary.getIfTrue());
        Collections.addAll(justRead, cTernary.getIfTrue());

        readFirst = filterOut(getReadFirst(shouldRead, justRead), justRead);
        assertExactMatch(readFirst, cTernary);
        Collections.addAll(justRead, cTernary);

        assertEquals(0, filterOut(getReadFirst(shouldRead, justRead), justRead).size());

        assertFalse(mExprModel.markBitsRead());
    }

    @Test
    public void testInterExprDeepDependency() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", boolean.class.getCanonicalName(), null);
        IdentifierExpr b = lb.addVariable("b", boolean.class.getCanonicalName(), null);
        IdentifierExpr c = lb.addVariable("c", boolean.class.getCanonicalName(), null);
        final TernaryExpr t1 = parse(lb, "c ? (a ? b : true) : false", TernaryExpr.class);
        final TernaryExpr t2 = parse(lb, "c ? (b ? a : false) : true", TernaryExpr.class);
        final TernaryExpr abTernary = (TernaryExpr) t1.getIfTrue();
        final TernaryExpr baTernary = (TernaryExpr) t2.getIfTrue();
        mExprModel.seal();
        List<Expr> shouldRead = getShouldRead();
        assertExactMatch(shouldRead, c);
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, a, b);
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, a, b, t1.getIfTrue(), t2.getIfTrue(), t1, t2);
        assertFlags(b, abTernary.getRequirementFlagIndex(true));
        assertFlags(a, baTernary.getRequirementFlagIndex(true));
        assertFalse(mExprModel.markBitsRead());
    }

    @Test
    public void testInterExprDependencyNotReadyYet() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr a = lb.addVariable("a", boolean.class.getCanonicalName(), null);
        IdentifierExpr b = lb.addVariable("b", boolean.class.getCanonicalName(), null);
        IdentifierExpr c = lb.addVariable("c", boolean.class.getCanonicalName(), null);
        IdentifierExpr d = lb.addVariable("d", boolean.class.getCanonicalName(), null);
        IdentifierExpr e = lb.addVariable("e", boolean.class.getCanonicalName(), null);
        final TernaryExpr cTernary = parse(lb, "c ? (a ? d : false) : false", TernaryExpr.class);
        final TernaryExpr baTernary = parse(lb, "b ? a : false", TernaryExpr.class);
        final TernaryExpr eaTernary = parse(lb, "e ? a : false", TernaryExpr.class);
        mExprModel.seal();
        List<Expr> shouldRead = getShouldRead();
        assertExactMatch(shouldRead, b, c, e);
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, a, baTernary, eaTernary);
        assertTrue(mExprModel.markBitsRead());
        shouldRead = getShouldRead();
        assertExactMatch(shouldRead, d, cTernary.getIfTrue(), cTernary);
        assertFalse(mExprModel.markBitsRead());
    }

    @Test
    public void testNoFlagsForNonBindingStatic() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        lb.addVariable("a", int.class.getCanonicalName(), null);
        final MathExpr parsed = parse(lb, "a * (3 + 2)", MathExpr.class);
        mExprModel.seal();
        // +1 for invalidate all flag
        assertEquals(1, parsed.getRight().getInvalidFlags().cardinality());
        // +1 for invalidate all flag
        assertEquals(2, parsed.getLeft().getInvalidFlags().cardinality());
        // +1 for invalidate all flag
        assertEquals(2, mExprModel.getInvalidateableFieldLimit());
    }

    @Test
    public void testFlagsForBindingStatic() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        lb.addVariable("a", int.class.getCanonicalName(), null);
        final Expr staticParsed = parse(lb, "3 + 2", MathExpr.class);
        final MathExpr parsed = parse(lb, "a * (3 + 2)", MathExpr.class);
        mExprModel.seal();
        assertTrue(staticParsed.isBindingExpression());
        // +1 for invalidate all flag
        assertEquals(1, staticParsed.getInvalidFlags().cardinality());
        assertEquals(parsed.getRight().getInvalidFlags(), staticParsed.getInvalidFlags());
        // +1 for invalidate all flag
        assertEquals(2, parsed.getLeft().getInvalidFlags().cardinality());
        // +1 for invalidate all flag
        assertEquals(2, mExprModel.getInvalidateableFieldLimit());
    }

    @Test
    public void testFinalFieldOfAVariable() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        IdentifierExpr user = lb.addVariable("user", User.class.getCanonicalName(),
                null);
        Expr fieldGet = parse(lb, "user.finalField", FieldAccessExpr.class);
        mExprModel.seal();
        assertTrue(fieldGet.isDynamic());
        // read user
        assertExactMatch(getShouldRead(), user, fieldGet);
        mExprModel.markBitsRead();
        // no need to read user.finalField
        assertEquals(0, getShouldRead().size());
    }

    @Test
    public void testFinalFieldOfAField() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        lb.addVariable("user", User.class.getCanonicalName(), null);
        Expr finalFieldGet = parse(lb, "user.subObj.finalField", FieldAccessExpr.class);
        mExprModel.seal();
        assertTrue(finalFieldGet.isDynamic());
        Expr userSubObjGet = finalFieldGet.getChildren().get(0);
        // read user
        List<Expr> shouldRead = getShouldRead();
        assertEquals(3, shouldRead.size());
        assertExactMatch(shouldRead, userSubObjGet.getChildren().get(0), userSubObjGet,
                finalFieldGet);
        mExprModel.markBitsRead();
        // no need to read user.subObj.finalField because it is final
        assertEquals(0, getShouldRead().size());
    }

    @Test
    public void testFinalFieldOfAMethod() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        lb.addVariable("user", User.class.getCanonicalName(), null);
        Expr finalFieldGet = parse(lb, "user.anotherSubObj.finalField", FieldAccessExpr.class);
        mExprModel.seal();
        assertTrue(finalFieldGet.isDynamic());
        Expr userSubObjGet = finalFieldGet.getChildren().get(0);
        // read user
        List<Expr> shouldRead = getShouldRead();
        assertEquals(3, shouldRead.size());
        assertExactMatch(shouldRead, userSubObjGet.getChildren().get(0), userSubObjGet,
                finalFieldGet);
        mExprModel.markBitsRead();
        // no need to read user.subObj.finalField because it is final
        assertEquals(0, getShouldRead().size());
    }

    @Test
    public void testFinalOfAClass() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        mExprModel.addImport("View", "android.view.View", null);
        FieldAccessExpr fieldAccess = parse(lb, "View.VISIBLE", FieldAccessExpr.class);
        assertFalse(fieldAccess.isDynamic());
        mExprModel.seal();
        assertEquals(0, getShouldRead().size());
    }

    @Test
    public void testStaticFieldOfInstance() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        lb.addVariable("myView", "android.view.View", null);
        FieldAccessExpr fieldAccess = parse(lb, "myView.VISIBLE", FieldAccessExpr.class);
        assertFalse(fieldAccess.isDynamic());
        mExprModel.seal();
        assertEquals(0, getShouldRead().size());
        final Expr child = fieldAccess.getChild();
        assertTrue(child instanceof StaticIdentifierExpr);
        StaticIdentifierExpr id = (StaticIdentifierExpr) child;
        assertEquals(id.getResolvedType().getCanonicalName(), "android.view.View");
        // on demand import
        assertEquals("android.view.View", mExprModel.getImports().get("View"));
    }

    @Test
    public void testOnDemandImportConflict() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        final IdentifierExpr myView = lb.addVariable("u", "android.view.View",
                null);
        mExprModel.addImport("View", User.class.getCanonicalName(), null);
        final StaticIdentifierExpr id = mExprModel.staticIdentifierFor(myView.getResolvedType());
        mExprModel.seal();
        // on demand import with conflict
        assertEquals("android.view.View", mExprModel.getImports().get("View1"));
        assertEquals("View1", id.getName());
        assertEquals("android.view.View", id.getUserDefinedType());
    }

    @Test
    public void testOnDemandImportAlreadyImported() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        final StaticIdentifierExpr ux = mExprModel.addImport("UX", User.class.getCanonicalName(),
                null);
        final IdentifierExpr u = lb.addVariable("u", User.class.getCanonicalName(),
                null);
        final StaticIdentifierExpr id = mExprModel.staticIdentifierFor(u.getResolvedType());
        mExprModel.seal();
        // on demand import with conflict
        assertSame(ux, id);
    }

    @Test
    public void testStaticMethodOfInstance() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        lb.addVariable("user", User.class.getCanonicalName(), null);
        MethodCallExpr methodCall = parse(lb, "user.ourStaticMethod()", MethodCallExpr.class);
        assertTrue(methodCall.isDynamic());
        mExprModel.seal();
        final Expr child = methodCall.getTarget();
        assertTrue(child instanceof StaticIdentifierExpr);
        StaticIdentifierExpr id = (StaticIdentifierExpr) child;
        assertEquals(id.getResolvedType().getCanonicalName(), User.class.getCanonicalName());
    }

    @Test
    public void testFinalOfStaticField() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        mExprModel.addImport("UX", User.class.getCanonicalName(), null);
        FieldAccessExpr fieldAccess = parse(lb, "UX.innerStaticInstance.finalStaticField",
                FieldAccessExpr.class);
        assertFalse(fieldAccess.isDynamic());
        mExprModel.seal();
        // nothing to read since it is all final and static
        assertEquals(0, getShouldRead().size());
    }

    @Test
    public void testFinalOfFinalStaticField() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        mExprModel.addImport("User", User.class.getCanonicalName(), null);
        FieldAccessExpr fieldAccess = parse(lb, "User.innerFinalStaticInstance.finalStaticField",
                FieldAccessExpr.class);
        assertFalse(fieldAccess.isDynamic());
        mExprModel.seal();
        assertEquals(0, getShouldRead().size());
    }

    @Test
    public void testLocationTracking() {
        MockLayoutBinder lb = new MockLayoutBinder();
        mExprModel = lb.getModel();
        final String input = "a > 3 ? b : c";
        TernaryExpr ternaryExpr = parse(lb, input, TernaryExpr.class);
        final Location location = ternaryExpr.getLocations().get(0);
        assertNotNull(location);
        assertEquals(0, location.startLine);
        assertEquals(0, location.startOffset);
        assertEquals(0, location.endLine);
        assertEquals(input.length() - 1, location.endOffset);

        final ComparisonExpr comparison = (ComparisonExpr) ternaryExpr.getPred();
        final Location predLoc = comparison.getLocations().get(0);
        assertNotNull(predLoc);
        assertEquals(0, predLoc.startLine);
        assertEquals(0, predLoc.startOffset);
        assertEquals(0, predLoc.endLine);
        assertEquals(4, predLoc.endOffset);

        final Location aLoc = comparison.getLeft().getLocations().get(0);
        assertNotNull(aLoc);
        assertEquals(0, aLoc.startLine);
        assertEquals(0, aLoc.startOffset);
        assertEquals(0, aLoc.endLine);
        assertEquals(0, aLoc.endOffset);

        final Location tLoc = comparison.getRight().getLocations().get(0);
        assertNotNull(tLoc);
        assertEquals(0, tLoc.startLine);
        assertEquals(4, tLoc.startOffset);
        assertEquals(0, tLoc.endLine);
        assertEquals(4, tLoc.endOffset);

        final Location bLoc = ternaryExpr.getIfTrue().getLocations().get(0);
        assertNotNull(bLoc);
        assertEquals(0, bLoc.startLine);
        assertEquals(8, bLoc.startOffset);
        assertEquals(0, bLoc.endLine);
        assertEquals(8, bLoc.endOffset);

        final Location cLoc = ternaryExpr.getIfFalse().getLocations().get(0);
        assertNotNull(cLoc);
        assertEquals(0, cLoc.startLine);
        assertEquals(12, cLoc.startOffset);
        assertEquals(0, cLoc.endLine);
        assertEquals(12, cLoc.endOffset);
    }

//    TODO uncomment when we have inner static access
//    @Test
//    public void testFinalOfInnerStaticClass() {
//        MockLayoutBinder lb = new MockLayoutBinder();
//        mExprModel = lb.getModel();
//        mExprModel.addImport("User", User.class.getCanonicalName());
//        FieldAccessExpr fieldAccess = parse(lb, "User.InnerStaticClass.finalStaticField", FieldAccessExpr.class);
//        assertFalse(fieldAccess.isDynamic());
//        mExprModel.seal();
//        assertEquals(0, getShouldRead().size());
//    }

    private void assertFlags(Expr a, int... flags) {
        BitSet bitset = new BitSet();
        for (int flag : flags) {
            bitset.set(flag);
        }
        assertEquals("flag test for " + a.getUniqueKey(), bitset, a.getShouldReadFlags());
    }

    private void assertFlags(Expr a, Expr... exprs) {
        BitSet bitSet = a.getShouldReadFlags();
        for (Expr expr : exprs) {
            BitSet clone = (BitSet) bitSet.clone();
            clone.and(expr.getInvalidFlags());
            assertEquals("should read flags of " + a.getUniqueKey() + " should include " + expr
                    .getUniqueKey(), expr.getInvalidFlags(), clone);
        }

        BitSet composite = new BitSet();
        for (Expr expr : exprs) {
            composite.or(expr.getInvalidFlags());
        }
        assertEquals("composite flags should match", composite, bitSet);
    }

    private void assertExactMatch(List<Expr> iterable, Expr... exprs) {
        int i = 0;
        String listLog = Arrays.toString(iterable.toArray());
        String itemsLog = Arrays.toString(exprs);
        String log = "list: " + listLog + "\nitems: " + itemsLog;
        log("list", iterable);
        for (Expr expr : exprs) {
            assertTrue((i++) + ":must contain " + expr.getUniqueKey() + "\n" + log,
                    iterable.contains(expr));
        }
        i = 0;
        for (Expr expr : iterable) {
            assertTrue((i++) + ":must be expected " + expr.getUniqueKey() + "\n" + log,
                    Arrays.asList(exprs).contains(expr));
        }
    }

    private <T extends Expr> T parse(LayoutBinder binder, String input, Class<T> klass) {
        final Expr parsed = binder.parse(input, false, null);
        assertTrue(klass.isAssignableFrom(parsed.getClass()));
        return (T) parsed;
    }

    private void log(String s, List<Expr> iterable) {
        L.d(s);
        for (Expr e : iterable) {
            L.d(": %s : %s allFlags: %s readSoFar: %s", e.getUniqueKey(), e.getShouldReadFlags(),
                    e.getShouldReadFlagsWithConditionals(), e.getReadSoFar());
        }
        L.d("end of %s", s);
    }

    private List<Expr> getReadFirst(List<Expr> shouldRead) {
        return getReadFirst(shouldRead, null);
    }

    private List<Expr> getReadFirst(List<Expr> shouldRead, final List<Expr> justRead) {
        List<Expr> result = new ArrayList<Expr>();
        for (Expr expr : shouldRead) {
            if (expr.shouldReadNow(justRead)) {
                result.add(expr);
            }
        }
        return result;
    }

    private List<Expr> getShouldRead() {
        return ExprModel.filterShouldRead(mExprModel.getPendingExpressions());
    }

    public static class User implements Observable {

        String name;

        String lastName;

        public final int finalField = 5;

        public static InnerStaticClass innerStaticInstance = new InnerStaticClass();

        public static final InnerStaticClass innerFinalStaticInstance = new InnerStaticClass();

        public SubObj subObj = new SubObj();

        public String getName() {
            return name;
        }

        public String getLastName() {
            return lastName;
        }

        public boolean getCond(int i) {
            return true;
        }

        public SubObj getAnotherSubObj() {
            return new SubObj();
        }

        public static boolean ourStaticMethod() {
            return true;
        }

        public String comment;

        @Bindable
        public boolean getUseComment() {
            return true;
        }

        @Override
        public void addOnPropertyChangedCallback(OnPropertyChangedCallback callback) {

        }

        @Override
        public void removeOnPropertyChangedCallback(OnPropertyChangedCallback callback) {

        }

        public static class InnerStaticClass {

            public static final int finalField = 3;

            public static final int finalStaticField = 3;
        }
    }

    public static class SubObj {

        public final int finalField = 5;
    }

}
