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

import org.antlr.v4.runtime.misc.Nullable;

import android.databinding.tool.processing.ErrorMessages;
import android.databinding.tool.processing.Scope;
import android.databinding.tool.processing.scopes.LocationScopeProvider;
import android.databinding.tool.reflection.ModelAnalyzer;
import android.databinding.tool.reflection.ModelClass;
import android.databinding.tool.store.Location;
import android.databinding.tool.util.L;
import android.databinding.tool.util.Preconditions;
import android.databinding.tool.writer.KCode;
import android.databinding.tool.writer.LayoutBinderWriterKt;

import java.util.ArrayList;
import java.util.BitSet;
import java.util.Collections;
import java.util.List;
import java.util.Map;

abstract public class Expr implements VersionProvider, LocationScopeProvider {

    public static final int NO_ID = -1;
    protected List<Expr> mChildren = new ArrayList<Expr>();

    // any expression that refers to this. Useful if this expr is duplicate and being replaced
    private List<Expr> mParents = new ArrayList<Expr>();

    private Boolean mIsDynamic;

    private ModelClass mResolvedType;

    private String mUniqueKey;

    private List<Dependency> mDependencies;

    private List<Dependency> mDependants = new ArrayList<Dependency>();

    private int mId = NO_ID;

    private int mRequirementId = NO_ID;

    private int mVersion = 0;

    // means this expression can directly be invalidated by the user
    private boolean mCanBeInvalidated = false;

    @Nullable
    private List<Location> mLocations = new ArrayList<Location>();

    /**
     * This set denotes the times when this expression is invalid.
     * If it is an Identifier expression, it is its index
     * If it is a composite expression, it is the union of invalid flags of its descendants
     */
    private BitSet mInvalidFlags;

    /**
     * Set when this expression is registered to a model
     */
    private ExprModel mModel;

    /**
     * This set denotes the times when this expression must be read.
     *
     * It is the union of invalidation flags of all of its non-conditional dependants.
     */
    BitSet mShouldReadFlags;

    BitSet mReadSoFar = new BitSet();// i've read this variable for these flags

    /**
     * calculated on initialization, assuming all conditionals are true
     */
    BitSet mShouldReadWithConditionals;

    private boolean mIsBindingExpression;

    /**
     * Used by generators when this expression is resolved.
     */
    private boolean mRead;
    private boolean mIsUsed = false;
    private boolean mIsTwoWay = false;

    Expr(Iterable<Expr> children) {
        for (Expr expr : children) {
            mChildren.add(expr);
        }
        addParents();
    }

    Expr(Expr... children) {
        Collections.addAll(mChildren, children);
        addParents();
    }

    public int getId() {
        Preconditions.check(mId != NO_ID, "if getId is called on an expression, it should have"
                + " an id: %s", this);
        return mId;
    }

    public void setId(int id) {
        Preconditions.check(mId == NO_ID, "ID is already set on %s", this);
        mId = id;
    }

    public void addLocation(Location location) {
        mLocations.add(location);
    }

    public List<Location> getLocations() {
        return mLocations;
    }

    public ExprModel getModel() {
        return mModel;
    }

    public BitSet getInvalidFlags() {
        if (mInvalidFlags == null) {
            mInvalidFlags = resolveInvalidFlags();
        }
        return mInvalidFlags;
    }

    private BitSet resolveInvalidFlags() {
        BitSet bitSet = (BitSet) mModel.getInvalidateAnyBitSet().clone();
        if (mCanBeInvalidated) {
            bitSet.set(getId(), true);
        }
        for (Dependency dependency : getDependencies()) {
            // TODO optional optimization: do not invalidate for conditional flags
            bitSet.or(dependency.getOther().getInvalidFlags());
        }
        return bitSet;
    }

    public void setBindingExpression(boolean isBindingExpression) {
        mIsBindingExpression = isBindingExpression;
    }

    public boolean isBindingExpression() {
        return mIsBindingExpression;
    }

    public boolean canBeEvaluatedToAVariable() {
        return true; // anything except arg expr can be evaluated to a variable
    }

    public boolean isObservable() {
        return getResolvedType().isObservable();
    }

