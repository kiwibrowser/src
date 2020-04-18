package com.google.typography.font.sfntly.table.opentype.component;

import com.google.typography.font.sfntly.table.core.PostScriptTable;

import java.util.Collection;
import java.util.LinkedList;

class RuleSegment extends LinkedList<GlyphGroup> {
  private static final long serialVersionUID = 4563803321401665616L;

  RuleSegment() {
    super();
  }

  RuleSegment(GlyphGroup glyphGroup) {
    addInternal(glyphGroup);
  }

  RuleSegment(int glyph) {
    GlyphGroup glyphGroup = new GlyphGroup(glyph);
    addInternal(glyphGroup);
  }

  RuleSegment(GlyphList glyphs) {
    for (int glyph : glyphs) {
      GlyphGroup glyphGroup = new GlyphGroup(glyph);
      addInternal(glyphGroup);
    }
  }

  boolean add(int glyph) {
    GlyphGroup glyphGroup = new GlyphGroup(glyph);
    return addInternal(glyphGroup);
  }

  @Override
  public boolean addAll(Collection<? extends GlyphGroup> glyphGroups) {
    for(GlyphGroup glyphGroup : glyphGroups) {
      if (glyphGroup == null) {
        throw new IllegalArgumentException("Null GlyphGroup not allowed");
      }
    }
    return super.addAll(glyphGroups);
  }

  private boolean addInternal(GlyphGroup glyphGroup) {
    if (glyphGroup == null) {
      throw new IllegalArgumentException("Null GlyphGroup not allowed");
    }
    return super.add(glyphGroup);
  }

  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder();
    for (GlyphGroup glyphGroup : this) {
      sb.append(glyphGroup.toString());
    }
    return sb.toString();
  }

  String toString(PostScriptTable post) {
    StringBuilder sb = new StringBuilder();
    for (GlyphGroup glyphGroup : this) {
      sb.append(glyphGroup.toString(post));
      sb.append(" ");
    }
    return sb.toString();
  }

}
