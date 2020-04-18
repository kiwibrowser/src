// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var BatterySettings = Polymer({
  is: 'battery-settings',

  behaviors: [Polymer.NeonAnimatableBehavior],

  properties: {
    /** The system's battery percentage. */
    batteryPercent: Number,

    /**
     * A string representing a value in the
     * PowerSupplyProperties_BatteryState enumeration.
     */
    batteryState: {
      type: String,
      observer: 'batteryStateChanged',
    },

    /**
     * An array representing the battery state options.
     * The names are ordered based on the
     * PowerSupplyProperties_BatteryState enumeration. These values must be
     * in sync.
     */
    batteryStateOptions: {
      type: Array,
      value: function() {
        return ['Full', 'Charging', 'Discharging', 'Not Present'];
      },
    },

    /**
     * Example charging devices that can be connected. Chargers are split
     * between dedicated chargers (which will always provide power if no
     * higher-power dedicated charger is connected) and dual-role USB chargers
     * (which only provide power if configured as a source and no dedicated
     * charger is connected).
     */
    powerSourceOptions: {
      type: Array,
      value: function() {
        return [
          {
            id: '0',
            name: 'AC Charger 1',
            type: 'DedicatedCharger',
            port: 0,
            connected: false,
            power: 'high'
          },
          {
            id: '1',
            name: 'AC Charger 2',
            type: 'DedicatedCharger',
            port: 0,
            connected: false,
            power: 'high'
          },
          {
            id: '2',
            name: 'USB Charger 1',
            type: 'DedicatedCharger',
            port: 0,
            connected: false,
            power: 'low',
            variablePower: true
          },
          {
            id: '3',
            name: 'USB Charger 2',
            type: 'DedicatedCharger',
            port: 0,
            connected: false,
            power: 'low',
            variablePower: true
          },
          {
            id: '4',
            name: 'Dual-role USB 1',
            type: 'DualRoleUSB',
            port: 0,
            connected: false,
            power: 'low'
          },
          {
            id: '5',
            name: 'Dual-role USB 2',
            type: 'DualRoleUSB',
            port: 1,
            connected: false,
            power: 'low'
          },
          {
            id: '6',
            name: 'Dual-role USB 3',
            type: 'DualRoleUSB',
            port: 2,
            connected: false,
            power: 'low'
          },
          {
            id: '7',
            name: 'Dual-role USB 4',
            type: 'DualRoleUSB',
            port: 3,
            connected: false,
            power: 'low'
          },
        ];
      },
    },

    /** The ID of the current power source, or the empty string. */
    selectedPowerSourceId: String,

    /** A string representing the time left until the battery is discharged. */
    timeUntilEmpty: String,

    /** A string representing the time left until the battery is at 100%. */
    timeUntilFull: String,
  },

  observers: [
    'powerSourcesChanged(powerSourceOptions.*)',
  ],

  ready: function() {
    chrome.send('requestPowerInfo');
  },

  onBatteryPercentChange: function(e) {
    this.percent = parseInt(e.target.value);
    if (!isNaN(this.percent))
      chrome.send('updateBatteryPercent', [this.percent]);
  },

  onSetAsSourceTap: function(e) {
    chrome.send('updatePowerSourceId', [e.model.item.id]);
  },

  batteryStateChanged: function(batteryState) {
    // Find the index of the selected battery state.
    var index = this.batteryStateOptions.indexOf(batteryState);
    if (index < 0)
      return;
    chrome.send('updateBatteryState', [index]);
  },

  powerSourcesChanged: function() {
    var connectedPowerSources =
        this.powerSourceOptions.filter(function(source) {
          return source.connected;
        });
    chrome.send('updatePowerSources', [connectedPowerSources]);
  },

  onTimeUntilEmptyChange: function(e) {
    this.timeUntilEmpty = parseInt(e.target.value);
    if (!isNaN(this.timeUntilEmpty))
      chrome.send('updateTimeToEmpty', [this.timeUntilEmpty]);
  },

  onTimeUntilFullChange: function(e) {
    this.timeUntilFull = parseInt(e.target.value);
    if (!isNaN(this.timeUntilFull))
      chrome.send('updateTimeToFull', [this.timeUntilFull]);
  },

  onPowerChanged: function(e) {
    e.model.set('item.power', e.target.value);
  },

  updatePowerProperties: function(power_properties) {
    this.batteryPercent = power_properties.battery_percent;
    this.batteryState =
        this.batteryStateOptions[power_properties.battery_state];
    this.timeUntilEmpty = power_properties.battery_time_to_empty_sec;
    this.timeUntilFull = power_properties.battery_time_to_full_sec;
    this.selectedPowerSourceId = power_properties.external_power_source_id;
  },

  isBatteryPresent: function() {
    return this.batteryState != 'Not Present';
  },

  isDualRole: function(source) {
    return source.type == 'DualRoleUSB';
  },

  isSelectedSource: function(source) {
    return source.id == this.selectedPowerSourceId;
  },

  canAmpsChange: function(type) {
    return type == 'USB';
  },

  canBecomeSource: function(source, selectedId, powerSourceOptionsChange) {
    if (!source.connected || !this.isDualRole(source))
      return false;
    return !this.powerSourceOptions.some(function(source) {
      return source.connected && source.type == 'DedicatedCharger';
    });
  },
});