    public Expr resolveListeners(ModelClass valueType, Expr parent) {
        for (int i = mChildren.size() - 1; i >= 0; i--) {
            Expr child = mChildren.get(i);
            child.resolveListeners(valueType, this);
        }
        resetResolvedType();
        return this;
    }

    public Expr resolveTwoWayExpressions(Expr parent) {
        for (int i = mChildren.size() - 1; i >= 0; i--) {
            final Expr child = mChildren.get(i);
            child.resolveTwoWayExpressions(this);
        }
        return this;
    }

    protected void resetResolvedType() {
        mResolvedType = null;
    }

    public BitSet getShouldReadFlags() {
        if (mShouldReadFlags == null) {
            getShouldReadFlagsWithConditionals();
            mShouldReadFlags = resolveShouldReadFlags();
        }
        return mShouldReadFlags;
    }

    public BitSet getShouldReadFlagsWithConditionals() {
        if (mShouldReadWithConditionals == null) {
            mShouldReadWithConditionals = resolveShouldReadWithConditionals();
        }
        return mShouldReadWithConditionals;
    }

    public void setModel(ExprModel model) {
        mModel = model;
    }

    public void setTwoWay(boolean isTwoWay) {
        mIsTwoWay = isTwoWay;
    }

    public boolean isTwoWay() {
        return mIsTwoWay;
    }

    protected String addTwoWay(String uniqueKey) {
        if (mIsTwoWay) {
            return "twoWay(" + uniqueKey + ")";
        } else {
            return "oneWay(" + uniqueKey + ")";
        }
    }

    private BitSet resolveShouldReadWithConditionals() {
        // ensure we have invalid flags
        BitSet bitSet = new BitSet();
        // if i'm invalid, that DOES NOT mean i should be read :/.
        if (mIsBindingExpression) {
            bitSet.or(getInvalidFlags());
        }

        for (Dependency dependency : getDependants()) {
            if (dependency.getCondition() == null) {
                bitSet.or(dependency.getDependant().getShouldReadFlagsWithConditionals());
            } else {
                bitSet.set(dependency.getDependant()
                        .getRequirementFlagIndex(dependency.getExpectedOutput()));
            }
        }
        return bitSet;
    }

    private BitSet resolveShouldReadFlags() {
        // ensure we have invalid flags
        BitSet bitSet = new BitSet();
        if (isRead()) {
            return bitSet;
        }
        if (mIsBindingExpression) {
            bitSet.or(getInvalidFlags());
        }
        for (Dependency dependency : getDependants()) {
            final boolean isUnreadElevated = isUnreadElevated(dependency);
            if (dependency.isConditional()) {
                continue; // will be resolved later when conditional is elevated
            }
            if (isUnreadElevated) {
                bitSet.set(dependency.getDependant()
                        .getRequirementFlagIndex(dependency.getExpectedOutput()));
            } else {
                bitSet.or(dependency.getDependant().getShouldReadFlags());
            }
        }
        bitSet.and(mShouldReadWithConditionals);
        bitSet.andNot(mReadSoFar);
        return bitSet;
    }

    private static boolean isUnreadElevated(Dependency input) {
        return input.isElevated() && !input.getDependant().isRead();
    }
    private void addParents() {
        for (Expr expr : mChildren) {
            expr.mParents.add(this);
        }
    }

    public void onSwappedWith(Expr existing) {
        for (Expr child : mChildren) {
            child.onParentSwapped(this, existing);
        }
    }

    private void onParentSwapped(Expr oldParent, Expr newParent) {
        Preconditions.check(mParents.remove(oldParent), "trying to remove non-existent parent %s"
                + " from %s", oldParent, mParents);
        mParents.add(newParent);
    }

    public List<Expr> getChildren() {
        return mChildren;
    }

    public List<Expr> getParents() {
        return mParents;
    }

    /**
     * Whether the result of this expression can change or not.
     *
     * For example, 3 + 5 can not change vs 3 + x may change.
     *
     * Default implementations checks children and returns true if any of them returns true
     *
     * @return True if the result of this expression may change due to variables
     */
    public boolean isDynamic() {
        if (mIsDynamic == null) {
            mIsDynamic = isAnyChildDynamic();
        }
        return mIsDynamic;
    }

