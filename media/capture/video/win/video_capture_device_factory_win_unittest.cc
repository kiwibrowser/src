// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mfidl.h>

#include <ks.h>
#include <mfapi.h>
#include <mferror.h>
#include <stddef.h>

#include "base/strings/sys_string_conversions.h"
#include "media/capture/video/win/video_capture_device_factory_win.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Mock;

namespace media {

namespace {

// MediaFoundation devices
const wchar_t* kMFDeviceId0 = L"\\\\?\\usb#vid_0000&pid_0000&mi_00";
const wchar_t* kMFDeviceName0 = L"Device 0";

const wchar_t* kMFDeviceId1 = L"\\\\?\\usb#vid_0001&pid_0001&mi_00";
const wchar_t* kMFDeviceName1 = L"Device 1";

const wchar_t* kMFDeviceId2 = L"\\\\?\\usb#vid_0002&pid_0002&mi_00";
const wchar_t* kMFDeviceName2 = L"Device 2";

// DirectShow devices
const wchar_t* kDirectShowDeviceId0 = L"\\\\?\\usb#vid_0000&pid_0000&mi_00";
const wchar_t* kDirectShowDeviceName0 = L"Device 0";

const wchar_t* kDirectShowDeviceId1 = L"\\\\?\\usb#vid_0001&pid_0001&mi_00#1";
const wchar_t* kDirectShowDeviceName1 = L"Device 1";

const wchar_t* kDirectShowDeviceId3 = L"Virtual Camera 3";
const wchar_t* kDirectShowDeviceName3 = L"Virtual Camera";

const wchar_t* kDirectShowDeviceId4 = L"Virtual Camera 4";
const wchar_t* kDirectShowDeviceName4 = L"Virtual Camera";

using iterator = VideoCaptureDeviceDescriptors::const_iterator;
iterator FindDescriptorInRange(iterator begin,
                               iterator end,
                               const std::string& device_id) {
  return std::find_if(
      begin, end, [device_id](const VideoCaptureDeviceDescriptor& descriptor) {
        return device_id == descriptor.device_id;
      });
}

class MockMFActivate : public base::RefCountedThreadSafe<MockMFActivate>,
                       public IMFActivate {
 public:
  MockMFActivate(const std::wstring& symbolic_link,
                 const std::wstring& name,
                 bool kscategory_video_camera,
                 bool kscategory_sensor_camera)
      : symbolic_link_(symbolic_link),
        name_(name),
        kscategory_video_camera_(kscategory_video_camera),
        kscategory_sensor_camera_(kscategory_sensor_camera) {}

  bool MatchesQuery(IMFAttributes* query, HRESULT* status) {
    UINT32 count;
    *status = query->GetCount(&count);
    if (FAILED(*status))
      return false;
    GUID value;
    *status = query->GetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &value);
    if (FAILED(*status))
      return false;
    if (value != MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)
      return false;
    *status = query->GetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_CATEGORY,
                             &value);
    if (SUCCEEDED(*status)) {
      if ((value == KSCATEGORY_SENSOR_CAMERA && kscategory_sensor_camera_) ||
          (value == KSCATEGORY_VIDEO_CAMERA && kscategory_video_camera_))
        return true;
    } else if (*status == MF_E_ATTRIBUTENOTFOUND) {
      // When no category attribute is specified, it should behave the same as
      // if KSCATEGORY_VIDEO_CAMERA is specified.
      *status = S_OK;
      if (kscategory_video_camera_)
        return true;
    }
    return false;
  }

  STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override {
    return E_NOTIMPL;
  }
  STDMETHOD_(ULONG, AddRef)() override {
    base::RefCountedThreadSafe<MockMFActivate>::AddRef();
    return 1U;
  }

