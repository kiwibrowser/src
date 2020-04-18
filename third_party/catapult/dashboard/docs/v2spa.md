The next version of the Chromeperf Dashboard is at the prototype stage,
available for preview at https://v2spa-dot-chromeperf.appspot.com .

In order to develop or deploy v2spa, a one-time setup is required:
```
cd dashboard
ln -sf ../third_party/polymer2
ln -sf ../third_party/redux
ln -sf ../tracing/tracing
ln -sf polymer2/bower_components
ln -sf redux/redux.min.js
```

In order to deploy v2spa.yaml to v2spa-dot-chromeperf.appspot.com, run
`dashboard/bin/deploy_v2spa`. That serves a vulcanized HTML file at `/` and the
same script request handlers as V1, which is configured in app.yaml and
continues to be deployed to chromeperf.appspot.com by `dashboard/bin/deploy`.

In order to develop v2spa locally, run `dev_appserver.py v2spa_dev.yaml` to
serve the unvulcanized sources at http://localhost:8080 to speed up reloading
changes. `v2spa_dev.yaml` is not intended to be deployed even to a dev instance.
When running on localhost, V2SPA does not send requests to the backend, so no
script request handlers are needed.
