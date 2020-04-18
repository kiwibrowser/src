// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.sfntly.sample.sfview;

import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.core.PostScriptTable;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.font.FontRenderContext;
import java.awt.font.LineMetrics;
import java.awt.geom.Rectangle2D;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.TreeMap;

/**
 * @author dougfelt@google.com (Doug Felt)
 */
class ViewableTaggedData {
  private List<Marker> markers = new ArrayList<Marker>();
  private final Style style;
  private final Metrics metrics;

  ViewableTaggedData(List<Marker> markers) {
    this(markers, new Style(), new Metrics());
  }

  private ViewableTaggedData(List<Marker> markers, Style style, Metrics metrics) {
    this.markers = markers;
    this.style = style;
    this.metrics = metrics;
  }

  private static class Style {
    private int marginScale;
    private int marginOffset;
    private int marginPad;
    private int columnPad;
    private Font dataFont;
    private Font labelFont;
    private Color positionColor;
    private Color dataColor;
    private Color altColor;
    private Color labelColor;
    // shades of blue hue 221.8 saturation 5% to 35% value 93.5 (yafla)
    // E3E6EE, D7DEEE, CBD6EE, BFCDEE, B3C5EE, A7BDEE, 9BB4EE
    private Color[] depthColors = {
        new Color(0x9BB4EE), new Color(0xB3C5EE), new Color(0xCBD6EE), new Color(0xE3E6EE) };

    private Style() {
      marginScale = 4; // distance between lines
      marginOffset = 1; // distance from origin to tip of arrow/base
      marginPad = 4; // distance from marginOffset to first line
      columnPad = 15; // extra padding for column widths
      dataFont = new Font("monospaced", Font.PLAIN, 13);
      labelFont = new Font("serif", Font.PLAIN, 13);
      positionColor = Color.BLACK;
      dataColor = Color.BLACK;
      altColor = new Color(0x8B0000);
      labelColor = Color.BLUE;
    }
  }

  private static class Metrics {
    private int lineHeight; // total line height
    private int baseline; // distance from bottom up to baseline
    private int xHeight; // distance from baseline up to xHeight
    private int marginWidth;
    private int positionWidth;
    private int dataWidth;
    private int altWidth;
    private int labelWidth;
    private int headerWidth;
    private int totalWidth;

    private Metrics() {
      lineHeight = 15;
      xHeight = 5;
      marginWidth = 50;
      positionWidth = 50;
      dataWidth = 70;
      altWidth = 30;
      labelWidth = 100;
      updateTotalWidth();
    }

    private void zero() {
      lineHeight = baseline = xHeight = 0;
      marginWidth = positionWidth = dataWidth = altWidth = labelWidth = headerWidth = totalWidth = 0;
    }

    private void updateTotalWidth() {
      totalWidth = marginWidth
          + Math.max(positionWidth + dataWidth + altWidth + labelWidth, headerWidth);
    }
  }

  int lineHeight() {
    return metrics.lineHeight;
  }

  int totalWidth() {
    return metrics.totalWidth;
  }

  void draw(Graphics g, int x, int y) {
    DrawContext context = new DrawContext(style, metrics, g, x, y);
    for (Marker m : markers) {
      m.draw(context);
    }
  }

  /**
   * Compute metrics and return the dimensions.
   *
   * @param zeroMetrics
   *          zero the metrics before computing (otherwise use existing cell
   *          widths and line height as minimums).
   * @return the dimensions
   */
  Dimension measure(boolean zeroMetrics) {
    if (zeroMetrics) {
      metrics.zero();
    }
    DrawContext context = new DrawContext(style, metrics, null, 0, 0);
    context.measureLineHeight();

    for (Marker m : markers) {
      m.draw(context);
    }
    return context.dimension();
  }

  public static class TaggedDataImpl implements TaggedData {
    private final List<Marker> markers = new ArrayList<Marker>();
    private RangeNode rangeStack;
    private final PostScriptTable post;

