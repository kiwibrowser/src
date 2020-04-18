# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Dispatches requests to request handler classes."""

import webapp2

from dashboard import add_histograms
from dashboard import add_histograms_queue
from dashboard import add_point
from dashboard import add_point_queue
from dashboard import alerts
from dashboard import associate_alerts
from dashboard import bug_details
from dashboard import buildbucket_job_status
from dashboard import change_internal_only
from dashboard import create_health_report
from dashboard import debug_alert
from dashboard import delete_test_data
from dashboard import deprecate_tests
from dashboard import dump_graph_json
from dashboard import edit_anomalies
from dashboard import edit_anomaly_configs
from dashboard import edit_bug_labels
from dashboard import edit_sheriffs
from dashboard import edit_site_config
from dashboard import email_summary
from dashboard import file_bug
from dashboard import get_diagnostics
from dashboard import get_histogram
from dashboard import graph_csv
from dashboard import graph_json
from dashboard import graph_revisions
from dashboard import group_report
from dashboard import layered_cache
from dashboard import list_monitored_tests
from dashboard import list_tests
from dashboard import load_from_prod
from dashboard import main
from dashboard import mark_recovered_alerts
from dashboard import memory_report
from dashboard import migrate_test_names
from dashboard import navbar
from dashboard import oauth2_decorator
from dashboard import pinpoint_request
from dashboard import post_bisect_results
from dashboard import put_entities_task
from dashboard import report
from dashboard import short_uri
from dashboard import speed_releasing
from dashboard import start_try_job
from dashboard import update_bug_with_results
from dashboard import update_test_suites
from dashboard.api import alerts as api_alerts
from dashboard.api import bugs
from dashboard.api import list_timeseries
from dashboard.api import timeseries


_URL_MAPPING = [
    ('/add_histograms', add_histograms.AddHistogramsHandler),
    ('/add_histograms_queue', add_histograms_queue.AddHistogramsQueueHandler),
    ('/add_point', add_point.AddPointHandler),
    ('/add_point_queue', add_point_queue.AddPointQueueHandler),
    ('/alerts', alerts.AlertsHandler),
    (r'/api/alerts/(.*)', api_alerts.AlertsHandler),
    (r'/api/bugs/(.*)', bugs.BugsHandler),
    (r'/api/list_timeseries/(.*)', list_timeseries.ListTimeseriesHandler),
    (r'/api/timeseries/(.*)', timeseries.TimeseriesHandler),
    ('/associate_alerts', associate_alerts.AssociateAlertsHandler),
    ('/bug_details', bug_details.BugDetailsHandler),
    (r'/buildbucket_job_status/(\d+)',
     buildbucket_job_status.BuildbucketJobStatusHandler),
    ('/change_internal_only', change_internal_only.ChangeInternalOnlyHandler),
    ('/create_health_report', create_health_report.CreateHealthReportHandler),
    ('/debug_alert', debug_alert.DebugAlertHandler),
    ('/delete_expired_entities', layered_cache.DeleteExpiredEntitiesHandler),
    ('/delete_test_data', delete_test_data.DeleteTestDataHandler),
    ('/dump_graph_json', dump_graph_json.DumpGraphJsonHandler),
    ('/edit_anomalies', edit_anomalies.EditAnomaliesHandler),
    ('/edit_anomaly_configs', edit_anomaly_configs.EditAnomalyConfigsHandler),
    ('/edit_bug_labels', edit_bug_labels.EditBugLabelsHandler),
    ('/edit_sheriffs', edit_sheriffs.EditSheriffsHandler),
    ('/edit_site_config', edit_site_config.EditSiteConfigHandler),
    ('/email_summary', email_summary.EmailSummaryHandler),
    ('/file_bug', file_bug.FileBugHandler),
    ('/get_diagnostics', get_diagnostics.GetDiagnosticsHandler),
    ('/get_histogram', get_histogram.GetHistogramHandler),
    ('/graph_csv', graph_csv.GraphCsvHandler),
    ('/graph_json', graph_json.GraphJsonHandler),
    ('/graph_revisions', graph_revisions.GraphRevisionsHandler),
    ('/group_report', group_report.GroupReportHandler),
    ('/list_monitored_tests', list_monitored_tests.ListMonitoredTestsHandler),
    ('/list_tests', list_tests.ListTestsHandler),
    ('/load_from_prod', load_from_prod.LoadFromProdHandler),
    ('/', main.MainHandler),
    ('/mark_recovered_alerts',
     mark_recovered_alerts.MarkRecoveredAlertsHandler),
    ('/memory_report', memory_report.MemoryReportHandler),
    ('/migrate_test_names', migrate_test_names.MigrateTestNamesHandler),
    ('/deprecate_tests', deprecate_tests.DeprecateTestsHandler),
    ('/navbar', navbar.NavbarHandler),
    ('/pinpoint/new/bisect',
     pinpoint_request.PinpointNewBisectRequestHandler),
    ('/pinpoint/new/perf_try',
     pinpoint_request.PinpointNewPerfTryRequestHandler),
    ('/pinpoint/new/prefill',
     pinpoint_request.PinpointNewPrefillRequestHandler),
    ('/post_bisect_results', post_bisect_results.PostBisectResultsHandler),
    ('/put_entities_task', put_entities_task.PutEntitiesTaskHandler),
    ('/report', report.ReportHandler),
    ('/short_uri', short_uri.ShortUriHandler),
    (r'/speed_releasing/(.*)',
     speed_releasing.SpeedReleasingHandler),
    ('/speed_releasing', speed_releasing.SpeedReleasingHandler),
    ('/start_try_job', start_try_job.StartBisectHandler),
    ('/update_bug_with_results',
     update_bug_with_results.UpdateBugWithResultsHandler),
    ('/update_test_suites', update_test_suites.UpdateTestSuitesHandler),
    (oauth2_decorator.DECORATOR.callback_path,
     oauth2_decorator.DECORATOR.callback_handler())
]

APP = webapp2.WSGIApplication(_URL_MAPPING, debug=False)
