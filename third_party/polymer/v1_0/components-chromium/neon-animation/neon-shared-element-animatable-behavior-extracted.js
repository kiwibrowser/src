/**
   * Use `Polymer.NeonSharedElementAnimatableBehavior` to implement elements containing shared element
   * animations.
   * @polymerBehavior Polymer.NeonSharedElementAnimatableBehavior
   */
  Polymer.NeonSharedElementAnimatableBehaviorImpl = {

    properties: {

      /**
       * A map of shared element id to node.
       */
      sharedElements: {
        type: Object,
        value: {}
      }

    }

  };

  /** @polymerBehavior Polymer.NeonSharedElementAnimatableBehavior */
  Polymer.NeonSharedElementAnimatableBehavior = [
    Polymer.NeonAnimatableBehavior,
    Polymer.NeonSharedElementAnimatableBehaviorImpl
  ];