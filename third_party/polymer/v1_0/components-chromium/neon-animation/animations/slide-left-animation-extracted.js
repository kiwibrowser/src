Polymer({

    is: 'slide-left-animation',

    behaviors: [
      Polymer.NeonAnimationBehavior
    ],

    configure: function(config) {
      var node = config.node;

      this._effect = new KeyframeEffect(node, [
        {'transform': 'none'},
        {'transform': 'translateX(-100%)'}
      ], this.timingFromConfig(config));

      if (config.transformOrigin) {
        this.setPrefixedProperty(node, 'transformOrigin', config.transformOrigin);
      } else {
        this.setPrefixedProperty(node, 'transformOrigin', '0 50%');
      }

      return this._effect;
    }

  });