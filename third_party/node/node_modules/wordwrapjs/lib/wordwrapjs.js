'use strict'
var arrayify = require('array-back')
var os = require('os')
var t = require('typical')

/**
Word wrapping, with a few features.

- multilingual - wraps any language using whitespace word separation.
- force-break option
- ignore pattern option (e.g. ansi escape sequences)
- wraps hypenated words

@module wordwrapjs
@example
Wrap some sick bars in a 20 character column.

```js
> wrap = require("wordwrapjs")

> bars = "I'm rapping. I'm rapping. I'm rap rap rapping. I'm rap rap rap rap rappity rapping."
> result = wrap(bars, { width: 20 })
```

`result` now looks like this:
```
I'm rapping. I'm
rapping. I'm rap rap
rapping. I'm rap rap
rap rap rappity
rapping.
```

By default, long words will not break. Unless you insist.
```js
> url = "https://github.com/75lb/wordwrapjs"

> wrap.lines(url, { width: 18 })
[ 'https://github.com/75lb/wordwrapjs' ]

> wrap.lines(url, { width: 18, break: true })
[ 'https://github.com', '/75lb/wordwrapjs' ]
```
*/
module.exports = wrap

var re = {
  nonWhitespaceCharsOrNewLine: /[^\s-]+?-\b|\S+|\r\n?|\n/g,
  singleNewLine: /^(\r\n?|\n)$/
}


/**
@param {string} - the input text to wrap
@param [options] {object} - optional config
@param [options.width=30] {number} - the max column width in characters
@param [options.ignore] {RegExp | RegExp[]} - one or more patterns to be ignored when sizing the newly wrapped lines. For example `ignore: /\u001b.*?m/g` will ignore unprintable ansi escape sequences.
@param [options.break] {boolean} - if true, words exceeding the specified `width` will be forcefully broken
@param [options.eol=os.EOL] {string} - the desired new line character to use, defaults to [os.EOL](https://nodejs.org/api/os.html#os_os_eol).
@return {string}
@alias module:wordwrapjs
*/
function wrap (text, options) {
  options = defaultOptions(options)
  text = validateInput(text)

  var lines = wrap.lines(text, options)
  return lines.join(options.eol)
}

/**
returns the wrapped output as an array of lines, rather than a single string
@param {string} - the input text to wrap
@param [options] {object} - same options as {@link module:wordwrapjs|wrap}
@return {Array}
@example
> bars = "I'm rapping. I'm rapping. I'm rap rap rapping. I'm rap rap rap rap rappity rapping."
> wrap.lines(bars)
[ "I'm rapping. I'm rapping. I'm",
  "rap rap rapping. I'm rap rap",
  "rap rap rappity rapping." ]
*/
wrap.lines = function (text, options) {
  options = defaultOptions(options)
  text = validateInput(text)

  var words = wrap.getWords(text)

  var lineLength = 0
  var lines = []
  var line = ''

  if (options.break) {
    var broken = []
    words.forEach(function (word) {
      var wordLength = options.ignore
        ? replaceIgnored(word, options.ignore).length
        : word.length

      if (wordLength > options.width) {
        var letters = word.split('')
        var section
        while ((section = letters.splice(0, options.width)).length) {
          broken.push(section.join(''))
        }
      } else {
        broken.push(word)
      }
    })
    words = broken
  }

  /* for each word, either extend the current `line` or create a new one */
  words.forEach(function (word) {
    if (re.singleNewLine.test(word)) {
      lines.push(line || '')
      line = ''
      lineLength = 0
    } else {
      var wordLength = options.ignore
        ? replaceIgnored(word, options.ignore).length
        : word.length

      var offset
      if (lineLength > options.width) {
        offset = 0
      } else {
        if (/-$/.test(line)) {
          offset = 0
        } else {
          offset = line ? 1 : 0
        }
      }

      lineLength += wordLength + offset

      if (lineLength > options.width) {
        /* Can't fit word on line, cache line and create new one */
        if (line) lines.push(line)
        line = word
        lineLength = wordLength
      } else {
        if (/-$/.test(line)) {
          line += word
        } else {
          line += (line ? ' ' : '') + word
        }
      }
    }
  })

  if (line) lines.push(line)

  return lines
}

/**
 * Returns true if the input text is wrappable
 * @param {string} - input text
 * @return {boolean}
 */
wrap.isWrappable = function(text) {
  if (t.isDefined(text)) {
    text = String(text)
    var matches = text.match(re.nonWhitespaceCharsOrNewLine)
    return matches ? matches.length > 1 : false
  }
}

/**
 * Splits the input text returning an array of words
 * @param {string} - input text
 * @returns {string[]}
 */
wrap.getWords = function(text) {
  return text.match(re.nonWhitespaceCharsOrNewLine) || []
}

function replaceIgnored (string, ignore) {
  arrayify(ignore).forEach(function (pattern) {
    string = string.replace(pattern, '')
  })
  return string
}

function defaultOptions (options) {
  options = options || {}
  options.width = options.width || 30
  options.eol = options.eol || os.EOL
  return options
}

function validateInput (input) {
  if (t.isString(input)) {
    return input
  } else if (!t.isDefined(input)) {
    return ''
  } else {
    return String(input)
  }
}