  STDMETHOD_(ULONG, Release)() override {
    base::RefCountedThreadSafe<MockMFActivate>::Release();
    return 1U;
  }
  STDMETHOD(GetItem)(REFGUID key, PROPVARIANT* value) override {
    return E_FAIL;
  }
  STDMETHOD(GetItemType)(REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType) override {
    return E_NOTIMPL;
  }
  STDMETHOD(CompareItem)
  (REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult) override {
    return E_NOTIMPL;
  }
  STDMETHOD(Compare)
  (IMFAttributes* pTheirs,
   MF_ATTRIBUTES_MATCH_TYPE MatchType,
   BOOL* pbResult) override {
    return E_NOTIMPL;
  }
  STDMETHOD(GetUINT32)(REFGUID key, UINT32* value) override {
    if (key == MF_MT_INTERLACE_MODE) {
      *value = MFVideoInterlace_Progressive;
      return S_OK;
    }
    return E_NOTIMPL;
  }
  STDMETHOD(GetUINT64)(REFGUID key, UINT64* value) override { return E_FAIL; }
  STDMETHOD(GetDouble)(REFGUID guidKey, double* pfValue) override {
    return E_NOTIMPL;
  }
  STDMETHOD(GetGUID)(REFGUID key, GUID* value) override { return E_FAIL; }
  STDMETHOD(GetStringLength)(REFGUID guidKey, UINT32* pcchLength) override {
    return E_NOTIMPL;
  }
  STDMETHOD(GetString)
  (REFGUID guidKey,
   LPWSTR pwszValue,
   UINT32 cchBufSize,
   UINT32* pcchLength) override {
    return E_NOTIMPL;
  }
  STDMETHOD(GetAllocatedString)
  (REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength) override {
    std::wstring value;
    if (guidKey == MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK) {
      value = symbolic_link_;
    } else if (guidKey == MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME) {
      value = name_;
    } else {
      return E_NOTIMPL;
    }
    *ppwszValue = static_cast<wchar_t*>(
        CoTaskMemAlloc((value.size() + 1) * sizeof(wchar_t)));
    wcscpy(*ppwszValue, value.c_str());
    *pcchLength = value.length();
    return S_OK;
  }
  STDMETHOD(GetBlobSize)(REFGUID guidKey, UINT32* pcbBlobSize) override {
    return E_NOTIMPL;
  }
  STDMETHOD(GetBlob)
  (REFGUID guidKey,
   UINT8* pBuf,
   UINT32 cbBufSize,
   UINT32* pcbBlobSize) override {
    return E_NOTIMPL;
  }
  STDMETHOD(GetAllocatedBlob)
  (REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize) override {
    return E_NOTIMPL;
  }
  STDMETHOD(GetUnknown)(REFGUID guidKey, REFIID riid, LPVOID* ppv) override {
    return E_NOTIMPL;
  }
  STDMETHOD(SetItem)(REFGUID guidKey, REFPROPVARIANT Value) override {
    return E_NOTIMPL;
  }
  STDMETHOD(DeleteItem)(REFGUID guidKey) override { return E_NOTIMPL; }
  STDMETHOD(DeleteAllItems)(void) override { return E_NOTIMPL; }
  STDMETHOD(SetUINT32)(REFGUID guidKey, UINT32 unValue) override {
    return E_NOTIMPL;
  }
  STDMETHOD(SetUINT64)(REFGUID guidKey, UINT64 unValue) override {
    return E_NOTIMPL;
  }
  STDMETHOD(SetDouble)(REFGUID guidKey, double fValue) override {
    return E_NOTIMPL;
  }
  STDMETHOD(SetGUID)(REFGUID guidKey, REFGUID guidValue) override {
    return E_NOTIMPL;
  }
  STDMETHOD(SetString)(REFGUID guidKey, LPCWSTR wszValue) override {
    return E_NOTIMPL;
  }
  STDMETHOD(SetBlob)
  (REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize) override {
    return E_NOTIMPL;
  }
  STDMETHOD(SetUnknown)(REFGUID guidKey, IUnknown* pUnknown) override {
    return E_NOTIMPL;
  }
  STDMETHOD(LockStore)(void) override { return E_NOTIMPL; }
  STDMETHOD(UnlockStore)(void) override { return E_NOTIMPL; }
  STDMETHOD(GetCount)(UINT32* pcItems) override { return E_NOTIMPL; }
  STDMETHOD(GetItemByIndex)
  (UINT32 unIndex, GUID* pguidKey, PROPVARIANT* pValue) override {
    return E_NOTIMPL;
  }
  STDMETHOD(CopyAllItems)(IMFAttributes* pDest) override { return E_NOTIMPL; }
  STDMETHOD(ActivateObject)(REFIID riid, void** ppv) override {
    return E_NOTIMPL;
  }
  STDMETHOD(DetachObject)(void) override { return E_NOTIMPL; }
  STDMETHOD(ShutdownObject)(void) override { return E_NOTIMPL; }

