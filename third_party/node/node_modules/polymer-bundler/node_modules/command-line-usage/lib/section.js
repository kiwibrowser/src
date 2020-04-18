'use strict'
const ansi = require('ansi-escape-sequences')
const os = require('os')
const arrayify = require('array-back')

class Section {
  constructor () {
    this.list = []
  }
  add (content) {
    arrayify(content).forEach(line => this.list.push(ansi.format(line)))
  }
  emptyLine () {
    this.list.push('')
  }
  header (text) {
    if (text) {
      this.add(ansi.format(text, [ 'underline', 'bold' ]))
      this.emptyLine()
    }
  }
  toString () {
    return this.list.join(os.EOL)
  }
}

module.exports = Section
