Polymer({
      is: 'paper-icon-button-light',

      behaviors: [
        Polymer.PaperRippleBehavior
      ],

      ready: function() {
        Polymer.RenderStatus.afterNextRender(this, () => {
          // Add lazy host listeners
          this.addEventListener('down', this._rippleDown.bind(this));
          this.addEventListener('up', this._rippleUp.bind(this));

          // Assume the button has already been distributed.
          var button = this.getEffectiveChildren()[0];
          this._rippleContainer = button;
          
          // We need to set the focus/blur listeners on the distributed button,
          // not the host, since the host isn't focusable.
          button.addEventListener('focus', this._rippleDown.bind(this));
          button.addEventListener('blur', this._rippleUp.bind(this));
        });
      },
      _rippleDown: function() {
        this.getRipple().uiDownAction();
      },
      _rippleUp: function() {
        this.getRipple().uiUpAction();
      },
      /**
       * @param {...*} var_args
       */
      ensureRipple: function(var_args) {
        var lastRipple = this._ripple;
        Polymer.PaperRippleBehavior.ensureRipple.apply(this, arguments);
        if (this._ripple && this._ripple !== lastRipple) {
          this._ripple.center = true;
          this._ripple.classList.add('circle');
        }
      }
    });