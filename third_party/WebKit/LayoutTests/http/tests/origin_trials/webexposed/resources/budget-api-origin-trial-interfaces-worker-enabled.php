<?php
// Generate token with the command:
// generate_token.py http://127.0.0.1:8000 BudgetQuery --expire-timestamp=2000000000
header("Origin-Trial: AgFtR2Ps1Z9M/FW14Tgcwbajvq7kvzc/b1SPPSaaucG/P4ba6xC/69I9v8Pqx4wbsJINoqMabs9GE/LxOnPRfQIAAABTeyJvcmlnaW4iOiAiaHR0cDovLzEyNy4wLjAuMTo4MDAwIiwgImZlYXR1cmUiOiAiQnVkZ2V0UXVlcnkiLCAiZXhwaXJ5IjogMjAwMDAwMDAwMH0");
header('Content-Type: application/javascript');
?>
importScripts('../../../resources/testharness.js');
importScripts('../../../resources/origin-trials-helper.js');

test(t => {
  OriginTrialsHelper.check_properties(this, {'BudgetService': ['getBudget', 'getCost']});
}, 'Budget API related properties on interfaces in Origin-Trial enabled Service Worker.');