 private:
  friend class base::RefCountedThreadSafe<MockMFActivate>;
  virtual ~MockMFActivate() = default;

  const std::wstring symbolic_link_;
  const std::wstring name_;
  const bool kscategory_video_camera_;
  const bool kscategory_sensor_camera_;
};

class StubPropertyBag : public base::RefCountedThreadSafe<StubPropertyBag>,
                        public IPropertyBag {
 public:
  StubPropertyBag(const wchar_t* device_path, const wchar_t* description)
      : device_path_(device_path), description_(description) {}

  STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override {
    return E_NOTIMPL;
  }
  STDMETHOD_(ULONG, AddRef)(void) override {
    base::RefCountedThreadSafe<StubPropertyBag>::AddRef();
    return 1U;
  }
  STDMETHOD_(ULONG, Release)(void) override {
    base::RefCountedThreadSafe<StubPropertyBag>::Release();
    return 1U;
  }
  STDMETHOD(Read)
  (LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog) override {
    if (pszPropName == std::wstring(L"Description")) {
      pVar->vt = VT_BSTR;
      pVar->bstrVal = SysAllocString(description_);
      return S_OK;
    }
    if (pszPropName == std::wstring(L"DevicePath")) {
      pVar->vt = VT_BSTR;
      pVar->bstrVal = SysAllocString(device_path_);
      return S_OK;
    }
    return E_NOTIMPL;
  }
  STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT* pVar) override {
    return E_NOTIMPL;
  }

 private:
  friend class base::RefCountedThreadSafe<StubPropertyBag>;
  virtual ~StubPropertyBag() = default;

  const wchar_t* device_path_;
  const wchar_t* description_;
};

class StubMoniker : public base::RefCountedThreadSafe<StubMoniker>,
                    public IMoniker {
 public:
  StubMoniker(const wchar_t* device_path, const wchar_t* description)
      : device_path_(device_path), description_(description) {}

  STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override {
    return E_NOTIMPL;
  }
  STDMETHOD_(ULONG, AddRef)(void) override {
    base::RefCountedThreadSafe<StubMoniker>::AddRef();
    return 1U;
  }
  STDMETHOD_(ULONG, Release)(void) override {
    base::RefCountedThreadSafe<StubMoniker>::Release();
    return 1U;
  }
  STDMETHOD(GetClassID)(CLSID* pClassID) override { return E_NOTIMPL; }
  STDMETHOD(IsDirty)(void) override { return E_NOTIMPL; }
  STDMETHOD(Load)(IStream* pStm) override { return E_NOTIMPL; }
  STDMETHOD(Save)(IStream* pStm, BOOL fClearDirty) override {
    return E_NOTIMPL;
  }
  STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pcbSize) override { return E_NOTIMPL; }
  STDMETHOD(BindToObject)
  (IBindCtx* pbc,
   IMoniker* pmkToLeft,
   REFIID riidResult,
   void** ppvResult) override {
    return E_NOTIMPL;
  }
  STDMETHOD(BindToStorage)
  (IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, void** ppvObj) override {
    StubPropertyBag* propertyBag =
        new StubPropertyBag(device_path_, description_);
    propertyBag->AddRef();
    *ppvObj = propertyBag;
    return S_OK;
  }
  STDMETHOD(Reduce)
  (IBindCtx* pbc,
   DWORD dwReduceHowFar,
   IMoniker** ppmkToLeft,
   IMoniker** ppmkReduced) override {
    return E_NOTIMPL;
  }
  STDMETHOD(ComposeWith)
  (IMoniker* pmkRight,
   BOOL fOnlyIfNotGeneric,
   IMoniker** ppmkComposite) override {
    return E_NOTIMPL;
  }
  STDMETHOD(Enum)(BOOL fForward, IEnumMoniker** ppenumMoniker) override {
    return E_NOTIMPL;
  }
  STDMETHOD(IsEqual)(IMoniker* pmkOtherMoniker) override { return E_NOTIMPL; }
  STDMETHOD(Hash)(DWORD* pdwHash) override { return E_NOTIMPL; }
  STDMETHOD(IsRunning)
  (IBindCtx* pbc, IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning) override {
    return E_NOTIMPL;
  }
  STDMETHOD(GetTimeOfLastChange)
  (IBindCtx* pbc, IMoniker* pmkToLeft, FILETIME* pFileTime) override {
    return E_NOTIMPL;
  }
  STDMETHOD(Inverse)(IMoniker** ppmk) override { return E_NOTIMPL; }
  STDMETHOD(CommonPrefixWith)
  (IMoniker* pmkOther, IMoniker** ppmkPrefix) override { return E_NOTIMPL; }
  STDMETHOD(RelativePathTo)
  (IMoniker* pmkOther, IMoniker** ppmkRelPath) override { return E_NOTIMPL; }
  STDMETHOD(GetDisplayName)
  (IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR* ppszDisplayName) override {
    return E_NOTIMPL;
  }
  STDMETHOD(ParseDisplayName)
  (IBindCtx* pbc,
   IMoniker* pmkToLeft,
   LPOLESTR pszDisplayName,
   ULONG* pchEaten,
   IMoniker** ppmkOut) override {
    return E_NOTIMPL;
  }
  STDMETHOD(IsSystemMoniker)(DWORD* pdwMksys) override { return E_NOTIMPL; }

 private:
  friend class base::RefCountedThreadSafe<StubMoniker>;
  virtual ~StubMoniker() = default;

  const wchar_t* device_path_;
  const wchar_t* description_;
};

