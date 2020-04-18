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
 * Implement LZCOMP compression algorithm as defined in MicroType Express, part of the EOT
 * draft spec at {@link "http://www.w3.org/Submission/MTX/"}
 * 
 * Java implementation based on http://www.w3.org/Submission/MTX/ reference code
 *
 * @author Raph Levien
 */
public class LzcompCompress {

  private static final int MAX_2BYTE_DIST = 512;
  private static final int DIST_MIN = 1;
  private static final int DIST_WIDTH = 3;
  private static final int LEN_MIN = 2;
  private static final int LEN_MIN3 = 3;
  private static final int LEN_WIDTH = 3;
  private static final int BIT_RANGE = LEN_WIDTH - 1;
  private static final int PRELOAD_SIZE = 2 * 32 * 96 + 4 * 256;
  private static final int DEFAULT_MAX_COPY_DIST = 0x7fffffff;

  private BitIOWriter bits;
  private boolean usingRunLength;
  private int length1;
  private int maxCopyDist = DEFAULT_MAX_COPY_DIST;
  private HuffmanEncoder distEncoder;
  private HuffmanEncoder lenEncoder;
  private HuffmanEncoder symEncoder;
  private int numDistRanges;
  private int distMax;
  private int dup2;
  private int dup4;
  private int dup6;
  private int numSyms;
  private byte[] buf;
  private HashNode[] hashTable;

  private static class HashNode {
    int index;
    HashNode next;
  }
  
  private LzcompCompress() {
    bits = new BitIOWriter();
    usingRunLength = false;
  }
  
  private void write(byte[] dataIn) {
    bits.writeBit(usingRunLength);
    length1 = dataIn.length;
    // If we want to do runlengths, here's the place, but I'm not convinced it's useful
    setDistRange(length1);
    distEncoder = new HuffmanEncoder(bits, 1 << DIST_WIDTH);
    lenEncoder = new HuffmanEncoder(bits, 1 << LEN_WIDTH);
    symEncoder = new HuffmanEncoder(bits, numSyms);
    buf = new byte[PRELOAD_SIZE + length1];
    System.arraycopy(dataIn, 0, buf, PRELOAD_SIZE, length1);
    encode();
    bits.flush();
  }

  void setDistRange(int length) {
    numDistRanges = 1;
    distMax = DIST_MIN + (1 << (DIST_WIDTH * numDistRanges)) - 1;
    while (distMax < length1) {
      numDistRanges++;
      distMax = DIST_MIN + (1 << (DIST_WIDTH * numDistRanges)) - 1;
    }
    dup2 = 256 + (1 << LEN_WIDTH) * numDistRanges;
    dup4 = dup2 + 1;
    dup6 = dup4 + 1;
    numSyms = dup6 + 1;
  }
  
  private void encode() {
    int maxIndex = length1 + PRELOAD_SIZE;
    initializeModel();
    bits.writeValue(length1, 24);
    int limit = length1 + PRELOAD_SIZE;
    int[] dist = new int[1];
    for (int i = PRELOAD_SIZE; i < limit; ) {
      int here = i;
      int len = makeCopyDecision(i++, dist);
      if (len > 0) {
        int distRanges = getNumDistRanges(dist[0]);
        encodeLength(len, dist[0], distRanges);
        encodeDistance2(dist[0], distRanges);
        for (int j = 1; j < len; j++) {
          updateModel(i++);
        }
      } else {
        byte c = buf[here];
        if (here >= 2 && c == buf[here - 2]) {
          symEncoder.writeSymbol(dup2);
        } else if (here >= 4 && c == buf[here - 4]) {
          symEncoder.writeSymbol(dup4);
        } else if (here >= 6 && c == buf[here - 6]) {
          symEncoder.writeSymbol(dup6);
        } else {
          symEncoder.writeSymbol(buf[here] & 0xff);
        }
      }
    }
  }
  void initializeModel() {
    hashTable = new HashNode[0x10000];
    int i = 0;
    for (int k = 0; k < 32; k++) {
      for (int j = 0; j < 96; j++) {
        buf[i] = (byte)k;
        updateModel(i++);
        buf[i]= (byte)j;
        updateModel(i++);
      }
    }
    for (int j = 0; i < PRELOAD_SIZE && j < 256; j++) {
      buf[i] = (byte)j;
      updateModel(i++);
      buf[i] = (byte)j;
      updateModel(i++);
      buf[i] = (byte)j;
      updateModel(i++);
      buf[i] = (byte)j;
      updateModel(i++);
    }
  }

