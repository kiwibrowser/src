/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define DEBUG_THIS 0
/*
 * Define as 1 to get debugging output.  Compiler's dead code
 * elimination should remove debug statements when defined as 0.
 */

#include <stdio.h>  /* tree print */

#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_multiset.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_range_tree.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_range_tree_intern.h"

struct NaClIntervalRangeTreeNode {
  struct NaClIntervalRangeTreeNode *left;
  struct NaClIntervalRangeTreeNode *right;
  int balance;  /* AVL height balance: -1, 0, 1 */

  uint32_t value;
  /* the value at this interval end point */

  uint32_t value_containment_delta;
  /*
   * Each interval that uses |value| as the start point would
   * increment |value_containment_delta| by 1; and each range that
   * uses it as the end point would decrement it by 1.  Can never be
   * negative, but may be zero, e.g., [x,y] [y,z] would make y's
   * value_containment_delta be zero.  We cannot distinguish
   * [x,y][y,z] from [x1,y][x2,y][y,z1][y,z2] when looking at y.
   */

  uint32_t refcount;
  /*
   * We could keep right_delta and left_delta for counting the number
   * of ranges that end and start at this value respectively, but that
   * would require a subtraction whenever we re-computed range
   * information.  Keeping a summed delta and a refcount simplifies
   * the updates, at the cost of a reduced maximum number of
   * (overlapped) ranges.  If we didn't do either of the above, then
   * overlaps could cause the summed delta to be zero after a delete,
   * and we cannot determine whether we can actually remove the tree
   * node or not.
   */

  /*
   * Cumulative information about the subtree rooted at this node.
   */
  uint32_t range_left;
  uint32_t range_right;
  /* invariant: range_left <= value <= range_right */
  int32_t subtree_containment_delta;
  /*
   * The subtree_containment_delta is the number of intervals that a
   * point would be contained in, if a probe point were moved from
   * just to the left of this subtree to just the right of the
   * subtree, i.e., from just less than range_left to just greater
   * than range_right.
   *
   * subtree_containment_delta = value_containment_delta +
   *  (NULL != left) ? left->subtree_containment_delta : 0 +
   *  (NULL != right) ? right->subtree_containment_delta : 0;
   */
};

struct NaClIntervalMultisetVtbl const kNaClIntervalRangeTreeVtbl;  /* fwd */

static INLINE void indent(int depth) {
  while (depth != 0) {
    putchar(' ');
    --depth;
  }
}

static void TreePrint(struct NaClIntervalRangeTreeNode *node,
                      int                              depth) {
  if (NULL == node) {
    return;
  }
  /* rotated head view */
  TreePrint(node->right, depth + 1);
  indent(depth);
  printf("v=%d [b=%d, d=%d, rl=%u, rr=%u, dd=%d]\n",
         node->value, node->balance, node->value_containment_delta,
         node->range_left, node->range_right, node->subtree_containment_delta);
  TreePrint(node->left, depth + 1);
}

static INLINE void NaClIntervalRangeTreePrint(
    struct NaClIntervalRangeTreeNode *node) {
  if (!DEBUG_THIS) {
    return;
  }
  putchar('\n');
  TreePrint(node, 0);
  putchar('\n');
}

#define NaCl_dprintf(alist) do { if (DEBUG_THIS) printf alist; } while (0)

static void NaClIntervalRangeTreeNodeInit(
    struct NaClIntervalRangeTreeNode *node,
    uint32_t value,
    int lr) {
  node->left = NULL;
  node->right = NULL;
  node->balance = 0;
  node->value = value;
  node->value_containment_delta = lr; /* 1 or -1 for L or R */
  node->refcount = 1;  /* caller */
  node->range_left = value;
  node->range_right = value;
  node->subtree_containment_delta = lr;
}

static struct NaClIntervalRangeTreeNode *NaClIntervalRangeTreeNodeFactory(
    uint32_t value,
    int lr) {
  struct NaClIntervalRangeTreeNode *obj;
  obj = (struct NaClIntervalRangeTreeNode *) malloc(sizeof *obj);
  if (NULL == obj) {
    NaClLog(LOG_FATAL, "NaClIntervalRangeTreeNodeFactory: out of memory!\n");
  }
  NaClIntervalRangeTreeNodeInit(obj, value, lr);
  return obj;
}

