function enableAccessibilityEventsPermission() {
  return new Promise(function(resolve, reject) {
    PermissionsHelper.setPermission(
        'accessibility-events', 'granted').then(function() {
      // Make sure AXObjectCacheImpl gets the notification too, its
      // listener may fire after this one.
      window.setTimeout(function() {
        resolve();
      }, 0);
    });
  });
}
