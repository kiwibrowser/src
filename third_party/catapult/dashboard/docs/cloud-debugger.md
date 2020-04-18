# Updating the cloud repository

[Cloud debugger](https://cloud.google.com/tools/cloud-debugger/)
can now be used to debug errors in production. This debugging
functionality requires having a copy of this repo in a [Cloud Source
Repository](https://cloud.google.com/tools/cloud-repositories/docs/).

In order to push the current state of this repository to the
Cloud Source Repository for the Chrome Performance Dashboard:

    gcloud auth login
    git config credential.helper gcloud.sh
    git remote add cloud-repo https://source.developers.google.com/p/chromeperf/
    git push --all cloud-repo

Note: If the Cloud Source Repository is changed to automatically mirror
from the official catapult repository, then this should be unnecessary.

