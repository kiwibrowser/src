'use strict';

// Copies the information out of a DataTransfer instance. This is necessary
// because dataTransfer instances get neutered after their event handlers exit.
//
// Returns a Promise that resolves to an object which mirrors the DataTransfer's
// files, items, and types attributes, as well as the return values of the
// getData function. The contents of files and file items are read using
// FileReader.
const copyDataTransfer = dataTransfer => {
  const types = dataTransfer.types.slice();
  const data = {};
  for (let type of types) {
    try {
      data[type] = dataTransfer.getData(type);
    } catch(e) {  // Catches SecurityError exceptions.
      data[type] = e;
    }
  }

  const readerPromises = [];

  const files = [];
  for (let file of Array.from(dataTransfer.files || [])) {
    const fileData = { file: file };
    files.push(fileData);

    readerPromises.push(new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onloadend = event => {
        if (event.target.error)
          fileData.error = event.target.error;
        else
          fileData.data = event.target.result;
        resolve();
      };
      reader.readAsText(file);
    }).catch(e => fileData.error = e));
  }

  const items = [];
  for (let item of Array.from(dataTransfer.items || [])) {
    const itemData = { kind: item.kind, type: item.type };
    items.push(itemData);

    readerPromises.push(new Promise((resolve, reject) => {
      if (itemData.kind === 'file') {
        itemData.file = item.getAsFile();
        if (itemData.file === null) {  // DataTransfer is in protected mode.
          resolve();
          return;
        }

        const reader = new FileReader();
        reader.onloadend = event => {
          if (event.target.error)
            itemData.data = event.target.error;
          else
            itemData.data = event.target.result;
          resolve();
        };
        reader.readAsText(itemData.file);
      }
    }).catch(e => itemData.error = e));
  }

  return Promise.all(readerPromises).then(() => {
    return { data, files, items, types };
  });
};
