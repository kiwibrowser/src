# Clovis in the Cloud: Developer Guide

This document describes the backend-side of the trace collection, using Google
Compute Engine.

When the [frontend][3] spawns new tasks, it pushes them into a [TaskQueue][4]
called `clovis-queue` with a unique tag.
Then it creates backend instances (as an instance group) and passes them the
TaskQueue tag.

The backend instances then pull tasks from the TaskQueue and process them until
it is empty. When there is no task left in the queue, the backend instances
kill themselves.

The main files for the backend are:

-   `startup-script.sh`: initializes an instance (installs the dependencies,
    downloads the code and the configuration).
-   `worker.py`: the main worker script.
-   Task handlers have a `Run()` method taking a `ClovisTask` parameter.
    -   `clovis_task_handler.py`: Main entry point, dispatches the tasks to the
        more specialized handlers below.
    -   `trace_task_handler.py`: Handles `trace` tasks.
    -   `report_task_handler.py`: Handles `report` tasks.

[TOC]

## Initial setup for development

Install the [gcloud command line tool][1].

## Deploy the code

This step deploys all the source code needed by the backend workers, as well as
the Chromium binaries required for trace collection.

```shell
# Build Chrome (do not use the component build).
BUILD_DIR=out/Release
ninja -C $BUILD_DIR -j1000 -l60 chrome chrome_sandbox

# Deploy to GCE
# CLOUD_STORAGE_PATH is the path in Google Cloud Storage under which the
# Clovis deployment will be uploaded.

./tools/android/loading/cloud/backend/deploy.sh $BUILD_DIR $CLOUD_STORAGE_PATH
```

## Start the app in the cloud

The application is automatically started by the frontend, and should not need to
be started manually.

If you really want to create an instance manually (when debugging for example),
this can be done like this:

```shell
gcloud compute instances create $INSTANCE_NAME \
 --machine-type n1-standard-1 \
 --image ubuntu-14-04 \
 --zone europe-west1-c \
 --scopes cloud-platform,https://www.googleapis.com/auth/cloud-taskqueue \
 --metadata \
    cloud-storage-path=$CLOUD_STORAGE_PATH,task-dir=dir,taskqueue-tag=tag \
 --metadata-from-file \
    startup-script=$CHROMIUM_SRC/tools/android/loading/cloud/backend/startup-script.sh
```

If you are debbugging, you probably want to set additional metadata:

-   `auto-start=false`: to start an instance without automatically starting the
    app on it. This can be useful when doing iterative development on the
    instance using ssh, to be able to stop and restart the app manually.
-   `self-destruct=false`: to prevent the instance from self-destructing when
    the queue is empty.

**Notes:**

-   If you use `auto-start=false`, and then try to ssh on the instance and
    launch `worker.py`, it will not work because of various issues, such as:
    -   Environment variables defined by the startup script are not available
        to your user and you will need to redefine them.
    -   You will not have permissions to access the files, and need to run
        `sudo chown` to give yourself permissions.
    -   You need to activate `virtualenv`.
    Get in touch with *droger@* if you need this or want to improve it.
-   It can take a few minutes for the instance to start. You can follow the
    progress of the startup script on the gcloud console web interface (menu
    "Compute Engine" > "VM instances" then click on your instance and scroll
    down to see the "Serial console output") or from the command line using:

```shell
gcloud compute instances get-serial-port-output $INSTANCE_NAME
```

## `worker.py` configuration file

`worker.py` takes a configuration file as command line parameter. This is a JSON
dictionary with the keys:

-   `project_name` (string): Name of the Google Cloud project
-   `task_storage_path` (string): Path in Google Storage where task output is
    generated.
-   `binaries_path` (string): Path to the executables (Containing chrome).
-   `src_path` (string): Path to the Chromium source directory.
-   `taskqueue_tag` (string): Tag used by the worker when pulling tasks from
    `clovis-queue`.
-   `ad_rules_filename` and `tracking_rules_filename` (string): Path to the ad
     and tracking filtering rules.
-   `instance_name` (string, optional): Name of the Compute Engine instance this
    script is running on.
-   `worker_log_path` (string, optional): Path to the log file capturing the
    output of `worker.py`, to be uploaded to Cloud Storage.
-   `self_destruct` (boolean, optional): Whether the worker will destroy the
    Compute Engine instance when there are no remaining tasks to process. This
    is only relevant when running in the cloud, and requires `instance_name` to
    be defined.

## Use the app

Create tasks from the associated AppEngine application, see [documentation][3].

If you want the frontend to send tasks to a particular instance that you created
manually, make sure the `tag` and `storage_bucket` of the AppEngine request
match the ones of your ComputeEngine instance, and set `instance_count` to `0`.

## Stop the app in the cloud

To stop a single instance that you started manually, do:

```shell
gcloud compute instances delete $INSTANCE_NAME
```

To stop instances that were created by the frontend, you must delete the
instance group, not the individual instances. Otherwise the instance group will
just recreate the deleted instances. You can do this from the Google Cloud
console web interface, or using the `gcloud compute groups` commands.

## Connect to the instance with SSH

```shell
gcloud compute ssh $INSTANCE_NAME
```

## Run the app locally

From a new directory, set up a local environment:

```shell
virtualenv env
source env/bin/activate
pip install -r \
    $CHROMIUM_SRC/tools/android/loading/cloud/backend/pip_requirements.txt
```

The first time, you may need to get more access tokens:

```shell
gcloud beta auth application-default login --scopes \
    https://www.googleapis.com/auth/cloud-taskqueue \
    https://www.googleapis.com/auth/cloud-platform
```

Create a local configuration file for `worker.py`. Example:

```shell
cat >$CONFIG_FILE << EOF
{
  "project_name" : "$PROJECT_NAME",
  "cloud_storage_path" : "$CLOUD_STORAGE_PATH",
  "binaries_path" : "$BUILD_DIR",
  "src_path" : "$CHROMIUM_SRC",
  "taskqueue_tag" : "some-tag"
}
EOF
```

Launch the app, passing the path to the deployment configuration file:

```shell
python $CHROMIUM_SRC/tools/android/loading/cloud/backend/worker.py \
    --config $CONFIG_FILE
```

You can now [use the app][2].

Tear down the local environment:

```shell
deactivate
```

[1]: https://cloud.google.com/sdk
[2]: #Use-the-app
[3]: ../frontend/README.md
[4]: https://cloud.google.com/appengine/docs/python/taskqueue
