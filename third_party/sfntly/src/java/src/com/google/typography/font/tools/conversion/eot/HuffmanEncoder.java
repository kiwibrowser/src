/*
 * Copyright 2011 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.typography.font.tools.conversion.eot;

/**
 * Adaptive huffman coder for LZCOMP compression algorithm
 *
 * @author Raph Levien
 */
public class HuffmanEncoder {

  private static final int ROOT = 1;
  
  private TreeNode[] tree;
  private short[] symbolIndex;
  private int bitCount2;
  private int range;
  private BitIOWriter bits;

  private static class TreeNode {
    short up;
    short left;
    short right;
    short code;
    int weight;
  }
  
  public HuffmanEncoder(BitIOWriter bits, int range) {
    this.bits = bits;
    this.range = range;
    bitsUsed(range - 1);
    if (range > 256 && range < 512) {
      bitCount2 = bitsUsed(range - 257);
    } else {
      bitCount2 = 0;
    }
    symbolIndex = new short[range];
    int limit = 2 * range;
    tree = new TreeNode[limit];
    for (int i = 0; i < limit; i++) {
      tree[i] = new TreeNode();
    }
    for (int i = 2; i < limit; i++) {
      tree[i].up = (short)(i / 2);
      tree[i].weight = 1;
    }
    for (int i = 1; i < range; i++) {
      tree[i].left = (short)(2 * i);
      tree[i].right = (short)(2 * i + 1);
    }
    for (int i = 0; i < range; i++) {
      tree[i].code = -1;
      tree[range + i].code = (short)i;
      tree[range + i].left = -1;
      tree[range + i].right = -1;
      symbolIndex[i] = (short)(range + i);
    }
    initWeight(ROOT);
    if (bitCount2 != 0) {
      updateWeight(symbolIndex[256]);
      updateWeight(symbolIndex[257]);
      // assert (258 < range)
      for (int i = 0; i < 12; i++) {
        updateWeight(symbolIndex[range - 3]);
      }
      for (int i = 0; i < 6; i++) {
        updateWeight(symbolIndex[range - 2]);
      }
    } else {
      for (int j = 0; j < 2; j++) {
        for (int i = 0; i < range; i++) {
          updateWeight(symbolIndex[i]);
        }
      }
    }
  }
  
  /* Check tree for internal consistency, return problem string or null if ok */
  String checkTree() {
    for (int i = ROOT; i < range; i++) {
      if (tree[i].code < 0) {
        if (tree[tree[i].left].up != i) {
          return "tree[tree[" + i + "].left].up == " + tree[tree[i].left].up + ", expected " + i;
        }
        if (tree[tree[i].right].up != i) {
          return "tree[tree[" + i + "].right].up == " + tree[tree[i].right].up + ", expected " + i;
        }
      }
    }
    for (int i = ROOT; i < range; i++) {
      if (tree[i].code < 0) {
        if (tree[i].weight != tree[tree[i].left].weight + tree[tree[i].right].weight) {
          return "tree[" + i + "].weight == " + tree[i].weight + ", expected " +
              tree[tree[i].left].weight + " + " + tree[tree[i].right].weight;
        }
      }
    }
    int j = range * 2 - 1;
    for (int i = ROOT; i < j; i++) {
      if (tree[i].weight < tree[i + 1].weight) {
        return "tree[" + i + "].weight == " + tree[i].weight +
            ", tree[" + (i + 1) + ".weight == " + tree[i+1].weight + ", not >=";
      }
    }
    for (int i = ROOT + 1; i < j; i++) {
      if (tree[i].code < 0) {
        int a = tree[i].left;
        int b = tree[i].right;
        if (a - b != 1 && a - b != -1) {
          return "tree[" + i + "].left == " + tree[i].left +
             ", tree[" + i + "].right] == " +tree[i].right + ", siblings not adjacent";
        }
      }
    }
    for (int i = ROOT + 1; i < range * 2; i++) {
      int a = tree[i].up;
      if (tree[a].left != i && tree[a].right != i) {
        return "tree[" + a + "].left != " + i + " && tree[" + a + "].right != " + i;
      }
    }
      
    return null;
  }

  private int initWeight(int a) {
    if (tree[a].code < 0) {
      tree[a].weight = initWeight(tree[a].left) + initWeight(tree[a].right);
    }
    return tree[a].weight;
  }

  private void updateWeight(int a) {
    for (; a != ROOT; a = tree[a].up) {
      int weightA = tree[a].weight;
      int b = a - 1;
      if (tree[b].weight == weightA) {
        do {
          b--;
        } while (tree[b].weight == weightA);
        b++;
        if (b > ROOT) {
          swapNodes(a, b);
          a = b;
        }
      }
      weightA++;
      tree[a].weight = weightA;
    }
    tree[a].weight++;
  }

  private void swapNodes(int a, int b) {
    short upa = tree[a].up;
    short upb = tree[b].up;
    TreeNode tmp = tree[a];
    tree[a] = tree[b];
    tree[b] = tmp;
    tree[a].up = upa;
    tree[b].up = upb;
    int code = tree[a].code;
    if (code < 0) {
      tree[tree[a].left].up = (short)a;
      tree[tree[a].right].up = (short)a;
    } else {
      symbolIndex[code] = (short)a;
    }
    code = tree[b].code;
    if (code < 0) {
      tree[tree[b].left].up = (short)b;
      tree[tree[b].right].up = (short)b;
    } else {
      symbolIndex[code] = (short)b;
    }
  }

  public int writeSymbolCost(int symbol) {
    int a = symbolIndex[symbol];
    int sp = 0;
    do {
      sp++;
      a = tree[a].up;
    } while (a != ROOT);
    return sp << 16;
  }
  
  public void writeSymbol(int symbol) {
    int a = symbolIndex[symbol];
    int aa = a;
    boolean[] stack = new boolean[50];
    int sp = 0;
    do {
      int up = tree[a].up;
      stack[sp++] = tree[up].right == a;
      a = up;
    } while (a != ROOT);
    do {
      bits.writeBit(stack[--sp]);
    } while (sp > 0);
    updateWeight(aa);
  }

  public static int bitsUsed(int x) {
    int i;
    for (i = 32; i > 1; i--) {
      if ((x & (1 << (i - 1))) != 0)
        break;
    }
    return i;
  }
}
