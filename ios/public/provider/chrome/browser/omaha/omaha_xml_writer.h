// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_OMAHA_OMAHA_XML_WRITER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_OMAHA_OMAHA_XML_WRITER_H_

// OmahaXmlWriter is a helper class to generate XML for the Omaha request.
class OmahaXmlWriter {
 public:
  OmahaXmlWriter() = default;
  virtual ~OmahaXmlWriter() = default;

  // Starts a new element with the given |name|.
  virtual void StartElement(const char* name) = 0;

  // Ends the current element.
  virtual void EndElement() = 0;

  // Writes an attribute with the given |name| and |value| to the current
  // element.
  virtual void WriteAttribute(const char* name, const char* value) = 0;

  // Finalizes the XML document.  Calling StartElement(), EndElement(), or
  // WriteAttribute() will have no effect after calling this method.
  virtual void Finalize() = 0;

  // Returns the generated XML. This method should only be called after
  // Finalize().
  virtual std::string GetContentAsString() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(OmahaXmlWriter);
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_OMAHA_OMAHA_XML_WRITER_H_
