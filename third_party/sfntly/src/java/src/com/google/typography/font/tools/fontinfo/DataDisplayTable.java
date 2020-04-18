// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.tools.fontinfo;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * A row-oriented table of strings with labeled columns that is intended to be
 * displayed by an application.
 *
 * @author Han-Wen Yeh
 *
 */
public class DataDisplayTable {
  private List<String> header;
  private List<List<String>> data;
  private List<Integer> maxColLengths;
  private List<Align> displayAlignment;
  private int numCols;
  private int numRows;

  /**
   * Enum representing the alignment of the text when being displayed
   */
  public enum Align {
    Left, Right
  }

  /**
   * Constructor. Creates an empty table with the given header labels. The
   * number of columns is the size of the header array, which cannot be changed
   * after initialisation.
   *
   * @param header
   *          the header columns of the table
   */
  public DataDisplayTable(List<String> header) {
    if (header.size() < 1) {
      throw new UnsupportedOperationException("Table must have at least one column");
    }

    this.header = Collections.unmodifiableList(new ArrayList<String>(header));
    data = new ArrayList<List<String>>();
    numCols = header.size();
    numRows = 0;

    // Initialise the maximum length for each column based on the header
    maxColLengths = new ArrayList<Integer>(numCols);
    for (int i = 0; i < numCols; i++) {
      maxColLengths.add(header.get(i).length());
    }

    // Initialise all columns to be left-aligned
    displayAlignment = new ArrayList<Align>(numCols);
    for (int i = 0; i < numCols; i++) {
      displayAlignment.add(Align.Left);
    }
  }

  /**
   * Sets display alignment of each table column.
   *
   * @param alignment
   *          array of values for each column alignment
   * @throws UnsupportedOperationException
   *           if array size is different from column count
   */
  public void setAlignment(List<Align> alignment) {
    if (alignment.size() != numCols) {
      throw new UnsupportedOperationException("Array is wrong size");
    }

    displayAlignment = Collections.unmodifiableList(new ArrayList<Align>(alignment));
  }

  /**
   * Inserts a row of data into the table.
   *
   * @param row
   *          the row of data to insert into the table
   * @throws UnsupportedOperationException
   *           if array size is different from column
   */
  public void add(List<String> row) {
    if (row.size() != numCols) {
      throw new UnsupportedOperationException("Array is wrong size");
    }

    data.add(Collections.unmodifiableList(new ArrayList<String>(row)));
    numRows++;

    // Modify the maximum size of each column
    for (int i = 0; i < numCols; i++) {
      if (row.get(i).length() > maxColLengths.get(i)) {
        maxColLengths.set(i, row.get(i).length());
      }
    }
  }

  /**
   * @return the table header
   */
  public List<String> getHeader() {
    return header;
  }

  /**
   * @return the table data
   */
  public List<List<String>> getData() {
    return Collections.unmodifiableList(data);
  }

  // TODO getRow(int row)

  // TODO getItem(int row, int col)

  /**
   * @return the number of columns in the table
   */
  public int getNumColumns() {
    return numCols;
  }

  /**
   * @return the number of data rows in the table
   */
  public int getNumRows() {
    return numRows;
  }

  /**
   * @return the maximum column lengths in each column
   */
  public List<Integer> getMaxColLengths() {
    return Collections.unmodifiableList(maxColLengths);
  }

  /**
   * @return the alignment of each column
   */
  public List<Align> getDisplayAlignment() {
    return displayAlignment;
  }

  /**
   * Gets a string representation of the table. This string contains the header
   * in the first line, a line for a separator, and each row of data in a new
   * line. The width of each column is set so that the largest element in each
   * column can fit into he column. Column alignment for printing can also be
   * set in the setAlignment method.
   * <p>
   * This function may run slowly for larger tables due to potential memory
   * issues, and it is suggested that the function prettyPrint be used in the
   * case of printing large tables to the console.
   *
   * @return a string representation of the table.
   */
  public String prettyString() {
    StringBuilder output = new StringBuilder();

    // Add header to output
    for (int i = 0; i < numCols - 1; i++) {
      output.append(padString(header.get(i), displayAlignment.get(i), maxColLengths.get(i)))
          .append("  ");
    }
    if (displayAlignment.get(numCols - 1) == Align.Left) {
      // Do not pad last column if left-aligned
      output.append(header.get(numCols - 1));
    } else {
      output.append(padString(header.get(numCols - 1), displayAlignment.get(numCols - 1),
          maxColLengths.get(numCols - 1)));
    }
    output.append("\n");

    // Add separator to output
    output.append(repeatCharacter('-', maxColLengths.get(0)));
    for (int i = 1; i < numCols; i++) {
      output.append("  ").append(repeatCharacter('-', maxColLengths.get(i)));
    }
    output.append("\n");

    // Add data to output
    for (List<String> row : data) {
      for (int i = 0; i < numCols - 1; i++) {
        output.append(padString(row.get(i), displayAlignment.get(i), maxColLengths.get(i)))
            .append("  ");
      }
      if (displayAlignment.get(numCols - 1) == Align.Left) {
        // Do not pad last column if left-aligned
        output.append(row.get(numCols - 1));
      } else {
        output.append(padString(row.get(numCols - 1), displayAlignment.get(numCols - 1),
            maxColLengths.get(numCols - 1)));
      }
      output.append("\n");
    }

    return output.toString();
  }

