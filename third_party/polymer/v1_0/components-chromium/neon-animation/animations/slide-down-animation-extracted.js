Polymer({

    is: 'slide-down-animation',

    behaviors: [
      Polymer.NeonAnimationBehavior
    ],

    configure: function(config) {
      var node = config.node;

      this._effect = new KeyframeEffect(node, [
        {'transform': 'translateY(0%)'},
        {'transform': 'translateY(100%)'}
      ], this.timingFromConfig(config));

      if (config.transformOrigin) {
        this.setPrefixedProperty(node, 'transformOrigin', config.transformOrigin);
      } else {
        this.setPrefixedProperty(node, 'transformOrigin', '50% 0');
      }

      return this._effect;
    }

  });