#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generate keyboard layout and hotkey data for the keyboard overlay.

This script fetches data from the keyboard layout and hotkey data spreadsheet,
and output the data depending on the option.

  --cc: Rewrites a part of C++ code in
      chrome/browser/chromeos/webui/keyboard_overlay_ui.cc

  --grd: Rewrites a part of grd messages in
      chrome/app/generated_resources.grd

  --js: Rewrites the entire JavaScript code in
      chrome/browser/resources/keyboard_overlay/keyboard_overlay_data.js

These options can be specified at the same time.

e.g.
python gen_keyboard_overlay_data.py --cc --grd --js

The output directory of the generated files can be changed with --outdir.

e.g. (This will generate tmp/keyboard_overlay.js)
python gen_keyboard_overlay_data.py --outdir=tmp --js
"""

import cStringIO
import datetime
import gdata.spreadsheet.service
import getpass
import json
import optparse
import os
import re
import sys

MODIFIER_SHIFT = 1 << 0
MODIFIER_CTRL = 1 << 1
MODIFIER_ALT = 1 << 2

KEYBOARD_GLYPH_SPREADSHEET_KEY = '0Ao3KldW9piwEdExLbGR6TmZ2RU9aUjFCMmVxWkVqVmc'
HOTKEY_SPREADSHEET_KEY = '0AqzoqbAMLyEPdE1RQXdodk1qVkFyTWtQbUxROVM1cXc'
CC_OUTDIR = 'chrome/browser/ui/webui/chromeos'
CC_FILENAME = 'keyboard_overlay_ui.cc'
GRD_OUTDIR = 'chrome/app'
GRD_FILENAME = 'chromeos_strings.grdp'
JS_OUTDIR = 'chrome/browser/resources/chromeos'
JS_FILENAME = 'keyboard_overlay_data.js'
CC_START = r'IDS_KEYBOARD_OVERLAY_INSTRUCTIONS_HIDE },'
CC_END = r'};'
GRD_START = r'  <!-- BEGIN GENERATED KEYBOARD OVERLAY STRINGS -->'
GRD_END = r'  <!-- END GENERATED KEYBOARD OVERLAY STRINGS -->'

LABEL_MAP = {
  'glyph_arrow_down': 'down',
  'glyph_arrow_left': 'left',
  'glyph_arrow_right': 'right',
  'glyph_arrow_up': 'up',
  'glyph_back': 'back',
  'glyph_backspace': 'backspace',
  'glyph_brightness_down': 'bright down',
  'glyph_brightness_up': 'bright up',
  'glyph_enter': 'enter',
  'glyph_forward': 'forward',
  'glyph_fullscreen': 'full screen',
  # Kana/Eisu key on Japanese keyboard
  'glyph_ime': u'\u304b\u306a\u0020\u002f\u0020\u82f1\u6570',
  'glyph_lock': 'lock',
  'glyph_overview': 'switch window',
  'glyph_power': 'power',
  'glyph_right': 'right',
  'glyph_reload': 'reload',
  'glyph_search': 'search',
  'glyph_shift': 'shift',
  'glyph_tab': 'tab',
  'glyph_tools': 'tools',
  'glyph_volume_down': 'vol. down',
  'glyph_volume_mute': 'mute',
  'glyph_volume_up': 'vol. up',
};

INPUT_METHOD_ID_TO_OVERLAY_ID = {
  'xkb:be::fra': 'fr',
  'xkb:be::ger': 'de',
  'xkb:be::nld': 'nl',
  'xkb:bg::bul': 'bg',
  'xkb:bg:phonetic:bul': 'bg',
  'xkb:br::por': 'pt_BR',
  'xkb:ca::fra': 'fr_CA',
  'xkb:ca:eng:eng': 'ca',
  'xkb:ch::ger': 'de',
  'xkb:ch:fr:fra': 'fr',
  'xkb:cz::cze': 'cs',
  'xkb:de::ger': 'de',
  'xkb:de:neo:ger': 'de_neo',
  'xkb:dk::dan': 'da',
  'xkb:ee::est': 'et',
  'xkb:es::spa': 'es',
  'xkb:es:cat:cat': 'ca',
  'xkb:fi::fin': 'fi',
  'xkb:fr::fra': 'fr',
  'xkb:gb:dvorak:eng': 'en_GB_dvorak',
  'xkb:gb:extd:eng': 'en_GB',
  'xkb:gr::gre': 'el',
  'xkb:hr::scr': 'hr',
  'xkb:hu::hun': 'hu',
  'xkb:il::heb': 'iw',
  'xkb:it::ita': 'it',
  'xkb:jp::jpn': 'ja',
  'xkb:latam::spa': 'es_419',
  'xkb:lt::lit': 'lt',
  'xkb:lv:apostrophe:lav': 'lv',
  'xkb:no::nob': 'no',
  'xkb:pl::pol': 'pl',
  'xkb:pt::por': 'pt_PT',
  'xkb:ro::rum': 'ro',
  'xkb:rs::srp': 'sr',
  'xkb:ru::rus': 'ru',
  'xkb:ru:phonetic:rus': 'ru',
  'xkb:se::swe': 'sv',
  'xkb:si::slv': 'sl',
  'xkb:sk::slo': 'sk',
  'xkb:tr::tur': 'tr',
  'xkb:ua::ukr': 'uk',
  'xkb:us::eng': 'en_US',
  'xkb:us::fil': 'en_US',
  'xkb:us::ind': 'en_US',
  'xkb:us::msa': 'en_US',
  'xkb:us:altgr-intl:eng': 'en_US_altgr_intl',
  'xkb:us:colemak:eng': 'en_US_colemak',
  'xkb:us:dvorak:eng': 'en_US_dvorak',
  'xkb:us:intl:eng': 'en_US_intl',
  'xkb:us:intl:nld': 'en_US_intl',
  'xkb:us:intl:por': 'en_US_intl',
  'xkb:us:workman:eng': 'en_US_workman',
  'xkb:us:workman-intl:eng': 'en_US_workman_intl',
}

# The file was first generated in 2012 and we have a policy of not updating
# copyright dates.
COPYRIGHT_HEADER=\
"""// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a generated file but may contain local modifications. See
// src/tools/gen_keyboard_overlay_data/gen_keyboard_overlay_data.py --help
"""

# A snippet for grd file
GRD_SNIPPET_TEMPLATE="""  <message name="%s" desc="%s">
    %s
  </message>
