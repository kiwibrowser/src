define([ ], function () {
    function serializeCsvCell(text) {
        if (typeof text === 'undefined' || text === null) {
            return serializeCsvCell('N/A');
        } else if (/[",]/.test(text)) {
            return '"' + text.replace(/"/g, '""') + '"';
        } else {
            return text;
        }
    }

    function th(text) {
        var th = document.createElement('th');
        th.textContent = text;
        return th;
    }

    function td(text) {
        var td = document.createElement('td');
        td.textContent = text;
        return td;
    }

    function tr(cells) {
        var tr = document.createElement('tr');
        cells.forEach(function (cell) {
            tr.appendChild(cell);
        });
        return tr;
    }

    var report = {
        makeTableLayout: function tabulate(sourceTable) {
            function levelKeys(level) {
                return Object.keys(level).filter(function (key) {
                    return !/^\$/.test(key);
                });
            }

            var levels = [ ];
            var c = sourceTable;
            while (c) {
                levels.push(c);
                c = c.$children;
            }

            // Array name means vertical; string name means horizontal
            var columns = [ ];
            levels.forEach(function (level, i) {
                switch (level.$mode) {
                default:
                case 'vertical':
                    columns.push({
                        name: levelKeys(level),
                        rowSpan: null,
                        title: level.$title,
                        level: level
                    });

                    if (i === levels.length - 1) {
                        // If the last level is vertical, insert a new column
                        // just for the data.
                        columns.push({
                            name: null,
                            rowSpan: 1,
                            title: '(null)',
                            level: null
                        });
                    }

                    break;

                case 'horizontal':
                    levelKeys(level).forEach(function (name) {
                        columns.push({
                            name: name,
                            rowSpan: null,
                            title: level[name],
                            level: level
                        });
                    });
                    break;
                }
            });

            var verticalSizes = [ ];
            var currentSize = 1;
            levels.reverse();
            levels.forEach(function (level) {
                verticalSizes.unshift(currentSize);

                switch (level.$mode) {
                default:
                case 'vertical':
                    currentSize *= levelKeys(level).length;
                    break;

                case 'horizontal':
                    levelKeys(level).slice(1).forEach(function () {
                        verticalSizes.unshift(currentSize);
                    });
                    break;
                }
            });
            levels.reverse();

            columns.forEach(function (column, i) {
                column.rowSpan = verticalSizes[i];
            });

            return {
                columns: columns,
                rowCount: currentSize
            };
        },

        tableTemplate: function table(rootPath, layout) {
            var table = document.createElement('table');
            var header = document.createElement('thead');
            table.appendChild(header);

            header.appendChild(tr(layout.columns.map(function (column) {
                return th(column.title);
            })));

            var body = document.createElement('tbody');
            table.appendChild(body);

            var rowIndex, columnIndex;
            for (rowIndex = 0; rowIndex < layout.rowCount; ++rowIndex) {
                var path = [ rootPath ];
                var columnEls = [ ];
                for (columnIndex = 0; columnIndex < layout.columns.length; ++columnIndex) {
                    var column = layout.columns[columnIndex];

                    var cell;
                    var name = column.name;
                    if (Array.isArray(name)) {
                        name = name[Math.floor(rowIndex / column.rowSpan) % name.length];
                        cell = th(column.level[name]);
                        path.push(name);
                    } else {
                        cell = td('');
                        cell.setAttribute('data-property', name);
                    }

                    if (column.level.$errors) {
                        var errorMessageEl = document.createElement('span');
                        errorMessageEl.className = 'error-message';
                        cell.appendChild(errorMessageEl);
                    }

                    if (rowIndex % column.rowSpan === 0) {
                        cell.rowSpan = column.rowSpan;
                        columnEls.push(cell);
                    }
                }

                var rowEl = tr(columnEls);
                rowEl.id = path.join('-');
                rowEl.className = 'test';
                body.appendChild(rowEl);
            }

            return table;
        },

        csvByTable: function csvByTable(records) {
            return records.map(function (record) {
                return record.map(serializeCsvCell).join(',');
            }).join('\n');
        },

        csvByObject: function csvByObject(object) {
            var records = [ ];

            function write(object, stack) {
                if (object && typeof object.valueOf === 'function') {
                    object = object.valueOf();
                }

                if (object === null) {
                    return;
                }

                switch (typeof object) {
                case 'object':
                    Object.keys(object).forEach(function (key) {
                        write(object[key], stack.concat([ key ]));
                    });
                    break;

                case 'string':
                case 'number':
                case 'boolean':
                    records.push(stack.concat([ String(object) ]));
                    break;

                default:
                    throw new TypeError('Cannot serialize object ' + object);
                }
            }

            write(object, [ ]);
            return report.csvByTable(records);
        },

        csvByLayout: function csv(data, layout, prefix) {
            function getValue(object, path) {
                path = path.slice();
                while (object && path.length) {
                    object = object[path.shift()];
                }
                return object;
            }

            var records = [ ];

            var rowIndex, columnIndex;
            for (rowIndex = 0; rowIndex < layout.rowCount; ++rowIndex) {
                var path = [ ];
                var record = prefix.slice(); // PREFIX IS A HACK FIXME
                records.push(record);
                for (columnIndex = 0; columnIndex < layout.columns.length; ++columnIndex) {
                    var column = layout.columns[columnIndex];

                    var cell;
                    var name = column.name;
                    if (Array.isArray(name)) {
                        name = name[Math.floor(rowIndex / column.rowSpan) % name.length];
                        cell = column.level[name];
                        path.push(name);
                    } else if (name === null) {
                        cell = getValue(data, path);
                    } else {
                        cell = getValue(data, path.concat([ name ]));
                    }

                    record.push(cell);
                }
            }

            return report.csvByTable(records);
        }
    };

    return report;
});
