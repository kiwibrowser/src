var rotateZeroDegreesTests = {
  style: 'margin: 5px;',
  tests: {
    animateRotateNoFromAxis: {
      keyframes: [
        { rotate: '0deg' },
        { rotate: '0 1 0 90deg' },
      ],
      style: 'background: magenta;',
      samples: getLinearSamples(20, 0, 1)
    },

    animateRotateNoToAxis: {
      keyframes: [
        { rotate: '1 0 0 0deg' },
        { rotate: '90deg' },
      ],
      style: 'background: yellow;',
      samples: getLinearSamples(20, 0, 1)
    },

    animateRotateFromZeroUnder360: {
      keyframes: [
        { rotate: '1 0 0 0deg' },
        { rotate: '0 1 0 90deg' },
      ],
      style: 'background: cyan;',
      samples: getLinearSamples(20, 0, 1)
    },

    animateRotateToZeroUnder360: {
      keyframes: [
        { rotate: '0 1 0 90deg' },
        { rotate: '1 0 0 0deg' },
      ],
      style: 'background: indigo;',
      samples: getLinearSamples(20, 0, 1)
    },

    animateRotateFromZero: {
      keyframes: [
        { rotate: '1 0 0 0deg' },
        { rotate: '0 1 0 450deg' },
      ],
      style: 'background: green;',
      samples: getLinearSamples(20, 0, 1)
    },

    animateRotateToZero: {
      keyframes: [
        { rotate: '0 1 0 450deg' },
        { rotate: '1 0 0 0deg' },
      ],
      style: 'background: red;',
      samples: getLinearSamples(20, 0, 1)
    },

    animateRotateFromAndToZero: {
      keyframes: [
        { rotate: '0 1 0 0deg' },
        { rotate: '1 0 0 0deg' },
      ],
      style: 'background: blue;',
      samples: getLinearSamples(20, 0, 1)
    },
  }
};

