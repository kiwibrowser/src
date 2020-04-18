// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.sfntly.sample.sfview;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.sample.sfview.ViewableTaggedData.TaggedDataImpl;
import com.google.typography.font.sfntly.table.core.PostScriptTable;
import com.google.typography.font.sfntly.table.opentype.GSubTable;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Rectangle;

import javax.swing.JComponent;
import javax.swing.Scrollable;
import javax.swing.SwingConstants;

/**
 * @author dougfelt@google.com (Doug Felt)
 */
class SFFontView extends JComponent implements Scrollable {
  private static final long serialVersionUID = 1L;
  private final ViewableTaggedData viewer;

  SFFontView(Font font) {
    setBackground(Color.WHITE);

    PostScriptTable post = font.getTable(Tag.post);
    TaggedDataImpl tdata = new ViewableTaggedData.TaggedDataImpl(post);
    OtTableTagger tagger = new OtTableTagger(tdata);
    GSubTable gsub = font.getTable(Tag.GSUB);
    tagger.tag(gsub);
    viewer = new ViewableTaggedData(tdata.getMarkers());

    Dimension dimensions = viewer.measure(true);

    Dimension minimumSize = new Dimension(400, 400);
    setMinimumSize(minimumSize);
    setPreferredSize(dimensions);
  }

  @Override
  public Dimension getPreferredScrollableViewportSize() {
    int width = Math.min(500, viewer.totalWidth());
    int height = 25 * viewer.lineHeight();
    return new Dimension(width, height);
  }

  @Override
  public void paintComponent(Graphics g) {
    super.paintComponent(g);
    g.setColor(getBackground());
    g.fillRect(0, 0, getWidth(), getHeight());

    viewer.draw(g, 0, 0);
  }

  @Override
  public int getScrollableUnitIncrement(Rectangle visibleRect, int orientation, int direction) {
    if (orientation == SwingConstants.HORIZONTAL) {
      return 50;
    }
    return viewer.lineHeight();
  }

  @Override
  public int getScrollableBlockIncrement(Rectangle visibleRect, int orientation, int direction) {
    if (orientation == SwingConstants.HORIZONTAL) {
      return viewer.totalWidth();
    }
    int lines = visibleRect.height / viewer.lineHeight() - 2;
    if (lines < 1) {
      lines = 1;
    }
    return lines * viewer.lineHeight();
  }

  @Override
  public boolean getScrollableTracksViewportWidth() {
    return false;
  }

  @Override
  public boolean getScrollableTracksViewportHeight() {
    return false;
  }
}