  private int makeCopyDecision(int index, int[] bestDist) {
    int[] dist1 = new int[1];
    int[] gain1 = new int[1];
    int[] costPerByte1 = new int[1];
    int here = index;
    int len1 = findMatch(index, dist1, gain1, costPerByte1);
    updateModel(index++);
    if (gain1[0] > 0) {
      int[] dist2 = new int[1];
      int[] gain2 = new int[1];
      int[] costPerByte2 = new int[1];
      int len2 = findMatch(index, dist2, gain2, costPerByte2);
      int symbolCost = symEncoder.writeSymbolCost(buf[here] & 0xff);
      if (gain2[0] >= gain1[0] && costPerByte1[0] > (costPerByte2[0] * len2 + symbolCost) /
          (len2 + 1)) {
        len1 = 0;
      } else if (len1 > 3) {
        len2 = findMatch(here + len1, dist2, gain2, costPerByte2);
        if (len2 >= 2) {
          int[] dist3 = new int[1];
          int[] gain3 = new int[1];
          int[] costPerByte3 = new int[1];
          int len3 = findMatch(here + len1 - 1, dist3, gain3, costPerByte3);
          if (len3 > len2 && costPerByte3[0] < costPerByte2[0]) {
            int distRanges = getNumDistRanges(dist1[0] + 1);
            int lenBitCount = encodeLengthCost(len1 - 1, dist1[0] + 1, distRanges);
            int distBitCount = encodeDistance2Cost(dist1[0] + 1, distRanges);
            int cost1B = lenBitCount + distBitCount + costPerByte3[0] * len3;
            int cost1A = costPerByte1[0] * len1 + costPerByte2[0] * len2;
            if ((cost1A / (len1 + len2)) > (cost1B / (len1 - 1 + len3))) {
              len1--;
              dist1[0]++;
            }
          }
        }
      }
      if (len1 == 2) {
        if (here >= 2 && buf[here] == buf[here - 2]) {
          int dup2Cost = symEncoder.writeSymbolCost(dup2);
          if (costPerByte1[0] * 2 > dup2Cost + symEncoder.writeSymbolCost(buf[here + 1] & 0xff)) {
            len1 = 0;
          }
        } else if (here >= 1 && here + 1 < buf.length && buf[here + 1] == buf[here - 1]) {
          int dup2Cost = symEncoder.writeSymbolCost(dup2);
          if (costPerByte1[0] * 2 > symbolCost + dup2Cost) {
            len1 = 0;
          }
        }
      }
    }
    bestDist[0] = dist1[0];
    return len1;
  }

  // consider refactoring signature to return PotentialMatch object with fields set...
  int findMatch(int index, int[] distOut, int[] gainOut, int[] costPerByteOut) {
    final int maxCostCacheLength = 32;
    int[] literalCostCache = new int[maxCostCacheLength + 1];
    int maxIndexMinusIndex = buf.length - index;
    int bestLength = 0;
    int bestDist = 0;
    int bestGain = 0;
    int bestCopyCost = 0;
    int maxComputedLength = 0;
    if (maxIndexMinusIndex > 1) {
      int pos = ((buf[index] & 0xff) << 8) | (buf[index + 1] & 0xff);
      HashNode prevNode = null;
      int hNodeCount = 0;
      for (HashNode hNode = hashTable[pos]; hNode != null; prevNode = hNode, hNode = hNode.next) {
        int i = hNode.index;
        int dist = index - i;
        hNodeCount++;
        if (hNodeCount > 256 || dist > maxCopyDist) {
          if (hashTable[pos] == hNode) {
            hashTable[pos] = null;
          } else {
            prevNode.next = null;
          }
          break;
        }
        int maxLen = index - i;
        if (maxIndexMinusIndex < maxLen) {
          maxLen = maxIndexMinusIndex;
        }
        if (maxLen < LEN_MIN) {
          continue;
        }
        i += 2;
        int length = 2;
        for (length = 2; length < maxLen && buf[i] == buf[index + length]; length++) {
          i++;
        }
        if (length < LEN_MIN) {
          continue;
        }
        dist = dist - length + 1;
        if (dist > distMax || (length == 2&& dist >= MAX_2BYTE_DIST)) {
          continue;
        }
        if (length <= bestLength && dist > bestDist) {
          if (length <= bestLength - 2) {
            continue;
          }
          if (dist > (bestDist << DIST_WIDTH)) {
            if (length < bestLength || dist > (bestDist << (DIST_WIDTH + 1))) {
              continue;
            }
          }
        }
        int literalCost = 0;
        if (length > maxComputedLength) {
          int limit = length;
          if (limit > maxCostCacheLength) limit = maxCostCacheLength;
          for (i = maxComputedLength; i < limit; i++) {
            byte c = buf[index + i];
            literalCostCache[i + 1] = literalCostCache[i] + symEncoder.writeSymbolCost(c & 0xff);
          }
          maxComputedLength = limit;
          if (length > maxCostCacheLength) {
            literalCost = literalCostCache[maxCostCacheLength];
            literalCost += literalCost / maxCostCacheLength * (length - maxCostCacheLength);
          } else {
            literalCost = literalCostCache[length];
          }
        } else {
          literalCost = literalCostCache[length];
        }
        if (literalCost > bestGain) {
          int distRanges = getNumDistRanges(dist);
          int copyCost = encodeLengthCost(length, dist, distRanges);
          if (literalCost - copyCost - (distRanges << 16) > bestGain) {
            copyCost += encodeDistance2Cost(dist, distRanges);
            int gain = literalCost - copyCost;
            if (gain > bestGain) {
              bestGain = gain;
              bestLength = length;
              bestDist = dist;
              bestCopyCost = copyCost;
            }
          }
        }
      }
    }
    costPerByteOut[0] = bestLength > 0 ? bestCopyCost / bestLength : 0;
    distOut[0] = bestDist;
    gainOut[0] = bestGain;
    return bestLength;
  }

