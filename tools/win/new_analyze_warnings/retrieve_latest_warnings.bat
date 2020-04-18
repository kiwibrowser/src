@rem Copyright (c) 2012 The Chromium Authors. All rights reserved.
@rem Use of this source code is governed by a BSD-style license that can be
@rem found in the LICENSE file.
@setlocal

@if "%ANALYZE_REPO%" == "" goto skipCD
@rem If ANALYZE_REPO is set then the results are put in that directory
cd /d "%ANALYZE_REPO%"
:skipCD

@rem Delete previous data
del set_analyze_revision.bat
@rem Retrieve the latest warnings
python %~dp0retrieve_warnings.py -0
@if not exist set_analyze_revision.bat goto failure

@rem Set ANALYZE_REVISION and ANALYZE_BUILD_NUMBER
call set_analyze_revision.bat

@set fullWarnings=analyze%ANALYZE_BUILD_NUMBER%_full.txt
@set summaryWarnings=analyze%ANALYZE_BUILD_NUMBER%_summary.txt

python %~dp0warnings_by_type.py %fullWarnings%

@set oldSummary=analyze%ANALYZE_PREV_BUILD_NUMBER%_summary.txt
@set oldFull=analyze%ANALYZE_PREV_BUILD_NUMBER%_full.txt
@if exist %oldSummary% goto doDiff
@if exist %oldFull% goto makeOldSummary
@echo No previous results. To get some earlier results for comparison
@echo use: %~dp0retrieve_warnings.py %ANALYZE_PREV_BUILD_NUMBER%
@goto skipDiff
:makeOldSummary
python %~dp0warnings_by_type.py %oldFull%
:doDiff
@set newWarnings=analyze%ANALYZE_BUILD_NUMBER%_new.txt
python %~dp0warning_diff.py %oldSummary% %summaryWarnings% >%newWarnings%
start %newWarnings%
:skipDiff

@if "%ANALYZE_REPO%" == "" goto notSet
if not exist "%ANALYZE_REPO%\src" goto notSet

cd src

@echo Retrieving source for the latest build results.

@rem Pull the latest code, go to the same revision that the builder used, and
@rem then do a gclient sync. echo has to be enabled after each call to git
@rem because it (erroneously) disables it.
call git fetch
@echo on
call git checkout %ANALYZE_REVISION%
@echo on
@rem Display where we are to make sure the git pull worked? Redirect to 'tee'
@rem to avoid invoking the pager.
call git log -1 | tee
@echo on
call gclient sync --jobs=16
@echo on

@exit /b

:notSet
@echo If ANALYZE_REPO is set to point at a repo -- parent of src directory --
@echo then that repo will be synced to match the build machine.
@echo See set_analyze_revision.bat for parameters.
@exit /b

:failure
@echo Failed to retrieve results from the latest build
@exit /b