    private boolean isAnyChildDynamic() {
        for (Expr expr : mChildren) {
            if (expr.isDynamic()) {
                return true;
            }
        }
        return false;
    }

    public ModelClass getResolvedType() {
        if (mResolvedType == null) {
            // TODO not get instance
            try {
                Scope.enter(this);
                mResolvedType = resolveType(ModelAnalyzer.getInstance());
                if (mResolvedType == null) {
                    L.e(ErrorMessages.CANNOT_RESOLVE_TYPE, this);
                }
            } finally {
                Scope.exit();
            }
        }
        return mResolvedType;
    }

    abstract protected ModelClass resolveType(ModelAnalyzer modelAnalyzer);

    abstract protected List<Dependency> constructDependencies();

    /**
     * Creates a dependency for each dynamic child. Should work for any expression besides
     * conditionals.
     */
    protected List<Dependency> constructDynamicChildrenDependencies() {
        List<Dependency> dependencies = new ArrayList<Dependency>();
        for (Expr node : mChildren) {
            if (!node.isDynamic()) {
                continue;
            }
            dependencies.add(new Dependency(this, node));
        }
        return dependencies;
    }

    public final List<Dependency> getDependencies() {
        if (mDependencies == null) {
            mDependencies = constructDependencies();
        }
        return mDependencies;
    }

    void addDependant(Dependency dependency) {
        mDependants.add(dependency);
    }

    public List<Dependency> getDependants() {
        return mDependants;
    }

    protected static final String KEY_JOIN = "~";

    /**
     * Returns a unique string key that can identify this expression.
     *
     * It must take into account any dependencies
     *
     * @return A unique identifier for this expression
     */
    public final String getUniqueKey() {
        if (mUniqueKey == null) {
            mUniqueKey = computeUniqueKey();
            Preconditions.checkNotNull(mUniqueKey,
                    "if there are no children, you must override computeUniqueKey");
            Preconditions.check(!mUniqueKey.trim().equals(""),
                    "if there are no children, you must override computeUniqueKey");
        }
        return mUniqueKey;
    }

    protected String computeUniqueKey() {
        return computeChildrenKey();
    }

    protected final String computeChildrenKey() {
        return join(mChildren);
    }

    public void enableDirectInvalidation() {
        mCanBeInvalidated = true;
    }

    public boolean canBeInvalidated() {
        return mCanBeInvalidated;
    }

    public void trimShouldReadFlags(BitSet bitSet) {
        mShouldReadFlags.andNot(bitSet);
    }

    public boolean isConditional() {
        return false;
    }

    public int getRequirementId() {
        return mRequirementId;
    }

    public void setRequirementId(int requirementId) {
        mRequirementId = requirementId;
    }

    /**
     * This is called w/ a dependency of mine.
     * Base method should thr
     */
    public int getRequirementFlagIndex(boolean expectedOutput) {
        Preconditions.check(mRequirementId != NO_ID, "If this is an expression w/ conditional"
                + " dependencies, it must be assigned a requirement ID. %s", this);
        return expectedOutput ? mRequirementId + 1 : mRequirementId;
    }

    public boolean hasId() {
        return mId != NO_ID;
    }

    public void markFlagsAsRead(BitSet flags) {
        mReadSoFar.or(flags);
    }

    public boolean isRead() {
        return mRead;
    }

    public boolean considerElevatingConditionals(Expr justRead) {
        boolean elevated = false;
        for (Dependency dependency : mDependencies) {
            if (dependency.isConditional() && dependency.getCondition() == justRead) {
                dependency.elevate();
                elevated = true;
            }
        }
        return elevated;
    }

    public void invalidateReadFlags() {
        mShouldReadFlags = null;
        mVersion ++;
    }

    @Override
    public int getVersion() {
        return mVersion;
    }

    public boolean hasNestedCannotRead() {
        if (isRead()) {
            return false;
        }
        if (getShouldReadFlags().isEmpty()) {
            return true;
        }
        for (Dependency dependency : getDependencies()) {
            if (hasNestedCannotRead(dependency)) {
                return true;
            }
        }
        return false;
    }

