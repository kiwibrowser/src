'use strict';

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; }

var Section = require('./section');
var Content = require('./content');

var ContentSection = function (_Section) {
  _inherits(ContentSection, _Section);

  function ContentSection(section) {
    _classCallCheck(this, ContentSection);

    var _this = _possibleConstructorReturn(this, (ContentSection.__proto__ || Object.getPrototypeOf(ContentSection)).call(this));

    _this.header(section.header);

    if (section.content) {
      if (section.raw) {
        _this.add(section.content);
      } else {
        var content = new Content(section.content);
        _this.add(content.lines());
      }

      _this.emptyLine();
    }
    return _this;
  }

  return ContentSection;
}(Section);

module.exports = ContentSection;