/*
 * Updates |node|'s range_left, range_right, and
 * subtree_containment_delta based on its left and right children.
 * Does not recursively walk!  |node| must not be NULL.
 */
static void NaClRangeStatsUpdate(struct NaClIntervalRangeTreeNode *node) {
  uint32_t range_left = node->value;
  uint32_t range_right = node->value;
  uint32_t cumulative_delta = node->value_containment_delta;

  if (NULL != node->left) {
    range_left = node->left->range_left;
    cumulative_delta += node->left->subtree_containment_delta;
  }
  if (NULL != node->right) {
    range_right = node->right->range_right;
    cumulative_delta += node->right->subtree_containment_delta;
  }
  node->range_left = range_left;
  node->range_right = range_right;
  node->subtree_containment_delta = cumulative_delta;
}

/*
 * returns 1 if (sub)tree at |*treep| grew in height, 0 otherwise.
 * |node| should be initialized, and this takes ownership.  The
 * refcount may be transferred to another pre-exsting node in the
 * tree, and |node| freed.
 */
static int NaClAvlTreeInsert(struct NaClIntervalRangeTreeNode **treep,
                             struct NaClIntervalRangeTreeNode *node) {
  struct NaClIntervalRangeTreeNode *tmp;
  struct NaClIntervalRangeTreeNode *tmp_l;
  struct NaClIntervalRangeTreeNode *tmp_r;
  int higher;

  if (NULL == (tmp = *treep)) {
    *treep = node;
    return 1;
  }
  if (node->value == tmp->value) {
    tmp->value_containment_delta += node->value_containment_delta;
    tmp->subtree_containment_delta += node->value_containment_delta;
    tmp->refcount++;
    free(node);
    return 0;
  } else if (node->value < tmp->value) {
    /* recurse left. */
    higher = NaClAvlTreeInsert(&tmp->left, node);
    if (!higher) {
      NaClRangeStatsUpdate(tmp);
      return 0;
    }
    --tmp->balance;
    if (0 == tmp->balance) {
      /* height unchanged */
      NaClRangeStatsUpdate(tmp);
      return 0;
    }
    if (-1 == tmp->balance) {
      /*
       * Left tree grew, but we're still within AVL balance criteria
       * for this subtree.  Subtree height grew, so let caller know.
       */
      NaClRangeStatsUpdate(tmp);
      return 1;
    }
    /* pivot */
    if (-1 == tmp->left->balance) {
      /*
       * L rotation:
       *
       *      x                    x
       *
       *      d                    b
       *   b     e      ->      a     d
       *  a c                        c e
       */
      *treep = tmp->left;
      tmp->left = (*treep)->right;
      (*treep)->right = tmp;
      (*treep)->balance = 0;
      tmp->balance = 0;

      tmp = *treep;  /* b */
      NaClRangeStatsUpdate(tmp->right);  /* d */
      NaClRangeStatsUpdate(tmp);
      return 0;
    } else {
      /*
       * LR rotation:
       *
       *        x                      x
       *
       *        f                      d
       *    b       g      ->      b       f
       *  a   d                  a   c   e   g
       *     c e
       */
      tmp = (*treep)->left->right;
      tmp_l = tmp->left;
      tmp_r = tmp->right;
      tmp->left = (*treep)->left;
      tmp->right = (*treep);
      *treep = tmp;
      tmp->left->right = tmp_l;
      tmp->right->left = tmp_r;
      tmp->left->balance = (tmp->balance <= 0) ? 0 : -1;
      tmp->right->balance = (tmp->balance >= 0) ? 0 : 1;
      tmp->balance = 0;
      NaClRangeStatsUpdate(tmp->left);
      NaClRangeStatsUpdate(tmp->right);
      NaClRangeStatsUpdate(tmp);
      return 0;
    }
  } else {
    /* node->value > tmp->value */
    higher = NaClAvlTreeInsert(&tmp->right, node);
    if (!higher) {
      NaClRangeStatsUpdate(tmp);
      return 0;
    }
    ++tmp->balance;
    if (0 == tmp->balance) {
      /* height unchanged */
      NaClRangeStatsUpdate(tmp);
      return 0;
    }
    if (1 == tmp->balance) {
      /*
       * Right tree grew, but we're still within AVL balance criteria
       * for this subtree.  Subtree height grew, so let caller know.
       */
      NaClRangeStatsUpdate(tmp);
      return 1;
    }
    /* pivot */
    if (1 == tmp->right->balance) {
      /*
       * R rotation:
       *
       *      x                    x
       *
       *      b                    d
       *   a     d      ->      b     e
       *        c e            a c
       */
      *treep = tmp->right;
      tmp->right = (*treep)->left;
      (*treep)->left = tmp;
      (*treep)->balance = 0;
      tmp->balance = 0;

      tmp = *treep;
      NaClRangeStatsUpdate(tmp->left);
      NaClRangeStatsUpdate(tmp);
      return 0;
    } else {
      /*
       * RL rotation:
       *
       *       x                      x
       *
       *       b                      d
       *   a       f      ->      b       f
       *         d   g          a   c   e   g
       *        c e
       */
      tmp = (*treep)->right->left;
      tmp_l = tmp->left;
      tmp_r = tmp->right;
      tmp->right = (*treep)->right;
      tmp->left = (*treep);
      *treep = tmp;
      tmp->right->left = tmp_r;
      tmp->left->right = tmp_l;
      tmp->right->balance = (tmp->balance >= 0) ? 0 : 1;
      tmp->left->balance = (tmp->balance <= 0) ? 0 : -1;
      tmp->balance = 0;
      NaClRangeStatsUpdate(tmp->right);
      NaClRangeStatsUpdate(tmp->left);
      NaClRangeStatsUpdate(tmp);
      return 0;
    }
  }
}

