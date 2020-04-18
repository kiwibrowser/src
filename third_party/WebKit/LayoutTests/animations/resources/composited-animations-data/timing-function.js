var timingFunctionTests = {
  tests: {
    translateSteps: {
      keyframes: [
        {transform: 'translateX(100px)'},
        {transform: 'translateX(500px)'}
      ],
      easing: 'steps(9)',
      style: 'background: maroon; float: none;',
      samples: [
        {at: 0},
        {at: 0.25},
        {at: 0.5},
        {at: 0.75},
        {at: 1.1},
      ]
    },

    translateStepsPerKeyframe: {
      keyframes: [
        {transform: 'translateX(100px)', easing: 'steps(18)'},
        {transform: 'translateX(400px)', easing: 'steps(9)'},
        {transform: 'translateX(500px)'}
      ],
      style: 'background: navy; float: none;',
      samples: [
        {at: 0},
        {at: 0.25},
        {at: 0.35},
        {at: 0.5},
        {at: 0.65},
        {at: 0.75},
        {at: 1.1},
      ]
    },

    translateStepsWithCubicBezier: {
      keyframes: [
        {transform: 'translateX(100px)', easing: 'steps(9)'},
        {transform: 'translateX(500px)'}
      ],
      easing: 'cubic-bezier(.5, -1, .5, 2)',
      style: 'background: black; float: none;',
      samples: [
        {at: 0},
        {at: 0.4},
        {at: 0.45},
        {at: 0.5},
        {at: 0.6},
        {at: 1.41},
      ]
    },

    translateCubicBezierWithSteps: {
      keyframes: [
        {transform: 'translateX(100px)', easing: 'cubic-bezier(.5, -1, .5, 2)'},
        {transform: 'translateX(500px)'}
      ],
      easing: 'steps(9)',
      style: 'background: peru; float: none;',
      samples: [
        {at: 0},
        {at: 0.25},
        {at: 0.35},
        {at: 0.5},
        {at: 0.65},
        {at: 0.75},
        {at: 1.39},
      ]
    },
  }
};
