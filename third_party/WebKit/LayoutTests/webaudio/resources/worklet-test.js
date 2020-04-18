// A simple noise generator implemented in an AudioWorkletNode.  This
// node must have at least one AudioParam with limited range for
// testing that appropriate warnings are produced when the AudioParam
// value is set outside the nominal range.
const workletScript =
`
class NoiseGenerator extends AudioWorkletProcessor {
  static get parameterDescriptors() {
    return [{name: 'amplitude', defaultValue: 0.25, minValue: 0, maxValue: 1}];
  }

  constructor(options) {
    super(options);
  }

  process(inputs, outputs, parameters) {
    let output = outputs[0];
    let amplitude = parameters.amplitude;
    for (let channel = 0; channel < output.length; ++channel) {
      let outputChannel = output[channel];
      for (let i = 0; i < outputChannel.length; ++i) {
        outputChannel[i] = 2 * (Math.random() - 0.5) * amplitude[i];
      }
    }

    return true;
  }
}

registerProcessor('noise-generator', NoiseGenerator);
`;

const NoiseGenWorkletUrl = window.URL.createObjectURL(
    new Blob([workletScript], {type: 'text/javascript'}));