/*
 * NaClAvlTreeBalanceLeft -- left subtree shrunk.  Adjust balance of
 * node, possibly rebalancing.
 */
static void NaClAvlTreeBalanceLeft(
    struct NaClIntervalRangeTreeNode **treep,
    int *height_changed) {
  int bal;
  struct NaClIntervalRangeTreeNode *tmp;
  struct NaClIntervalRangeTreeNode *tmp_l;
  struct NaClIntervalRangeTreeNode *tmp_r;

  bal = ++(*treep)->balance;
  if (0 == bal) {
    return;
  }
  if (1 == bal) {
    *height_changed = 0;
    return;
  }
  /* Rebalance needed. */
  tmp = (*treep)->right;
  if (tmp->balance >= 0) {
    /*
     * RR rotation (same rotation as R, but balance adjusts differ)
     *
     *      x                    x
     *
     *      b                    d
     *   a     d      ->      b     e
     *        c e            a c
     */
    (*treep)->right = tmp->left;
    tmp->left = (*treep);
    *treep = tmp;
    if (0 == tmp->balance) {
      tmp->balance = -1;
      tmp->left->balance = 1;
      *height_changed = 0;
    } else {
      tmp->balance = 0;
      tmp->left->balance = 0;
    }
    NaClRangeStatsUpdate(tmp->left);
    NaClRangeStatsUpdate(tmp);
  } else {
    /*
     * RL rotation.
     *
     *       x                      x
     *
     *       b                      d
     *   a       f      ->      b       f
     *         d   g          a   c   e   g
     *        c e
     */
    tmp = tmp->left;  /* d */
    tmp_l = tmp->left;
    tmp_r = tmp->right;
    tmp->left = *treep;
    tmp->right = (*treep)->right;
    *treep = tmp;
    tmp->left->right = tmp_l;
    tmp->right->left = tmp_r;
    tmp->left->balance = (tmp->balance > 0) ? -1 : 0;
    tmp->right->balance = (tmp->balance < 0) ? 1 : 0;
    tmp->balance = 0;
    /*
     * *h == 1 remains true.
     */
    NaClRangeStatsUpdate(tmp->left);
    NaClRangeStatsUpdate(tmp->right);
    NaClRangeStatsUpdate(tmp);
  }
}

/*
 * NaClAvlTreeBalanceRight -- right subtree shrunk.  Adjust balance of
 * node, possibly rebalancing.
 */
