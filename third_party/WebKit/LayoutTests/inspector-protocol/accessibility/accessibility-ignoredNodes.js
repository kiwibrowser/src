(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`
    <div>Non-hidden div for comparison</div>
      <div role='img'>
          <svg data-dump>
              <!-- Children of img role are presentational -->
              <circle xmlns:svg='http://www.w3.org/2000/svg' cx='150px' cy='100px' r='50px' fill='#ff0000' stroke='#000000' stroke-width='5px'/>
          </svg>
      </div>
      <button>
          <span data-dump>Buttons are leaf nodes</span>
      </button>
      <div data-dump aria-hidden='true'>
          <div data-dump>Descendant of aria-hidden node</div>
      </div>
      <ol role='none' data-dump><!-- list is presentational -->
        <li data-dump>List item also presentational</li>
        <div data-dump>Div in list isn't presentational</div>
      </ol>

      <label for='checkbox' data-dump><span data-dump>Content within label refers to label container</span></label>
      <input type='checkbox' id='checkbox'>
      <div style='display: none' data-dump>
        Non-rendered div
        <span data-dump>Span within non-rendered div</span>
        <button aria-hidden='false'>aria-hidden false button</button>
      </div>

      <canvas style='height: 1px; width: 1px;' data-dump></canvas>

      <canvas role='presentation' data-dump><div>Canvas fallback content</div></canvas>

      <select data-dump>
        <option data-dump>Options should be</option>
        <option>sent down even though</option>
        <option>they are grandchildren</option>
      </select>

      <button inert data-dump>inert button</button>
      <div id='inert-root' inert>
        <button data-dump>button in inert subtree</button>
      </div>
  `, 'Tests accessibility values of ignored nodes.');

  var dumpAccessibilityNodesBySelectorAndCompleteTest =
      (await testRunner.loadScript('../resources/accessibility-dumpAccessibilityNodes.js'))(testRunner, session);

  var msg = await dp.DOM.getDocument();
  dumpAccessibilityNodesBySelectorAndCompleteTest('[data-dump]', true, msg);
})
