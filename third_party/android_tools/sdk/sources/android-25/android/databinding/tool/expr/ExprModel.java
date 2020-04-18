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

import android.databinding.tool.BindingTarget;
import android.databinding.tool.InverseBinding;
import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.reflection.ModelClass;
import android.databinding.tool.reflection.ModelMethod;
import android.databinding.tool.store.Location;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;
import android.databinding.tool.writer.FlagSet;

import org.antlr.v4.runtime.ParserRuleContext;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ExprModel {

    Map<String, Expr> mExprMap = new HashMap<String, Expr>();

    List<Expr> mBindingExpressions = new ArrayList<Expr>();

    private int mInvalidateableFieldLimit = 0;

    private int mRequirementIdCount = 0;

    // each arg list receives a unique id even if it is the same arguments and method.
    private int mArgListIdCounter = 0;

    private static final String TRUE_KEY_SUFFIX = "== true";
    private static final String FALSE_KEY_SUFFIX = "== false";

    /**
     * Any expression can be invalidated by invalidating this flag.
     */
    private BitSet mInvalidateAnyFlags;
    private int mInvalidateAnyFlagIndex;

    /**
     * Used by code generation. Keeps the list of expressions that are waiting to be evaluated.
     */
    private List<Expr> mPendingExpressions;

    /**
     * Used for converting flags into identifiers while debugging.
     */
    private String[] mFlagMapping;

    private BitSet mInvalidateableFlags;
    private BitSet mConditionalFlags;

    private int mFlagBucketCount;// how many buckets we use to identify flags

    private List<Expr> mObservables;

    private boolean mSealed = false;

    private Map<String, String> mImports = new HashMap<String, String>();

    private ParserRuleContext mCurrentParserContext;
    private Location mCurrentLocationInFile;
    /**
     * Adds the expression to the list of expressions and returns it.
     * If it already exists, returns existing one.
     *
     * @param expr The new parsed expression
     * @return The expression itself or another one if the same thing was parsed before
     */
    public <T extends Expr> T register(T expr) {
        Preconditions.check(!mSealed, "Cannot add expressions to a model after it is sealed");
        Location location = null;
        if (mCurrentParserContext != null) {
            location = new Location(mCurrentParserContext);
            location.setParentLocation(mCurrentLocationInFile);
        }
        T existing = (T) mExprMap.get(expr.getUniqueKey());
        if (existing != null) {
            Preconditions.check(expr.getParents().isEmpty(),
                    "If an expression already exists, it should've never been added to a parent,"
                            + "if thats the case, somewhere we are creating an expression w/o"
                            + "calling expression model");
            // tell the expr that it is being swapped so that if it was added to some other expr
            // as a parent, those can swap their references
            expr.onSwappedWith(existing);
            if (location != null) {
                existing.addLocation(location);
            }
            return existing;
        }
        mExprMap.put(expr.getUniqueKey(), expr);
        expr.setModel(this);
        if (location != null) {
            expr.addLocation(location);
        }
        return expr;
    }

    public void setCurrentParserContext(ParserRuleContext currentParserContext) {
        mCurrentParserContext = currentParserContext;
    }

    public Map<String, Expr> getExprMap() {
        return mExprMap;
    }

    public int size() {
        return mExprMap.size();
    }

    public ComparisonExpr comparison(String op, Expr left, Expr right) {
        return register(new ComparisonExpr(op, left, right));
    }

    public InstanceOfExpr instanceOfOp(Expr expr, String type) {
        return register(new InstanceOfExpr(expr, type));
    }

    public FieldAccessExpr field(Expr parent, String name) {
        return register(new FieldAccessExpr(parent, name));
    }

    public FieldAccessExpr observableField(Expr parent, String name) {
        return register(new FieldAccessExpr(parent, name, true));
    }

    public SymbolExpr symbol(String text, Class type) {
        return register(new SymbolExpr(text, type));
    }

    public TernaryExpr ternary(Expr pred, Expr ifTrue, Expr ifFalse) {
        return register(new TernaryExpr(pred, ifTrue, ifFalse));
    }

    public IdentifierExpr identifier(String name) {
        return register(new IdentifierExpr(name));
    }

    public StaticIdentifierExpr staticIdentifier(String name) {
        return register(new StaticIdentifierExpr(name));
    }

    public BuiltInVariableExpr builtInVariable(String name, String type, String accessCode) {
        return register(new BuiltInVariableExpr(name, type, accessCode));
    }

    public ViewFieldExpr viewFieldExpr(BindingTarget bindingTarget) {
        return register(new ViewFieldExpr(bindingTarget));
    }

    /**
     * Creates a static identifier for the given class or returns the existing one.
     */
    public StaticIdentifierExpr staticIdentifierFor(final ModelClass modelClass) {
        final String type = modelClass.getCanonicalName();
        // check for existing
        for (Expr expr : mExprMap.values()) {
            if (expr instanceof StaticIdentifierExpr) {
                StaticIdentifierExpr id = (StaticIdentifierExpr) expr;
                if (id.getUserDefinedType().equals(type)) {
                    return id;
                }
            }
        }

        // does not exist. Find a name for it.
        int cnt = 0;
        int dotIndex = type.lastIndexOf(".");
        String baseName;
        Preconditions.check(dotIndex < type.length() - 1, "Invalid type %s", type);
        if (dotIndex == -1) {
            baseName = type;
        } else {
            baseName = type.substring(dotIndex + 1);
        }
        while (true) {
            String candidate = cnt == 0 ? baseName : baseName + cnt;
            if (!mImports.containsKey(candidate)) {
                return addImport(candidate, type, null);
            }
            cnt ++;
            Preconditions.check(cnt < 100, "Failed to create an import for " + type);
        }
    }

    public MethodCallExpr methodCall(Expr target, String name, List<Expr> args) {
        return register(new MethodCallExpr(target, name, args));
    }

    public MathExpr math(Expr left, String op, Expr right) {
        return register(new MathExpr(left, op, right));
    }

    public TernaryExpr logical(Expr left, String op, Expr right) {
        if ("&&".equals(op)) {
            // left && right
            // left ? right : false
            return register(new TernaryExpr(left, right, symbol("false", boolean.class)));
        } else {
            // left || right
            // left ? true : right
            return register(new TernaryExpr(left, symbol("true", boolean.class), right));
        }
    }

    public BitShiftExpr bitshift(Expr left, String op, Expr right) {
        return register(new BitShiftExpr(left, op, right));
    }

    public UnaryExpr unary(String op, Expr expr) {
        return register(new UnaryExpr(op, expr));
    }

    public Expr group(Expr grouped) {
        return register(new GroupExpr(grouped));
    }

    public Expr resourceExpr(String packageName, String resourceType, String resourceName,
            List<Expr> args) {
        return register(new ResourceExpr(packageName, resourceType, resourceName, args));
    }

    public Expr bracketExpr(Expr variableExpr, Expr argExpr) {
        return register(new BracketExpr(variableExpr, argExpr));
    }

    public Expr castExpr(String type, Expr expr) {
        return register(new CastExpr(type, expr));
    }

    public TwoWayListenerExpr twoWayListenerExpr(InverseBinding inverseBinding) {
        return register(new TwoWayListenerExpr(inverseBinding));
    }
    public List<Expr> getBindingExpressions() {
        return mBindingExpressions;
    }

    public StaticIdentifierExpr addImport(String alias, String type, Location location) {
        Preconditions.check(!mImports.containsKey(alias),
                "%s has already been defined as %s", alias, type);
        final StaticIdentifierExpr id = staticIdentifier(alias);
        L.d("adding import %s as %s klass: %s", type, alias, id.getClass().getSimpleName());
        id.setUserDefinedType(type);
        if (location != null) {
            id.addLocation(location);
        }
        mImports.put(alias, type);
        return id;
    }

    public Map<String, String> getImports() {
        return mImports;
    }

    /**
     * The actual thingy that is set on the binding target.
     *
     * Input must be already registered
     */
    public Expr bindingExpr(Expr bindingExpr) {
        Preconditions.check(mExprMap.containsKey(bindingExpr.getUniqueKey()),
                "Main expression should already be registered");
        if (!mBindingExpressions.contains(bindingExpr)) {
            mBindingExpressions.add(bindingExpr);
        }
        return bindingExpr;
    }

    public void removeExpr(Expr expr) {
        Preconditions.check(!mSealed, "Can't modify the expression list after sealing the model.");
        mBindingExpressions.remove(expr);
        mExprMap.remove(expr.computeUniqueKey());
    }

    public List<Expr> getObservables() {
        return mObservables;
    }

    /**
     * Give id to each expression. Will be useful if we serialize.
     */
    public void seal() {
        L.d("sealing model");
        List<Expr> notifiableExpressions = new ArrayList<Expr>();
        //ensure class analyzer. We need to know observables at this point
        final ModelAnalyzer modelAnalyzer = ModelAnalyzer.getInstance();
        updateExpressions(modelAnalyzer);

        int counter = 0;
        final Iterable<Expr> observables = filterObservables(modelAnalyzer);
        List<String> flagMapping = new ArrayList<String>();
        mObservables = new ArrayList<Expr>();
        for (Expr expr : observables) {
            // observables gets initial ids
            flagMapping.add(expr.getUniqueKey());
            expr.setId(counter++);
            mObservables.add(expr);
            notifiableExpressions.add(expr);
            L.d("observable %s", expr.getUniqueKey());
        }

        // non-observable identifiers gets next ids
        final Iterable<Expr> nonObservableIds = filterNonObservableIds(modelAnalyzer);
        for (Expr expr : nonObservableIds) {
            flagMapping.add(expr.getUniqueKey());
            expr.setId(counter++);
            notifiableExpressions.add(expr);
            L.d("non-observable %s", expr.getUniqueKey());
        }

        // descendants of observables gets following ids
        for (Expr expr : observables) {
            for (Expr parent : expr.getParents()) {
                if (parent.hasId()) {
                    continue;// already has some id, means observable
                }
                // only fields earn an id
                if (parent instanceof FieldAccessExpr) {
                    FieldAccessExpr fae = (FieldAccessExpr) parent;
                    L.d("checking field access expr %s. getter: %s", fae,fae.getGetter());
                    if (fae.isDynamic() && fae.getGetter().canBeInvalidated()) {
                        flagMapping.add(parent.getUniqueKey());
                        parent.setId(counter++);
                        notifiableExpressions.add(parent);
                        L.d("notifiable field %s : %s for %s : %s", parent.getUniqueKey(),
                                Integer.toHexString(System.identityHashCode(parent)),
                                expr.getUniqueKey(),
                                Integer.toHexString(System.identityHashCode(expr)));
                    }
                }
            }
        }

        // now all 2-way bound view fields
        for (Expr expr : mExprMap.values()) {
            if (expr instanceof FieldAccessExpr) {
                FieldAccessExpr fieldAccessExpr = (FieldAccessExpr) expr;
                if (fieldAccessExpr.getChild() instanceof ViewFieldExpr) {
                    flagMapping.add(fieldAccessExpr.getUniqueKey());
                    fieldAccessExpr.setId(counter++);
                }
            }
        }

        // non-dynamic binding expressions receive some ids so that they can be invalidated
        L.d("list of binding expressions");
        for (int i = 0; i < mBindingExpressions.size(); i++) {
            L.d("[%d] %s", i, mBindingExpressions.get(i));
        }
        // we don't assign ids to constant binding expressions because now invalidateAll has its own
        // flag.

        for (Expr expr : notifiableExpressions) {
            expr.enableDirectInvalidation();
        }

        // make sure all dependencies are resolved to avoid future race conditions
        for (Expr expr : mExprMap.values()) {
            expr.getDependencies();
        }
        mInvalidateAnyFlagIndex = counter ++;
        flagMapping.add("INVALIDATE ANY");
        mInvalidateableFieldLimit = counter;
        mInvalidateableFlags = new BitSet();
        for (int i = 0; i < mInvalidateableFieldLimit; i++) {
            mInvalidateableFlags.set(i, true);
        }

        // make sure all dependencies are resolved to avoid future race conditions
        for (Expr expr : mExprMap.values()) {
            if (expr.isConditional()) {
                L.d("requirement id for %s is %d", expr, counter);
                expr.setRequirementId(counter);
                flagMapping.add(expr.getUniqueKey() + FALSE_KEY_SUFFIX);
                flagMapping.add(expr.getUniqueKey() + TRUE_KEY_SUFFIX);
                counter += 2;
            }
        }
        mConditionalFlags = new BitSet();
        for (int i = mInvalidateableFieldLimit; i < counter; i++) {
            mConditionalFlags.set(i, true);
        }
        mRequirementIdCount = (counter - mInvalidateableFieldLimit) / 2;

        // everybody gets an id
        for (Map.Entry<String, Expr> entry : mExprMap.entrySet()) {
            final Expr value = entry.getValue();
            if (!value.hasId()) {
                value.setId(counter++);
            }
        }

        mFlagMapping = new String[flagMapping.size()];
        flagMapping.toArray(mFlagMapping);

        mFlagBucketCount = 1 + (getTotalFlagCount() / FlagSet.sBucketSize);
        mInvalidateAnyFlags = new BitSet();
        mInvalidateAnyFlags.set(mInvalidateAnyFlagIndex, true);

        for (Expr expr : mExprMap.values()) {
            expr.getShouldReadFlagsWithConditionals();
        }

        for (Expr expr : mExprMap.values()) {
            // ensure all types are calculated
            expr.getResolvedType();
        }

        mSealed = true;
    }

    /**
     * Run updateExpr on each binding expression until no new expressions are added.
     * <p>
     * Some expressions (e.g. field access) may replace themselves and add/remove new dependencies
     * so we need to make sure each expression's update is called at least once.
     */
    private void updateExpressions(ModelAnalyzer modelAnalyzer) {
        int startSize = -1;
        while (startSize != mExprMap.size()) {
            startSize = mExprMap.size();
            ArrayList<Expr> exprs = new ArrayList<Expr>(mBindingExpressions);
            for (Expr expr : exprs) {
                expr.updateExpr(modelAnalyzer);
            }
        }
    }

    public int getFlagBucketCount() {
        return mFlagBucketCount;
    }

    public int getTotalFlagCount() {
        return mRequirementIdCount * 2 + mInvalidateableFieldLimit;
    }

    public int getInvalidateableFieldLimit() {
        return mInvalidateableFieldLimit;
    }

    public String[] getFlagMapping() {
        return mFlagMapping;
    }

    public String getFlag(int id) {
        return mFlagMapping[id];
    }

    private List<Expr> filterNonObservableIds(final ModelAnalyzer modelAnalyzer) {
        List<Expr> result = new ArrayList<Expr>();
        for (Expr input : mExprMap.values()) {
            if (input instanceof IdentifierExpr
                    && !input.hasId()
                    && !input.isObservable()
                    && input.isDynamic()) {
                result.add(input);
            }
        }
        return result;
    }

    private Iterable<Expr> filterObservables(final ModelAnalyzer modelAnalyzer) {
        List<Expr> result = new ArrayList<Expr>();
        for (Expr input : mExprMap.values()) {
            if (input.isObservable()) {
                result.add(input);
            }
        }
        return result;
    }

    public List<Expr> getPendingExpressions() {
        if (mPendingExpressions == null) {
            mPendingExpressions = new ArrayList<Expr>();
            for (Expr expr : mExprMap.values()) {
                // if an expression is NOT dynanic but has conditional dependants, still return it
                // so that conditional flags can be set
                if (!expr.isRead() && (expr.isDynamic() || expr.hasConditionalDependant())) {
                    mPendingExpressions.add(expr);
                }
            }
        }
        return mPendingExpressions;
    }

    public boolean markBitsRead() {
        // each has should read flags, we set them back on them
        List<Expr> markedSomeFlagsRead = new ArrayList<Expr>();
        for (Expr expr : filterShouldRead(getPendingExpressions())) {
            expr.markFlagsAsRead(expr.getShouldReadFlags());
            markedSomeFlagsRead.add(expr);
        }
        return pruneDone(markedSomeFlagsRead);
    }

    private boolean pruneDone(List<Expr> markedSomeFlagsAsRead) {
        boolean marked = true;
        List<Expr> markedAsReadList = new ArrayList<Expr>();
        while (marked) {
            marked = false;
            for (Expr expr : mExprMap.values()) {
                if (expr.isRead()) {
                    continue;
                }
                if (expr.markAsReadIfDone()) {
                    L.d("marked %s as read ", expr.getUniqueKey());
                    marked = true;
                    markedAsReadList.add(expr);
                    markedSomeFlagsAsRead.remove(expr);
                }
            }
        }
        boolean elevated = false;
        for (Expr markedAsRead : markedAsReadList) {
            for (Dependency dependency : markedAsRead.getDependants()) {
                if (dependency.getDependant().considerElevatingConditionals(markedAsRead)) {
                    elevated = true;
                }
            }
        }
        for (Expr partialRead : markedSomeFlagsAsRead) {
            // even if all paths are not satisfied, we can elevate certain conditional dependencies
            // if all of their paths are satisfied.
            for (Dependency dependency : partialRead.getDependants()) {
                Expr dependant = dependency.getDependant();
                if (dependant.isConditional() && dependant.getAllCalculationPaths()
                        .areAllPathsSatisfied(partialRead.mReadSoFar)) {
                    if (dependant.considerElevatingConditionals(partialRead)) {
                        elevated = true;
                    }
                }
            }
        }
        if (elevated) {
            // some conditionals are elevated. We should re-calculate flags
            for (Expr expr : getPendingExpressions()) {
                if (!expr.isRead()) {
                    expr.invalidateReadFlags();
                }
            }
            mPendingExpressions = null;
        }
        return elevated;
    }

    private static boolean hasConditionalOrNestedCannotReadDependency(Expr expr) {
        for (Dependency dependency : expr.getDependencies()) {
            if (dependency.isConditional() || dependency.getOther().hasNestedCannotRead()) {
                return true;
            }
        }
        return false;
    }

    public static List<Expr> filterShouldRead(Iterable<Expr> exprs) {
        List<Expr> result = new ArrayList<Expr>();
        for (Expr expr : exprs) {
            if (!expr.getShouldReadFlags().isEmpty() &&
                    !hasConditionalOrNestedCannotReadDependency(expr)) {
                result.add(expr);
            }
        }
        return result;
    }

    /**
     * May return null if flag is equal to invalidate any flag.
     */
    public Expr findFlagExpression(int flag) {
        if (mInvalidateAnyFlags.get(flag)) {
            return null;
        }
        final String key = mFlagMapping[flag];
        if (mExprMap.containsKey(key)) {
            return mExprMap.get(key);
        }
        int falseIndex = key.indexOf(FALSE_KEY_SUFFIX);
        if (falseIndex > -1) {
            final String trimmed = key.substring(0, falseIndex);
            return mExprMap.get(trimmed);
        }
        int trueIndex = key.indexOf(TRUE_KEY_SUFFIX);
        if (trueIndex > -1) {
            final String trimmed = key.substring(0, trueIndex);
            return mExprMap.get(trimmed);
        }
        // log everything we call
        StringBuilder error = new StringBuilder();
        error.append("cannot find flag:").append(flag).append("\n");
        error.append("invalidate any flag:").append(mInvalidateAnyFlags).append("\n");
        error.append("key:").append(key).append("\n");
        error.append("flag mapping:").append(Arrays.toString(mFlagMapping));
        L.e(error.toString());
        return null;
    }

    public BitSet getInvalidateAnyBitSet() {
        return mInvalidateAnyFlags;
    }

    public int getInvalidateAnyFlagIndex() {
        return mInvalidateAnyFlagIndex;
    }

    public Expr argListExpr(Iterable<Expr> expressions) {
        return register(new ArgListExpr(mArgListIdCounter ++, expressions));
    }

    public void setCurrentLocationInFile(Location location) {
        mCurrentLocationInFile = location;
    }

    public Expr listenerExpr(Expr expression, String name, ModelClass listenerType,
            ModelMethod listenerMethod) {
        return register(new ListenerExpr(expression, name, listenerType, listenerMethod));
    }
}
