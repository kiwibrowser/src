// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_LISTENER_XML_ELEMENT_UTIL_H_
#define JINGLE_NOTIFIER_LISTENER_XML_ELEMENT_UTIL_H_

#include <string>

namespace buzz {
class XmlElement;
}

namespace notifier {

std::string XmlElementToString(const buzz::XmlElement& xml_element);

// The functions below are helpful for building notifications-related
// XML stanzas.

buzz::XmlElement* MakeBoolXmlElement(const char* name, bool value);

buzz::XmlElement* MakeIntXmlElement(const char* name, int value);

buzz::XmlElement* MakeStringXmlElement(const char* name, const char* value);

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_LISTENER_XML_ELEMENT_UTIL_H_
