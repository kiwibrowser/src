/*
 * Copyright 2010 Google Inc. All Rights Reserved.
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

package com.google.typography.font.sfntly;

import com.google.typography.font.sfntly.Font.Builder;
import com.google.typography.font.sfntly.data.FontData;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.data.WritableFontData;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PushbackInputStream;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;

/**
 * The font factory. This is the root class for the creation and loading of fonts.
 *
 * @author Stuart Gill
 */
public final class FontFactory {
  private static final int LOOKAHEAD_SIZE = 4;

  // font building settings
  private boolean fingerprint = false;

  // font serialization settings
  List<Integer> tableOrdering;

  /**
   * Offsets to specific elements in the underlying data. These offsets are relative to the
   * start of the table or the start of sub-blocks within the table.
   */
  private enum Offset {
    // Offsets within the main directory
    TTCTag(0),
    Version(4),
    numFonts(8),
    OffsetTable(12),

    // TTC Version 2.0 extensions
    // offsets from end of OffsetTable
    ulDsigTag(0),
    ulDsigLength(4),
    ulDsigOffset(8);

    private final int offset;

    private Offset(int offset) {
      this.offset = offset;
    }
  }

  /**
   * Constructor.
   */
  private FontFactory() {
    // Prevent construction.
  }

  /**
   * Factory method for the construction of a font factory.
   *
   * @return a new font factory
   */
  public static FontFactory getInstance(/*buffer builder factory*/) {
    return new FontFactory();
  }

  // font building settings

  /**
   * Toggle whether fonts that are loaded are fingerprinted with a SHA-1 hash.
   * If a font is fingerprinted then a SHA-1 hash is generated at load time and stored in the
   * font. This is useful for uniquely identifying fonts. By default this is turned on.
   * @param fingerprint whether fingerprinting should be turned on or off
   * @see #fingerprintFont()
   * @see Font#digest()
   */
  public void fingerprintFont(boolean fingerprint) {
    this.fingerprint= fingerprint;
  }

  /**
   * Get the state of the fingerprinting option for fonts that are loaded.
   * @return true if fingerprinting is turned on; false otherwise
   * @see #fingerprintFont(boolean)
   * @see Font#digest()
   */
  public boolean fingerprintFont() {
    return this.fingerprint;
  }

  // input stream font loading

  /**
   * Load the font(s) from the input stream. The current settings on the factory
   * are used during the loading process. One or more fonts are returned if the
   * stream contains valid font data. Some font container formats may have more
   * than one font and in this case multiple font objects will be returned. If
   * the data in the stream cannot be parsed or is invalid an array of size zero
   * will be returned.
   *
   * @param is the input stream font data
   * @return one or more fonts
   * @throws IOException
   */
  public Font[] loadFonts(InputStream is) throws IOException {
    PushbackInputStream pbis =
      new PushbackInputStream(new BufferedInputStream(is), FontFactory.LOOKAHEAD_SIZE);
    if (isCollection(pbis)) {
      return loadCollection(pbis);
    }
    return new Font[] {loadSingleOTF(pbis) };
  }

  /**
   * Load the font(s) from the input stream into font builders. The current
   * settings on the factory are used during the loading process. One or more
   * font builders are returned if the stream contains valid font data. Some
   * font container formats may have more than one font and in this case
   * multiple font builder objects will be returned. If the data in the stream
   * cannot be parsed or is invalid an array of size zero will be returned.
   *
   * @param is the input stream font data
   * @return one or more font builders
   * @throws IOException
   */
  public Builder[] loadFontsForBuilding(InputStream is) throws IOException {
    PushbackInputStream pbis =
      new PushbackInputStream(new BufferedInputStream(is), FontFactory.LOOKAHEAD_SIZE);
    if (isCollection(pbis)) {
      return loadCollectionForBuilding(pbis);
    }
    return new Builder[] {loadSingleOTFForBuilding(pbis) };
  }

  private Font loadSingleOTF(InputStream is) throws IOException {
    return loadSingleOTFForBuilding(is).build();
  }

  private Font[] loadCollection(InputStream is) throws IOException {
    Font.Builder[] builders = loadCollectionForBuilding(is);
    Font[] fonts = new Font[builders.length];
    for (int i = 0; i < fonts.length; i++) {
      fonts[i] = builders[i].build();
    }
    return fonts;
  }

  private Font.Builder loadSingleOTFForBuilding(InputStream is) throws IOException {
    MessageDigest digest = null;
    if (this.fingerprintFont()) {
      try {
        digest = MessageDigest.getInstance("SHA-1");
      } catch (NoSuchAlgorithmException e) {
        throw new IOException("Unable to get requested message digest algorithm.", e);
      }
      DigestInputStream dis = new DigestInputStream(is, digest);
      is = dis;
    }
    Builder builder = Builder.getOTFBuilder(this, is);
    if (this.fingerprintFont()) {
      builder.setDigest(digest.digest());
    }
    return builder;
  }

