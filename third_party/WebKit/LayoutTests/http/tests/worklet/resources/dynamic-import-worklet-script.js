import('./empty-worklet-script.js')
    .then(() => console.error('Should not reach here.'))
    .catch(e => console.error(e.name + ': ' + e.message));
