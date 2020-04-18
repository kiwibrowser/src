#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script handles all of the processing for versioning packages.

package_version.py manages all of the various operations done between
packages, including archiving, extracting, uploading, and downloading
packages. For a list of options and commands, see the help for the script.

Glossary:
  Package: A list of archives, such as "nacl_x86_glibc" or "nacl_x86_newlib".
  Package Archive: An archive (usually a tar file) that is part of a package.
  Package Target: Package targets consists of packages. Each package target
    has it's own version of a package. An example of a package target would
    be something such as "win_x86_nacl_x86" or "mac_x86_nacl_x86". In that case,
    "win_x86_nacl_x86" and "mac_x86_nacl_x86" would each have their own version
    of "nacl_x86_glibc" and "nacl_x86_newlib" for windows and mac respectively.
  Revision: The revision identifier of a sanctioned version.
    This is used to synchronize packages to sanctioned versions.

JSON Files:
  Packages File - A file which describes the various package targets for each
    platform/architecture along with the packages associated with each package
    target.
    [Default file: build/package_version/standard_packages.json].
  Package File - A file which contains the list of package archives within
    a package.
    [Default file: toolchain/.tars/$PACKAGE_TARGET/$PACKAGE.json]
  Archive File - A file which describes an archive within a package. Each
    archive description file will contain information about an archive such
    as name, URL to download from, and hash.
    [Default File: toolchain/.tars/$PACKAGE_TARGET/$PACKAGE/$ARCHIVE.json]
  Revision File - A file which describes the sanctioned version of package
    for each of the package targets associated with it.
    [Default file: toolchain_revisions/$PACKAGE.json]