    TaggedDataImpl(PostScriptTable post) {
      this.post = post;
    }

    @Override
    public
    void tagRange(String string, int start, int length, int depth) {
      boolean hasEnd = (length & ~0xffff) == 0;
      if (!hasEnd) {
        depth = -1;
      }
      Range range = new Range(string, start, length, depth);
      markers.add(new RangeStart(range));
      if (hasEnd) {
        markers.add(new RangeEnd(range));
      }
    }

    @Override
    public void tagField(int position, int width, int value, String alt, String label) {
      markers.add(new Field(position, width, value, alt, label));
    }

    @Override
    public void tagTarget(int position, int value, int targetPosition, String label) {
      Reference reference = new Reference(position, targetPosition);
      markers.add(new ReferenceSource(reference, value, label));
      markers.add(new ReferenceTarget(reference));
    }

    List<Marker> getMarkers() {
      Collections.sort(markers);
      return markers;
    }

    // Range-related apis

    private static class RangeNode {
      String label;
      ReadableFontData data;
      RangeNode next;
      int depth;
      int base; // offset from absolute data start
      int pos; // offset from base, where we next read a field

      /**
       * Represent a range.
       *
       * @param label
       *          label to use for the range
       * @param data
       *          the data in the range
       * @param next
       *          the next node in the change
       * @param base
       *          the base of this node as an absolute position in the data
       *          (includes data.boundOffset())
       */
      RangeNode(String label, ReadableFontData data, RangeNode next, int base) {
        this.label = label;
        this.data = data;
        this.next = next;
        this.depth = next == null ? 0 : next.depth + 1;
        this.base = base;
      }
    }

    @Override
    public void pushRange(String label, ReadableFontData data) {
      rangeStack = new RangeNode(label, data, rangeStack, data.dataOffset());
    }

    @Override
    public void pushRangeAtOffset(String label, int base) {
      if (rangeStack == null) {
        throw new IllegalStateException("can't push offset range without data");
      }
      rangeStack = new RangeNode(label, rangeStack.data, rangeStack, base);
    }

    @Override
    public void popRange() {
      if (rangeStack == null) {
        throw new IllegalStateException("not in a range");
      }
      tagRange(rangeStack.label, rangeStack.base, rangeStack.pos, rangeStack.depth);
      rangeStack = rangeStack.next;
    }

    @Override
    public void setRangePosition(int rangePosition) {
      if (rangeStack == null) {
        throw new IllegalStateException("not in a range");
      }
      rangeStack.pos = rangePosition;
    }

    @Override
    public int tagRangeField(FieldType ft, String label) {
      if (rangeStack == null) {
        throw new IllegalStateException("not in a range");
      }
      ReadableFontData data = rangeStack.data;
      int base = rangeStack.base;
      int pos = rangeStack.pos;

      int position = base + pos;
      int width;
      int value;
      String alt;
      switch (ft) {
      case OFFSET_NONZERO:
        value = data.readUShort(pos);
        if (value == 0) {
          alt = "NULL";
          width = 2;
          break;
        }
        // fall through
      case OFFSET:
        value = data.readUShort(pos);
        alt = String.format("#%04x", base + value);
        width = 2;
        tagTarget(position, value, base + value, null);
        break;
      case OFFSET32:
        value = data.readULongAsInt(pos);
        alt = String.format("#%04x", base + value);
        width = 4;
        tagTarget(position, value, base + value, null);
        break;
      case SHORT:
        value = data.readUShort(pos);
        alt = String.valueOf(value);
        width = 2;
        break;
      case SHORT_IGNORED:
        value = data.readUShort(pos);
        alt = null;
        width = 2;
        break;
      case SHORT_IGNORED_FFFF:
        value = data.readUShort(pos);
        alt = value == 0xffff ? null : String.valueOf(value);
        width = 2;
        break;
      case TAG:
        value = data.readULongAsInt(pos);
        alt = Tag.stringValue(value);
        width = 4;
        break;
      case GLYPH:
        value = data.readUShort(pos);
        alt = String.valueOf(value);
        String glyphName = post.glyphName(value);
        if (glyphName != null) {
          if (label == null) {
            label = "glyph name";
          }
          label = label + ": " + glyphName;
        }
        width = 2;
        break;
      default:
        throw new IllegalStateException("unimplemented field type");
      }
      tagField(position, width, value, alt, label);
      rangeStack.pos += width;

      switch (ft) {
      case OFFSET:
      case OFFSET32:
        value += base;
        break;
      case OFFSET_NONZERO:
        if (value != 0) {
          value += base;
        }
        break;
      default:
        break;
      }
      return value;
    }
  }

