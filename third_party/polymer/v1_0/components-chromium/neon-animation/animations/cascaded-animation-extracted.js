Polymer({

    is: 'cascaded-animation',

    behaviors: [
      Polymer.NeonAnimationBehavior
    ],

    /**
     * @param {{
     *   animation: string,
     *   nodes: !Array<!Element>,
     *   nodeDelay: (number|undefined),
     *   timing: (Object|undefined)
     *  }} config
     */
    configure: function(config) {
      this._animations = [];
      var nodes = config.nodes;
      var effects = [];
      var nodeDelay = config.nodeDelay || 50;

      config.timing = config.timing || {};
      config.timing.delay = config.timing.delay || 0;

      var oldDelay = config.timing.delay;
      var abortedConfigure;
      for (var node, index = 0; node = nodes[index]; index++) {
        config.timing.delay += nodeDelay;
        config.node = node;

        var animation = document.createElement(config.animation);
        if (animation.isNeonAnimation) {
          var effect = animation.configure(config);

          this._animations.push(animation);
          effects.push(effect);
        } else {
          console.warn(this.is + ':', config.animation, 'not found!');
          abortedConfigure = true;
          break;
        }
      }
      config.timing.delay = oldDelay;
      config.node = null;
      // if a bad animation was configured, abort config.
      if (abortedConfigure) {
        return;
      }

      this._effect = new GroupEffect(effects);
      return this._effect;
    },

    complete: function() {
      for (var animation, index = 0; animation = this._animations[index]; index++) {
        animation.complete(animation.config);
      }
    }

  });