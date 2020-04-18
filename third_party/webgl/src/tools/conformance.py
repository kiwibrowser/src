# -*- coding: utf-8 -*-
import argparse
import atexit
import datetime
import inspect
import json
import logging
import os
import platform
import re
import urllib2
import shutil
import socket
import subprocess
import sys
import time

try:
    import selenium
    from selenium import webdriver
    from selenium.common.exceptions import NoSuchElementException
    from selenium.common.exceptions import TimeoutException
    from selenium.common.exceptions import WebDriverException
    from selenium.webdriver.support.select import Select
    from selenium.webdriver.support.ui import WebDriverWait
except ImportError:
    print('Please install package selenium')
    exit(1)


class Util(object):
    LOGGER_NAME = __file__

    @staticmethod
    def diff_list(a, b):
        return list(set(a).difference(set(b)))

    @staticmethod
    def intersect_list(a, b):
        return list(set(a).intersection(set(b)))

    @staticmethod
    def ensure_dir(dir_path):
        if not os.path.exists(dir_path):
            os.makedirs(dir_path)

    @staticmethod
    def ensure_nodir(dir_path):
        if os.path.exists(dir_path):
            shutil.rmtree(dir_path)

    @staticmethod
    def ensure_file(file_path):
        Util.ensure_dir(os.path.dirname(os.path.abspath(file_path)))
        if not os.path.exists(file_path):
            Cmd('touch ' + file_path)

    @staticmethod
    def ensure_nofile(file_path):
        if os.path.exists(file_path):
            os.remove(file_path)

    @staticmethod
    def error(msg):
        _logger = Util.get_logger()
        _logger.error(msg)
        exit(1)

    @staticmethod
    def not_implemented():
        Util.error('not_mplemented() at line %s' % inspect.stack()[1][2])

    @staticmethod
    def get_caller_name():
        return inspect.stack()[1][3]

    @staticmethod
    def get_datetime(format='%Y%m%d%H%M%S'):
        return time.strftime(format, time.localtime())

    @staticmethod
    def get_env(env):
        return os.getenv(env)

    @staticmethod
    def set_env(env, value):
        if value:
            os.environ[env] = value

    @staticmethod
    def unset_env(env):
        if env in os.environ:
            del os.environ[env]

    @staticmethod
    def get_executable_suffix(host_os):
        if host_os.is_win():
            return '.exe'
        else:
            return ''

    @staticmethod
    def get_logger():
        return logging.getLogger(Util.LOGGER_NAME)

    @staticmethod
    def set_logger(log_file, level, show_time=False):
        if show_time:
            formatter = logging.Formatter('[%(asctime)s - %(levelname)s] %(message)s', '%Y-%m-%d %H:%M:%S')
        else:
            formatter = logging.Formatter('[%(levelname)s] %(message)s')
        logger = logging.getLogger(Util.LOGGER_NAME)
        logger.setLevel(level)

        log_file = logging.FileHandler(log_file)
        log_file.setFormatter(formatter)
        logger.addHandler(log_file)

        console = logging.StreamHandler()
        console.setFormatter(formatter)
        logger.addHandler(console)

    @staticmethod
    def has_pkg(pkg):
        cmd = Cmd('dpkg -s ' + pkg)
        if cmd.status:
            return False
        else:
            return True

    @staticmethod
    def read_file(file_path):
        if not os.path.exists(file_path):
            return []

        f = open(file_path)
        lines = [line.rstrip('\n') for line in f]
        if len(lines) > 0:
            while (lines[-1] == ''):
                del lines[-1]
        f.close()
        return lines

    @staticmethod
    def use_slash(s):
        if s:
            return s.replace('\\', '/')
        else:
            return s


class AndroidDevice(object):
    def __init__(self, id):
        self.id = id

    def get_prop(self, key):
        cmd = AdbShellCmd('getprop | grep %s' % key, device_id=self.id)
        match = re.search('\[%s\]: \[(.*)\]' % key, cmd.output)
        if match:
            return match.group(1)
        else:
            Util.error('Could not find %s' % key)


class AndroidDevices():
    def __init__(self):
        self.devices = []
        cmd = Cmd('adb devices')
        for device_line in cmd.output.split('\n'):
            if re.match('List of devices attached', device_line):
                continue
            elif re.match('^\s*$', device_line):
                continue
            elif re.search('offline', device_line):
                continue
            else:
                id = device_line.split()[0]
                self.devices.append(AndroidDevice(id))

        if len(self.devices) < 1:
            Util.error('Could not find available Android device')

    def get_device(self, device_id):
        if not device_id:
            if len(self.devices) > 1:
                self._logger.warning('There are more than one devices, and the first one will be used')
            return self.devices[0]
        else:
            for device in self.devices:
                if device.id == device_id:
                    return device
            else:
                Util.error('Cound not find Android device with id %s' % device_id)