class StubEnumMoniker : public base::RefCountedThreadSafe<StubEnumMoniker>,
                        public IEnumMoniker {
 public:
  void AddMoniker(StubMoniker* moniker) { monikers_.push_back(moniker); }

  STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override {
    return E_NOTIMPL;
  }
  STDMETHOD_(ULONG, AddRef)() override {
    base::RefCountedThreadSafe<StubEnumMoniker>::AddRef();
    return 1U;
  }
  STDMETHOD_(ULONG, Release)() override {
    base::RefCountedThreadSafe<StubEnumMoniker>::Release();
    return 1U;
  }
  STDMETHOD(Next)
  (ULONG celt, IMoniker** rgelt, ULONG* pceltFetched) override {
    cursor_position_ = cursor_position_ + celt;
    if (cursor_position_ >= monikers_.size())
      return E_FAIL;
    IMoniker* moniker = monikers_.at(cursor_position_);
    *rgelt = moniker;
    moniker->AddRef();
    return S_OK;
  }
  STDMETHOD(Skip)(ULONG celt) override { return E_NOTIMPL; }
  STDMETHOD(Reset)(void) override {
    cursor_position_ = unsigned(-1);
    return S_OK;
  }
  STDMETHOD(Clone)(IEnumMoniker** ppenum) override { return E_NOTIMPL; }

 private:
  friend class base::RefCountedThreadSafe<StubEnumMoniker>;
  virtual ~StubEnumMoniker() = default;

  std::vector<IMoniker*> monikers_;
  ULONG cursor_position_ = unsigned(-1);
};

HRESULT __stdcall MockMFEnumDeviceSources(IMFAttributes* attributes,
                                          IMFActivate*** devices,
                                          UINT32* count) {
  MockMFActivate* mock_devices[] = {
      new MockMFActivate(kMFDeviceId0, kMFDeviceName0, true, false),
      new MockMFActivate(kMFDeviceId1, kMFDeviceName1, true, true),
      new MockMFActivate(kMFDeviceId2, kMFDeviceName2, false, true)};
  // Iterate once to get the match count and check for errors.
  *count = 0U;
  HRESULT hr;
  for (MockMFActivate* device : mock_devices) {
    if (device->MatchesQuery(attributes, &hr))
      (*count)++;
    if (FAILED(hr))
      return hr;
  }
  // Second iteration packs the returned devices and increments their
  // reference count.
  *devices = static_cast<IMFActivate**>(
      CoTaskMemAlloc(sizeof(IMFActivate*) * (*count)));
  int offset = 0;
  for (MockMFActivate* device : mock_devices) {
    if (!device->MatchesQuery(attributes, &hr))
      continue;
    *(*devices + offset++) = device;
    device->AddRef();
  }
  return S_OK;
}