  private int getNumDistRanges(int dist) {
    int bitsNeeded = HuffmanEncoder.bitsUsed(dist - DIST_MIN);
    int distRanges = (bitsNeeded + DIST_WIDTH - 1) / DIST_WIDTH;
    return distRanges;
  }
  
  private void encodeLength(int value, int dist, int numDistRanges) {
    if (dist >= MAX_2BYTE_DIST) {
      value -= LEN_MIN3;
    } else {
      value -= LEN_MIN;
    }
    int bitsUsed = HuffmanEncoder.bitsUsed(value);
    int i = BIT_RANGE;
    while (i < bitsUsed) {
      i += BIT_RANGE;
    }
    int mask = 1 << (i - 1);
    int symbol = bitsUsed > BIT_RANGE ? 2 : 0;
    if ((value & mask) != 0) {
      symbol |= 1;
    }
    mask >>= 1;
    symbol <<= 1;
    if ((value & mask) != 0) {
      symbol |= 1;
    }
    mask >>= 1;
    symEncoder.writeSymbol(256 + symbol + (numDistRanges - 1) * (1 << LEN_WIDTH));
    for (i = bitsUsed - BIT_RANGE; i >= 1; i -= BIT_RANGE) {
      symbol = i > BIT_RANGE ? 2 : 0;
      if ((value & mask) != 0) {
        symbol |= 1;
      }
      mask >>= 1;
      symbol <<= 1;
      if ((value & mask) != 0) {
        symbol |= 1;
      }
      mask >>= 1;
      lenEncoder.writeSymbol(symbol);
    }
  }
  
  private int encodeLengthCost(int value, int dist, int numDistRanges) {
    if (dist >= MAX_2BYTE_DIST) {
      value -= LEN_MIN3;
    } else {
      value -= LEN_MIN;
    }
    int bitsUsed = HuffmanEncoder.bitsUsed(value);
    int i = BIT_RANGE;
    while (i < bitsUsed) {
      i += BIT_RANGE;
    }
    int mask = 1 << (i - 1);
    int symbol = bitsUsed > BIT_RANGE ? 2 : 0;
    if ((value & mask) != 0) {
      symbol |= 1;
    }
    mask >>= 1;
    symbol <<= 1;
    if ((value & mask) != 0) {
      symbol |= 1;
    }
    mask >>= 1;
    int cost = symEncoder.writeSymbolCost(256 + symbol + (numDistRanges - 1) * (1 << LEN_WIDTH));
    for (i = bitsUsed - BIT_RANGE; i >= 1; i -= BIT_RANGE) {
      symbol = i > BIT_RANGE ? 2 : 0;
      if ((value & mask) != 0) {
        symbol |= 1;
      }
      mask >>= 1;
      symbol <<= 1;
      if ((value & mask) != 0) {
        symbol |= 1;
      }
      mask >>= 1;
      cost += lenEncoder.writeSymbolCost(symbol);
    }
    return cost;
  }

  private void encodeDistance2(int value, int distRanges) {
    value -= DIST_MIN;
    final int mask = (1 << DIST_WIDTH) - 1;
    for (int i = (distRanges - 1) * DIST_WIDTH; i >= 0; i -= DIST_WIDTH) {
      distEncoder.writeSymbol((value >> i) & mask);
    }
  }

  private int encodeDistance2Cost(int value, int distRanges) {
    int cost = 0;
    value -= DIST_MIN;
    final int mask = (1 << DIST_WIDTH) - 1;
    for (int i = (distRanges - 1) * DIST_WIDTH; i >= 0; i -= DIST_WIDTH) {
      cost += distEncoder.writeSymbolCost((value >> i) & mask);
    }
    return cost;
  }
  
  private void updateModel(int index) {
    byte c = buf[index];
    if (index > 0) {
      HashNode hNode = new HashNode();
      byte prevC = buf[index - 1];
      int pos = ((prevC & 0xff) << 8) | (c & 0xff);
      hNode.index = index - 1;
      hNode.next = hashTable[pos];
      hashTable[pos] = hNode;
    }
  }

  private byte[] toByteArray() {
    return bits.toByteArray();
  }
  
  public static byte[] compress(byte[] dataIn) {
    LzcompCompress compressor = new LzcompCompress();
    compressor.write(dataIn);
    return compressor.toByteArray();
  }
  
  public static int getPreloadSize() {
    return PRELOAD_SIZE;
  }
}