    private static boolean hasNestedCannotRead(Dependency input) {
        return input.isConditional() || input.getOther().hasNestedCannotRead();
    }

    public boolean markAsReadIfDone() {
        if (mRead) {
            return false;
        }
        // TODO avoid clone, we can calculate this iteratively
        BitSet clone = (BitSet) mShouldReadWithConditionals.clone();

        clone.andNot(mReadSoFar);
        mRead = clone.isEmpty();

        if (!mRead && !mReadSoFar.isEmpty()) {
            // check if remaining dependencies can be satisfied w/ existing values
            // for predicate flags, this expr may already be calculated to get the predicate
            // to detect them, traverse them later on, see which flags should be calculated to calculate
            // them. If any of them is completely covered w/ our non-conditional flags, no reason
            // to add them to the list since we'll already be calculated due to our non-conditional
            // flags
            boolean allCovered = true;
            for (int i = clone.nextSetBit(0); i != -1; i = clone.nextSetBit(i + 1)) {
                final Expr expr = mModel.findFlagExpression(i);
                if (expr == null) {
                    continue;
                }
                if (!expr.isConditional()) {
                    allCovered = false;
                    break;
                }
                final BitSet readForConditional = (BitSet) expr.findConditionalFlags().clone();

                // FIXME: this does not do full traversal so misses some cases
                // to calculate that conditional, i should've read /readForConditional/ flags
                // if my read-so-far bits cover that; that means i would've already
                // read myself
                readForConditional.andNot(mReadSoFar);
                if (!readForConditional.isEmpty()) {
                    allCovered = false;
                    break;
                }
            }
            mRead = allCovered;
        }
        if (mRead) {
            mShouldReadFlags = null; // if we've been marked as read, clear should read flags
        }
        return mRead;
    }

    BitSet mConditionalFlags;

    private BitSet findConditionalFlags() {
        Preconditions.check(isConditional(), "should not call this on a non-conditional expr");
        if (mConditionalFlags == null) {
            mConditionalFlags = new BitSet();
            resolveConditionalFlags(mConditionalFlags);
        }
        return mConditionalFlags;
    }

    private void resolveConditionalFlags(BitSet flags) {
        flags.or(getPredicateInvalidFlags());
        // if i have only 1 dependency which is conditional, traverse it as well
        if (getDependants().size() == 1) {
            final Dependency dependency = getDependants().get(0);
            if (dependency.getCondition() != null) {
                flags.or(dependency.getDependant().findConditionalFlags());
                flags.set(dependency.getDependant()
                        .getRequirementFlagIndex(dependency.getExpectedOutput()));
            }
        }
    }


    @Override
    public String toString() {
        return getUniqueKey();
    }

    public BitSet getReadSoFar() {
        return mReadSoFar;
    }

    private Node mCalculationPaths = null;

    /**
     * All flag paths that will result in calculation of this expression.
     */
    protected Node getAllCalculationPaths() {
        if (mCalculationPaths == null) {
            Node node = new Node();
            if (isConditional()) {
                node.mBitSet.or(getPredicateInvalidFlags());
            } else {
                node.mBitSet.or(getInvalidFlags());
            }
            for (Dependency dependency : getDependants()) {
                final Expr dependant = dependency.getDependant();
                if (dependency.getCondition() != null) {
                    Node cond = new Node();
                    cond.setConditionFlag(
                            dependant.getRequirementFlagIndex(dependency.getExpectedOutput()));
                    cond.mParents.add(dependant.getAllCalculationPaths());
                    node.mParents.add(cond);
                } else {
                    node.mParents.add(dependant.getAllCalculationPaths());
                }
            }
            mCalculationPaths = node;
        }
        return mCalculationPaths;
    }

    public String getDefaultValue() {
        return ModelAnalyzer.getInstance().getDefaultValue(getResolvedType().toJavaCode());
    }

    protected BitSet getPredicateInvalidFlags() {
        throw new IllegalStateException(
                "must override getPredicateInvalidFlags in " + getClass().getSimpleName());
    }

