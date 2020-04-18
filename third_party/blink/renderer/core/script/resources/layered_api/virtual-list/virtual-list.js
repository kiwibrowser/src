import {Repeats} from './virtual-repeater.js';

export class RangeChangeEvent extends Event {
  constructor(type, init) {
    super(type, init);
    this._first = Math.floor(init.first || 0);
    this._last = Math.floor(init.last || 0);
  }
  get first() {
    return this._first;
  }
  get last() {
    return this._last;
  }
}

export const RepeatsAndScrolls = Superclass => class extends Repeats
(Superclass) {
  constructor(config) {
    super();
    this._num = 0;
    this._first = -1;
    this._last = -1;
    this._prevFirst = -1;
    this._prevLast = -1;

    this._pendingUpdateView = null;
    this._isContainerVisible = false;
    this._containerElement = null;

    if (config) {
      Object.assign(this, config);
    }
  }

  get container() {
    return this._container;
  }
  set container(container) {
    if (container === this._container) {
      return;
    }

    removeEventListener('scroll', this);
    removeEventListener('resize', this);

    super.container = container;

    if (container) {
      addEventListener('scroll', this);
      addEventListener('resize', this);
      this._scheduleUpdateView();
    }

    // Update the containerElement, copy min-width/height styles to new
    // container.
    let containerStyle = null;
    if (this._containerElement) {
      containerStyle = this._containerElement.getAttribute('style');
      this._containerElement.removeAttribute('style');
    }
    // Consider document fragments as shadowRoots.
    this._containerElement =
        (container && container.nodeType === Node.DOCUMENT_FRAGMENT_NODE) ?
        container.host :
        container;

    if (this._containerElement && containerStyle) {
      this._containerElement.setAttribute('style', containerStyle);
    }
  }

  get layout() {
    return this._layout;
  }
  set layout(layout) {
    if (layout === this._layout) {
      return;
    }

    if (this._layout) {
      this._measureCallback = null;
      this._layout.removeEventListener('scrollsizechange', this);
      this._layout.removeEventListener('scrollerrorchange', this);
      this._layout.removeEventListener('itempositionchange', this);
      this._layout.removeEventListener('rangechange', this);
      // Remove min-width/height from containerElement so
      // layout can get correct viewport size.
      if (this._containerElement) {
        this._containerElement.removeAttribute('style');
        this.requestRemeasure();
      }
    }

    this._layout = layout;

    if (this._layout) {
      if (typeof this._layout.updateItemSizes === 'function') {
        this._measureCallback = this._layout.updateItemSizes.bind(this._layout);
      }
      this._layout.addEventListener('scrollsizechange', this);
      this._layout.addEventListener('scrollerrorchange', this);
      this._layout.addEventListener('itempositionchange', this);
      this._layout.addEventListener('rangechange', this);
      this._scheduleUpdateView();
    }
  }

  requestReset() {
    super.requestReset();
    this._scheduleUpdateView();
  }

  /**
   * @param {!Event} event
   * @private
   */
  handleEvent(event) {
    switch (event.type) {
      case 'scroll':
      case 'resize':
        this._scheduleUpdateView();
        break;
      case 'scrollsizechange':
        this._sizeContainer(event.detail);
        break;
      case 'scrollerrorchange':
        this._correctScrollError(event.detail);
        break;
      case 'itempositionchange':
        this._positionChildren(event.detail);
        break;
      case 'rangechange':
        this._adjustRange(event.detail);
        break;
      default:
        console.warn('event not handled', event);
    }
  }

  // Rename _ordered to _kids?
  /**
   * @protected
   */
  get _kids() {
    return this._ordered;
  }
  /**
   * @private
   */
  _scheduleUpdateView() {
    if (!this._pendingUpdateView && this._container && this._layout) {
      this._pendingUpdateView =
          Promise.resolve().then(() => this._updateView());
    }
  }
  /**
   * @private
   */
  _updateView() {
    this._pendingUpdateView = null;

    this._layout.totalItems = this._items ? this._items.length : 0;

    const listBounds = this._containerElement.getBoundingClientRect();
    // Avoid updating viewport if container is not visible.
    this._isContainerVisible = Boolean(
        listBounds.width || listBounds.height || listBounds.top ||
        listBounds.left);
    if (!this._isContainerVisible) {
      return;
    }

    const scrollerWidth = window.innerWidth;
    const scrollerHeight = window.innerHeight;
    const xMin = Math.max(0, Math.min(scrollerWidth, listBounds.left));
    const yMin = Math.max(0, Math.min(scrollerHeight, listBounds.top));
    const xMax = this._layout.direction === 'vertical' ?
        Math.max(0, Math.min(scrollerWidth, listBounds.right)) :
        scrollerWidth;
    const yMax = this._layout.direction === 'vertical' ?
        scrollerHeight :
        Math.max(0, Math.min(scrollerHeight, listBounds.bottom));
    const width = xMax - xMin;
    const height = yMax - yMin;
    this._layout.viewportSize = {width, height};

    const left = Math.max(0, -listBounds.x);
    const top = Math.max(0, -listBounds.y);
    this._layout.scrollTo({top, left});
  }
  /**
   * @private
   */
  _sizeContainer(size) {
    const style = this._containerElement.style;
    style.minWidth = size.width ? size.width + 'px' : null;
    style.minHeight = size.height ? size.height + 'px' : null;
  }
  /**
   * @private
   */
  async _positionChildren(pos) {
    await Promise.resolve();
    const kids = this._kids;
    Object.keys(pos).forEach(key => {
      const idx = key - this._first;
      const child = kids[idx];
      if (child) {
        const {top, left} = pos[key];
        // console.debug(`_positionChild #${this._container.id} > #${child.id}:
        // top ${top}`);
        child.style.position = 'absolute';
        child.style.transform = `translate(${left}px, ${top}px)`;
      }
    });
  }
  /**
   * @private
   */
  _adjustRange(range) {
    this.num = range.num;
    this.first = range.first;
    this._incremental = !(range.stable);
    if (range.remeasure) {
      this.requestRemeasure();
    } else if (range.stable) {
      this._notifyStable();
    }
  }
  /**
   * @protected
   */
  _shouldRender() {
    return Boolean(
        this._isContainerVisible && this._layout && super._shouldRender());
  }
  /**
   * @private
   */
  _correctScrollError(err) {
    window.scroll(window.scrollX - err.left, window.scrollY - err.top);
  }
  /**
   * @protected
   */
  _notifyStable() {
    const {first, num} = this;
    const last = first + num;
    this._container.dispatchEvent(
        new RangeChangeEvent('rangechange', {first, last}));
  }
};

export const VirtualList = RepeatsAndScrolls(class {});
