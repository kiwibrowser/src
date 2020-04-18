importScripts('../../../resources/testharness.js');
importScripts('../../../resources/origin-trials-helper.js');

test(t => {
  OriginTrialsHelper.check_properties(this, {'BudgetService': ['getBudget', 'getCost']});
}, 'Budget API related properties on interfaces in Origin-Trial regular Service Worker.');
