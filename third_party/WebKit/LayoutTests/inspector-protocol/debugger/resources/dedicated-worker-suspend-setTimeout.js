var global_value = 1;
onmessage = function(event) {
  setTimeout(function()
  {
      global_value = 2014;
  }, 0);
  debugger;
};