  private Font.Builder[] loadCollectionForBuilding(InputStream is) throws IOException {
    WritableFontData wfd = WritableFontData.createWritableFontData(is.available());
    wfd.copyFrom(is);
    // TOOD(stuartg): add collection loading from a stream
    return loadCollectionForBuilding(wfd);
  }

  static private boolean isCollection(PushbackInputStream pbis) throws IOException {
    byte[] tag = new byte[4];
    pbis.read(tag);
    pbis.unread(tag);
    return Tag.ttcf == Tag.intValue(tag);
  }

  // ByteArray font loading
  /**
   * Load the font(s) from the byte array. The current settings on the factory
   * are used during the loading process. One or more fonts are returned if the
   * stream contains valid font data. Some font container formats may have more
   * than one font and in this case multiple font objects will be returned. If
   * the data in the stream cannot be parsed or is invalid an array of size zero
   * will be returned.
   *
   * @param b the font data
   * @return one or more fonts
   * @throws IOException
   */
  public Font[] loadFonts(byte[] b) throws IOException {
    // TODO(stuartg): make a ReadableFontData when block loading moved to
    // FontFactory
    WritableFontData rfd = WritableFontData.createWritableFontData(b);
    if (isCollection(rfd)) {
      return loadCollection(rfd);
    }
    return new Font[] {loadSingleOTF(rfd)};
  }

  /**
   * Load the font(s) from the byte array into font builders. The current
   * settings on the factory are used during the loading process. One or more
   * font builders are returned if the stream contains valid font data. Some
   * font container formats may have more than one font and in this case
   * multiple font builder objects will be returned. If the data in the stream
   * cannot be parsed or is invalid an array of size zero will be returned.
   *
   * @param b the byte array font data
   * @return one or more font builders
   * @throws IOException
   */
  public Font.Builder[] loadFontsForBuilding(byte[] b) throws IOException {
    WritableFontData wfd = WritableFontData.createWritableFontData(b);
    if (isCollection(wfd)) {
      return loadCollectionForBuilding(wfd);
    }
    return new Font.Builder[] {loadSingleOTFForBuilding(wfd, 0)};
  }

  private Font loadSingleOTF(WritableFontData wfd) throws IOException {
    return loadSingleOTFForBuilding(wfd, 0).build();
  }

  private Font[] loadCollection(WritableFontData wfd) throws IOException {
    Font.Builder[] builders = loadCollectionForBuilding(wfd);
    Font[] fonts = new Font[builders.length];
    for (int i = 0; i < fonts.length; i++) {
      fonts[i] = builders[i].build();
    }
    return fonts;
  }

  private Font.Builder loadSingleOTFForBuilding(WritableFontData wfd, int offsetToOffsetTable)
      throws IOException {
    MessageDigest digest = null;
    if (this.fingerprintFont()) {
      // TODO(stuartg): digest of ByteArray
    }
    Font.Builder builder = Font.Builder.getOTFBuilder(this, wfd, offsetToOffsetTable);
    return builder;
  }

  private Font.Builder[] loadCollectionForBuilding(WritableFontData wfd) throws IOException {
    int ttcTag = wfd.readULongAsInt(Offset.TTCTag.offset);
    long version = wfd.readFixed(Offset.Version.offset);
    int numFonts = wfd.readULongAsInt(Offset.numFonts.offset);

    Font.Builder[] builders = new Font.Builder[numFonts];
    int offsetTableOffset = Offset.OffsetTable.offset;
    for (int fontNumber = 0; fontNumber < numFonts; fontNumber++,
        offsetTableOffset += FontData.DataSize.ULONG.size()) {
      int offset = wfd.readULongAsInt(offsetTableOffset);
      builders[fontNumber] = this.loadSingleOTFForBuilding(wfd, offset);
    }
    return builders;
  }

  static private boolean isCollection(ReadableFontData rfd) {
    byte[] tag = new byte[4];
    rfd.readBytes(0, tag, 0, tag.length);
    return Tag.ttcf == Tag.intValue(tag);
  }

  // font serialization

  /**
   * Serialize the font to the output stream.
   *
   * @param font the font to serialize
   * @param os the destination stream for the font
   * @throws IOException
   */
  public void serializeFont(Font font, OutputStream os) throws IOException {
    // TODO(stuartg) should have serialization options somewhere
    font.serialize(os, tableOrdering);
  }

  /**
   * Set the table ordering to be used in serializing a font. The table ordering
   * is an ordered list of table ids and tables will be serialized in the order
   * given. Any tables whose id is not listed in the ordering will be placed in
   * an unspecified order following those listed.
   *
   * @param tableOrdering the table ordering
   */
  public void setSerializationTableOrdering(List<Integer> tableOrdering) {
    this.tableOrdering = new ArrayList<Integer>(tableOrdering);
  }

  // new fonts

  /**
   * Get an empty font builder for creating a new font from scratch.
   *
   * @return an empty font builder
   */
  public Builder newFontBuilder() {
    return Font.Builder.getOTFBuilder(this);
  }
}
