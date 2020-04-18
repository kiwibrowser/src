package com.google.typography.font.tools.fontinfo;

import com.beust.jcommander.Parameter;

import java.util.ArrayList;
import java.util.List;

public class CommandOptions {
  @Parameter(description = "FONT_LOCATION")
  public List<String> files = new ArrayList<String>();

  @Parameter(names = {
      "-h", "-?", "--help" }, help = true, description = "Show a list of possible options")
  public Boolean help = false;

  @Parameter(names = { "-a", "--all" }, description = "Show all the information for the font")
  public Boolean all = false;

  @Parameter(names = { "-c", "--csv" }, description = "Show the information in CSV format")
  public Boolean csv = false;

  @Parameter(names = { "-d", "--detailed" }, description = "Show more detailed information")
  public Boolean detailed = false;

  @Parameter(names = { "-m", "--metrics" }, description = "Display font metrics")
  public Boolean metrics = false;

  @Parameter(names = { "-g", "--general" }, description = "Display general font information")
  public Boolean general = false;

  @Parameter(names = { "-p", "--cmap" }, description = "Display list of cmaps")
  public Boolean cmap = false;

  @Parameter(names = { "-r", "--chars" }, description = "Display list of all characters in the font")
  public Boolean chars = false;

  @Parameter(names = {
      "-b", "--blocks" }, description = "Display block coverage for the characters in the font")
  public Boolean blocks = false;

  @Parameter(names = {
      "-s", "--scripts" }, description = "Display script coverage for the characters in the font")
  public Boolean scripts = false;

  @Parameter(names = {
      "-l", "--glyphs" }, description = "Display information about glyphs in the font")
  public Boolean glyphs = false;
}