  private static class DrawContext {
    private final Style style;
    private final Metrics metrics;
    private final Graphics g; // if null, we are measuring
    private FontRenderContext frc; // used when measuring
    private final int x; // current position of 'position' column (margin is to
    // left)
    private int y; // current base of line
    private int lc; // line count
    private int rangeDepth;
    private int lastMarkedPosition;
    private int lastRenderedPosition;
    private int expectedPosition = -1;

    private DrawContext(Style style, Metrics metrics, Graphics g, int x, int y) {
      this.style = style;
      this.metrics = metrics;
      this.g = g;
      this.x = x;
      this.y = y;
      if (g != null) {
        frc = ((Graphics2D) g).getFontRenderContext();
      } else {
        frc = new FontRenderContext(null, true, false);
      }
      this.lc = 0;
    }

    private void measureLineHeight() {
      LineMetrics dataMetrics = style.dataFont.getLineMetrics("0123456789abcdef", frc);
      LineMetrics labelMetrics = style.labelFont.getLineMetrics("ABC", frc);

      int lineHeight = (int) Math.ceil(Math.max(dataMetrics.getHeight(), labelMetrics.getHeight()));
      int baseline = (int) Math.ceil(Math.max(dataMetrics.getDescent() + dataMetrics.getLeading(),
          labelMetrics.getDescent() + labelMetrics.getLeading()));
      int xHeight = (int) Math.ceil(Math.max(dataMetrics.getAscent() - dataMetrics.getLeading(),
          labelMetrics.getAscent() - labelMetrics.getLeading()) / 2.0 - baseline);

      metrics.lineHeight = lineHeight;
      metrics.baseline = baseline;
      metrics.xHeight = xHeight - 3; // this is just not coming out right
    }

    private Dimension dimension() {
      metrics.marginWidth += style.columnPad;
      metrics.positionWidth += style.columnPad;
      metrics.dataWidth += style.columnPad;
      metrics.altWidth += style.columnPad;
      metrics.labelWidth += style.columnPad;
      metrics.updateTotalWidth();

      int width = metrics.totalWidth + style.columnPad;
      int height = lc * metrics.lineHeight + style.columnPad;

      return new Dimension(width, height);
    }

    private void newLine() {
      lc += 1;
      y += metrics.lineHeight;
    }

    private void srcRef(Reference ref) {
      ref.setSrc(x, y);
      if (ref.sourcePosition < ref.targetPosition) {
        return;
      }
      drawRef(ref);
    }

    private void trgRef(Reference ref) {
      ref.setTrg(x, y);
      if (ref.sourcePosition > ref.targetPosition) {
        return;
      }
      drawRef(ref);
    }

    private boolean measuring() {
      return g == null;
    }

    private static final Color[] REF_COLORS = { Color.BLUE,
      Color.RED,
      Color.BLACK,
      Color.GREEN,
      Color.LIGHT_GRAY,
      Color.PINK,
      Color.CYAN,
      Color.DARK_GRAY,
      Color.MAGENTA,
      Color.ORANGE };

    private Color colorForM(int m) {
      return REF_COLORS[m % REF_COLORS.length];
    }

    private RefWidthFinder refWidthFinder = new RefWidthFinder();