static void NaClAvlTreeBalanceRight(
    struct NaClIntervalRangeTreeNode **treep,
    int *height_changed) {
  int bal;
  struct NaClIntervalRangeTreeNode *tmp;
  struct NaClIntervalRangeTreeNode *tmp_l;
  struct NaClIntervalRangeTreeNode *tmp_r;

  bal = --(*treep)->balance;
  if (0 == bal) {
    return;
  }
  if (-1 == bal) {
    *height_changed = 0;
    return;
  }
  /* Rebalance needed. */
  tmp = (*treep)->left;
  if (tmp->balance <= 0) {
    /*
     * LL rotation (same rotation as L, but balance adjusts differ)
     *
     *
     *      x                    x
     *
     *      d                    b
     *   b     e      ->      a     d
     *  a c                        c e
     */
    (*treep)->left = tmp->right;
    tmp->right = (*treep);
    *treep = tmp;
    if (0 == tmp->balance) {
      tmp->balance = 1;
      tmp->right->balance = -1;
      *height_changed = 0;
    } else {
      tmp->balance = 0;
      tmp->right->balance = 0;
    }
    NaClRangeStatsUpdate(tmp->right);
    NaClRangeStatsUpdate(tmp);
  } else {
    /*
     * LR rotation.
     *
     *        x                      x
     *
     *        f                      d
     *    b       g      ->      b       f
     *  a   d                  a   c   e   g
     *     c e
     */
    tmp = tmp->right;  /* d */
    tmp_l = tmp->left;
    tmp_r = tmp->right;
    tmp->left = (*treep)->left;
    tmp->right = *treep;
    *treep = tmp;
    tmp->left->right = tmp_l;
    tmp->right->left = tmp_r;
    tmp->left->balance = (tmp->balance > 0) ? -1 : 0;
    tmp->right->balance = (tmp->balance < 0) ? 1 : 0;
    tmp->balance = 0;
    /*
     * *h == 1 remains true.
     */
    NaClRangeStatsUpdate(tmp->left);
    NaClRangeStatsUpdate(tmp->right);
    NaClRangeStatsUpdate(tmp);
  }
}

static struct NaClIntervalRangeTreeNode *NaClAvlFindRightMost(
    struct NaClIntervalRangeTreeNode **treep,
    int *height_changed) {
  struct NaClIntervalRangeTreeNode *tmp;

  if (NULL != (*treep)->right) {
    tmp = NaClAvlFindRightMost(&(*treep)->right, height_changed);
    NaClRangeStatsUpdate(*treep);
    if (*height_changed) {
      NaClAvlTreeBalanceRight(treep, height_changed);
    }
    return tmp;
  }
  tmp = *treep;
  *treep = tmp->left;
  *height_changed = 1;
  return tmp;
}

static struct NaClIntervalRangeTreeNode *NaClAvlFindLeftMost(
    struct NaClIntervalRangeTreeNode **treep,
    int *height_changed) {
  struct NaClIntervalRangeTreeNode *tmp;

  if (NULL != (*treep)->left) {
    tmp = NaClAvlFindLeftMost(&(*treep)->left, height_changed);
    NaClRangeStatsUpdate(*treep);
    if (*height_changed) {
      NaClAvlTreeBalanceLeft(treep, height_changed);
    }
    return tmp;
  }
  tmp = *treep;
  *treep = tmp->right;
  *height_changed = 1;
  return tmp;
}

