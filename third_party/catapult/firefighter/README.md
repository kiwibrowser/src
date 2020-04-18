<!-- Copyright 2015 The Chromium Authors. All rights reserved.
     Use of this source code is governed by a BSD-style license that can be
     found in the LICENSE file.
-->

Firefighter
===========

Firefighter is an App Engine dashboard that visualizes time series data.

The overall process is to:
1. Ingest multiple streams of data, either by polling data sources, or by bot uploads.
1. Convert everything to a trace event and tag it with metadata.
1. Filter the events on dozens of parameters with low cost and latency, using BigQuery.
1. Produce arbitrary visualizations for arbitrary data on-demand.

Prerequisites
-------------

Follow the instructions for setting up [Google App Engine Managed VMs](https://cloud.google.com/appengine/docs/managed-vms/getting-started).

1. Download and install the [Google Cloud SDK](https://cloud.google.com/sdk/#Quick_Start).

1. Ensure you are authorized to run gcloud commands.

        $ gcloud auth login

1. Set the default project name.

        $ gcloud config set project PROJECT

1. Install the gcloud app component.

        $ gcloud components update app

Development Server
------------------

You must have the [Google Cloud SDK](https://cloud.google.com/sdk/) installed. Run:

    stats$ bin/run

Deployment
----------

You must have the [Google Cloud SDK](https://cloud.google.com/sdk/) installed. Run:

    stats$ bin/deploy

Code Organization
-----------------

The app is divided into two modules: `default` and `update`. The `update` module handles ingestion of data, through either polling or uploading from an external service. The `default` module handles user queries. `base/` contains code shared between both modules.
