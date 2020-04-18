package com.google.typography.font.sfntly.table.opentype.component;

enum LookupFlag {
  RIGHT_TO_LEFT(1),
  IGNORE_BASE_GLYPHS(2),
  IGNORE_LIGATURES(4),
  IGNORE_MARKS(8);

  boolean isSet(int flags) {
    return isFlagSet(flags, mask);
  }

  int set(int flags) {
    return setFlag(flags, mask);
  }

  int clear(int flags) {
    return clearFlag(flags, mask);
  }

  private final int mask;
  private LookupFlag(int mask) {
    this.mask = mask;
  }
  
  static boolean isFlagSet(int flags, int mask) {
    return (flags & mask) != 0;
  }

  static int setFlag(int flags, int mask) {
    return flags | mask;
  }

  static int clearFlag(int flags, int mask) {
    return flags & ~mask;
  }
}