static struct NaClIntervalRangeTreeNode *NaClAvlTreeExtract(
    struct NaClIntervalRangeTreeNode **treep,
    struct NaClIntervalRangeTreeNode *key,
    int *height_changed) {
  struct NaClIntervalRangeTreeNode *p;
  struct NaClIntervalRangeTreeNode *tmp;

  if (NULL == *treep) {
    NaClLog(LOG_FATAL, "NaClAvlTreeExtract: node not found\n");
  }
  NaCl_dprintf(("TreeExtract k->v %u, k->d %d, h %d\n",
           key->value, key->value_containment_delta, *height_changed));
  NaClIntervalRangeTreePrint(*treep);
  if (key->value < (*treep)->value) {
    p = NaClAvlTreeExtract(&(*treep)->left, key, height_changed);
    NaClRangeStatsUpdate(*treep);
    if (*height_changed) {
      NaClAvlTreeBalanceLeft(treep, height_changed);
    }
    return p;
  } else if (key->value > (*treep)->value) {
    p = NaClAvlTreeExtract(&(*treep)->right, key, height_changed);
    NaClRangeStatsUpdate(*treep);
    if (*height_changed) {
      NaClAvlTreeBalanceRight(treep, height_changed);
    }
    return p;
  } else {
    /*
     * found elt
     */
    p = *treep;
    if (0 != --p->refcount) {
      p->value_containment_delta -= key->value_containment_delta;
      p->subtree_containment_delta -= key->value_containment_delta;
      return NULL;
    }
    /* zero refcount, should delete */
    if (NULL == p->right) {
      *treep = p->left;
      *height_changed = 1;
    } else if (NULL == p->left) {
      *treep = p->right;
      *height_changed = 1;
    } else if (p->balance <= 0) {
      tmp = NaClAvlFindRightMost(&p->left, height_changed);
      tmp->left = p->left;
      tmp->right = p->right;
      tmp->balance = p->balance;
      NaClRangeStatsUpdate(tmp);
      *treep = tmp;
      if (*height_changed) {
        NaClAvlTreeBalanceLeft(treep, height_changed);
      }
    } else {
      tmp = NaClAvlFindLeftMost(&p->right, height_changed);
      tmp->left = p->left;
      tmp->right = p->right;
      tmp->balance = p->balance;
      NaClRangeStatsUpdate(tmp);
      *treep = tmp;
      if (*height_changed) {
        NaClAvlTreeBalanceRight(treep, height_changed);
      }
    }
    p->left = NULL;
    p->right = NULL;
    return p;
  }
}

static struct NaClIntervalRangeTreeNode *NaClIntervalRangeTreeExtract(
    struct NaClIntervalRangeTree *self,
    struct NaClIntervalRangeTreeNode *keyp) {
  int height_changed = 0;

  return NaClAvlTreeExtract(&self->root, keyp, &height_changed);
}

static void NaClIntervalRangeTreeRemoveVal(struct NaClIntervalRangeTree *self,
                                           uint32_t value,
                                           int lr) {
  struct NaClIntervalRangeTreeNode key;
  struct NaClIntervalRangeTreeNode *zero_ref_node;

  NaCl_dprintf(("RemoveVal %u %d\n", value, lr));
  NaClIntervalRangeTreeNodeInit(&key, value, lr);
  zero_ref_node = NaClIntervalRangeTreeExtract(self, &key);
  if (NULL != zero_ref_node) {
    free(zero_ref_node);
  }
  NaCl_dprintf(("result tree\n"));
  NaClIntervalRangeTreePrint(self->root);
}

/*
 * Returns a lower bound for the containment count at |value| (number
 * of intervals in the interval set that contains it).  This lower
 * bound will never be zero if the actual containment count is
 * non-zero.  If the count is zero, then the memory pointed to by
 * |gap_left| is written with the starting value of the open interval
 * that is the gap containing the value, i.e., *gap_left will be one
 * larger than the range_right value of the closed interval
 * immediately to the left.  NB: 0 is possible for this value, i.e.,
 * when there are no interval that bound the gap from the left.  The
 * value is undefined when the return value is non-zero.
 *
 * We could also provide gap_right, but it's not necessary for the
 * correctness of our algorithm.
 *
 * This function is called with |left_delta| equal to zero,
 * |left_range| equal to 0, and |tree| the root of the tree.
 */
static uint32_t NaClAvlTreeFindValue(
    struct NaClIntervalRangeTreeNode *tree,
    uint32_t left_delta,
    uint32_t left_range,
    uint32_t value,
    uint32_t *gap_left) {
  uint32_t delta;

  NaCl_dprintf(("TreeFind ld %u lr %u v %u\n", left_delta, left_range, value));
  NaClIntervalRangeTreePrint(tree);
  if (NULL == tree) {
    *gap_left = left_range;
    return left_delta;
  }
  if (value < tree->value) {
    return NaClAvlTreeFindValue(tree->left, left_delta, left_range, value,
                                gap_left);
  } else if (value == tree->value) {
    delta = left_delta;
    if (NULL != tree->left) {
      delta += tree->left->subtree_containment_delta;
    }
    delta += tree->value_containment_delta;
    if (0 == delta) {
      /*
       * This case occurs if interval set contains [x,y] and we probed
       * for y.  Since this is an inclusive interval, we need to
       * increment rv -- the containment count cannot possibly be zero
       * at a closed-interval end point.  The returned value can be a
       * lower bound for other reasons, e.g., if the set contained
       * intervals [x,y][y,z] and we probed for y, the
       * value_containment_delta will be zero because y is both the
       * end and the start of intervals, and we'd return 1 when in
       * reality y is included in two intervals.
       */
      delta = 1;
    }

    /* *gap_left undefined */
    return delta;
  } else {
    delta = left_delta;
    if (NULL != tree->left) {
      delta += tree->left->subtree_containment_delta;
    }
    delta += tree->value_containment_delta;
    left_range = tree->value + 1;

    return NaClAvlTreeFindValue(tree->right,
                                delta,
                                left_range,
                                value,
                                gap_left);
  }
}