    private void drawRef(Reference ref) {
      int m = refWidthFinder.add(ref);

      int srcx = ref.srcx - style.marginOffset;
      int srcy = ref.srcy - metrics.baseline - metrics.xHeight;
      int trgx = ref.trgx - style.marginOffset;
      int trgy = ref.trgy - metrics.baseline - metrics.xHeight;

      int margin = -m * style.marginScale;
      int mx = Math.min(srcx, trgx) - style.marginPad + margin;
      if (measuring()) {
        if (-mx > metrics.marginWidth) {
          metrics.marginWidth = -mx;
        }
        return;
      }

      srcx += metrics.marginWidth;
      trgx += metrics.marginWidth;
      mx += metrics.marginWidth;

      g.setColor(colorForM(m));
      // Debug: g.drawString(ref.sourcePosition + " => " + ref.targetPosition, 0, srcy);
      g.drawLine(srcx, srcy, mx, srcy);
      g.drawLine(mx, srcy, mx, trgy);
      g.drawLine(mx, trgy, trgx, trgy);
      int[] xpts = { trgx, trgx - 3, trgx - 3 };
      int[] ypts = { trgy, trgy - 2, trgy + 2 };
      g.fillPolygon(xpts, ypts, 3);
    }

    private int updateWidth(String s, Font f, int w) {
      Rectangle2D bounds = style.dataFont.getStringBounds(s, frc);
      int width = (int) Math.ceil(bounds.getWidth());
      if (width > w) {
        return width;
      }
      return w;
    }

    private void markPosition(int position) {
      if (position == lastMarkedPosition) {
        return;
      }
      lastMarkedPosition = position;
      if (position > expectedPosition && expectedPosition != -1) {
        newLine();
        String s = "...";
        if (measuring()) {
          metrics.positionWidth = updateWidth(s, style.dataFont, metrics.positionWidth);
        } else {
          int x = this.x + metrics.marginWidth;
          int y = this.y - metrics.baseline;
          g.setFont(style.dataFont);
          g.setColor(style.positionColor);
          g.drawString(s, x, y);
        }
        expectedPosition = position;
      }
      newLine();
    }

    private void drawRangeBackground() {
      if (measuring()) {
        return;
      }
      Color[] colors = style.depthColors;
      int colorIndex = rangeDepth % colors.length;
      g.setColor(rangeDepth == -1 ? Color.WHITE : colors[colorIndex]);
      g.fillRect(metrics.marginWidth, y - metrics.lineHeight,
          metrics.totalWidth - metrics.marginWidth, metrics.lineHeight);
    }

    private void drawLine(int position, int value, int width, String alt, String label) {
      markPosition(position);
      if (lastRenderedPosition == position) {
        if (alt == null && label == null) {
          return;
        }
        newLine();
      } else {
        lastRenderedPosition = position;
      }
      drawRangeBackground();
      int x = this.x + metrics.marginWidth;
      int y = this.y - metrics.baseline;

      String s = String.format("%04x", position);
      if (measuring()) {
        metrics.positionWidth = updateWidth(s, style.dataFont, metrics.positionWidth);
      } else {
        g.setFont(style.dataFont);
        g.setColor(style.positionColor);
        g.drawString(s, x, y);
      }
      x += metrics.positionWidth;

      if (width > 0) {
        s = String.format("%0" + width * 2 + "x", value);
        if (measuring()) {
          metrics.dataWidth = updateWidth(s, style.dataFont, metrics.dataWidth);
        } else {
          g.setColor(style.dataColor);
          g.drawString(s, x, y);
        }
      }
      x += metrics.dataWidth;

      if (alt != null) {
        if (measuring()) {
          metrics.altWidth = updateWidth(alt, style.labelFont, metrics.altWidth);
        } else {
          g.setFont(style.labelFont);
          g.setColor(style.altColor);
          g.drawString(alt, x, y);
        }
      }
      x += metrics.altWidth;

      if (label != null) {
        if (measuring()) {
          metrics.labelWidth = updateWidth(label, style.labelFont, metrics.labelWidth);
        } else {
          g.setFont(style.labelFont);
          g.setColor(style.labelColor);
          g.drawString(label, x, y);
        }
      }
      x += metrics.labelWidth;

      expectedPosition = position + width;
    }

