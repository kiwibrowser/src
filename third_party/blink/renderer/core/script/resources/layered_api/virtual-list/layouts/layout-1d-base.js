export default class Layout extends EventTarget {
  constructor(config) {
    super();

    this._physicalMin = 0;
    this._physicalMax = 0;

    this._first = -1;
    this._last = -1;

    this._latestCoords = {left: 0, top: 0};

    this._itemSize = {width: 100, height: 100};
    this._spacing = 0;

    this._virtualScroll = false;

    this._sizeDim = 'height';
    this._secondarySizeDim = 'width';
    this._positionDim = 'top';
    this._secondaryPositionDim = 'left';
    this._direction = 'vertical';

    this._scrollPosition = 0;
    this._viewportSize = {width: 0, height: 0};
    this._totalItems = 0;

    this._scrollSize = 1;

    this._overhang = 150;

    Object.assign(this, config);
  }

  // public properties

  set virtualScroll(bool) {
    this._virtualScroll = bool;
  }

  get virtualScroll() {
    return this._virtualScroll;
  }

  set spacing(px) {
    if (px !== this._spacing) {
      this._spacing = px;
      this._scheduleReflow();
    }
  }

  get spacing() {
    return this._spacing;
  }

  set itemSize(dims) {
    const {_itemDim1, _itemDim2} = this;
    Object.assign(this._itemSize, dims);
    if (_itemDim1 !== this._itemDim1 || _itemDim2 !== this._itemDim2) {
      if (_itemDim2 !== this._itemDim2) {
        this._itemDim2Changed();
      } else {
        this._scheduleReflow();
      }
    }
  }

  _itemDim2Changed() {
    // Override
  }

  get _delta() {
    return this._itemDim1 + this._spacing;
  }

  get _itemDim1() {
    return this._itemSize[this._sizeDim];
  }

  get _itemDim2() {
    return this._itemSize[this._secondarySizeDim];
  }

  get itemSize() {
    return this._itemSize;
  }

  set direction(dir) {
    // Force it to be either horizontal or vertical.
    dir = (dir === 'horizontal') ? dir : 'vertical';
    if (dir !== this._direction) {
      this._direction = dir;
      this._sizeDim = (dir === 'horizontal') ? 'width' : 'height';
      this._secondarySizeDim = (dir === 'horizontal') ? 'height' : 'width';
      this._positionDim = (dir === 'horizontal') ? 'left' : 'top';
      this._secondaryPositionDim = (dir === 'horizontal') ? 'top' : 'left';
      this._scheduleReflow();
    }
  }

  get direction() {
    return this._direction;
  }

  set viewportSize(dims) {
    const {_viewDim1, _viewDim2} = this;
    Object.assign(this._viewportSize, dims);
    if (_viewDim1 !== this._viewDim1 || _viewDim2 !== this._viewDim2) {
      if (_viewDim2 !== this._viewDim2) {
        this._viewDim2Changed();
      } else {
        this._checkThresholds();
      }
    }
  }

  _viewDim2Changed() {
    // Override
  }

  get _viewDim1() {
    return this._viewportSize[this._sizeDim];
  }

  get _viewDim2() {
    return this._viewportSize[this._secondarySizeDim];
  }

  get viewportSize() {
    return this._viewportSize;
  }

  set totalItems(num) {
    if (num !== this._totalItems) {
      this._totalItems = num;
      this._maxIdx = num - 1;
      this._scheduleReflow();
    }
  }

  get totalItems() {
    return this._totalItems;
  }

  // private properties

  get _num() {
    if (this._first === -1 || this._last === -1) {
      return 0;
    }
    return this._last - this._first + 1;
  }

  // public methods

  scrollTo(coords) {
    this._latestCoords = coords;
    this._scroll();
  }

  //

  _scroll() {
    this._scrollPosition = this._latestCoords[this._positionDim];

    this._checkThresholds();
  }

  _getActiveItems() {
    // Override
  }

  // TODO: Does this need to be public?
  _reflow() {
    const {_first, _last, _scrollSize} = this;

    this._updateScrollSize();
    this._getActiveItems();

    if (this._scrollSize !== _scrollSize) {
      this._emitScrollSize();
    }

    if (this._first === -1 && this._last === -1) {
      this._emitRange();
    } else if (
        this._first !== _first || this._last !== _last ||
        this._spacingChanged) {
      this._emitRange();
      this._emitChildPositions();
    }
    this._pendingReflow = null;
  }

  _scheduleReflow() {
    if (!this._pendingReflow) {
      this._pendingReflow = Promise.resolve().then(() => this._reflow());
    }
  }

  _updateScrollSize() {
    // Ensure we have at least 1px - this allows getting at least 1 item to be
    // rendered.
    this._scrollSize = Math.max(1, this._totalItems * this._delta);
  }

  _checkThresholds() {
    if (this._viewDim1 === 0 && this._num > 0) {
      this._scheduleReflow();
    } else {
      const min = Math.max(0, this._scrollPosition - this._overhang);
      const max = Math.min(
          this._scrollSize,
          this._scrollPosition + this._viewDim1 + this._overhang);
      if (this._physicalMin > min || this._physicalMax < max) {
        this._scheduleReflow();
      }
    }
  }

  ///

  _emitRange(inProps) {
    const detail = Object.assign(
        {
          first: this._first,
          last: this._last,
          num: this._num,
          stable: true,
        },
        inProps);
    this.dispatchEvent(new CustomEvent('rangechange', {detail}));
  }

  _emitScrollSize() {
    const detail = {
      [this._sizeDim]: this._scrollSize,
    };
    this.dispatchEvent(new CustomEvent('scrollsizechange', {detail}));
  }

  _emitScrollError() {
    if (this._scrollError) {
      const detail = {
        [this._positionDim]: this._scrollError,
        [this._secondaryPositionDim]: 0,
      };
      this.dispatchEvent(new CustomEvent('scrollerrorchange', {detail}));
      this._scrollError = 0;
    }
  }

  _emitChildPositions() {
    const detail = {};
    for (let idx = this._first; idx <= this._last; idx++) {
      detail[idx] = this._getItemPosition(idx);
    }
    this.dispatchEvent(new CustomEvent('itempositionchange', {detail}));
  }

  _getItemPosition(idx) {
    // Override.
  }
}