  /**
   * Prints the data represented by the table to the terminal. The format of the
   * output is: The header in the first line, a separator in the second line,
   * and each row of the data is in a new line afterwards. The width of each
   * column is set so that the largest element in each column can fit into he
   * column. Column alignment for printing can also be set in the setAlignment
   * method.
   */
  public void prettyPrint() {
    System.out.println(prettyString());
  }

  /**
   * Gets the table as a string of comma-separated values
   *
   * @return a CSV string that represents the table
   */
  public String csvString() {
    String[] csvArr = csvStringArray();
    StringBuilder output = new StringBuilder();
    for (String row : csvArr) {
      output.append(row).append('\n');
    }
    return output.toString();
  }

  /**
   * Gets the table as an array of strings, where each string is a row in the
   * table as comma-separated values. This allows for the appending of
   * additional values to each row before serialising to a CSV file
   *
   * @return an array of CSV strings
   */
  public String[] csvStringArray() {
    String[] output = new String[this.numRows + 1];

    // Add header to output
    StringBuilder rowString = new StringBuilder(csvFormat(header.get(0)));
    for (int i = 1; i < numCols; i++) {
      rowString.append(",").append(csvFormat(header.get(i)));
    }
    output[0] = rowString.toString();

    // Add data to output
    for (int i = 0; i < numRows; i++) {
      List<String> row = data.get(i);
      rowString = new StringBuilder(csvFormat(row.get(0)));
      for (int j = 1; j < numCols; j++) {
        rowString.append(",").append(csvFormat(row.get(j)));
      }
      output[i + 1] = rowString.toString();
    }

    return output;
  }

  /**
   * Formats a string and returns it so that it can be inserted into a CSV file
   * without disrupting the formatting of the file and the string. Specifically,
   * quotation marks are added around the string if it contains new-line
   * characters, commas, or quotation marks. Each quotation mark inside the
   * string is also replaced with two quotation marks.
   *
   * @param s
   * @return the formatted string
   */
  private static String csvFormat(String s) {
    if (s.contains("\"") || s.contains("\n") || s.contains(",")) {
      return "\"" + s.replace("\"", "\"\"") + "\"";
    }
    return s;
  }

  @Override
  public String toString() {
    StringBuilder debugString = new StringBuilder();
    debugString.append(numRows).append("x").append(numCols).append(" table, ");
    debugString.append("header=[").append(header.get(0));
    for (int i = 1; i < numCols; i++) {
      debugString.append(", ").append(header.get(i));
    }
    debugString.append("]");

    return debugString.toString();
  }

  /**
   * Adds padding to the a string with a repeating character if the string's
   * length is less than the minimum length.
   *
   * @param s
   *          the string to add padding to
   * @param alignment
   *          the way the string should be aligned in a list after the padding
   * @param minLength
   *          the length to pad the string to
   * @return the padded string
   */
  private static String padString(String s, Align alignment, int minLength) {
    if (alignment == Align.Left) {
      return padRight(s, minLength);
    } else if (alignment == Align.Right) {
      return padLeft(s, minLength);
    } else {
      throw new IndexOutOfBoundsException("Invalid alignment");
    }
  }

  /**
   * Adds padding to the beginning of a string with a repeating character if the
   * string's length is less than the minimum length
   *
   * @param s
   *          the string to add padding to
   * @param minLength
   *          the length to pad the string to
   * @return the padded string
   */
  private static String padLeft(String s, int minLength) {
    return String.format("%1$" + minLength + "s", s);
  }

  /**
   * Adds padding to the end of a string with a repeating character if the
   * string's length is less than the minimum length
   *
   * @param s
   *          the string to add padding to
   * @param minLength
   *          the length to pad the string to
   * @return the padded string
   */
  private static String padRight(String s, int minLength) {
    return String.format("%1$-" + minLength + "s", s);
  }

  /**
   * Returns a string that is a character repeated a specified number of times
   *
   * @param c
   *          the character to repeat
   * @param frequency
   *          the number of times to repeat the character
   * @return a string that is a character repeated a specified number of times
   */
  private static String repeatCharacter(char c, int frequency) {
    StringBuilder output = new StringBuilder(frequency);
    for (int i = 0; i < frequency; i++) {
      output.append(c);
    }
    return output.toString();
  }
}