    private void drawHeader(int position, String header) {
      markPosition(position);
      if (lastRenderedPosition == position) {
        newLine();
      } else {
        lastRenderedPosition = position;
      }
      if (measuring()) {
        metrics.headerWidth = updateWidth(header, style.labelFont, metrics.headerWidth);
      } else {
        g.setFont(style.labelFont);
        g.setColor(style.labelColor);
        g.drawString(header, x + metrics.marginWidth, y - metrics.baseline);
      }
    }

    private void rangeTransition(Range range, boolean start) {
      if (range.length >= 0) {
        rangeDepth = start ? range.depth : -1;
      }
    }
  }

  private static class Range {
    private final String name;
    private final int start;
    private final int length;
    private int depth;

    private Range(String name, int start, int length, int depth) {
      this.name = name;
      this.start = start;
      this.length = length;
      this.depth = depth;
    }

    private int start() {
      return start;
    }

    private int limit() {
      return start + length;
    }
  }

  private static class Reference {
    private final int sourcePosition;
    private final int targetPosition;
    private int srcx, srcy, trgx, trgy;
    private Reference(int sourcePosition, int targetPosition) {
      this.sourcePosition = sourcePosition;
      this.targetPosition = targetPosition;
    }

    private void setSrc(int x, int y) {
      srcx = x - 1;
      srcy = y - 5;
    }

    private void setTrg(int x, int y) {
      trgx = x;
      trgy = y - 5;
    }
  }

  private static class WidthUsageRecord {
    private final Map<Integer, Integer> widthUsage = new HashMap<Integer, Integer>();
    private int width;
    private int src;
    static private final WidthUsageRecord EMPTY = new WidthUsageRecord();

    private static WidthUsageRecord copyWithWidthAdded(WidthUsageRecord other, int width, int src) {
      WidthUsageRecord current = new WidthUsageRecord();
      current.width = width;
      current.src = src;

      current.widthUsage.putAll(other.widthUsage);
      int count = 0;
      if (current.widthUsage.containsKey(width)) {
        count = current.widthUsage.get(width);
      }
      current.widthUsage.put(width, count + 1);

      return current;
    }

    private int lowestEquality(WidthUsageRecord other) {
      for (int i = 0; this.widthUsage.containsKey(i); i++) {
        if (other.widthUsage.get(i) == this.widthUsage.get(i)) {
          return i;
        }
      }
      return other.widthUsage.keySet().size();
    }

    private int width() {
      return width;
    }

    private int src() {
      return src;
    }

    @Override
    public String toString() {
      return "{width=" + width + " src=" + src + widthUsage.toString() + "}";
    }
  }

  private static class RefWidthFinder {
    private final TreeMap<Integer, WidthUsageRecord> tgt2widthUsage;
    static private final int MAX_WIDTH = 600;

    private RefWidthFinder() {
      tgt2widthUsage = new TreeMap<Integer, WidthUsageRecord>();
    }

    private int add(Reference ref) {
      int src = ref.sourcePosition;
      int trg = ref.targetPosition;
      WidthUsageRecord match = null;
      if (tgt2widthUsage.containsKey(trg)) {
        match = tgt2widthUsage.get(trg);
        if (match.src() <= src) {
          return match.width();
        }
        // Now there is a previous entry with same target position but higher source position value.
      }

      Entry<Integer, WidthUsageRecord> entry = tgt2widthUsage.floorEntry(src);
      WidthUsageRecord srcWidthUsage = entry != null ? entry.getValue() : WidthUsageRecord.EMPTY;

      // If there is a match and we haven't returned means there is already shorter range with
      // same target has been encountered. So ignore that entry and check for the penultimate target.
      // It is ok to overlap with the matched range.
      // Remember we are always going in the increasing order of target position.
      entry = match != null ? tgt2widthUsage.floorEntry(trg - 1) : tgt2widthUsage.lastEntry();
      WidthUsageRecord lastWidthUsage = entry != null ? entry.getValue() : WidthUsageRecord.EMPTY;

      int width = srcWidthUsage.lowestEquality(lastWidthUsage);
      if (width > MAX_WIDTH) {
        width = MAX_WIDTH;
      }
      WidthUsageRecord trgWidthUsage = WidthUsageRecord.copyWithWidthAdded(lastWidthUsage, width, src);

      tgt2widthUsage.put(trg, trgWidthUsage);
      return width;
    }
  }