class Cmd(object):
    def __init__(self, cmd, show_cmd=False, dryrun=False, abort=False):
        self._logger = Util.get_logger()
        self.cmd = cmd
        self.show_cmd = show_cmd
        self.dryrun = dryrun
        self.abort = abort

        if self.show_cmd:
            self._logger.info('[CMD]: %s' % self.cmd)

        if self.dryrun:
            self.status = 0
            self.output = ''
            self.process = None
            return

        tmp_output = ''
        process = subprocess.Popen(self.cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        while True:
            nextline = process.stdout.readline()
            if nextline == '' and process.poll() is not None:
                break
            tmp_output += nextline

        self.status = process.returncode
        (out, error) = process.communicate()
        self.output = tmp_output + out + error
        self.process = process

        if self.abort and self.status:
            Util.error('Failed to execute %s' % cmd, error_code=self.status)


class AdbShellCmd(Cmd):
    def __init__(self, cmd, device_id, dryrun=False, abort=False):
        fail_str = 'FAIL'
        cmd = 'adb -s %s shell "(%s) || echo %s"' % (device_id, cmd, fail_str)
        super(AdbShellCmd, self).__init__(cmd, dryrun, abort=False)
        if re.search('FAIL', self.output):
            if abort:
                Util.error('Failed to execute %s' % cmd, error_code=self.status)
            self.status = False
        else:
            self.status = True


class Timer(object):
    def __init__(self, use_ms=False):
        self.use_ms = use_ms
        self.timer = [0, 0]
        if self.use_ms:
            self.timer[0] = datetime.datetime.now()
        else:
            self.timer[0] = datetime.datetime.now().replace(microsecond=0)

    def stop(self):
        if self.use_ms:
            self.timer[1] = datetime.datetime.now()
        else:
            self.timer[1] = datetime.datetime.now().replace(microsecond=0)

    def diff(self):
            return self.timer[1] - self.timer[0]


class GPU(object):
    VENDOR_NAME_ID = {
        'amd': '1002',
        'intel': '8086',
        'nvidia': '10DE',
        'qualcomm': '5143',
    }
    VENDOR_NAMES = VENDOR_NAME_ID.keys()

    # produce info is from https://en.wikipedia.org/wiki/List_of_Intel_graphics_processing_units
    INTEL_GEN_ID = {
        '6': '0102,0106,0112,0116,0122,0126,010A',
        '7': '0152,0156,015A,0162,0166,016A',
        '7.5': '0402,0406,040A,040B,040E,0A02,0A06,0A0A,0A0B,0A0E,0C02,0C06,0C0A,0C0B,0C0E,0D02,0D06,0D0A,0D0B,0D0E' +
               '0412,0416,041A,041B,041E,0A12,0A16,0A1A,0A1B,0A1E,0C12,0C16,0C1A,0C1B,0C1E,0D12,0D16,0D1A,0D1B,0D1E' +
               '0422,0426,042A,042B,042E,0A22,0A26,0A2A,0A2B,0A2E,0C22,0C26,0C2A,0C2B,0C2E,0D22,0D26,0D2A,0D2B,0D2E',
        '8': '1606,161E,1616,1612,1626,162B,1622,22B0,22B1,22B2,22B3',
        '9': '1906,1902,191E,1916,191B,1912,191D,1926,193B,193D,0A84,1A84,1A85,5A84,5A85',
        '9.5': '5912',
    }

    def __init__(self, vendor_name, vendor_id, product_name, product_id, driver_version):
        self.vendor_name = vendor_name.lower()
        self.vendor_id = vendor_id
        # We may not get vendor_name and vendor_id at the same time. For example, only vendor_name is available on Android.
        for vendor_name in self.VENDOR_NAME_ID:
            if self._is_vendor_name(vendor_name):
                if not self.vendor_name:
                    self.vendor_name = vendor_name
                if not self.vendor_id:
                    self.vendor_id = self.VENDOR_NAME_ID[vendor_name]

        self.product_name = product_name
        self.product_id = product_id
        self.driver_version = driver_version

        # intel_gen
        if self.is_intel():
            for gen in self.INTEL_GEN_ID:
                if self.product_id in self.INTEL_GEN_ID[gen]:
                    self.intel_gen = gen
                    break
            else:
                self.intel_gen = ''
        else:
            self.intel_gen = ''

    def is_amd(self):
        return self._is_vendor_name('amd')

    def is_intel(self):
        return self._is_vendor_name('intel')

    def is_nvidia(self):
        return self._is_vendor_name('nvidia')

    def is_qualcomm(self):
        return self._is_vendor_name('qualcomm')

    def _is_vendor_name(self, vendor_name):
        return self.vendor_name == vendor_name or self.vendor_id == self.VENDOR_NAME_ID[vendor_name]

    def __str__(self):
        return json.dumps({
            'vendor_name': self.vendor_name,
            'vendor_id': self.vendor_id,
            'product_name': self.product_name,
            'product_id': self.product_id,
            'intel_gen': self.intel_gen
        })


class GPUs(object):
    def __init__(self, os, android_device, driver=None):
        self._logger = Util.get_logger()
        self.gpus = []

        vendor_name = []
        vendor_id = []
        product_name = []
        product_id = []
        driver_version = []

        if os.is_android():
            cmd = AdbShellCmd('dumpsys | grep GLES', android_device.id)
            for line in cmd.output.split('\n'):
                if re.match('GLES', line):
                    fields = line.replace('GLES:', '').strip().split(',')
                    vendor_name.append(fields[0])
                    vendor_id.append('')
                    product_name.append(fields[1])
                    product_id.append('')
                    driver_version.append('')
                    break

        elif os.is_cros():
            driver.get('chrome://gpu')
            try:
                WebDriverWait(driver, 60).until(lambda driver: driver.find_element_by_id('basic-info'))
            except TimeoutException:
                Util.error('Could not get GPU info')

            trs = driver.find_element_by_id('basic-info').find_elements_by_xpath('./div/table/tbody/tr')
            for tr in trs:
                tds = tr.find_elements_by_xpath('./td')
                key = tds[0].find_element_by_xpath('./span').text
                if key == 'GPU0':
                    value = tds[1].find_element_by_xpath('./span').text
                    match = re.search('VENDOR = 0x(\S{4}), DEVICE.*= 0x(\S{4})', value)
                    vendor_id.append(match.group(1))
                    vendor_name.append('')
                    product_id.append(match.group(2))
                if key == 'Driver version':
                    driver_version.append(tds[1].find_element_by_xpath('./span').text)
                if key == 'GL_RENDERER':
                    product_name.append(tds[1].find_element_by_xpath('./span').text)
                    break

        elif os.is_linux():
            cmd = Cmd('lshw -numeric -c display')
            lines = cmd.output.split('\n')
            for line in lines:
                line = line.strip()
                match = re.search('product: (.*) \[(.*)\]$', line)
                if match:
                    product_name.append(match.group(1))
                    product_id.append(match.group(2).split(':')[1].upper())
                match = re.search('vendor: (.*) \[(.*)\]$', line)
                if match:
                    vendor_name.append(match.group(1))
                    vendor_id.append(match.group(2).upper())
                    driver_version.append('')
                    break

        elif os.is_mac():
            cmd = Cmd('system_profiler SPDisplaysDataType')
            lines = cmd.output.split('\n')
            for line in lines:
                line = line.strip()
                match = re.match('Chipset Model: (.*)', line)
                if match:
                    product_name.append(match.group(1))
                match = re.match('Vendor: (.*) \(0x(.*)\)', line)
                if match:
                    vendor_name.append(match.group(1))
                    vendor_id.append(match.group(2))
                match = re.match('Device ID: 0x(.*)', line)
                if match:
                    product_id.append(match.group(1))
                    driver_version.append('')

        elif os.is_win():
            cmd = Cmd('wmic path win32_videocontroller get /format:list')
            lines = cmd.output.split('\n')
            for line in lines:
                line = line.rstrip('\r')
                match = re.match('AdapterCompatibility=(.*)', line)
                if match:
                    vendor_name.append(match.group(1))
                match = re.match('DriverVersion=(.*)', line)
                if match:
                    driver_version.append(match.group(1))
                match = re.match('Name=(.*)', line)
                if match:
                    product_name.append(match.group(1))
                match = re.match('PNPDeviceID=.*VEN_(\S{4})&.*DEV_(\S{4})&', line)
                if match:
                    vendor_id.append(match.group(1))
                    product_id.append(match.group(2))

        for index in range(len(vendor_name)):
            self.gpus.append(GPU(vendor_name[index], vendor_id[index], product_name[index], product_id[index], driver_version[index]))

        if len(self.gpus) < 1:
            Util.error('Could not find any GPU')

    def get_active(self, driver):
        if not driver or len(self.gpus) == 1:
            return self.gpus[0]
        else:
            try:
                debug_info = driver.execute_script('''
                    var canvas = document.createElement("canvas");
                    var gl = canvas.getContext("webgl");
                    var ext = gl.getExtension("WEBGL_debug_renderer_info");
                    return gl.getParameter(ext.UNMASKED_VENDOR_WEBGL) + " " + gl.getParameter(ext.UNMASKED_RENDERER_WEBGL);
                ''')
            except WebDriverException:
                self._logger.warning('WEBGL_debug_renderer_info is not supported, so we assume first GPU from %s will be used' % self.gpus[0].vendor_name)
            else:
                for gpu in self.gpus:
                    if re.search(gpu.vendor_name, debug_info, re.I) or re.search(gpu.product_name, debug_info, re.I):
                        return gpu
                else:
                    self._logger.warning('Could not find the active GPU, so we assume first GPU from %s will be used' % self.gpus[0].vendor_name)


class OS(object):
    def __init__(self, name, version=''):
        self.name = name
        self.version = version

    def is_android(self):
        return self._is_name('android')

    def is_cros(self):
        return self._is_name('cros')

    def is_linux(self):
        return self._is_name('linux')

    def is_mac(self):
        return self._is_name('mac')

    def is_win(self):
        return self._is_name('win')

    def _is_name(self, name):
        return self.name == name

    def __str__(self):
        return json.dumps({
            'name': self.name,
            'version': self.version,
        })


class HostOS(OS):
    def __init__(self):
        # name
        system = platform.system().lower()
        if system == 'linux':
            cmd = Cmd('cat /etc/lsb-release')
            if re.search('CHROMEOS', cmd.output, re.I):
                self.name = 'cros'
            else:
                self.name = 'linux'
        elif system == 'darwin':
            self.name = 'mac'
        elif system == 'windows':
            self.name = 'win'

        # version
        if self.is_cros():
            version = platform.platform()
        elif self.is_linux():
            version = platform.dist()[1]
        elif self.is_mac():
            version = platform.mac_ver()[0]
        elif self.is_win():
            version = platform.version()

        super(HostOS, self).__init__(self.name, version)

        # host_os specific variables
        if self.is_win():
            self.appdata = Util.use_slash(Util.get_env('APPDATA'))
            self.programfiles = Util.use_slash(Util.get_env('PROGRAMFILES'))
            self.programfilesx86 = Util.use_slash(Util.get_env('PROGRAMFILES(X86)'))
            self.windir = Util.use_slash(Util.get_env('WINDIR'))
            self.username = os.getenv('USERNAME')
        else:
            self.username = os.getenv('USER')

    def __str__(self):
        str_dict = json.loads(super(HostOS, self).__str__())
        if self.is_win():
            str_dict['username'] = self.username
        return json.dumps(str_dict)


class AndroidOS(OS):
    def __init__(self, device):
        version = device.get_prop('ro.build.version.release')
        super(AndroidOS, self).__init__('android', version)


class Browser(object):
    def __init__(self, name, path, options, os):
        self.name = name
        self.os = os
        self.version = ''
        self._logger = Util.get_logger()

        # path
        if path:
            self.path = Util.use_slash(path)
        elif self.os.is_android():
            if self.name == 'chrome_stable' or self.name == 'chrome':
                self.path = '/data/app/com.android.chrome-1'
        elif self.os.is_cros():
            self.path = '/opt/google/chrome/chrome'
        elif self.os.is_linux():
            if self.name == 'chrome':
                self.path = '/opt/google/chrome/google-chrome'
        elif self.os.is_mac():
            if self.name == 'chrome' or self.name == 'chrome_stable':
                self.path = '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'
            if self.name == 'chrome_canary':
                self.path = '/Applications/Google Chrome Canary.app/Contents/MacOS/Google Chrome Canary'
        elif self.os.is_win():
            if self.name == 'chrome' or self.name == 'chrome_stable':
                self.path = '%s/Google/Chrome/Application/chrome.exe' % self.os.programfilesx86
            if self.name == 'chrome_canary':
                self.path = '%s/../Local/Google/Chrome SxS/Application/chrome.exe' % self.os.appdata
            elif self.name == 'firefox' or self.name == 'firefox_stable':
                self.path = '%s/Mozilla Firefox/firefox.exe' % self.os.programfilesx86
            elif self.name == 'firefox_nightly':
                self.path = '%s/Nightly/firefox.exe' % self.os.programfiles
            elif self.name == 'edge':
                self.path = '%s/systemapps/Microsoft.MicrosoftEdge_8wekyb3d8bbwe/MicrosoftEdge.exe' % self.os.windir
        else:
            Util.not_implemented()

        # option
        self.options = options
        if self.is_chrome():
            if not self.os.is_android() and not self.os.is_cros():
                self.options.append('--disk-cache-size=1')
                if self.os.is_linux():
                    self.options.append('--disk-cache-dir=/dev/null')

            # fullscreen to ensure webdriver can test correctly
            if os.is_linux() or os.is_win():
                self.options.append('--start-maximized')
            # --start-maximized doesn't work on mac
            elif os.is_mac():
                self.options.append('--start-fullscreen')

    def update(self, driver):
        # version
        if not self.os.is_win():
            ua = driver.execute_script('return navigator.userAgent;')
            if self.is_chrome():
                match = re.search('Chrome/(.*) ', ua)
            elif self.is_edge():
                match = re.search('Edge/(.*)$', ua)
            elif self.is_firefox():
                match = re.search('rv:(.*)\)', ua)
            if match:
                self.version = match.group(1)

    def is_chrome(self):
        return self._is_browser('chrome')

    def is_edge(self):
        return self._is_browser('edge')

    def is_firefox(self):
        return self._is_browser('firefox')

    def is_safari(self):
        return self._is_browser('safari')

    def _is_browser(self, name):
        return re.search(name, self.name, re.I)

    def __str__(self):
        return json.dumps({
            'name': self.name,
            'path': self.path,
            'options': ','.join(self.options),
        })


class Webdriver(object):
    CHROME_WEBDRIVER_NAME = 'chromedriver'
    EDGE_WEBDRIVER_NAME = 'MicrosoftWebDriver'
    FIREFOX_WEBDRIVER_NAME = 'geckodriver'

    ANDROID_CHROME_NAME_PKG = {
        'chrome': 'com.android.chrome',
        'chrome_stable': 'com.android.chrome',
        'chrome_beta': 'com.chrome.beta',
        'chrome_public': 'org.chromium.chrome',
    }

    def __init__(self, path, browser, host_os, target_os, android_device=None, debug=False):
        self._logger = Util.get_logger()
        self.path = path
        self.target_os = target_os

        # path
        if target_os.is_cros():
            self.path = '/usr/local/chromedriver/chromedriver'

        executable_suffix = Util.get_executable_suffix(host_os)
        if not self.path and browser.is_chrome() and host_os == target_os:
            if host_os.is_mac():
                browser_dir = browser.path.replace('/Chromium.app/Contents/MacOS/Chromium', '')
            else:
                browser_dir = os.path.dirname(os.path.realpath(browser.path))
            tmp_path = Util.use_slash(browser_dir + '/chromedriver')
            tmp_path += executable_suffix
            if os.path.exists(tmp_path):
                self.path = tmp_path

        if not self.path:
            tmp_path = 'webdriver/%s/' % host_os.name
            if browser.is_chrome():
                tmp_path += self.CHROME_WEBDRIVER_NAME
            elif browser.is_edge():
                tmp_path += self.EDGE_WEBDRIVER_NAME
            elif browser.is_firefox():
                tmp_path += self.FIREFOX_WEBDRIVER_NAME
            tmp_path += executable_suffix
            if os.path.exists(tmp_path):
                self.path = tmp_path

        # webdriver
        if target_os.is_android() or target_os.is_cros():
            # This needs to be done before server process is created
            if target_os.is_cros():
                from telemetry.internal.browser import browser_finder, browser_options
                finder_options = browser_options.BrowserFinderOptions()
                finder_options.browser_type = ('system')
                if browser.options:
                    finder_options.browser_options.AppendExtraBrowserArgs(browser.options)
                finder_options.verbosity = 0
                finder_options.CreateParser().parse_args(args=[])
                b_options = finder_options.browser_options
                b_options.disable_component_extensions_with_background_pages = False
                b_options.create_browser_with_oobe = True
                b_options.clear_enterprise_policy = True
                b_options.dont_override_profile = False
                b_options.disable_gaia_services = True
                b_options.disable_default_apps = True
                b_options.disable_component_extensions_with_background_pages = True
                b_options.auto_login = True
                b_options.gaia_login = False
                b_options.gaia_id = b_options.gaia_id
                open('/mnt/stateful_partition/etc/collect_chrome_crashes', 'w').close()
                self._browser_to_create = browser_finder.FindBrowser(finder_options)
                self._browser_to_create.SetUpEnvironment(
                    finder_options.browser_options)
                self._browser = self._browser_to_create.Create()
                self._browser.tabs[0].Close()

            webdriver_args = [self.path]
            port = self._get_unused_port()
            webdriver_args.append('--port=%d' % port)
            self.server_process = subprocess.Popen(webdriver_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, env=None)
            capabilities = {}
            capabilities['chromeOptions'] = {}
            self.server_url = 'http://localhost:%d' % port

            if target_os.is_android():
                capabilities['chromeOptions']['androidDeviceSerial'] = android_device.id
                capabilities['chromeOptions']['androidPackage'] = self.ANDROID_CHROME_NAME_PKG[browser.name]
                capabilities['chromeOptions']['args'] = browser.options
            elif target_os.is_cros():
                remote_port = self._get_chrome_remote_debugging_port()
                urllib2.urlopen('http://localhost:%i/json/new' % remote_port)
                capabilities['chromeOptions']['debuggerAddress'] = ('localhost:%d' % remote_port)

            self.driver = webdriver.Remote(command_executor=self.server_url, desired_capabilities=capabilities)
        # other OS
        else:
            if browser.is_chrome():
                chrome_options = selenium.webdriver.ChromeOptions()
                for option in browser.options:
                    chrome_options.add_argument(option)
                    chrome_options.binary_location = browser.path
                if debug:
                    service_args = ['--verbose', '--log-path=log/chromedriver.log']
                else:
                    service_args = []
                self.driver = selenium.webdriver.Chrome(executable_path=self.path, chrome_options=chrome_options, service_args=service_args)
            elif browser.is_safari():
                Util.not_implemented()
            elif browser.is_edge():
                self.driver = selenium.webdriver.Edge(self.path)
            elif browser.is_firefox():
                from selenium.webdriver.common.desired_capabilities import DesiredCapabilities
                capabilities = DesiredCapabilities.FIREFOX
                capabilities['marionette'] = True
                capabilities['binary'] = browser.path
                self.driver = selenium.webdriver.Firefox(capabilities=capabilities, executable_path=self.path)

        # check
        if not browser.path:
            Util.error('Could not find browser at %s' % browser.path)
        else:
            self._logger.info('Use browser at %s' % browser.path)
        if not self.path:
            Util.error('Could not find webdriver at %s' % self.path)
        else:
            self._logger.info('Use webdriver at %s' % self.path)
        if not self.driver:
            Util.error('Could not get webdriver')

        atexit.register(self._quit)

    def _get_chrome_remote_debugging_port(self):
        chrome_pid = int(subprocess.check_output(['pgrep', '-o', '^chrome$']))
        command = subprocess.check_output(['ps', '-p', str(chrome_pid), '-o', 'command='])
        matches = re.search('--remote-debugging-port=([0-9]+)', command)
        if matches:
            return int(matches.group(1))

    def _get_unused_port(self):
        def try_bind(port, socket_type, socket_proto):
            s = socket.socket(socket.AF_INET, socket_type, socket_proto)
            try:
                try:
                    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                    s.bind(('', port))
                    return s.getsockname()[1]
                except socket.error:
                    return None
            finally:
                s.close()

        while True:
            port = try_bind(0, socket.SOCK_STREAM, socket.IPPROTO_TCP)
            if port and try_bind(port, socket.SOCK_DGRAM, socket.IPPROTO_UDP):
                return port

    def _quit(self):
        self.driver.quit()
        if self.target_os.is_android() or self.target_os.is_cros():
            try:
                urllib2.urlopen(self.server_url + '/shutdown', timeout=10).close()
            except Exception:
                pass
            self.server_process.stdout.close()
            self.server_process.stderr.close()

            if self.target_os.is_cros():
                self._browser.Close()
                del self._browser
                self._browser_to_create.CleanUpEnvironment()


class Status(object):
    PASS = 'PASS'
    FAIL = 'FAIL'
    CRASH = 'CRASH'
    FILTER = 'FILTER'
    PYTIMEOUT = 'PYTIMEOUT'
    JSTIMEOUT = 'JSTIMEOUT'
    NOTEXIST = 'NOTEXIST'


class Case(object):
    def __init__(self, path='', status='', total_count=0, pass_count=0, time=0):
        self.path = path
        self.status = status
        self.total_count = total_count
        self.pass_count = pass_count
        self.time = time

    def is_pass(self):
        return self._is_status(Status.PASS)

    def is_fail(self):
        return self._is_status(Status.FAIL)

    def is_crash(self):
        return self._is_status(Status.CRASH)

    def is_filter(self):
        return self._is_status(Status.FILTER)

    def is_pytimeout(self):
        return self._is_status(Status.PYTIMEOUT)

    def is_jstimeout(self):
        return self._is_status(Status.JSTIMEOUT)

    def _is_status(self, status):
        if self.status == status:
            return True
        else:
            return False

    def __str__(self):
        return json.dumps({
            'path': self.path,
            'status': self.status,
            'total_count': self.total_count,
            'pass_count': self.pass_count,
            'time': self.time
        })


class Suite(object):
    def __init__(self, exp_suite=None):
        self.suite = []
        self.path_index = {}
        self.count = 0
        self.exp_suite = exp_suite

        self.issue_path = []
        self.filter_path = []
        self.retry_index = []

    def add_case(self, case):
        self.suite.append(case)
        self.path_index[case.path] = self.count
        if not case.is_pass():
            self.issue_path.append(case.path)
        if case.is_filter():
            self.filter_path.append(case.path)
        if case.is_fail() and self.exp_suite and case.path not in self.exp_suite.issue_path:
            self.retry_index.append(self.count)
        self.count += 1

    def get_case(self, index):
        return self.suite[index]

    def remove_issue(self, index):
        path = self.suite[index].path
        del self.path_index[path]
        self.issue_path.remove(path)


class Change(object):
    def __init__(self, exp_case, cur_case):
        self.exp_case = exp_case
        self.cur_case = cur_case


class Conformance(object):
    VERSION_TYPE = {
        '1.0.0': 'stable',
        '1.0.1': 'stable',
        '1.0.2': 'stable',
        '1.0.3': 'stable',
        '2.0.0': 'stable',
        '1.0.4': 'beta',
        '2.0.1': 'beta',
    }

    TEST_DONE = 'TESTDONE'
    TOP_TIME_COUNT = 20

    def __init__(self):
        # argument
        parser = argparse.ArgumentParser(description='Khronos WebGL Conformance Test Script', formatter_class=argparse.ArgumentDefaultsHelpFormatter, epilog='''
    examples:
      python %(prog)s --browser_name chrome --version 2.0.1 --suite conformance/attribs
    ''')
        parser.add_argument('--browser-name', dest='browser_name', help='name of browser')
        parser.add_argument('--browser-options', dest='browser_options', help='extra options of browser, split by ","')
        parser.add_argument('--browser-path', dest='browser_path', help='path of browser')
        parser.add_argument('--webdriver-path', dest='webdriver_path', help='path of webdriver')
        parser.add_argument('--version', dest='version', help='WebGL conformance test version', default='2.0.1')
        parser.add_argument('--url', dest='url', help='url for website other than default Khronos WebGL CTS')
        parser.add_argument('--suite', dest='suite', help='instead of whole suite, we may test specific cases, e.g., conformance/attibs or "conformance/attribs/gl-bindAttribLocation-matrix.html"', default='all')
        parser.add_argument('--os-name', dest='os_name', help='OS to run test on')
        parser.add_argument('--android-device-id', dest='android_device_id', help='id of Android device to run test on')
        parser.add_argument('--mesa-dir', dest='mesa_dir', help='directory of Mesa')
        parser.add_argument('--gles', dest='gles', help='gles', action='store_true')
        parser.add_argument('--logging-level', dest='logging_level', help='level of logging', default=logging.INFO)
        parser.add_argument('--timeout', dest='timeout', help='timeout seconds for each test', type=int, default=60)

        debug_group = parser.add_argument_group('debug')
        debug_group.add_argument('--fixed-time', dest='fixed_time', help='fixed time', action='store_true')
        debug_group.add_argument('--dryrun-test', dest='dryrun_test', help='dryrun test', action='store_true')
        args = parser.parse_args()

        # timestamp
        if args.fixed_time:
            self.timestamp = Util.get_datetime(format='%Y%m%d')
        else:
            self.timestamp = Util.get_datetime()

        # log
        work_dir = Util.use_slash(sys.path[0])
        os.chdir(work_dir)
        self.log_dir = 'log'
        Util.ensure_dir(self.log_dir)
        self.log_file = '%s/%s.log' % (self.log_dir, self.timestamp)
        Util.ensure_nofile(self.log_file)
        Util.set_logger(self.log_file, args.logging_level)
        self._logger = Util.get_logger()
        self.resume_file = '%s/resume' % self.log_dir

        # result
        self.result_dir = 'result'
        Util.ensure_dir(self.result_dir)
        self.result_file = '%s/%s.html' % (self.result_dir, self.timestamp)

        # device
        if args.os_name == 'android':
            self.android_device = AndroidDevices().get_device(args.android_device_id)
        else:
            self.android_device = None

        # OS
        self.host_os = HostOS()
        if args.os_name == 'android':
            self.target_os = AndroidOS(self.android_device)
        else:
            self.target_os = self.host_os

        # browser
        if args.browser_name:
            browser_name = args.browser_name
        elif self.target_os.is_cros():
            browser_name = 'chrome'
        else:
            Util.error('Please designate browser name')
        if args.browser_options:
            browser_options = args.browser_options.split(',')
        else:
            browser_options = []

        if args.gles and self.target_os.is_linux() and 'chrome' in browser_name:
            browser_options.append('--use-gl=egl')

        if 'chrome' in browser_name and not self.target_os.is_android() and not self.target_os.is_cros():
            user_data_dir = 'user-data-dir-%s' % self.target_os.username
            browser_options.append('--user-data-dir=%s' % (work_dir + '/' + user_data_dir))
            Util.ensure_nodir(user_data_dir)
            Util.ensure_dir(user_data_dir)

        self.browser = Browser(name=browser_name, path=args.browser_path, options=browser_options, os=self.target_os)

        # others
        self.webdriver_path = args.webdriver_path
        self.args = args
        self.timeout = args.timeout

        # url
        self.version = args.version
        if args.url:
            self.url = args.url
        else:
            self.url = 'https://www.khronos.org/registry/webgl'
            if self.version not in self.VERSION_TYPE:
                Util.error('The version %s is not supported' % self.version)
            type = self.VERSION_TYPE[self.version]
            if type == 'stable':
                self.url += '/conformance-suites/%s/webgl-conformance-tests.html' % self.version
            elif type == 'beta':
                self.url += '/sdk/tests/webgl-conformance-tests.html?version=%s' % self.version

        # runtime env
        mesa_dir = args.mesa_dir
        if self.target_os.is_linux() and mesa_dir:
            Util.set_env('LD_LIBRARY_PATH', mesa_dir + '/lib')
            Util.set_env('LIBGL_DRIVERS_PATH', mesa_dir + '/lib/dri')

        if args.gles and self.target_os.is_linux() and self.gpu.is_intel():
            pkg = 'libgles-mesa'
            if not Util.has_pkg(pkg):
                Util.error('Package %s is not installed' % pkg)
        if args.gles and self.gpu.is_nvidia():
            Util.set_env('LD_LIBRARY_PATH', '/usr/lib/nvidia-' + self.gpu.version.split('.')[0])

        # test
        if args.dryrun_test:
            self.exp_suite = Suite()
            self.cur_suite = Suite(self.exp_suite)
            self.driver = None
            self.gpu = self.gpus.get_active(self.driver)
        else:
            self._start(is_firstrun=True)
            self._run('firstrun')
            self._run('retry')

        # report
        self._gen_report()

    # Crash in previous case may only be found in current case, so we just log
    # the previous result so that we don't need to modify a record.
    def _append_resume(self, f, index):
        if index < 1:
            return
        case = self.cur_suite.get_case(index - 1)
        f.write('%s,%s,%s,%s,%s\n' % (case.path, case.status, case.total_count, case.pass_count, case.time))

    def _crash(self, mode, index):
        if mode == 'firstrun':
            crash_case = self.cur_suite.get_case(index - 1)
        else:
            crash_case = self.cur_suite.get_case(self.cur_suite.retry_index[index - 1])
        crash_case.status = Status.CRASH
        crash_case.total_count = 1
        crash_case.pass_count = 0
        self._logger.warning('Case %s crashed' % crash_case.path)
        self._start()

    def _gen_report(self):
        # summary
        summary = []
        path_index = {}
        for case in self.cur_suite.suite:
            path = case.path.split('/')[0]
            if path not in path_index:
                path_index[path] = len(path_index)
                case = Case(path, total_count=case.total_count, pass_count=case.pass_count)
                summary.append(case)
            else:
                index = path_index[path]
                summary[index].total_count += case.total_count
                summary[index].pass_count += case.pass_count
        total_count = 0
        pass_count = 0
        for case in summary:
            total_count += case.total_count
            pass_count += case.pass_count
        case = Case('all', total_count=total_count, pass_count=pass_count)
        summary.append(case)

        # detail
        cur_diff_exp_path = Util.diff_list(self.cur_suite.issue_path, self.exp_suite.issue_path)
        exp_diff_cur_path = Util.diff_list(self.exp_suite.issue_path, self.cur_suite.issue_path)
        cur_exp_common_path = Util.intersect_list(self.cur_suite.issue_path, self.exp_suite.issue_path)
        improve_pass_detail = []  # passrate == 100%
        improve_fail_detail = []  # passrate < 100%
        regress_detail = []
        remain_detail = []

        for path in cur_diff_exp_path:
            cur_case = self.cur_suite.get_case(self.cur_suite.path_index[path])
            exp_case = Case(path, Status.PASS, cur_case.total_count, cur_case.total_count)
            regress_detail.append(Change(exp_case, cur_case))
        for path in exp_diff_cur_path:
            exp_case = self.exp_suite.get_case(self.exp_suite.path_index[path])
            if path in self.cur_suite.path_index:
                cur_case = self.cur_suite.get_case(self.cur_suite.path_index[path])
                category = improve_pass_detail
            else:
                if exp_case.status == Status.FILTER:
                    status = exp_case.status
                    category = remain_detail
                else:  # case was removed
                    status = Status.NOTEXIST
                    category = improve_pass_detail
                cur_case = Case(exp_case.path, status)
            category.append(Change(exp_case, cur_case))
        for path in cur_exp_common_path:
            exp_case = self.exp_suite.get_case(self.exp_suite.path_index[path])
            cur_case = self.cur_suite.get_case(self.cur_suite.path_index[path])
            exp_passrate = self._get_passrate(exp_case.total_count, exp_case.pass_count)
            cur_passrate = self._get_passrate(cur_case.total_count, cur_case.pass_count)
            if cur_passrate < exp_passrate:
                category = regress_detail
            elif cur_passrate > exp_passrate:
                if cur_passrate == 100:
                    category = improve_pass_detail
                else:
                    category = improve_fail_detail
            else:
                category = remain_detail
            category.append(Change(exp_case, cur_case))

        improve_pass_detail = sorted(improve_pass_detail, cmp=lambda x, y: cmp(x.exp_case.path, y.exp_case.path))
        improve_fail_detail = sorted(improve_fail_detail, cmp=lambda x, y: cmp(x.exp_case.path, y.exp_case.path))
        regress_detail = sorted(regress_detail, cmp=lambda x, y: cmp(x.exp_case.path, y.exp_case.path))
        remain_detail = sorted(remain_detail, cmp=lambda x, y: cmp(x.exp_case.path, y.exp_case.path))

        # top_time
        top_time = []
        all_time = sorted(self.cur_suite.suite, cmp=lambda x, y: cmp(x.time, y.time), reverse=True)
        count = 0
        for case in all_time:
            if not case.path:
                break
            if count == self.TOP_TIME_COUNT:
                break
            top_time.append([case.path, case.time])
            count += 1

        # generate html
        content = '''
<html>
  <head>
    <meta http-equiv="content-type" content="text/html; charset=windows-1252">
    <style type="text/css">
      table {
        border: 2px solid black;
        border-collapse: collapse;
        border-spacing: 0;
      }
      table tr td {
        border: 1px solid black;
      }
    </style>
  </head>
  <body>
        '''

        # environment
        content += '''
    <h2>Environment</h2>
    <table>
      <tbody>
        '''
        for env in ['gpu', 'host_os', 'target_os', 'browser']:
            if env == 'target_os' and self.host_os == self.target_os:
                continue
            content += '''
        <tr bgcolor="#FFFF93"><td align="left" colspan="2"><strong>''' + env.upper() + '''</strong></td></tr>
            '''
            env_dict = json.loads(str(eval('self.' + env)))
            for key in env_dict:
                content += '''
        <tr>
          <td align="left"><strong>''' + str(key) + '''</strong></td>
          <td align="left">''' + str(env_dict[key]) + '''</td>
        </tr>
            '''

        content += '''
      </tbody>
    </table>
    '''

        # summary
        content += '''
    <h2>Summary</h2>
    <table>
      <tbody>
        <tr>
          <td align="left"><strong>Test Case Category </strong></td>
          <td align="left"><strong>All</strong> </td>
          <td align="left"><strong>Pass </strong> </td>
          <td align="left"><strong>Pass Rate %</strong> </td>
        </tr>
    '''
        for case in summary:
            content += '''
        <tr>
          <td align="left"> ''' + case.path + ''' </td>
          <td align="left"> ''' + str(case.total_count) + ''' </td>
          <td align="left"> ''' + str(case.pass_count) + ''' </td>
          <td align="left"> ''' + str(self._get_passrate(case.total_count, case.pass_count)) + ''' </td>
        </tr>
        '''

        content += '''
      </tbody>
    </table>
    '''

        # detail
        content += '''
    <h2>Details</h2>
    <table>
      <tbody>
        <tr>
          <td align="left"><strong>Case</strong></td>
          <td align="left"><strong>Expectation Status</strong></td>
          <td align="left"><strong>Expectation All</strong></td>
          <td align="left"><strong>Expectation Pass</strong></td>
          <td align="left"><strong>Expectation Pass Rate</strong></td>
          <td align="left"><strong>Current Status</strong></td>
          <td align="left"><strong>Current All</strong></td>
          <td align="left"><strong>Current Pass</strong></td>
          <td align="left"><strong>Current Pass Rate</strong></td>
          <td align="left"><strong>Change</strong></td>
        </tr>
    '''

        for detail in ['improve_pass_detail', 'improve_fail_detail', 'regress_detail', 'remain_detail']:
            if detail == 'improve_pass_detail':
                bgcolor = '00FF00'
            elif detail == 'improve_fail_detail':
                bgcolor = 'A6FFA6'
            elif detail == 'regress_detail':
                bgcolor = 'FF9797'
            elif detail == 'remain_detail':
                bgcolor = 'FFFF93'
            for change in eval(detail):
                content += '''
        <tr bgcolor=#''' + bgcolor + '''>
          <td align="left"> ''' + str(change.exp_case.path) + '''</td>
          <td align="left"> ''' + str(change.exp_case.status) + '''</td>
          <td align="left"> ''' + str(change.exp_case.total_count) + '''</td>
          <td align="left"> ''' + str(change.exp_case.pass_count) + '''</td>
          <td align="left"> ''' + str(self._get_passrate(change.exp_case.total_count, change.exp_case.pass_count)) + '''</td>
          <td align="left"> ''' + str(change.cur_case.status) + '''</td>
          <td align="left"> ''' + str(change.cur_case.total_count) + '''</td>
          <td align="left"> ''' + str(change.cur_case.pass_count) + '''</td>
          <td align="left"> ''' + str(self._get_passrate(change.cur_case.total_count, change.cur_case.pass_count)) + '''</td>
          <td align="left"> ''' + detail.replace('_detail', '') + '''</td>
        </tr>
            '''

        content += '''
      </tbody>
    </table>
    '''

        # retry
        content += '''
    <h2>Retry Cases</h2>
    <table>
      <tbody>
        <tr>
          <td align="left"> <strong>Case</strong>  </td>
        </tr>
    '''
        for index in self.cur_suite.retry_index:
            content += '''
        <tr>
          <td align="left"> ''' + self.cur_suite.get_case(index).path + ''' </td>
        </tr>
        '''
        content += '''
      </tbody>
    </table>
'''

        # top time consuming
        if self.version != '1.0.3':
            content += '''
    <h2>Top Time Consuming Cases</h2>
    <table>
      <tbody>
        <tr>
          <td align="left"><strong>Case</strong>  </td>
          <td align="left"><strong>Time (ms)</strong> </td>
        </tr>
    '''
            for case in top_time:
                content += '''
        <tr>
          <td align="left"> ''' + case[0] + ''' </td>
          <td align="left"> ''' + str(case[1]) + ''' </td>
        </tr>
        '''
            content += '''
      </tbody>
    </table>
'''

        # tail
        content += '''
  </body>
</html>
    '''
        f = open(self.result_file, 'w')
        f.write(content)
        f.close()

    def _get_case_elements(self):
        if re.match('all', self.args.suite):
            suite = self.args.suite
        else:
            suite = 'all/' + self.args.suite

        if re.search('.html$', suite):
            folder_name = os.path.dirname(suite)
            case_name = suite.split('/')[-1]
        else:
            folder_name = suite
            case_name = ''

        folder_name_elements = self.driver.find_elements_by_class_name('folderName')
        for folder_name_element in folder_name_elements:
            if folder_name_element.text == folder_name:
                tmp_case_elements = folder_name_element.find_elements_by_xpath('../..//*[@class="testpage"]')
                if not case_name:
                    case_elements = tmp_case_elements
                    break
                for case_element in tmp_case_elements:
                    if '%s/%s' % ('all', case_element.find_element_by_xpath('./div/a').text) == suite:
                        case_elements = [case_element]
                        break
                if case_elements:
                    break
        else:
            Util.error('Could not find suite %s' % suite)

        self.case_elements = case_elements

    def _get_passrate(self, total, passed):
        if float(total) == 0:
            return 0
        return float('%.2f' % (float(passed) / float(total) * 100))

    def _get_result(self, text):
        # passed includes both results of passed and skipped
        if self.version == '1.0.3':
            p = '(\d+) of (\d+) (.+)'
            match = re.search(p, text)
            if match:
                total = int(match.group(2))
                passed = int(match.group(1))
            else:
                total = 1
                passed = 0
            skipped = 0
            time = 0
            if total == passed + skipped:
                status = Status.PASS
            else:
                status = Status.FAIL
        else:
            p = '(.*) in (.+) ms'
            match = re.search(p, text)
            if match:
                time = float(match.group(2))
                text_detail = match.group(1)
                total = 0
                match_detail = re.search('Passed: (\d+)/(\d+)', text_detail)
                if match_detail:
                    passed = int(match_detail.group(1))
                    total_tmp = int(match_detail.group(2))
                    if not total:
                        total = total_tmp
                    if total != total_tmp:
                        Util.error('Total is not consistent')
                else:
                    passed = 0
                match_detail = re.search('Skipped: (\d+)/(\d+)', text_detail)
                if match_detail:
                    skipped = int(match_detail.group(1))
                    total_tmp = int(match_detail.group(2))
                    if not total:
                        total = total_tmp
                    if total != total_tmp:
                        Util.error('Total is not consistent')
                else:
                    skipped = 0
                match_detail = re.search('Failed: (\d+)/(\d+)', text_detail)
                if match_detail:
                    failed = int(match_detail.group(1))
                    total_tmp = int(match_detail.group(2))
                    if not total:
                        total = total_tmp
                    if total != total_tmp:
                        Util.error('Total is not consistent')
                else:
                    failed = 0

                if passed + skipped + failed != total:
                    Util.error('Total is not the sum of passed, skipped and failed')

                if total == passed + skipped:
                    status = Status.PASS
                else:
                    status = Status.FAIL
            else:
                total = 1
                status = Status.JSTIMEOUT
                passed = 0
                skipped = 0
                time = 0

        return (status, total, passed + skipped, time)

    def _log_resume(self, index, total_count, msg, case_path):
        self._logger.info('(%s/%s) %s %s' % (index + 1, total_count, msg, case_path))

    def _run(self, mode):
        if mode == 'firstrun':
            total_count = len(self.case_elements)
        elif mode == 'retry':
            total_count = len(self.cur_suite.retry_index)
        else:
            Util.error('Mode %s is not supported' % mode)

        if total_count < 1 and mode == 'firstrun':
            Util.error('No case will be tested')
        elif total_count < 1 and mode == 'retry':
            f = open(self.resume_file, 'a')
            f.write(self.TEST_DONE + '\n')
            f.close()
            self._logger.info('No need the %s' % mode)
            return
        else:
            self._logger.info('Begin the %s...' % mode)

        if mode == 'firstrun':
            # resume_count
            if os.path.exists(self.resume_file):
                resume_lines = Util.read_file(self.resume_file)
                resume_count = len(resume_lines)
                if resume_count > 0 and resume_lines[-1].rstrip('\n') == self.TEST_DONE:
                    resume_count = 0
            else:
                resume_count = 0

            # resume file
            if resume_count == 0:
                Util.ensure_nofile(self.resume_file)
                Util.ensure_file(self.resume_file)
            f = open(self.resume_file, 'a')

            # resume
            if resume_count > 0:
                if resume_count > len(self.case_elements) or resume_lines[-1].split(',')[0] != self.case_elements[resume_count - 1].find_element_by_xpath('./div/a').text:
                    Util.error('The suite currently tested is different from the resumed one')

                for resume_line in resume_lines:
                    fields = resume_line.split(',')
                    case = Case(fields[0], fields[1], int(fields[2]), int(fields[3]), float(fields[4]))
                    self.cur_suite.add_case(case)
                self._logger.info('Resume %s cases' % resume_count)

            index = resume_count
        else:
            index = 0
        while index < total_count:
            if mode == 'firstrun':
                case_index = index
                case_element = self.case_elements[case_index]
            else:
                case_index = self.cur_suite.retry_index[index]
                case_element = self.case_elements[case_index]
                case = self.cur_suite.get_case(case_index)
            case_path = case_element.find_element_by_xpath('./div/a').text

            # filter
            if mode == 'firstrun' and case_path in self.exp_suite.filter_path:
                case = Case(case_path, Status.FILTER)
                self.cur_suite.add_case(case)
                self._log_resume(index, total_count, 'Filter', case_path)
                if mode == 'firstrun':
                    self._append_resume(f, index)
                index += 1
                continue

            # run test
            self._log_resume(index, total_count, 'Run', case_path)
            try:
                button = case_element.find_element_by_xpath('./div/input[@type="button"]')
                button.click()
            except NoSuchElementException:
                self._crash(mode, index)
                continue

            # handle result
            try:
                WebDriverWait(self.driver, self.timeout).until(lambda driver: re.search('(passed|skipped|failed|timeout)', case_element.find_element_by_xpath('./div').text, re.I))
            except TimeoutException:
                if mode == 'firstrun':
                    case = Case(case_path, Status.PYTIMEOUT)
                    self.cur_suite.add_case(case)
                self._logger.warning('Case %s timeout in python script' % case_path)
                self._start()
                index += 1
            else:
                (case_status, case_total_count, case_pass_count, case_time) = self._get_result(case_element.find_element_by_xpath('./div').text)
                if mode == 'firstrun':
                    case = Case(case_path, case_status, case_total_count, case_pass_count, case_time)
                    self.cur_suite.add_case(case)
                else:
                    case.status = case_status
                    case.total_count = case_total_count
                    case.pass_count = case_pass_count
                    case.time = case_time
                self._logger.info(case.status)

                if case.is_pass():
                    if mode == 'retry':
                        self.cur_suite.remove_issue(case_index)
                elif case_element.find_element_by_xpath('./ul').find_elements_by_tag_name('li') and re.search('Unable to fetch WebGL rendering context for Canvas', case_element.find_element_by_xpath('./ul/li').text):
                    self._crash(mode, index)
                    continue

                if mode == 'firstrun':
                    self._append_resume(f, index)
                index += 1

        if mode == 'firstrun':
            self._append_resume(f, index)
        if mode == 'retry':
            f = open(self.resume_file, 'a')
            f.write(self.TEST_DONE + '\n')
        f.close()

    def _start(self, is_firstrun=False):
        self.webdriver = Webdriver(browser=self.browser, path=self.webdriver_path, host_os=self.host_os, target_os=self.target_os, android_device=self.android_device)
        self.driver = self.webdriver.driver

        if is_firstrun:
            self.browser.update(self.driver)
            self.gpus = GPUs(self.target_os, self.android_device, self.driver)
            self.gpu = self.gpus.get_active(self.driver)
            self.exp_suite = Suite()
            for exp in Expectations().expectations:
                if exp.is_valid(self.gpu, self.target_os, self.browser) and (self.args.suite == 'all' or re.match(self.args.suite, exp.path)):
                    self.exp_suite.add_case(Case(exp.path, exp.status, exp.total_count, exp.pass_count))
            self.cur_suite = Suite(self.exp_suite)

        self.driver.get(self.url)
        try:
            WebDriverWait(self.driver, 60).until(lambda driver: self.driver.find_element_by_id('page0'))
        except TimeoutException:
            Util.error('Could not open %s correctly' % self.url)

        if is_firstrun:
            option_element = Select(self.driver.find_element_by_id("testVersion")).first_selected_option
            real_version = option_element.text
            type = self.VERSION_TYPE[self.version]
            if type == 'beta':
                real_version = real_version.replace(' (beta)', '')
            if self.version != real_version:
                Util.error('The designated version does not match the real version')

        self._get_case_elements()


class Expectation(object):
    def __init__(self, version, path, status, total_count=0, pass_count=0, gpu=None, os=None, browser=None):
        self.version = version
        self.path = path
        self.status = status
        self.total_count = total_count
        self.pass_count = pass_count
        self.gpu = gpu
        self.os = os
        self.browser = browser

    def is_valid(self, gpu, os, browser):
        return True


class Expectations(object):
    def __init__(self):
        self.expectations = []

        # win_os = OS('windows')
        # self._add_exp('2.0.1', 'deqp/functional/gles3/builtinprecision/atan2.html', Status.FAIL, 25, 17, os=win_os)

    def _add_exp(self, version, path, status, total_count=0, pass_count=0, gpu=None, os=None, browser=None):
        self.expectations.append(Expectation(version, path, status, total_count, pass_count, gpu, os, browser))


if __name__ == '__main__':
    conformance = Conformance()