HRESULT EnumerateStubDirectShowDevices(IEnumMoniker** enum_moniker) {
  StubMoniker* monikers[] = {
      new StubMoniker(kDirectShowDeviceId0, kDirectShowDeviceName0),
      new StubMoniker(kDirectShowDeviceId1, kDirectShowDeviceName1),
      new StubMoniker(kDirectShowDeviceId3, kDirectShowDeviceName3),
      new StubMoniker(kDirectShowDeviceId4, kDirectShowDeviceName4)};

  StubEnumMoniker* stub_enum_moniker = new StubEnumMoniker();
  for (StubMoniker* moniker : monikers)
    stub_enum_moniker->AddMoniker(moniker);

  stub_enum_moniker->AddRef();
  *enum_moniker = stub_enum_moniker;
  return S_OK;
}

}  // namespace

class VideoCaptureDeviceFactoryWinTest : public ::testing::Test {
 protected:
  VideoCaptureDeviceFactoryWinTest()
      : media_foundation_supported_(
            VideoCaptureDeviceFactoryWin::PlatformSupportsMediaFoundation()) {}

  bool ShouldSkipMFTest() {
    if (media_foundation_supported_)
      return false;
    DVLOG(1) << "Media foundation is not supported by the current platform. "
                "Skipping test.";
    return true;
  }

  VideoCaptureDeviceFactoryWin factory_;
  const bool media_foundation_supported_;
};

class VideoCaptureDeviceFactoryMFWinTest
    : public VideoCaptureDeviceFactoryWinTest {
  void SetUp() override { factory_.set_use_media_foundation_for_testing(true); }
};

TEST_F(VideoCaptureDeviceFactoryMFWinTest, GetDeviceDescriptors) {
  if (ShouldSkipMFTest())
    return;
  factory_.set_mf_enum_device_sources_func_for_testing(
      &MockMFEnumDeviceSources);
  factory_.set_direct_show_enum_devices_func_for_testing(
      base::BindRepeating(&EnumerateStubDirectShowDevices));
  VideoCaptureDeviceDescriptors descriptors;
  factory_.GetDeviceDescriptors(&descriptors);
  EXPECT_EQ(descriptors.size(), 5U);
  for (auto it = descriptors.begin(); it != descriptors.end(); it++) {
    // Verify that there are no duplicates.
    EXPECT_EQ(FindDescriptorInRange(descriptors.begin(), it, it->device_id),
              it);
  }
  iterator it = FindDescriptorInRange(descriptors.begin(), descriptors.end(),
                                      base::SysWideToUTF8(kMFDeviceId0));
  EXPECT_NE(it, descriptors.end());
  EXPECT_EQ(it->capture_api, VideoCaptureApi::WIN_MEDIA_FOUNDATION);
  EXPECT_EQ(it->display_name(), base::SysWideToUTF8(kMFDeviceName0));

  it = FindDescriptorInRange(descriptors.begin(), descriptors.end(),
                             base::SysWideToUTF8(kMFDeviceId1));
  EXPECT_NE(it, descriptors.end());
  EXPECT_EQ(it->capture_api, VideoCaptureApi::WIN_MEDIA_FOUNDATION);
  EXPECT_EQ(it->display_name(), base::SysWideToUTF8(kMFDeviceName1));

  it = FindDescriptorInRange(descriptors.begin(), descriptors.end(),
                             base::SysWideToUTF8(kMFDeviceId2));
  EXPECT_NE(it, descriptors.end());
  EXPECT_EQ(it->capture_api, VideoCaptureApi::WIN_MEDIA_FOUNDATION_SENSOR);
  EXPECT_EQ(it->display_name(), base::SysWideToUTF8(kMFDeviceName2));

  it = FindDescriptorInRange(descriptors.begin(), descriptors.end(),
                             base::SysWideToUTF8(kDirectShowDeviceId3));
  EXPECT_NE(it, descriptors.end());
  EXPECT_EQ(it->capture_api, VideoCaptureApi::WIN_DIRECT_SHOW);
  EXPECT_EQ(it->display_name(), base::SysWideToUTF8(kDirectShowDeviceName3));

  it = FindDescriptorInRange(descriptors.begin(), descriptors.end(),
                             base::SysWideToUTF8(kDirectShowDeviceId4));
  EXPECT_NE(it, descriptors.end());
  EXPECT_EQ(it->capture_api, VideoCaptureApi::WIN_DIRECT_SHOW);
  EXPECT_EQ(it->display_name(), base::SysWideToUTF8(kDirectShowDeviceName4));
}

}  // namespace media
