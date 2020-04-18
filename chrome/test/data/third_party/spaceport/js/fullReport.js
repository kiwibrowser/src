define([ 'tables', 'util/report' ], function (tables, report) {
    function val(id) {
        var el = document.getElementById(id);
        var customEl = document.getElementById(id + '-custom');

        return el.value || (customEl && customEl.value);
    }

    function getAgentMetadata() {
        if (typeof window !== 'undefined' && window) {
            return {
                userAgent: window.navigator.userAgent,
                language: window.navigator.language,
                browser: val('ua-browser'),
                name: val('ua-device-name'),
                os: val('ua-os'),
                type: val('ua-type'),
                misc: val('ua-misc')
            };
        } else {
            return null;
        }
    }

    function csvReport(results, agentMetadata) {
        agentMetadata = agentMetadata || getAgentMetadata();

        var reports = [ ];
        if (agentMetadata) {
            reports.push(report.csvByObject(agentMetadata));
        }

        Object.keys(tables.performance).forEach(function (testName) {
            var layout = report.makeTableLayout(tables.performance[testName]);
            reports.push(report.csvByLayout(results[testName], layout, [ testName ]));
        });

        return reports.join('\n\n') + '\n';
    }

    return {
        csvReport: csvReport,
        getAgentMetadata: getAgentMetadata
    };
});