"""

import argparse
import collections
import logging
import os
import shutil
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR))
import cygtar

sys.path.append(os.path.dirname(os.path.dirname(SCRIPT_DIR)))
import pynacl.file_tools
import pynacl.gsd_storage
import pynacl.log_tools
import pynacl.platform
import pynacl.working_directory

import archive_info
import error
import package_info
import package_locations
import packages_info
import revision_info

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.dirname(CURRENT_DIR)
NACL_DIR = os.path.dirname(BUILD_DIR)

TEMP_SUFFIX = '.tmp'

DEFAULT_PACKAGES_JSON = os.path.join(CURRENT_DIR, 'standard_packages.json')
DEFAULT_REVISIONS_DIR = os.path.join(NACL_DIR, 'toolchain_revisions')
DEFAULT_DEST_DIR = os.path.join(NACL_DIR, 'toolchain')
DEFAULT_CLOUD_BUCKET = 'nativeclient-archive2'


#
# These are helper functions that help each command.
#

def CleanTempFiles(directory):
  """Cleans up all temporary files ending with TEMP_SUFFIX in a directory."""
  for root, dirs, files in os.walk(directory):
    for file_name in files:
      if file_name.endswith(TEMP_SUFFIX):
        file_path = os.path.join(root, file_name)
        os.unlink(file_path)


def GetPackageTargetPackages(custom_package_name, package_target_packages):
  """Returns a list of package target packages given a custom package name.

  A custom package name can either have a specified package target attached
  to it (IE. $PACKAGE_TARGET/PACKAGE_NAME) or be extracted out of a default
  list of package targets along with their packages.

  Args:
    custom_package_name: Package name with an optional package target.
    package_target_packages: List of tuples (package_target, package).
  Returns:
    List of package target tuples matching the package name.
  """
  package_path = custom_package_name.replace('\\', os.path.sep)
  package_path = package_path.replace('/', os.path.sep)
  if os.path.sep in package_path:
    # Package target is part of the custom package name, just return it.
    package_target, package_name = package_path.split(os.path.sep, 1)
    return [(package_target, package_name)]

  # Package target is not part of the package name, filter from list of passed
  # in package targets.
  return [
      (package_target, package)
      for package_target, package in package_target_packages
      if package == custom_package_name
  ]


def DownloadPackageArchives(tar_dir, package_target, package_name, package_desc,
                            downloader=None, revision_num=None,
                            include_logs=False):
  """Downloads package archives from the cloud to the tar directory.

  Args:
    tar_dir: Root tar directory where archives will be downloaded to.
    package_target: Package target of the package to download.
    package_name: Package name of the package to download.
    package_desc: package_info object of the package to download.
    downloader: function which takes a url and a file path for downloading.
  Returns:
    The list of files that were downloaded.
  """
  downloaded_files = []
  if downloader is None:
    downloader = pynacl.gsd_storage.HttpDownload
  local_package_file = package_locations.GetLocalPackageFile(tar_dir,
                                                             package_target,
                                                             package_name)

  # Download packages information file along with each of the package
  # archives described in the information file. Also keep track of what
  # new package names matches old package names. We will have to delete
  # stale package names after we are finished.
  update_archives = []
  for archive_obj in package_desc.GetArchiveList():
    archive_desc = archive_obj.GetArchiveData()
    local_archive_file = package_locations.GetLocalPackageArchiveFile(
        tar_dir,
        archive_desc.name,
        archive_desc.hash)

    old_hash = archive_info.GetArchiveHash(local_archive_file)
    if archive_desc.hash == old_hash:
        logging.debug('Skipping matching archive: %s', archive_desc.name)
        continue

    archive_tuple = (local_archive_file, archive_obj.GetArchiveData())
    update_archives.append(archive_tuple)

  if update_archives:
    logging.info('--Syncing %s to revision %s--' % (package_name, revision_num))
    num_archives = len(update_archives)
    for index, archive_tuple in enumerate(update_archives):
      local_archive_file, archive_desc = archive_tuple
      pynacl.file_tools.MakeParentDirectoryIfAbsent(local_archive_file)

      if archive_desc.url is None:
        raise error.Error('Error, no URL for archive: %s' % archive_desc.name)

      logging.info('Downloading package archive: %s (%d/%d)' %
                   (archive_desc.name, index+1, num_archives))
      try:
        downloader(archive_desc.url, local_archive_file)
      except Exception as e:
        raise error.Error('Could not download URL (%s): %s' %
                          (archive_desc.url, e))

      # Delete any stale log files
      local_archive_log = package_locations.GetLocalPackageArchiveLogFile(
          local_archive_file)
      if os.path.isfile(local_archive_log):
        os.unlink(local_archive_log)

      verified_hash = archive_info.GetArchiveHash(local_archive_file)
      if verified_hash != archive_desc.hash:
        raise error.Error('Package hash check failed: %s != %s' %
                          (verified_hash, archive_desc.hash))

      downloaded_files.append(local_archive_file)

  # Download any logs if include_logs is True.
  if include_logs:
    download_logs = []
    for archive_obj in package_desc.GetArchiveList():
      archive_desc = archive_obj.GetArchiveData()
      if archive_desc.log_url:
        local_archive_file = package_locations.GetLocalPackageArchiveFile(
            tar_dir,
            archive_desc.name,
            archive_desc.hash)
        local_archive_log = package_locations.GetLocalPackageArchiveLogFile(
            local_archive_file)
        if not os.path.isfile(local_archive_log):
          download_log_tuple = (archive_desc.name,
                                archive_desc.log_url,
                                local_archive_log)
          download_logs.append(download_log_tuple)

    if download_logs:
      logging.info('--Syncing %s Logs--' % (package_name))
      num_logs = len(download_logs)
      for index, download_log_tuple in enumerate(download_logs):
        name, log_url, local_log_file = download_log_tuple
        logging.info('Downloading archive log: %s (%d/%d)' %
                     (name, index+1, num_logs))

        try:
          downloader(log_url, local_log_file)
        except Exception as e:
          raise IOError('Could not download log URL (%s): %s' %
                        (log_url, e))

  # Save the package file so we know what we currently have.
  if not update_archives and os.path.isfile(local_package_file):
    try:
      local_package_desc = package_info.PackageInfo(local_package_file)
      if local_package_desc == package_desc:
        return downloaded_files
    except:
      # Something is wrong with our package file, just resave it.
      pass

  package_desc.SavePackageFile(local_package_file)
  return downloaded_files


def ArchivePackageArchives(tar_dir, package_target, package_name, archives,
                           extra_archives=[]):
  """Archives local package archives to the tar directory.

  Args:
    tar_dir: Root tar directory where archives live.
    package_target: Package target of the package to archive.
    package_name: Package name of the package to archive.
    archives: List of archive file paths where archives currently live.
    extra_archives: Extra archives that are expected to be build elsewhere.
  Returns:
    Returns the local package file that was archived.
  """
  local_package_file = package_locations.GetLocalPackageFile(tar_dir,
                                                             package_target,
                                                             package_name)

  valid_archive_files = set()
  archive_list = []

  package_desc = package_info.PackageInfo()
  package_archives = ([(archive, False) for archive in archives] +
                      [(archive, True) for archive in extra_archives])
  for archive, skip_missing in package_archives:
    archive_url = None
    archive_log_url = None
    if '@' in archive:
      archive, archive_url = archive.split('@', 1)
      if ',' in archive_url:
        archive_url, archive_log_url = archive_url.split(',', 1)

    extract_param = ''
    tar_src_dir = ''
    extract_dir = ''
    if ',' in archive:
      archive, extract_param = archive.split(',', 1)
      if ':' in extract_param:
        tar_src_dir, extract_dir = extract_param.split(':', 1)
      else:
        tar_src_dir = extract_param

    archive_hash = archive_info.GetArchiveHash(archive)
    archive_name = os.path.basename(archive)
    archive_desc = archive_info.ArchiveInfo(name=archive_name,
                                            hash=archive_hash,
                                            url=archive_url,
                                            tar_src_dir=tar_src_dir,
                                            extract_dir=extract_dir,
                                            log_url=archive_log_url)
    package_desc.AppendArchive(archive_desc)

    if archive_hash is None:
      if skip_missing:
        logging.info('Skipping archival of missing file: %s', archive)
        continue
      raise error.Error('Invalid package: %s.' % archive)
    archive_list.append(archive)

    archive_basename = os.path.basename(archive)
    archive_json = archive_basename + '.json'
    valid_archive_files.update([archive_basename, archive_json])

  # We do not need to archive the package if it already matches. But if the
  # local package file is invalid or does not match, then we should recreate
  # the json file.
  if os.path.isfile(local_package_file):
    try:
      current_package_desc = package_info.PackageInfo(local_package_file,
                                                      skip_missing=True)
      if current_package_desc == package_desc:
        return
    except ValueError:
      pass

  # Copy each of the packages over to the tar directory first.
  for archive_file in archive_list:
    archive_name = os.path.basename(archive_file)
    archive_hash = archive_info.GetArchiveHash(archive_file)
    local_archive_file = package_locations.GetLocalPackageArchiveFile(
        tar_dir,
        archive_name,
        archive_hash
    )

    if archive_hash == archive_info.GetArchiveHash(local_archive_file):
      logging.info('Skipping archive of duplicate file: %s', archive_file)
    else:
      logging.info('Archiving file: %s', archive_file)
      pynacl.file_tools.MakeParentDirectoryIfAbsent(local_archive_file)
      shutil.copyfile(archive_file, local_archive_file)

  # Once all the copying is completed, update the local packages file.
  logging.info('Package "%s" archived: %s', package_name, local_package_file)
  pynacl.file_tools.MakeParentDirectoryIfAbsent(local_package_file)
  package_desc.SavePackageFile(local_package_file)

  return local_package_file


def UploadPackage(storage, revision, tar_dir, package_target, package_name,
                  is_shared_package, annotate=False, skip_missing=False,
                  custom_package_file=None):
  """Uploads a local package file to the supplied cloud storage object.

  By default local package files are expected to be found in the standardized
  location within the tar directory, however a custom package file may be
  specified to upload from a different location. Package archives that do not
  have their URL field set will automaticaly have the archives uploaded so that
  someone accessing the package file from the cloud storage will also have
  access to the package archives.

  Args:
    storage: Cloud storage object which supports PutFile and GetFile.
    revision: Revision identifier the package should be associated with.
    tar_dir: Root tar directory where archives live.
    package_target: Package target of the package to archive.
    package_name: Package name of the package to archive.
    is_shared_package: Is this package shared among all package targets?
    annotate: Print annotations for build bots?
    skip_missing: Skip missing package archive files?
    custom_package_file: File location for a custom package file.
  Returns:
    Returns remote download key for the uploaded package file.
  """
  if custom_package_file is not None:
    local_package_file = custom_package_file
  else:
    local_package_file = package_locations.GetLocalPackageFile(
        tar_dir,
        package_target,
        package_name)

  # Upload the package file and also upload any local package archives so
  # that they are downloadable.
  package_desc = package_info.PackageInfo(local_package_file,
                                          skip_missing=skip_missing)
  upload_package_desc = package_info.PackageInfo()

  for archive_obj in package_desc.GetArchiveList():
    archive_desc = archive_obj.GetArchiveData()
    url = archive_desc.url
    if archive_desc.hash and url is None:
      if annotate:
        print '@@@BUILD_STEP Archive:%s (upload)@@@' % archive_desc.name

      archive_file = package_locations.GetLocalPackageArchiveFile(
          tar_dir,
          archive_desc.name,
          archive_desc.hash)
      archive_hash = archive_info.GetArchiveHash(archive_file)
      if archive_hash is None:
        raise error.Error('Missing Archive File: %s' % archive_file)
      elif archive_hash != archive_desc.hash:
        raise error.Error(
            'Archive hash does not match package hash: %s' % archive_file
            + '\n  Archive Hash: %s' % archive_hash
            + '\n  Package Hash: %s' % archive_desc.hash)

      logging.warn('Missing archive URL: %s', archive_desc.name)
      logging.warn('Uploading archive to be publically available...')
      remote_archive_key = package_locations.GetRemotePackageArchiveKey(
          archive_desc.name,
          archive_desc.hash)
      url = storage.PutFile(archive_file, remote_archive_key, clobber=True)
      if annotate:
        print '@@@STEP_LINK@download@%s@@@' % url

    updated_archive_obj = archive_obj.Copy(url=url)
    upload_package_desc.AppendArchive(updated_archive_obj)

  upload_package_file = local_package_file + '.upload'
  pynacl.file_tools.MakeParentDirectoryIfAbsent(upload_package_file)
  upload_package_desc.SavePackageFile(upload_package_file)

  logging.info('Uploading package information: %s', package_name)
  remote_package_key = package_locations.GetRemotePackageKey(
      is_shared_package,
      revision,
      package_target,
      package_name)
  package_info.UploadPackageInfoFiles(storage, package_target, package_name,
                                      remote_package_key, upload_package_file,
                                      skip_missing=skip_missing,
                                      annotate=annotate)

  return remote_package_key


def ExtractPackageTargets(package_target_packages, tar_dir, dest_dir,
                          downloader=None, skip_missing=False,
                          overlay_tar_dir=None, quiet=False):
  """Extracts package targets from the tar directory to the destination.

  Each package archive within a package will be verified before being
  extracted. If a package archive does not exist or does not match the hash
  stored within the package file, it will be re-downloaded before being
  extracted.

  Args:
    package_target_packages: List of tuples of package target and package names.
    tar_dir: Source tar directory where package archives live.
    dest_dir: Root destination directory where packages will be extracted to.
    downloader: function which takes a url and a file path for downloading.
  """
  if downloader is None:
    downloader = pynacl.gsd_storage.HttpDownload

  for package_target, package_name in package_target_packages:
    package_file = package_locations.GetLocalPackageFile(tar_dir,
                                                         package_target,
                                                         package_name)
    package_desc = package_info.PackageInfo(package_file,
                                            skip_missing=skip_missing)
    dest_package_dir = package_locations.GetFullDestDir(dest_dir,
                                                        package_target,
                                                        package_name)
    dest_package_file = package_locations.GetDestPackageFile(dest_dir,
                                                             package_target,
                                                             package_name)

    # Get a list of overlay archives.
    overlaid_archives = set()
    if overlay_tar_dir:
      overlay_file = package_locations.GetLocalPackageFile(overlay_tar_dir,
                                                           package_target,
                                                           package_name)
      logging.debug('Checking overlaid package file: %s', overlay_file)
      if os.path.isfile(overlay_file):
        logging.info('Found overlaid package file: %s', overlay_file)
        overlay_package_desc = package_info.PackageInfo(overlay_file,
                                                        skip_missing=True)

        combined_archives = dict([(archive_obj.GetArchiveData().name,
                                   archive_obj)
                                 for archive_obj
                                 in package_desc.GetArchiveList()])

        for archive_obj in overlay_package_desc.GetArchiveList():
          archive_desc = archive_obj.GetArchiveData()
          if archive_desc.hash:
            overlaid_archives.add(archive_desc.name)
            combined_archives[archive_desc.name] = archive_obj

        package_desc = package_info.PackageInfo()
        for archive_name, archive_obj in combined_archives.iteritems():
          package_desc.AppendArchive(archive_obj)

    # Only do the extraction if the extract packages do not match.
    if os.path.isfile(dest_package_file):
      try:
        dest_package_desc = package_info.PackageInfo(dest_package_file)
        if dest_package_desc == package_desc:
          logging.debug('Skipping extraction for package (%s)', package_name)
          continue
      except:
        # Destination package file cannot be trusted, if invalid re-extract.
        pass

      # Delete the old package file before we extract.
      os.unlink(dest_package_file)

    if os.path.isdir(dest_package_dir):
      logging.debug('Deleting old package directory: %s', dest_package_dir)
      pynacl.file_tools.RemoveDirectoryIfPresent(dest_package_dir)

    logging.info('Extracting package (%s) to directory: %s',
                 package_name, dest_package_dir)
    archive_list = package_desc.GetArchiveList()
    num_archives = len(archive_list)
    for index, archive_obj in enumerate(archive_list):
      archive_desc = archive_obj.GetArchiveData()
      archive_file = None
      if archive_desc.name in overlaid_archives:
        archive_file = package_locations.GetLocalPackageArchiveFile(
            overlay_tar_dir,
            archive_desc.name,
            archive_desc.hash)
        logging.info('Using overlaid tar: %s', archive_file)
      else:
        archive_file = package_locations.GetLocalPackageArchiveFile(
            tar_dir,
            archive_desc.name,
            archive_desc.hash)

      # Upon extraction, some files may not be downloaded (or have stale files),
      # we need to check the hash of each file and attempt to download it if
      # they do not match.
      archive_hash = archive_info.GetArchiveHash(archive_file)
      if archive_hash != archive_desc.hash:
        if archive_desc.url is None:
          if skip_missing:
            logging.info('Skipping extraction of missing archive: %s' %
                         archive_file)
            continue
          raise error.Error('Invalid archive file and URL: %s' % archive_file)

        logging.warn('Archive missing, downloading: %s', archive_desc.name)
        logging.info('Downloading %s: %s', archive_desc.name, archive_desc.url)

        pynacl.file_tools.MakeParentDirectoryIfAbsent(archive_file)
        downloader(archive_desc.url, archive_file)
        archive_hash = archive_info.GetArchiveHash(archive_file)
        if archive_hash != archive_desc.hash:
          raise error.Error('Downloaded archive file does not match hash.'
                      ' [%s] Expected %s, received %s.' %
                      (archive_file, archive_desc.hash, archive_hash))

      destination_dir = os.path.join(dest_package_dir, archive_desc.extract_dir)
      logging.info('Extracting %s (%d/%d)' %
                   (archive_desc.name, index+1, num_archives))

      temp_dir = os.path.join(destination_dir, '.tmp')
      pynacl.file_tools.RemoveDirectoryIfPresent(temp_dir)
      os.makedirs(temp_dir)
      tar_output = not quiet
      tar = cygtar.CygTar(archive_file, 'r:*', verbose=tar_output)
      curdir = os.getcwd()
      os.chdir(temp_dir)
      try:
        tar.Extract()
        tar.Close()
      finally:
        os.chdir(curdir)

      temp_src_dir = os.path.join(temp_dir, archive_desc.tar_src_dir)
      pynacl.file_tools.MoveAndMergeDirTree(temp_src_dir, destination_dir)
      pynacl.file_tools.RemoveDirectoryIfPresent(temp_dir)

    pynacl.file_tools.MakeParentDirectoryIfAbsent(dest_package_file)
    package_desc.SavePackageFile(dest_package_file)


def CleanupTarDirectory(tar_dir):
  """Deletes any files within the tar directory that are not referenced.

  Files such as package archives are shared between packages and therefore
  non-trivial to delete. Package files may also change so old packages may
  stay on the local hard drive even though they are not read anymore. This
  function will walk through the tar directory and cleanup any stale files
  it does not recognize.

  Args:
    tar_dir: Source tar directory where package archives live.
  """
  # Keep track of the names of all known files and directories. Because of
  # case insensitive file systems, we should lowercase all the paths so
  # that we do not accidentally delete any files.
  known_directories = set()
  known_files = set()

  for package_target, package_list in package_locations.WalkPackages(tar_dir):
    for package_name in package_list:
      package_file = package_locations.GetLocalPackageFile(tar_dir,
                                                           package_target,
                                                           package_name)
      try:
        package_desc = package_info.PackageInfo(package_file, skip_missing=True)
      except:
        continue

      for archive_obj in package_desc.GetArchiveList():
        archive_desc = archive_obj.GetArchiveData()
        if not archive_desc.hash:
          continue
        archive_file = package_locations.GetLocalPackageArchiveFile(
            tar_dir,
            archive_desc.name,
            archive_desc.hash)
        log_file = package_locations.GetLocalPackageArchiveLogFile(archive_file)

        known_files.add(archive_file.lower())
        known_files.add(log_file.lower())

      package_name = package_info.GetLocalPackageName(package_file)
      package_directory = os.path.join(os.path.dirname(package_file),
                                       package_name)

      known_files.add(package_file.lower())
      known_directories.add(package_directory.lower())

  # We are going to be deleting all files that do not match any known files,
  # so do a sanity check that this is an actual tar directory. If we have no
  # known files or directories, we probably do not have a valid tar directory.
  if not known_directories or not known_files:
    raise error.Error('No packages found for tar directory: %s' % tar_dir)

  for dirpath, dirnames, filenames in os.walk(tar_dir, topdown=False):
    if dirpath.lower() in known_directories:
      continue

    for filename in filenames:
      full_path = os.path.join(dirpath, filename)
      if full_path.lower() in known_files:
        continue

      logging.debug('Removing stale file: %s', full_path)
      os.unlink(full_path)

    if not os.listdir(dirpath):
      logging.debug('Removing stale directory: %s', dirpath)
      os.rmdir(dirpath)


#
# Each Command has 2 functions that describes it:
#   1. A parser function which specifies the extra command options each command
#   will have.
#   2. An execution function which is called when a user actually executes
#   the command.
#
def _ListCmdArgParser(subparser):
  subparser.description = 'Lists package information.'


def _DoListCmd(arguments):
  package_targets = collections.defaultdict(list)
  for package_target, package in arguments.package_target_packages:
    package_targets[package_target].append(package)

  modes_dict = arguments.packages_desc.GetPackageModes()
  if not modes_dict:
    print 'No Package Modes Found.'
  else:
    print 'Listing Modes:'
    for mode, package_list in modes_dict.iteritems():
      print ' [%s]' % mode
      for package in package_list:
        print '  ', package

  if arguments.mode:
    print
    print 'Current Mode Selected:', arguments.mode

  print
  print 'Listing Package Targets and Packages:'
  for package_target, packages in package_targets.iteritems():
    print ' [%s]:' % package_target
    for package in sorted(packages):
      print '  ', package


def _ArchiveCmdArgParser(subparser):
  subparser.description = 'Archive package archives to tar directory.'
  subparser.add_argument(
    '--archive-package', metavar='NAME', dest='archive__package',
    required=True,
    help='Package name archives will be packaged into.')
  subparser.add_argument(
    '--extra-archive', metavar='ARCHIVE', dest='archive__extra_archive',
    action='append', default=[],
    help='Extra archives that are expected to be built elsewhere.')
  subparser.add_argument(
    metavar='TAR(,SRCDIR(:EXTRACTDIR))(@URL,LOGURL)', dest='archive__archives',
    nargs='+',
    help='Package archive with an optional tar information and url.'
         ' SRCDIR is the root directory where files live inside of the tar.'
         ' EXTRACTDIR is the directory to extract files to relative to the'
         ' destination directory. The URL is where the package can be'
         ' downloaded from.')
  subparser.add_argument(
    '-x', '--extract', dest='archive__extract',
    action='store_true', default=False,
    help='Extract package archives after they have been archived.')


def _DoArchiveCmd(arguments):
  package_target_packages = GetPackageTargetPackages(
      arguments.archive__package,
      arguments.package_target_packages
  )
  if not package_target_packages:
    raise error.Error('Unknown package: %s.' % arguments.archive__package
                + ' Did you forget to add "$PACKAGE_TARGET/"?')

  for package_target, package_name in package_target_packages:
    ArchivePackageArchives(arguments.tar_dir,
                           package_target,
                           package_name,
                           arguments.archive__archives,
                           extra_archives=arguments.archive__extra_archive)

    if arguments.archive__extract:
      ExtractPackageTargets([(package_target, package_name)],
                            arguments.tar_dir,
                            arguments.dest_dir,
                            skip_missing=True,
                            quiet=arguments.quiet)


def _ExtractCmdArgParser(subparser):
  subparser.description = 'Extract packages from tar directory.'
  subparser.add_argument(
    '--skip-missing', dest='extract__skip_missing',
    action='store_true', default=False,
    help='Skip missing archive files when extracting rather than erroring out.')
  subparser.add_argument(
    '--overlay-tar-dir', dest='overlay_tar_dir',
    default=None,
    help='Extracts tar directories as usual, except uses any packages' +
         ' found within the overlay tar directory first.')


def _DoExtractCmd(arguments):
  ExtractPackageTargets(
      arguments.package_target_packages,
      arguments.tar_dir,
      arguments.dest_dir,
      skip_missing=arguments.extract__skip_missing,
      overlay_tar_dir=arguments.overlay_tar_dir,
      quiet=arguments.quiet)


def _UploadCmdArgParser(subparser):
  subparser.description = 'Upload a package file.'
  subparser.add_argument(
    '--upload-package', metavar='NAME', dest='upload__package', required=True,
    help='Package to upload.')
  subparser.add_argument(
    '--revision', metavar='ID', dest='upload__revision', required=True,
    help='Revision of the package to upload.')
  subparser.add_argument(
    '--package-file', metavar='FILE', dest='upload__file',
    default=None,
    help='Use custom package file instead of standard package file found'
         ' in the tar directory.')
  subparser.add_argument(
    '--skip-missing', dest='upload__skip_missing',
    action='store_true', default=False,
    help='Skip missing archive files when uploading package archives.')


def _DoUploadCmd(arguments):
  package_target_packages = GetPackageTargetPackages(
      arguments.upload__package,
      arguments.package_target_packages
  )
  if not package_target_packages:
    raise error.Error('Unknown package: %s.' % arguments.upload__package
                + ' Did you forget to add "$PACKAGE_TARGET/"?')

  for package_target, package_name in package_target_packages:
    UploadPackage(
        arguments.gsd_store,
        arguments.upload__revision,
        arguments.tar_dir,
        package_target,
        package_name,
        arguments.packages_desc.IsSharedPackage(package_name),
        annotate=arguments.annotate,
        skip_missing=arguments.upload__skip_missing,
        custom_package_file=arguments.upload__file
    )


def _SyncCmdArgParser(subparser):
  subparser.description = 'Download package archives to the tar directory.'
  subparser.add_argument(
    '--revision', metavar='ID', dest='sync__revision',
    default=None,
    help='Revision identifier of the packages to download.')
  subparser.add_argument(
    '--include-logs', dest='sync__include_logs',
    action='store_true', default=False,
    help='Also download logs next to each archive if available.')
  subparser.add_argument(
    '-x', '--extract', dest='sync__extract',
    action='store_true', default=False,
    help='Extract package archives after they have been downloaded.')


def _DoSyncCmd(arguments):
  for package_target, package_name in arguments.package_target_packages:
    if arguments.sync__revision is None:
      # When the sync revision number is not specified, use the set
      # revision number found in the revision directory.
      revision_file = package_locations.GetRevisionFile(
          arguments.revisions_dir,
          package_name)
      revision_desc = revision_info.RevisionInfo(
          arguments.packages_desc,
          revision_file)
      package_desc = revision_desc.GetPackageInfo(package_target)
      revision_num = revision_desc.GetRevisionNumber()
    else:
      # When the sync revision number is specified, find the package to
      # download remotely using the revision.
      revision_num = arguments.sync__revision
      remote_package_key = package_locations.GetRemotePackageKey(
          arguments.packages_desc.IsSharedPackage(package_name),
          arguments.sync__revision,
          package_target,
          package_name)
      with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
        temp_package_file = os.path.join(
            work_dir,
            os.path.basename(remote_package_key) + TEMP_SUFFIX)

        package_info.DownloadPackageInfoFiles(
            temp_package_file,
            remote_package_key,
            downloader=arguments.gsd_store.GetFile)

        package_desc = package_info.PackageInfo(temp_package_file)

    DownloadPackageArchives(
        arguments.tar_dir,
        package_target,
        package_name,
        package_desc,
        revision_num=revision_num,
        include_logs=arguments.sync__include_logs)

  CleanTempFiles(arguments.tar_dir)

  if arguments.sync__extract:
    ExtractPackageTargets(
        arguments.package_target_packages,
        arguments.tar_dir,
        arguments.dest_dir,
        quiet=arguments.quiet)


def _SetRevisionCmdArgParser(subparser):
  subparser.description = 'Specify the revision of a package.'
  subparser.add_argument(
    '--revision-package', metavar='NAME', dest='setrevision__package',
    action='append', default=[],
    help='Package name to set revision of.')
  subparser.add_argument(
    '--revision-set', metavar='SET-NAME', dest='setrevision__revset',
    action='append', default=[],
    help='Revision set to set revision for.')
  subparser.add_argument(
    '--revision', metavar='ID', dest='setrevision__revision',
    required=True,
    help='Revision identifier of the package to set.')


def _DoSetRevisionCmd(arguments):
  packages_list = arguments.setrevision__package
  revision_sets = arguments.setrevision__revset
  revision_num = arguments.setrevision__revision

  for revision_set in revision_sets:
    set_packages = arguments.packages_desc.GetRevisionSet(revision_set)
    if set_packages is None:
      raise error.Error('Invalid Revision Set: %s' % revision_set)
    packages_list.extend(set_packages)

  if not packages_list:
    raise error.Error('No revision packages have been supplied.')

  for package_name in packages_list:
    revision_desc = revision_info.RevisionInfo(arguments.packages_desc)
    revision_desc.SetRevisionNumber(revision_num)

    custom_package_targets = GetPackageTargetPackages(package_name, [])
    if not custom_package_targets:
      package_targets = arguments.packages_desc.GetPackageTargetsForPackage(
          package_name
      )
    else:
      package_targets = [target[0] for target in custom_package_targets]
      first_target = custom_package_targets[0]
      package_name = first_target[1]

    for package_target in package_targets:
      with pynacl.working_directory.TemporaryWorkingDirectory() as work_dir:
        remote_package_key = package_locations.GetRemotePackageKey(
            arguments.packages_desc.IsSharedPackage(package_name),
            revision_num,
            package_target,
            package_name)

        temp_package_file = os.path.join(
            work_dir,
            os.path.basename(remote_package_key) + TEMP_SUFFIX)

        package_info.DownloadPackageInfoFiles(
            temp_package_file,
            remote_package_key,
            downloader=arguments.gsd_store.GetFile)

        package_desc = package_info.PackageInfo(temp_package_file)

        logging.info('Setting %s:%s to revision %s',
                     package_target, package_name, revision_num)
        revision_desc.SetTargetRevision(
            package_name,
            package_target,
            package_desc)

    revision_file = package_locations.GetRevisionFile(
        arguments.revisions_dir,
        package_name)
    pynacl.file_tools.MakeParentDirectoryIfAbsent(revision_file)
    revision_desc.SaveRevisionFile(revision_file)

  CleanTempFiles(arguments.revisions_dir)


def _GetRevisionCmdArgParser(subparser):
  subparser.description = 'Get the revision of a package.'
  subparser.add_argument(
    '--revision-package', metavar='NAME', dest='getrevision__packages',
    action='append', default=[],
    help='Package name to get revision of.')
  subparser.add_argument(
    '--revision-set', metavar='SET-NAME', dest='getrevision__revset',
    action='append', default=[],
    help='Revision set to set revision for.')


def _DoGetRevisionCmd(arguments):
  packages_list = arguments.getrevision__packages
  revision_sets = arguments.getrevision__revset

  for revision_set in revision_sets:
    set_packages = arguments.packages_desc.GetRevisionSet(revision_set)
    if set_packages is None:
      raise error.Error('Invalid Revision Set: %s' % revision_set)
    packages_list.extend(set_packages)

  if not packages_list:
    raise error.Error('No revision packages have been supplied.')

  revision_number = None
  for package_name in packages_list:
    custom_package_targets = GetPackageTargetPackages(package_name, [])
    if custom_package_targets:
      custom_target, package_name = custom_package_targets[0]

    revision_file = package_locations.GetRevisionFile(arguments.revisions_dir,
                                                      package_name)

    if not os.path.isfile(revision_file):
      raise error.Error('No revision set for package: %s.' % package_name)

    revision_desc = revision_info.RevisionInfo(arguments.packages_desc,
                                               revision_file)

    package_revision = revision_desc.GetRevisionNumber()
    if revision_number is None:
      revision_number = package_revision
    elif revision_number != package_revision:
      logging.error('Listing Get Revision Packages:')
      for package in packages_list:
        logging.error('  %s', package)
      raise error.Error('Package revisions are not set to the same revision.')

  print revision_number


def _RevPackagesCmdArgParser(subparser):
  subparser.description = 'Prints list of packages for a revision set name.'
  subparser.add_argument(
    '--revision-set', metavar='NAME', dest='revpackages__name',
    required=True,
    help='Name of the package or revision set.')


def _DoRevPackagesCmd(arguments):
  revision_package = arguments.revpackages__name
  packages_list = [revision_package]

  # Check if the package_name is a revision set.
  revision_set = arguments.packages_desc.GetRevisionSet(revision_package)
  if revision_set is not None:
    packages_list = revision_set

  for package_name in packages_list:
    print package_name


def _FillEmptyTarsParser(subparser):
  subparser.description = 'Fill missing archives with empty ones in a package.'
  subparser.add_argument(
    '--fill-package', metavar='NAME', dest='fillemptytars_package',
    required=True,
    help='Package name to fill empty archives of.')


def _DoFillEmptyTarsCmd(arguments):
  package_target_packages = GetPackageTargetPackages(
      arguments.fillemptytars_package,
      arguments.package_target_packages
  )
  if not package_target_packages:
    raise error.Error('Unknown package: %s.' % arguments.fillemptytars_package
                + ' Did you forget to add "$PACKAGE_TARGET/"?')

  for package_target, package_name in package_target_packages:
    package_path = package_locations.GetLocalPackageFile(arguments.tar_dir,
                                                         package_target,
                                                         package_name)

    package_desc = package_info.PackageInfo(package_path, skip_missing=True)
    output_package_desc = package_info.PackageInfo()
    for archive in package_desc.GetArchiveList():
      # If archive does not exist, fill it with an empty one.
      archive_data = archive.GetArchiveData()
      if archive_data.hash:
        output_package_desc.AppendArchive(archive)
      else:
        logging.info('Filling missing archive: %s.', archive_data.name)
        if (archive_data.name.endswith('.tar.gz') or
            archive_data.name.endswith('.tgz')):
          mode = 'w:gz'
        elif archive_data.name.endswith('.bz2'):
          mode = 'w:bz2'
        elif archive_data.name.endswith('.tar'):
          mode = 'w:'
        else:
          raise error.Error('Unknown archive type: %s.' % archive_data.name)

        temp_archive_file = os.path.join(arguments.tar_dir, archive_data.name)
        tar_file = cygtar.CygTar(temp_archive_file, mode)
        tar_file.Close()
        tar_hash = archive_info.GetArchiveHash(temp_archive_file)

        archive_file = package_locations.GetLocalPackageArchiveFile(
            arguments.tar_dir,
            archive_data.name,
            tar_hash)
        pynacl.file_tools.MakeParentDirectoryIfAbsent(archive_file)
        os.rename(temp_archive_file, archive_file)

        empty_archive = archive_info.ArchiveInfo(name=archive_data.name,
                                                 hash=tar_hash)
        output_package_desc.AppendArchive(empty_archive)

    output_package_desc.SavePackageFile(package_path)


def _RecalcRevsParser(subparser):
  subparser.description = 'Recalculates hashes for files in revision directory.'


def _DoRecalcRevsCmd(arguments):
  for json_file in os.listdir(arguments.revisions_dir):
    if json_file.endswith('.json'):
      revision_file = os.path.join(arguments.revisions_dir, json_file)
      revision_desc = revision_info.RevisionInfo(arguments.packages_desc)
      revision_desc.LoadRevisionFile(revision_file, skip_hash_verify=True)
      revision_desc.SaveRevisionFile(revision_file)


def _CleanupParser(subparser):
  subparser.description = 'Cleans up any unused package archives files.'


def _DoCleanupCmd(arguments):
  CleanupTarDirectory(arguments.tar_dir)


CommandFuncs = collections.namedtuple(
    'CommandFuncs',
    ['parse_func', 'do_cmd_func'])


COMMANDS = {
    'list': CommandFuncs(_ListCmdArgParser, _DoListCmd),
    'archive': CommandFuncs(_ArchiveCmdArgParser, _DoArchiveCmd),
    'extract': CommandFuncs(_ExtractCmdArgParser, _DoExtractCmd),
    'upload': CommandFuncs(_UploadCmdArgParser, _DoUploadCmd),
    'sync': CommandFuncs(_SyncCmdArgParser, _DoSyncCmd),
    'setrevision': CommandFuncs(_SetRevisionCmdArgParser, _DoSetRevisionCmd),
    'getrevision': CommandFuncs(_GetRevisionCmdArgParser, _DoGetRevisionCmd),
    'revpackages': CommandFuncs(_RevPackagesCmdArgParser, _DoRevPackagesCmd),
    'fillemptytars': CommandFuncs(_FillEmptyTarsParser, _DoFillEmptyTarsCmd),
    'recalcrevisions': CommandFuncs(_RecalcRevsParser, _DoRecalcRevsCmd),
    'cleanup': CommandFuncs(_CleanupParser, _DoCleanupCmd),
}


def ParseArgs(args):
  parser = argparse.ArgumentParser()

  host_platform = pynacl.platform.GetOS()
  host_arch = pynacl.platform.GetArch3264()

  # List out global options for all commands.
  parser.add_argument(
    '-v', '--verbose', dest='verbose',
    action='store_true', default=False,
    help='Verbose output')
  parser.add_argument(
    '-q', '--quiet', dest='quiet',
    action='store_true', default=False,
    help='Quiet output')
  parser.add_argument(
    '--platform', dest='host_platform',
    default=host_platform,
    help='Custom platform other than the current (%s).' % host_platform)
  parser.add_argument(
    '--arch', dest='host_arch',
    default=host_arch,
    help='Custom architecture other than the current (%s).' % host_arch)
  parser.add_argument(
    '--package-targets', dest='package_targets',
    default=None,
    help='Custom package targets specifed as comma separated names. Defaults'
         ' to package targets defined for host platform and architecture inside'
         ' of the packages json file.')
  parser.add_argument(
    '--mode', dest='mode',
    default=None,
    help='Specify a package mode to filter by, modes are specified within'
         ' the packages json file. For a list of modes use the "list" command.')
  parser.add_argument(
    '--packages', dest='packages',
    default=None,
    help='Custom packages specified as comma separated package names. Custom'
         ' packages not defined by the packages json file must be prefixed by'
         ' the package_target directory (IE. $PACKAGE_TARGET/$PACKAGE).')
  parser.add_argument(
    '--append', metavar='PACKAGE', dest='append_packages',
    action='append', default=[],
    help='Append extra package to current list of packages.')
  parser.add_argument(
    '--exclude', metavar='PACKAGE', dest='exclude_packages',
    action='append', default=[],
    help='Exclude package from current list of packages.')
  parser.add_argument(
    '--packages-json', dest='packages_json',
    default=DEFAULT_PACKAGES_JSON, type=argparse.FileType('rt'),
    help='Packages description file.'
         ' [Default: %s]' % DEFAULT_PACKAGES_JSON)
  parser.add_argument(
    '--revisions-dir', dest='revisions_dir',
    default=DEFAULT_REVISIONS_DIR,
    help='Revisions directory where packages revisions will be found.')
  parser.add_argument(
    '--dest-dir', dest='dest_dir',
    default=DEFAULT_DEST_DIR,
    help='Destination directory where all the packages will be extracted to.')
  parser.add_argument(
    '--tar-dir', dest='tar_dir',
    default=None,
    help='Directory for package archive files. Defaults to "$DEST-DIR/.tars".')
  parser.add_argument(
    '--annotate', dest='annotate',
    action='store_true', default=False,
    help='Print out build bot annotations.')
  parser.add_argument(
    '--cloud-bucket', dest='cloud_bucket',
    default=DEFAULT_CLOUD_BUCKET,
    help='Google storage cloud bucket name.'
         ' [Default: %s]' % DEFAULT_CLOUD_BUCKET)

  # Add subparsers for all commands. These are flags for specific commands,
  # IE. [options] command [command-options]
  command_parser = parser.add_subparsers(title='command', dest='command')
  for command, cmd_funcs in COMMANDS.iteritems():
    sub_parser = command_parser.add_parser(command)
    cmd_funcs.parse_func(sub_parser)

  arguments = parser.parse_args(args)
  pynacl.log_tools.SetupLogging(
      verbose=arguments.verbose, quiet=arguments.quiet)
  if arguments.tar_dir is None:
    arguments.tar_dir = os.path.join(arguments.dest_dir, '.tars')

  # Parse the package description up front and store it into the arguments
  # object. Almost all the commands need to use this information.
  packages_desc = packages_info.PackagesInfo(arguments.packages_json)
  arguments.packages_desc = packages_desc

  # Based on the host platform and host architecture, we can determine the set
  # of package targets used from the packages description. Minimize platform
  # and architecture errors by standardizing the names using pynacl.platform.
  if arguments.package_targets is None:
    package_targets = packages_desc.GetPackageTargets(
        pynacl.platform.GetOS(arguments.host_platform),
        pynacl.platform.GetArch3264(arguments.host_arch))
  else:
    package_targets = arguments.package_targets.split(',')

  # If the packages argument were not set, use the default list of packages
  # for each package target.
  packages_set = set()
  if arguments.packages is None:
    for package_target in package_targets:
      packages = packages_desc.GetPackages(package_target)
      if packages is None:
        raise error.Error('No packages defined for Package Target: %s.' %
                          package_target)
      packages_set.update(packages)
  else:
    packages_set.update(arguments.packages.split(','))

  # If a mode was set, only use packages listed in the mode.
  if arguments.mode:
    modes_dict = packages_desc.GetPackageModes()
    if arguments.mode not in modes_dict:
      logging.info('Valid Package Modes:')
      for mode in modes_dict:
        logging.info('  %s', mode)
      raise error.Error('Invalid Package Mode: %s.' % arguments.mode)
    packages_set.intersection_update(modes_dict[arguments.mode])

  # Append/exclude any extra packages that were specified.
  packages_set.update(arguments.append_packages)
  packages_set.difference_update(arguments.exclude_packages)

  # Build a dictionary that organizes packages to their respective package
  # targets. Packages may exist in multiple package targets so we will have
  # to have the key be package and value be a list of package targets.
  package_targets_dict = collections.defaultdict(list)
  for package_target in package_targets:
    for package in packages_desc.GetPackages(package_target):
      package_targets_dict[package].append(package_target)

  # Use the list of packages to determine the set of package target packages
  # we are operating on, custom package targets will have the package target
  # inside of the name of the package name (see help for "--packages" argument).
  # The package_target_packages is a list of tuples (package_target, package),
  # for every package along with the associated package target.
  package_target_packages = []
  for package in sorted(packages_set):
    package_targets = package_targets_dict.get(package, None)
    if package_targets is None:
      custom_package_targets = GetPackageTargetPackages(package, [])
      if not custom_package_targets:
        raise error.Error('Invalid custom package: "%s".'
                          ' Expected $PACKAGE_TARGET/$PACKAGE' % package)
      package_target_packages.extend(custom_package_targets)
    else:
      for package_target in package_targets:
        package_target_packages.append((package_target, package))

  arguments.package_target_packages = package_target_packages

  # Create a GSD Storage object for those who need it.
  cloud_bucket = arguments.cloud_bucket
  gsd_store = pynacl.gsd_storage.GSDStorage(cloud_bucket, [cloud_bucket])
  arguments.gsd_store = gsd_store

  return arguments


def main(args):
  # If verbose is on, do not catch error.Error() exceptions separately but
  # allow python to catch the errors and print out the entire callstack.
  # Note that we cannot rely on ParseArgs() to parse if verbose is on, because
  # ParseArgs() could throw an exception.
  if '-v' in args or '--verbose' in args:
    arguments = ParseArgs(args)
    return COMMANDS[arguments.command].do_cmd_func(arguments)
  else:
    try:
      arguments = ParseArgs(args)
      return COMMANDS[arguments.command].do_cmd_func(arguments)
    except error.Error as e:
      sys.stderr.write('package_version: ' + str(e) + '\n')
      return 1


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
