// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';


var ARGS_HELP_TEXT = 'Enter arguments as a comma separated list of the form: ' +
                     'arg1,arg2,...,argN.';

var ARGS_ACTION_HELP_TEXT = '\n\nYou must enter the arguments: ';

var KWARGS_HELP_TEXT = 'Enter keyword arguments as a comma separated list of ' +
                       'equal sign separated values of the form: ' +
                       'kwarg1=value1,kwarg2=value2,...,kwargN=valueN.';

var KWARGS_ACTION_HELP_TEXT = '\n\nYou may enter zero or more of the' +
                              ' following arguments: ';

var NO_ARGS_TEXT = '\n\nNo arguments for you to add.';


function ActionRepairDialog(service, actionInfo) {
  // The actionInfo parameter is an object with the following fields:
  //  action: A string. The name of the repair action.
  //  info: A string. A description of the repair action.
  //  args: An array. The positional arguments taken by the action.
  //  kwargs: An object. The keyword arguments taken by the action.

  var actionRepairDialog = this;

  var templateData = {
    action: actionInfo.action,
    info: actionInfo.info
  };

  this.service = service;
  this.actionInfo = actionInfo;

  this.dialogElement_ = $(
      renderTemplate('actionrepairdialog', templateData)).dialog({
    autoOpen: false,
    width: 575,
    modal: true,

    close: function(event, ui) {
      $(this).dialog('destroy').remove();
    },

    buttons: {
      'Reset': function() {
        actionRepairDialog.reset();
      },
      'Submit': function() {
        actionRepairDialog.submit();
      }
    }
  });

  // Commonly used elements of the dialog ui.
  var d = this.dialogElement_;
  this.dialogArgs = $(d).find('#args')[0];
  this.dialogArgsHelp = $(d).find('#argsHelp')[0];
  this.dialogKwargs = $(d).find('#kwargs')[0];
  this.dialogKwargsHelp = $(d).find('#kwargsHelp')[0];

  // Set default action info.
  this.reset();
}

ActionRepairDialog.prototype.open = function() {
  this.dialogElement_.dialog('open');
};

ActionRepairDialog.prototype.close = function() {
  this.dialogElement_.dialog('close');
};

ActionRepairDialog.prototype.reset = function() {
  var actionInfo = this.actionInfo;

  // Clear old input.
  this.dialogArgs.value = '';
  this.dialogKwargs.value = '';

  // Set the argument information.
  if (!isEmpty(actionInfo.args)) {
    $(this.dialogArgsHelp).text(ARGS_HELP_TEXT + ARGS_ACTION_HELP_TEXT +
                                actionInfo.args.join(','));
    this.dialogArgs.disabled = false;
  }
  else {
    $(this.dialogArgsHelp).text(ARGS_HELP_TEXT + NO_ARGS_TEXT);
    this.dialogArgs.disabled = true;
  }

  // Set the kwarg information.
  if (!isEmpty(actionInfo.kwargs)) {
    var kwargs = [];
    Object.keys(this.actionInfo.kwargs).forEach(function(key) {
      kwargs.push(key + '=' + actionInfo.kwargs[key]);
    });

    this.dialogKwargs.value = kwargs.join(',');
    this.dialogKwargs.disabled = false;
    $(this.dialogKwargsHelp).text(
        KWARGS_HELP_TEXT + KWARGS_ACTION_HELP_TEXT +
        Object.keys(actionInfo.kwargs).join(','));
  }
  else {
    $(this.dialogKwargsHelp).text(KWARGS_HELP_TEXT + NO_ARGS_TEXT);
    this.dialogKwargs.disabled = true;
  }
};

ActionRepairDialog.prototype.submit = function() {
  // Caller must define the function 'submitHandler' on the created dialog.
  // The submitHandler will be passed the following arguments:
  //  service: A string.
  //  action: A string.
  //  args: An array.
  //  kwargs: An object.

  if (!this.submitHandler) {
    alert('Caller must define submitHandler for ActionRepairDialog.');
    return;
  }

  // Validate the argument input.
  var args = this.dialogArgs.value;
  var kwargs = this.dialogKwargs.value;

  if (args && !/^([^,]+,)*[^,]+$/g.test(args)) {
    alert('Arguments are not well-formed.\n' +
          'Expected form: a1,a2,...,aN');
    return;
  }

  if (kwargs && !/^([^,=]+=[^,=]+,)*[^,]+=[^,=]+$/g.test(kwargs)) {
    alert('Keyword argumetns are not well-formed.\n' +
          'Expected form: kw1=foo,...,kwN=bar');
    return;
  }

  // Submit the action.
  var submitArgs = args ? args.split(',') : [];
  var submitKwargs = {};
  kwargs.split(',').forEach(function(elem, index, array) {
    var kv = elem.split('=');
    submitKwargs[kv[0]] = kv[1];
  });


  this.submitHandler(this.service, this.actionInfo.action, submitArgs,
                     submitKwargs);
  this.close();
};
