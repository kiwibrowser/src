'use strict'
const Section = require('./section')
const Content = require('./content')

class ContentSection extends Section {
  constructor (section) {
    super()
    this.header(section.header)

    if (section.content) {
      /* add content without indentation or wrapping */
      if (section.raw) {
        this.add(section.content)
      } else {
        const content = new Content(section.content)
        this.add(content.lines())
      }

      this.emptyLine()
    }
  }
}

module.exports = ContentSection