  private static abstract class Marker implements Comparable<Marker> {
    final int position;

    private Marker(int position) {
      this.position = position;
    }

    abstract int order(Marker rhs);

    abstract void draw(DrawContext c);

    @Override
    public int compareTo(Marker rhs) {
      int result = this.position - rhs.position;
      if (result != 0) {
        return result;
      }
      Class<? extends Marker> thisClass = this.getClass();
      Class<? extends Marker> thatClass = rhs.getClass();
      result = classOrder(thisClass) - classOrder(thatClass);
      if (result != 0) {
        return result;
      }
      return order(rhs);
    }

    private static final Object[] classOrder = { RangeEnd.class, RangeStart.class,
        ReferenceTarget.class,
      Field.class, ReferenceSource.class };

    private static int classOrder(Class<? extends Marker> clzz) {
      for (int i = 0; i < classOrder.length; ++i) {
        if (classOrder[i] == clzz) {
          return i;
        }
      }
      throw new IllegalStateException("No order for class: " + clzz);
    }
  }

  private static class RangeStart extends Marker {
    private final Range range;

    private RangeStart(Range range) {
      super(range.start());
      this.range = range;
    }

    @Override
    void draw(DrawContext c) {
      c.rangeTransition(range, true);
      c.drawHeader(range.start(), range.name);
    }

    @Override
    int order(Marker rhs) {
      return range.depth - ((RangeStart) rhs).range.depth;
    }
  }

  private static class RangeEnd extends Marker {
    private final Range range;

    private RangeEnd(Range range) {
      super(range.limit());
      this.range = range;
    }

    @Override
    void draw(DrawContext c) {
    }

    @Override
    int order(Marker rhs) {
      return ((RangeEnd) rhs).range.depth - range.depth;
    }
  }

  private static class Field extends Marker {
    private final int width;
    private final int value;
    private final String alt;
    private final String label;

    private Field(int position, int width, int value, String alt, String label) {
      super(position);
      this.width = width;
      this.value = value;
      this.alt = alt;
      this.label = label;
    }

    @Override
    void draw(DrawContext c) {
      c.drawLine(position, value, width, alt, label);
    }

    @Override
    int order(Marker rhs) {
      return 0; // no default ordering for two fields at same position
    }
  }

  private static class ReferenceSource extends Marker {
    private final Reference ref;
    private final int value;
    private final String label;

    private ReferenceSource(Reference ref, int value, String label) {
      super(ref.sourcePosition);
      this.ref = ref;
      this.value = value;
      this.label = label;
    }

    @Override
    void draw(DrawContext c) {
      c.drawLine(position, value, 2, null, label);
      c.srcRef(ref);
    }

    @Override
    int order(Marker rhs) {
      // the one with the larger target comes first
      return ((ReferenceSource) rhs).ref.targetPosition - ref.targetPosition;
    }
  }

  private static class ReferenceTarget extends Marker {
    private final Reference ref;

    private ReferenceTarget(Reference ref) {
      super(ref.targetPosition);
      this.ref = ref;
    }

    @Override
    void draw(DrawContext c) {
      c.markPosition(position);
      c.trgRef(ref);
    }

    @Override
    int order(Marker rhs) {
      // The one with the larger source comes first
      return ((ReferenceTarget) rhs).ref.sourcePosition - ref.sourcePosition;
    }
  }
}