    /**
     * Used by code generation
     */
    public boolean shouldReadNow(final List<Expr> justRead) {
        if (getShouldReadFlags().isEmpty()) {
            return false;
        }
        for (Dependency input : getDependencies()) {
            boolean dependencyReady = input.getOther().isRead() || (justRead != null &&
                    justRead.contains(input.getOther()));
            if(!dependencyReady) {
                return false;
            }
        }
        return true;
    }

    public boolean isEqualityCheck() {
        return false;
    }

    public void setIsUsed(boolean isUsed) {
        mIsUsed = isUsed;
        for (Expr child : getChildren()) {
            child.setIsUsed(isUsed);
        }
    }

    public boolean isUsed() {
        return mIsUsed;
    }

    public void updateExpr(ModelAnalyzer modelAnalyzer) {
        final Map<String, Expr> exprMap = mModel.getExprMap();
        for (int i = mParents.size() - 1; i >= 0; i--) {
            final Expr parent = mParents.get(i);
            if (exprMap.get(parent.getUniqueKey()) != parent) {
                mParents.remove(i);
            }
        }
        for (Expr child : mChildren) {
            child.updateExpr(modelAnalyzer);
        }
    }

    protected static String join(String... items) {
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < items.length; i ++) {
            if (i > 0) {
                result.append(KEY_JOIN);
            }
            result.append(items[i]);
        }
        return result.toString();
    }

    protected static String join(List<Expr> items) {
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < items.size(); i ++) {
            if (i > 0) {
                result.append(KEY_JOIN);
            }
            result.append(items.get(i).getUniqueKey());
        }
        return result.toString();
    }

    protected String asPackage() {
        return null;
    }

    @Override
    public List<Location> provideScopeLocation() {
        return mLocations;
    }

    public KCode toCode() {
        return toCode(false);
    }

    protected KCode toCode(boolean expand) {
        if (!expand && isDynamic()) {
            return new KCode(LayoutBinderWriterKt.getExecutePendingLocalName(this));
        }
        return generateCode(expand);
    }

    public KCode toFullCode() {
        return generateCode(false);
    }

    protected abstract KCode generateCode(boolean expand);

    public KCode toInverseCode(KCode value) {
        throw new IllegalStateException("expression does not support two-way binding");
    }

    public void assertIsInvertible() {
        final String errorMessage = getInvertibleError();
        if (errorMessage != null) {
            L.e(ErrorMessages.EXPRESSION_NOT_INVERTIBLE, toFullCode().generate(),
                    errorMessage);
        }
    }

    /**
     * @return The reason the expression wasn't invertible or null if it was invertible.
     */
    protected abstract String getInvertibleError();

    /**
     * This expression is the predicate for 1 or more ternary expressions.
     */
    public boolean hasConditionalDependant() {
        for (Dependency dependency : getDependants()) {
            Expr dependant = dependency.getDependant();
            if (dependant.isConditional() && dependant instanceof TernaryExpr) {
                TernaryExpr ternary = (TernaryExpr) dependant;
                return ternary.getPred() == this;
            }
        }
        return false;
    }

    static class Node {

        BitSet mBitSet = new BitSet();
        List<Node> mParents = new ArrayList<Node>();
        int mConditionFlag = -1;

        public boolean areAllPathsSatisfied(BitSet readSoFar) {
            if (mConditionFlag != -1) {
                return readSoFar.get(mConditionFlag)
                        || mParents.get(0).areAllPathsSatisfied(readSoFar);
            } else {
                final BitSet myBitsClone = (BitSet) mBitSet.clone();
                myBitsClone.andNot(readSoFar);
                if (!myBitsClone.isEmpty()) {
                    // read so far does not cover all of my invalidation. The only way I could be
                    // covered is that I only have 1 conditional dependent which is covered by this.
                    if (mParents.size() == 1 && mParents.get(0).mConditionFlag != -1) {
                        return mParents.get(0).areAllPathsSatisfied(readSoFar);
                    }
                    return false;
                }
                if (mParents.isEmpty()) {
                    return true;
                }
                for (Node parent : mParents) {
                    if (!parent.areAllPathsSatisfied(readSoFar)) {
                        return false;
                    }
                }
                return true;
            }
        }

        public void setConditionFlag(int requirementFlagIndex) {
            mConditionFlag = requirementFlagIndex;
        }
    }
}
