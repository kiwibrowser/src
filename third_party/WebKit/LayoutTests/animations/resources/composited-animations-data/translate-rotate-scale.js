var translateRotateScaleTests = {
  tests: {
    animateTranslate: {
      keyframes: [
        {translate: '0px'},
        {translate: '600px'}
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

    animateRotate: {
      keyframes: [
        {rotate: '-10deg'},
        {rotate: '470deg'}
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

    animateScale: {
      keyframes: [
        {scale: '0.1'},
        {scale: '1'}
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

    animateScale: {
      keyframes: [
        {scale: '0.1 1'},
        {scale: '1'}
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
  }
};
