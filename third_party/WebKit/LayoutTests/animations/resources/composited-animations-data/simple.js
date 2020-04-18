var simpleTests = {
  tests: {
    transformTranslate: {
      keyframes: [
        {transform: 'translateX(0px)'},
        {transform: 'translateX(600px)'}
      ],
      style: 'background: darkgreen; float: none;',
      samples: [
        {at: 0},
        {at: 0.15},
        {at: 0.25},
        {at: 0.35},
        {at: 0.5},
        {at: 0.65},
        {at: 0.75},
        {at: 0.85},
        {at: 1.05},
        {at: 1.15},
      ]
    },

    transformRotate: {
      keyframes: [
        {transform: 'rotate(-10deg)'},
        {transform: 'rotate(360deg)'}
      ],
      style: 'background: maroon; margin: 5px;',
      samples: [
        {at: 0},
        {at: 0.05},
        {at: 0.15},
        {at: 0.25},
        {at: 0.35},
        {at: 0.45},
        {at: 0.5},
        {at: 0.55},
        {at: 0.65},
        {at: 0.75},
        {at: 0.85},
        {at: 0.95},
        {at: 1.05},
        {at: 1.15},
      ]
    },

    transformScale: {
      keyframes: [
        {transform: 'scale(0.1)'},
        {transform: 'scale(1)'}
      ],
      style: 'background: peru; margin: 5px;',
      markerStyle: '', // No marker. This mustn't affect cc/blink consistency, crbug.com/531290
      samples: [
        {at: 0},
        {at: 0.05},
        {at: 0.15},
        {at: 0.25},
        {at: 0.35},
        {at: 0.45},
        {at: 0.5},
        {at: 0.55},
        {at: 0.65},
        {at: 0.75},
        {at: 0.85},
        {at: 0.95},
        {at: 1.05},
        {at: 1.15},
      ]
    },

    animateOpacity: {
      keyframes: [
        {opacity: 0},
        {opacity: 1}
      ],
      style: 'background: navy; margin: 5px;',
      samples: [
        {at: 0},
        {at: 0.05},
        {at: 0.15},
        {at: 0.25},
        {at: 0.35},
        {at: 0.45},
        {at: 0.5},
        {at: 0.55},
        {at: 0.65},
        {at: 0.75},
        {at: 0.85},
        {at: 0.95},
        {at: 1.05},
        {at: 1.15},
      ]
    },
  }
};