void NaClAvlTreeFree(struct NaClIntervalRangeTreeNode *t) {
  if (NULL == t) {
    return;
  }
  NaClAvlTreeFree(t->left);
  t->left = NULL;
  NaClAvlTreeFree(t->right);
  t->right = NULL;
  free(t);
}

int NaClIntervalRangeTreeCtor(struct NaClIntervalRangeTree *self) {
  self->root = NULL;
  self->base.vtbl = &kNaClIntervalRangeTreeVtbl;
  return 1;
}

void NaClIntervalRangeTreeDtor(struct NaClIntervalMultiset *vself) {
  struct NaClIntervalRangeTree  *self = (struct NaClIntervalRangeTree *) vself;

  NaClAvlTreeFree(self->root);
  self->root = NULL;
  self->base.vtbl = NULL;
}

void NaClIntervalRangeTreeAddInterval(struct NaClIntervalMultiset *vself,
                                      uint32_t first_val, uint32_t last_val) {
  struct NaClIntervalRangeTree *self;
  struct NaClIntervalRangeTreeNode *range_start;
  struct NaClIntervalRangeTreeNode *range_end;

  self = (struct NaClIntervalRangeTree *) vself;
  range_start = NaClIntervalRangeTreeNodeFactory(first_val, 1);
  range_end = NaClIntervalRangeTreeNodeFactory(last_val, -1);
  NaClIntervalRangeTreePrint(self->root);
  (void) NaClAvlTreeInsert(&self->root, range_start);
  NaClIntervalRangeTreePrint(self->root);
  (void) NaClAvlTreeInsert(&self->root, range_end);
  NaClIntervalRangeTreePrint(self->root);
}

void NaClIntervalRangeTreeRemoveInterval(struct NaClIntervalMultiset *vself,
                                         uint32_t first_val,
                                         uint32_t last_val) {
  struct NaClIntervalRangeTree *self = (struct NaClIntervalRangeTree *) vself;

  NaClIntervalRangeTreeRemoveVal(self, last_val, -1);
  NaClIntervalRangeTreeRemoveVal(self, first_val, 1);
}

int NaClIntervalRangeTreeOverlapsWith(struct NaClIntervalMultiset *vself,
                                      uint32_t first_val, uint32_t last_val) {
  struct NaClIntervalRangeTree *self = (struct NaClIntervalRangeTree *) vself;
  uint32_t gap_left_first;
  uint32_t gap_left_last;

  NaCl_dprintf(("OverlapsWith [%u, %u]\n", first_val, last_val));
  if (0 != NaClAvlTreeFindValue(self->root, 0, 0, first_val, &gap_left_first)) {
    NaCl_dprintf(("left not in gap\n"));
    return 1;
  }
  if (0 != NaClAvlTreeFindValue(self->root, 0, 0, last_val, &gap_left_last)) {
    NaCl_dprintf(("right not in gap\n"));
    return 1;
  }

  NaCl_dprintf(("gap first %u gap last %u\n", gap_left_first, gap_left_last));
  if (gap_left_first == gap_left_last) {
    /*
     * Both first_val and last_val had a containment count of zero,
     * and they are in the same gap.
     */
    return 0;
  }
  return 1;
}

struct NaClIntervalMultisetVtbl const kNaClIntervalRangeTreeVtbl = {
  NaClIntervalRangeTreeDtor,
  NaClIntervalRangeTreeAddInterval,
  NaClIntervalRangeTreeRemoveInterval,
  NaClIntervalRangeTreeOverlapsWith,
};
