# App List Model

Design doc: go/move-applist

## Introduction

This directory holds all app list model classes.

Currently the app list is running in the Chrome process but in the future it'll
be moved into Ash. Everything in //ui/app_list is being refactored and migrated
to //ash/app_list.

During the migration we'll have build dependencies changed. The following paths
will depend on this since they're referring to the app list model:

- //chrome/browser/extensions
- //chrome/browser/sync
- //chrome/browser/ui/app_list
- //ui/app_list