"""

# A snippet for C++ file
CC_SNIPPET_TEMPLATE="""  { "%s", %s },
"""


def SplitBehavior(behavior):
  """Splits the behavior to compose a message or i18n-content value.

  Examples:
    'Activate last tab' => ['Activate', 'last', 'tab']
    'Close tab' => ['Close', 'tab']
  """
  return [x for x in re.split('[ ()"-.,]', behavior) if len(x) > 0]


def ToMessageName(behavior):
  """Composes a message name for grd file.

  Examples:
    'Activate last tab' => IDS_KEYBOARD_OVERLAY_ACTIVATE_LAST_TAB
    'Close tab' => IDS_KEYBOARD_OVERLAY_CLOSE_TAB
  """
  segments = [segment.upper() for segment in SplitBehavior(behavior)]
  return 'IDS_KEYBOARD_OVERLAY_' + ('_'.join(segments))


def ToMessageDesc(description):
  """Composes a message description for grd file."""
  message_desc = 'The text in the keyboard overlay to explain the shortcut'
  if description:
    message_desc = '%s (%s).' % (message_desc, description)
  else:
    message_desc += '.'
  return message_desc


def Toi18nContent(behavior):
  """Composes a i18n-content value for HTML/JavaScript files.

  Examples:
    'Activate last tab' => keyboardOverlayActivateLastTab
    'Close tab' => keyboardOverlayCloseTab
  """
  segments = [segment.lower() for segment in SplitBehavior(behavior)]
  result = 'keyboardOverlay'
  for segment in segments:
    result += segment[0].upper() + segment[1:]
  return result


def ToKeys(hotkey):
  """Converts the action value to shortcut keys used from JavaScript.

  Examples:
    'Ctrl - 9' => '9<>CTRL'
    'Ctrl - Shift - Tab' => 'tab<>CTRL<>SHIFT'
  """
  values = hotkey.split(' - ')
  modifiers = sorted(value.upper() for value in values
                     if value in ['Shift', 'Ctrl', 'Alt', 'Search'])
  keycode = [value.lower() for value in values
             if value not in ['Shift', 'Ctrl', 'Alt', 'Search']]
  # The keys which are highlighted even without modifier keys.
  base_keys = ['backspace', 'power']
  if not modifiers and (keycode and keycode[0] not in base_keys):
    return None
  return '<>'.join(keycode + modifiers)


def ParseOptions():
  """Parses the input arguemnts and returns options."""
  # default_username = os.getusername() + '@google.com';
  default_username = '%s@google.com' % os.environ.get('USER')
  parser = optparse.OptionParser()
  parser.add_option('--key', dest='key',
                    help='The key of the spreadsheet (required).')
  parser.add_option('--username', dest='username',
                    default=default_username,
                    help='Your user name (default: %s).' % default_username)
  parser.add_option('--password', dest='password',
                    help='Your password.')
  parser.add_option('--account_type', default='GOOGLE', dest='account_type',
                    help='Account type used for gdata login (default: GOOGLE)')
  parser.add_option('--js', dest='js', default=False, action='store_true',
                    help='Output js file.')
  parser.add_option('--grd', dest='grd', default=False, action='store_true',
                    help='Output resource file.')
  parser.add_option('--cc', dest='cc', default=False, action='store_true',
                    help='Output cc file.')
  parser.add_option('--outdir', dest='outdir', default=None,
                    help='Specify the directory files are generated.')
  (options, unused_args) = parser.parse_args()

  if not options.username.endswith('google.com'):
    print 'google.com account is necessary to use this script.'
    sys.exit(-1)

  if (not (options.js or options.grd or options.cc)):
    print 'Either --js, --grd, or --cc needs to be specified.'
    sys.exit(-1)

  # Get the password from the terminal, if needed.
  if not options.password:
    options.password = getpass.getpass(
        'Application specific password for %s: ' % options.username)
  return options


def InitClient(options):
  """Initializes the spreadsheet client."""
  client = gdata.spreadsheet.service.SpreadsheetsService()
  client.email = options.username
  client.password = options.password
  client.source = 'Spread Sheet'
  client.account_type = options.account_type
  print 'Logging in as %s (%s)' % (client.email, client.account_type)
  client.ProgrammaticLogin()
  return client


def PrintDiffs(message, lhs, rhs):
  """Prints the differences between |lhs| and |rhs|."""
  dif = set(lhs).difference(rhs)
  if dif:
    print message, ', '.join(dif)


def FetchSpreadsheetFeeds(client, key, sheets, cols):
  """Fetch feeds from the spreadsheet.

  Args:
    client: A spreadsheet client to be used for fetching data.
    key: A key string of the spreadsheet to be fetched.
    sheets: A list of the sheet names to read data from.
    cols: A list of columns to read data from.
  """
  worksheets_feed = client.GetWorksheetsFeed(key)
  print 'Fetching data from the worksheet: %s' % worksheets_feed.title.text
  worksheets_data = {}
  titles = []
  for entry in worksheets_feed.entry:
    worksheet_id = entry.id.text.split('/')[-1]
    list_feed = client.GetListFeed(key, worksheet_id)
    list_data = []
    # Hack to deal with sheet names like 'sv (Copy of fl)'
    title = list_feed.title.text.split('(')[0].strip()
    titles.append(title)
    if title not in sheets:
      continue
    print 'Reading data from the sheet: %s' % list_feed.title.text
    for i, entry in enumerate(list_feed.entry):
      line_data = {}
      for k in entry.custom:
        if (k not in cols) or (not entry.custom[k].text):
          continue
        line_data[k] = entry.custom[k].text
      list_data.append(line_data)
    worksheets_data[title] = list_data
  PrintDiffs('Exist only on the spreadsheet: ', titles, sheets)
  PrintDiffs('Specified but do not exist on the spreadsheet: ', sheets, titles)
  return worksheets_data


def FetchKeyboardGlyphData(client):
  """Fetches the keyboard glyph data from the spreadsheet."""
  glyph_cols = ['scancode', 'p0', 'p1', 'p2', 'p3', 'p4', 'p5', 'p6', 'p7',
                'p8', 'p9', 'label', 'format', 'notes']
  keyboard_glyph_data = FetchSpreadsheetFeeds(
      client, KEYBOARD_GLYPH_SPREADSHEET_KEY,
      INPUT_METHOD_ID_TO_OVERLAY_ID.values(), glyph_cols)
  ret = {}
  for lang in keyboard_glyph_data:
    ret[lang] = {}
    keys = {}
    for line in keyboard_glyph_data[lang]:
      scancode = line.get('scancode')
      if (not scancode) and line.get('notes'):
        ret[lang]['layoutName'] = line['notes']
        continue
      del line['scancode']
      if 'notes' in line:
        del line['notes']
      if 'label' in line:
        line['label'] = LABEL_MAP.get(line['label'], line['label'])
      keys[scancode] = line
    # Add a label to space key
    if '39' not in keys:
      keys['39'] = {'label': 'space'}
    ret[lang]['keys'] = keys
  return ret


def FetchLayoutsData(client):
  """Fetches the keyboard glyph data from the spreadsheet."""
  layout_names = ['U_layout', 'J_layout', 'E_layout', 'B_layout']
  cols = ['scancode', 'x', 'y', 'w', 'h']
  layouts = FetchSpreadsheetFeeds(client, KEYBOARD_GLYPH_SPREADSHEET_KEY,
                                  layout_names, cols)
  ret = {}
  for layout_name, layout in layouts.items():
    ret[layout_name[0]] = []
    for row in layout:
      line = []
      for col in cols:
        value = row.get(col)
        if not value:
          line.append('')
        else:
          if col != 'scancode':
            value = float(value)
          line.append(value)
      ret[layout_name[0]].append(line)
  return ret


def FetchHotkeyData(client):
  """Fetches the hotkey data from the spreadsheet."""
  hotkey_sheet = ['Cross Platform Behaviors']
  hotkey_cols = ['behavior', 'context', 'kind', 'actionctrlctrlcmdonmac',
                 'chromeos', 'descriptionfortranslation']
  hotkey_data = FetchSpreadsheetFeeds(client, HOTKEY_SPREADSHEET_KEY,
                                      hotkey_sheet, hotkey_cols)
  action_to_id = {}
  id_to_behavior = {}
  # (behavior, action)
  result = []
  for line in hotkey_data['Cross Platform Behaviors']:
    if (not line.get('chromeos')) or (line.get('kind') != 'Key'):
      continue
    action = ToKeys(line['actionctrlctrlcmdonmac'])
    if not action:
      continue
    behavior = line['behavior'].strip()
    description = line.get('descriptionfortranslation')
    result.append((behavior, action, description))
  return result


def UniqueBehaviors(hotkey_data):
  """Retrieves a sorted list of unique behaviors from |hotkey_data|."""
  return sorted(set((behavior, description) for (behavior, _, description)
                    in hotkey_data),
                cmp=lambda x, y: cmp(ToMessageName(x[0]), ToMessageName(y[0])))


def GetPath(path_from_src):
  """Returns the absolute path of the specified path."""
  path = os.path.join(os.path.dirname(__file__), '../..', path_from_src)
  if not os.path.isfile(path):
    print 'WARNING: %s does not exist. Maybe moved or renamed?' % path
  return path


def OutputFile(outpath, snippet):
  """Output the snippet into the specified path."""
  out = file(outpath, 'w')
  out.write(COPYRIGHT_HEADER + '\n')
  out.write(snippet)
  print 'Output ' + os.path.normpath(outpath)


def RewriteFile(start, end, original_dir, original_filename, snippet,
                outdir=None):
  """Replaces a part of the specified file with snippet and outputs it."""
  original_path = GetPath(os.path.join(original_dir, original_filename))
  original = file(original_path, 'r')
  original_content = original.read()
  original.close()
  if outdir:
    outpath = os.path.join(outdir, original_filename)
  else:
    outpath = original_path
  out = file(outpath, 'w')
  rx = re.compile(r'%s\n.*?%s\n' % (re.escape(start), re.escape(end)),
                  re.DOTALL)
  new_content = re.sub(rx, '%s\n%s%s\n' % (start, snippet, end),
                       original_content)
  out.write(new_content)
  out.close()
  print 'Output ' + os.path.normpath(outpath)


def OutputJson(keyboard_glyph_data, hotkey_data, layouts, var_name, outdir):
  """Outputs the keyboard overlay data as a JSON file."""
  action_to_id = {}
  for (behavior, action, _) in hotkey_data:
    i18nContent = Toi18nContent(behavior)
    action_to_id[action] = i18nContent
  data = {'keyboardGlyph': keyboard_glyph_data,
          'shortcut': action_to_id,
          'layouts': layouts,
          'inputMethodIdToOverlayId': INPUT_METHOD_ID_TO_OVERLAY_ID}

  if not outdir:
    outdir = JS_OUTDIR
  outpath = GetPath(os.path.join(outdir, JS_FILENAME))
  json_data =  json.dumps(data, sort_keys=True, indent=2)
  # Remove redundant spaces after ','
  json_data = json_data.replace(', \n', ',\n')
  # Replace double quotes with single quotes to avoid lint warnings.
  json_data = json_data.replace('\"', '\'')
  snippet = 'var %s = %s;\n' % (var_name, json_data)
  OutputFile(outpath, snippet)


def OutputGrd(hotkey_data, outdir):
  """Outputs a part of messages in the grd file."""
  snippet = cStringIO.StringIO()
  for (behavior, description) in UniqueBehaviors(hotkey_data):
    # Do not generate message for 'Show wrench menu'. It is handled manually
    # based on branding.
    if behavior == 'Show wrench menu':
      continue
    snippet.write(GRD_SNIPPET_TEMPLATE %
                  (ToMessageName(behavior), ToMessageDesc(description),
                   behavior))

  RewriteFile(GRD_START, GRD_END, GRD_OUTDIR, GRD_FILENAME, snippet.getvalue(),
              outdir)


def OutputCC(hotkey_data, outdir):
  """Outputs a part of code in the C++ file."""
  snippet = cStringIO.StringIO()
  for (behavior, _) in UniqueBehaviors(hotkey_data):
    message_name = ToMessageName(behavior)
    output = CC_SNIPPET_TEMPLATE % (Toi18nContent(behavior), message_name)
    # Break the line if the line is longer than 80 characters
    if len(output) > 80:
      output = output.replace(' ' + message_name, '\n    %s' % message_name)
    snippet.write(output)

  RewriteFile(CC_START, CC_END, CC_OUTDIR, CC_FILENAME, snippet.getvalue(),
              outdir)


def main():
  options = ParseOptions()
  client = InitClient(options)
  hotkey_data = FetchHotkeyData(client)

  if options.js:
    keyboard_glyph_data = FetchKeyboardGlyphData(client)

  if options.js:
    layouts = FetchLayoutsData(client)
    OutputJson(keyboard_glyph_data, hotkey_data, layouts, 'keyboardOverlayData',
               options.outdir)
  if options.grd:
    OutputGrd(hotkey_data, options.outdir)
  if options.cc:
    OutputCC(hotkey_data, options.outdir)


if __name__ == '__main__':
  main()
