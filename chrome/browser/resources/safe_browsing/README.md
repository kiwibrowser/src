# Behavior of Download File Types in Chrome

This describes how to adjust file-type download behavior in
Chrome including interactions with Safe Browsing. The metadata described
here, and stored in `download_file_types.asciipb`, will be both baked into
Chrome released and pushable to Chrome between releases (via
`FileTypePolicies` class).  http://crbug.com/596555

Rendered version of this file: https://chromium.googlesource.com/chromium/src/+/master/chrome/browser/resources/safe_browsing/README.md


## Procedure for adding/modifying file type(s)
  * **Edit** `download_file_types.asciipb` and update `enums.xml`
  * Get it reviewed, **submit.**
  * **Push** it to all users via component update:
    * Wait 1-3 day for this to run on Canary to verify it doesn't crash Chrome.
    * In a synced checkout, run the following to generate protos for all
      platforms and push them to GCS. Replace the arg with your build directory:
        * % `chrome/browser/resources/safe_browsing/push_file_type_proto.py -d
          out-gn/Debug`
    * It will ask you to double check its actions before proceeding.  It will
      fail if you're not a member of
      `chrome-file-type-policies-pushers@google.com`, since that's required for
      access to the GCS bucket.
    * The Component Updater system will notice those files and push them to
      users withing ~6 hours. If not, contact `waffles@.`


## Guidelines for a DownloadFileType entry:
See `download_file_types.proto` for all fields.

  * `extension`: (required) Value must be unique within the config. It should be
    lowercase ASCII and not contain a dot. If there _is_ a duplicate,
    first one wins. Only the `default_file_type` should leave this unset.

  * `uma_value`: (required) must be unique and match one in the
    `SBClientDownloadExtensions` enum in `enums.xml`.

  * `is_archive`: `True` if this filetype is a container for other files.
     Leave it unset for `false`.

  * `ping_setting`:  (required). This controls what sort of ping is sent
     to Safe Browsing and if a verdict is checked before the user can
     access the file.

    * `SAMPLED_PING`: Don't send a full Safe Browsing ping, but
       send a no-PII "light-ping" for a random sample of SBER users.
       This should be the default for unknown types. The verdict won't
       be used.
    * `NO_PING`:  Don’t send any pings. This file is whitelisted. All
      NOT_DANGEROUS files should normally use this.
    * `FULL_PING`: Send full pings and use the verdict. All dangerous
      file should use this.

  * `platform_settings`: (repeated) Zero or more settings to differentiate
     behavior by platform. Keep them sorted by platform. At build time,
     this list will be filtered to contain exactly one setting by chosing
     as follows before writing out the binary proto.

       1. If there's an entry matching the built platform,
         that will be preferred. Otherwise,

       2. If there's a "PLATFORM_ANY" (i.e. `platform` is not set),
       that will be used. Otherwise,

       3. The `default_file_type`'s settings will be filled in.

  * `platform_settings.danger_level`: (required) Controls how files should be
    handled by the UI in the absence of a better signal from the Safe Browsing
    ping. This applies to all file types where `ping_setting` is either
    `SAMPLED_PING` or `NO_PING`, and downloads where the Safe Browsing ping
    either fails, is disabled, or returns an `UNKNOWN` verdict. Exceptions are
    noted below.

    The warning controlled here is a generic "This file may harm your computer."
    If the Safe Browsing verdict is `UNCOMMON`, `POTENTIALLY_UNWANTED`,
    `DANGEROUS_HOST`, or `DANGEROUS`, Chrome will show that more severe warning
    regardless of this setting.

    This policy also affects also how subresources are handled for *"Save As
    ..."* downloads of complete web pages. If any subresource ends up with a
    file type that is considered `DANGEROUS` or `ALLOW_ON_USER_GESTURE`, then
    the filename will be changed to end in `.download`. This is done to prevent
    the file from being opened accidentally.

    * `NOT_DANGEROUS`: Safe to download and open, even if the download
       was accidental. No additional warnings are necessary.
    * `DANGEROUS`: Always warn the user that this file may harm their
      computer. We let them continue or discard the file. If Safe
      Browsing returns a `SAFE` verdict, we still warn the user.
    * `ALLOW_ON_USER_GESTURE`: Potentially dangerous, but is likely harmless if
      the user is familiar with host and if the download was intentional. Chrome
      doesn't warn the user if both of the following conditions are true:

        * There is a user gesture associated with the network request that
          initiated the download.
        * There is a recorded visit to the referring origin that's older than
          the most recent midnight. This is taken to imply that the user has a
          history of visiting the site.

      In addition, Chrome skips the warning if the download was explicit (i.e.
      the user selected "Save link as ..." from the context menu), or if the
      navigation that resulted in the download was initiated using the Omnibox.

  * `platform_settings.auto_open_hint`: (required).
    * `ALLOW_AUTO_OPEN`: File type can be opened automatically if the user
      selected that option from the download tray on a previous download
      of this type.
    * `DISALLOW_AUTO_OPEN`:  Never let the file automatically open.
      Files that should be disallowed from auto-opening include those that
      execute arbitrary or harmful code with user privileges, or change
      configuration of the system to cause harmful behavior immediately
      or at some time in the future. We *do* allow auto-open for files
      that upon opening sufficiently warn the user about the fact that it
      was downloaded from the internet and can do damage. **Note**:
      Some file types (e.g.: .local and .manifest) aren't dangerous
      to open. However, their presence on the file system may cause
      potentially dangerous changes in behavior for other programs. We
      allow automatically opening these file types, but always warn when
      they are downloaded.

  * TODO(nparker): Support this: `platform_settings.unpacker`:
     optional. Specifies which archive unpacker internal to Chrome
     should be used. If potentially dangerous file types are found,
     Chrome will send a full-ping for the entire file. Otherwise, it'll
     follow the ping settings. Can be one of UNPACKER_ZIP or UNPACKER_DMG.

## Guidelines for the top level DownloadFileTypeConfig entry:
  * `version_id`: Must be increased (+1) every time the file is checked in.
     Will be logged to UMA.

  * `sampled_ping_probability`: For what fraction of extended-reporting
    users' downloads with unknown extensions (or
    ping_setting=SAMPLED_PING) should we send light-pings? [0.0 .. 1.0]

  * `file_types`: The big list of all known file types. Keep them
     sorted by extension.

  * `default_file_type`: Settings used if a downloaded file is not in
    the above list. `extension` is ignored, but other settings are used.
    The ping_setting should be SAMPLED_PING for all platforms.

