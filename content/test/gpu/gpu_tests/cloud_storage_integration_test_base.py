# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Base classes for a test which uploads results (reference images,
error images) to cloud storage."""

import logging
import os
import re
import tempfile

from py_utils import cloud_storage
from telemetry.util import image_util
from telemetry.util import rgba_color

from gpu_tests import gpu_integration_test

test_data_dir = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', 'data', 'gpu'))

default_generated_data_dir = os.path.join(test_data_dir, 'generated')

error_image_cloud_storage_bucket = 'chromium-browser-gpu-tests'


class _ReferenceImageParameters(object):
  def __init__(self):
    # Parameters for cloud storage reference images.
    self.vendor_id = None
    self.device_id = None
    self.vendor_string = None
    self.device_string = None
    self.msaa = False
    self.model_name = None


class CloudStorageIntegrationTestBase(gpu_integration_test.GpuIntegrationTest):
  # This class is abstract; don't warn about the superclass's abstract
  # methods that aren't overridden.
  # pylint: disable=abstract-method

  # This information is class-scoped, so that it can be shared across
  # invocations of tests; but it's zapped every time the browser is
  # restarted with different command line arguments.
  _reference_image_parameters = None

  # The command line options (which are passed to subclasses'
  # GenerateGpuTests) *must* be configured here, via a call to
  # SetParsedCommandLineOptions. If they are not, an error will be
  # raised when running the tests.
  _parsed_command_line_options = None

  @classmethod
  def SetParsedCommandLineOptions(cls, options):
    cls._parsed_command_line_options = options

  @classmethod
  def GetParsedCommandLineOptions(cls):
    return cls._parsed_command_line_options

  @classmethod
  def AddCommandlineArgs(cls, parser):
    parser.add_option(
      '--build-revision',
      help='Chrome revision being tested.',
      default="unknownrev")
    parser.add_option(
      '--upload-refimg-to-cloud-storage',
      dest='upload_refimg_to_cloud_storage',
      action='store_true', default=False,
      help='Upload resulting images to cloud storage as reference images')
    parser.add_option(
      '--download-refimg-from-cloud-storage',
      dest='download_refimg_from_cloud_storage',
      action='store_true', default=False,
      help='Download reference images from cloud storage')
    parser.add_option(
      '--refimg-cloud-storage-bucket',
      help='Name of the cloud storage bucket to use for reference images; '
      'required with --upload-refimg-to-cloud-storage and '
      '--download-refimg-from-cloud-storage. Example: '
      '"chromium-gpu-archive/reference-images"')
    parser.add_option(
      '--os-type',
      help='Type of operating system on which the pixel test is being run, '
      'used only to distinguish different operating systems with the same '
      'graphics card. Any value is acceptable, but canonical values are '
      '"win", "mac", and "linux", and probably, eventually, "chromeos" '
      'and "android").',
      default='')
    parser.add_option(
      '--test-machine-name',
      help='Name of the test machine. Specifying this argument causes this '
      'script to upload failure images and diffs to cloud storage directly, '
      'instead of relying on the archive_gpu_pixel_test_results.py script.',
      default='')
    parser.add_option(
      '--generated-dir',
      help='Overrides the default on-disk location for generated test images '
      '(only used for local testing without a cloud storage account)',
      default=default_generated_data_dir)
    parser.add_option(
      '--dont-restore-color-profile-after-test',
      dest='dont_restore_color_profile_after_test',
      action='store_true', default=False,
      help='(Mainly on Mac) don\'t restore the system\'s original color '
      'profile after the test completes; leave the system using the sRGB color '
      'profile. See http://crbug.com/784456.')

  def _CompareScreenshotSamples(self, tab, screenshot, expected_colors,
                                device_pixel_ratio, test_machine_name):
    # First scan through the expected_colors and see if there are any scale
    # factor overrides that would preempt the device pixel ratio. This
    # is mainly a workaround for complex tests like the Maps test.
    for expectation in expected_colors:
      if 'scale_factor_overrides' in expectation:
        for override in expectation['scale_factor_overrides']:
          # Require exact matches to avoid confusion, because some
          # machine models and names might be subsets of others
          # (e.g. Nexus 5 vs Nexus 5X).
          if ('device_type' in override and
              (tab.browser.platform.GetDeviceTypeName() ==
               override['device_type'])):
            logging.warning(
              'Overriding device_pixel_ratio ' + str(device_pixel_ratio) +
              ' with scale factor ' + str(override['scale_factor']) +
              ' for device type ' + override['device_type'])
            device_pixel_ratio = override['scale_factor']
            break
          if (test_machine_name and 'machine_name' in override and
              override["machine_name"] == test_machine_name):
            logging.warning(
              'Overriding device_pixel_ratio ' + str(device_pixel_ratio) +
              ' with scale factor ' + str(override['scale_factor']) +
              ' for machine name ' + test_machine_name)
            device_pixel_ratio = override['scale_factor']
            break
        # Only support one "scale_factor_overrides" in the expectation format.
        break
    for expectation in expected_colors:
      if "scale_factor_overrides" in expectation:
        continue
      location = expectation["location"]
      size = expectation["size"]
      x0 = int(location[0] * device_pixel_ratio)
      x1 = int((location[0] + size[0]) * device_pixel_ratio)
      y0 = int(location[1] * device_pixel_ratio)
      y1 = int((location[1] + size[1]) * device_pixel_ratio)
      for x in range(x0, x1):
        for y in range(y0, y1):
          if (x < 0 or y < 0 or x >= image_util.Width(screenshot) or
              y >= image_util.Height(screenshot)):
            self.fail(
                ('Expected pixel location [%d, %d] is out of range on ' +
                 '[%d, %d] image') %
                (x, y, image_util.Width(screenshot),
                 image_util.Height(screenshot)))

          actual_color = image_util.GetPixelColor(screenshot, x, y)
          expected_color = rgba_color.RgbaColor(
              expectation["color"][0],
              expectation["color"][1],
              expectation["color"][2])
          if not actual_color.IsEqual(expected_color, expectation["tolerance"]):
            self.fail('Expected pixel at ' + str(location) +
                ' (actual pixel (' + str(x) + ', ' + str(y) + ')) ' +
                ' to be ' +
                str(expectation["color"]) + " but got [" +
                str(actual_color.r) + ", " +
                str(actual_color.g) + ", " +
                str(actual_color.b) + "]")

  ###
  ### Routines working with the local disk (only used for local
  ### testing without a cloud storage account -- the bots do not use
  ### this code path).
  ###

  def _UrlToImageName(self, url):
    image_name = re.sub(r'^(http|https|file)://(/*)', '', url)
    image_name = re.sub(r'\.\./', '', image_name)
    image_name = re.sub(r'(\.|/|-)', '_', image_name)
    return image_name

  def _WriteImage(self, image_path, png_image):
    output_dir = os.path.dirname(image_path)
    if not os.path.exists(output_dir):
      os.makedirs(output_dir)
    image_util.WritePngFile(png_image, image_path)

  def _WriteErrorImages(self, img_dir, img_name, screenshot, ref_png):
    full_image_name = img_name + '_' + str(
      self.GetParsedCommandLineOptions().build_revision)
    full_image_name = full_image_name + '.png'

    # Always write the failing image.
    self._WriteImage(
        os.path.join(img_dir, 'FAIL_' + full_image_name), screenshot)

    if ref_png is not None:
      # Save the reference image.
      # This ensures that we get the right revision number.
      self._WriteImage(
          os.path.join(img_dir, full_image_name), ref_png)

      # Save the difference image.
      diff_png = image_util.Diff(screenshot, ref_png)
      self._WriteImage(
          os.path.join(img_dir, 'DIFF_' + full_image_name), diff_png)

  ###
  ### Cloud storage code path -- the bots use this.
  ###

  @classmethod
  def ResetGpuInfo(cls):
    cls._reference_image_parameters = None

  @classmethod
  def _ComputeGpuInfo(cls, tab, page):
    if cls._reference_image_parameters:
      return
    browser = cls.browser
    if not browser.supports_system_info:
      raise Exception('System info must be supported by the browser')
    system_info = browser.GetSystemInfo()
    if not system_info.gpu:
      raise Exception('GPU information was absent')
    device = system_info.gpu.devices[0]
    cls._reference_image_parameters = _ReferenceImageParameters()
    params = cls._reference_image_parameters
    if device.vendor_id and device.device_id:
      params.vendor_id = device.vendor_id
      params.device_id = device.device_id
    elif device.vendor_string and device.device_string:
      params.vendor_string = device.vendor_string
      params.device_string = device.device_string
    elif page.gpu_process_disabled:
      # Match the vendor and device IDs that the browser advertises
      # when the software renderer is active.
      params.vendor_id = 65535
      params.device_id = 65535
    else:
      raise Exception('GPU device information was incomplete')
    # TODO(senorblanco): This should probably be checking
    # for the presence of the extensions in system_info.gpu_aux_attributes
    # in order to check for MSAA, rather than sniffing the blacklist.
    params.msaa = not (
        ('disable_chromium_framebuffer_multisample' in
          system_info.gpu.driver_bug_workarounds) or
        ('disable_multisample_render_to_texture' in
          system_info.gpu.driver_bug_workarounds))
    params.model_name = system_info.model_name

  @classmethod
  def _FormatGpuInfo(cls, tab, page):
    cls._ComputeGpuInfo(tab, page)
    params = cls._reference_image_parameters
    msaa_string = '_msaa' if params.msaa else '_non_msaa'
    if params.vendor_id:
      os_type = cls.GetParsedCommandLineOptions().os_type
      if str(cls.browser.platform.GetOSVersionName()).lower() == "win10":
        # Allow separate baselines for Windows 10 and Windows 7.
        os_type = "win10"
      return '%s_%04x_%04x%s' % (
        os_type, params.vendor_id, params.device_id, msaa_string)
    else:
      # This is the code path for Android devices. Include the model
      # name (e.g. "Nexus 9") in the GPU string to disambiguate
      # multiple devices on the waterfall which might have the same
      # device string ("NVIDIA Tegra") but different screen
      # resolutions and device pixel ratios.
      return '%s_%s_%s_%s%s' % (
        cls.GetParsedCommandLineOptions().os_type,
        params.vendor_string, params.device_string,
        params.model_name, msaa_string)

  @classmethod
  def _FormatReferenceImageName(cls, img_name, page, tab):
    return '%s_v%s_%s.png' % (
      img_name,
      page.revision,
      cls._FormatGpuInfo(tab, page))

  @classmethod
  def _UploadBitmapToCloudStorage(cls, bucket, name, bitmap, public=False):
    # This sequence of steps works on all platforms to write a temporary
    # PNG to disk, following the pattern in bitmap_unittest.py. The key to
    # avoiding PermissionErrors seems to be to not actually try to write to
    # the temporary file object, but to re-open its name for all operations.
    temp_file = tempfile.NamedTemporaryFile(suffix='.png').name
    image_util.WritePngFile(bitmap, temp_file)
    cloud_storage.Insert(bucket, name, temp_file, publicly_readable=public)

  @classmethod
  def _ConditionallyUploadToCloudStorage(cls, img_name, page, tab, screenshot):
    """Uploads the screenshot to cloud storage as the reference image
    for this test, unless it already exists. Returns True if the
    upload was actually performed."""
    if not cls.GetParsedCommandLineOptions().refimg_cloud_storage_bucket:
      raise Exception('--refimg-cloud-storage-bucket argument is required')
    cloud_name = cls._FormatReferenceImageName(img_name, page, tab)
    if not cloud_storage.Exists(
        cls.GetParsedCommandLineOptions().refimg_cloud_storage_bucket,
        cloud_name):
      cls._UploadBitmapToCloudStorage(
        cls.GetParsedCommandLineOptions().refimg_cloud_storage_bucket,
        cloud_name,
        screenshot)
      return True
    return False

  @classmethod
  def _DownloadFromCloudStorage(cls, img_name, page, tab):
    """Downloads the reference image for the given test from cloud
    storage, returning it as a Telemetry Bitmap object."""
    # TODO(kbr): there's a race condition between the deletion of the
    # temporary file and gsutil's overwriting it.
    if not cls.GetParsedCommandLineOptions().refimg_cloud_storage_bucket:
      raise Exception('--refimg-cloud-storage-bucket argument is required')
    temp_file = tempfile.NamedTemporaryFile(suffix='.png').name
    cloud_storage.Get(
      cls.GetParsedCommandLineOptions().refimg_cloud_storage_bucket,
      cls._FormatReferenceImageName(img_name, page, tab),
      temp_file)
    return image_util.FromPngFile(temp_file)

  @classmethod
  def _UploadErrorImagesToCloudStorage(cls, image_name, screenshot, ref_img):
    """For a failing run, uploads the failing image, reference image (if
    supplied), and diff image (if reference image was supplied) to cloud
    storage. This subsumes the functionality of the
    archive_gpu_pixel_test_results.py script."""
    machine_name = re.sub(r'\W+', '_',
                          cls.GetParsedCommandLineOptions().test_machine_name)
    upload_dir = '%s_%s_telemetry' % (
      cls.GetParsedCommandLineOptions().build_revision, machine_name)
    base_bucket = '%s/runs/%s' % (error_image_cloud_storage_bucket, upload_dir)
    image_name_with_revision = '%s_%s.png' % (
      image_name, cls.GetParsedCommandLineOptions().build_revision)
    cls._UploadBitmapToCloudStorage(
      base_bucket + '/gen', image_name_with_revision, screenshot,
      public=True)
    if ref_img is not None:
      cls._UploadBitmapToCloudStorage(
        base_bucket + '/ref', image_name_with_revision, ref_img, public=True)
      diff_img = image_util.Diff(screenshot, ref_img)
      cls._UploadBitmapToCloudStorage(
        base_bucket + '/diff', image_name_with_revision, diff_img,
        public=True)
    print ('See http://%s.commondatastorage.googleapis.com/'
           'view_test_results.html?%s for this run\'s test results') % (
      error_image_cloud_storage_bucket, upload_dir)

  def _ValidateScreenshotSamples(self, tab, url,
                                 screenshot, expectations, device_pixel_ratio):
    """Samples the given screenshot and verifies pixel color values.
       The sample locations and expected color values are given in expectations.
       In case any of the samples do not match the expected color, it raises
       a Failure and dumps the screenshot locally or cloud storage depending on
       what machine the test is being run."""
    try:
      self._CompareScreenshotSamples(
        tab, screenshot, expectations,
        device_pixel_ratio,
        self.GetParsedCommandLineOptions().test_machine_name)
    except Exception:
      # An exception raised from self.fail() indicates a failure.
      image_name = self._UrlToImageName(url)
      if self.GetParsedCommandLineOptions().test_machine_name:
        self._UploadErrorImagesToCloudStorage(image_name, screenshot, None)
      else:
        self._WriteErrorImages(
          self.GetParsedCommandLineOptions().generated_dir, image_name,
          screenshot, None)
      raise
