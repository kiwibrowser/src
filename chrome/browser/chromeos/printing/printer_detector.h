// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_DETECTOR_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_DETECTOR_H_

#include <vector>

#include "chromeos/printing/ppd_provider.h"
#include "chromeos/printing/printer_configuration.h"

namespace chromeos {

// Interface for automatic printer detection.  This API allows for
// incremental discovery of printers and notification when discovery
// is complete.
//
// All of the interface calls in this class must be called from the
// same sequence, but do not have to be on any specific thread.
//
// The usual usage of this interface by a class that wants to maintain
// an up-to-date list of the printers detected is this:
//
// auto detector_ = PrinterDetectorImplementation::Create();
// detector_->AddObserver(this);
// printers_ = detector_->GetPrinters();
//
class CHROMEOS_EXPORT PrinterDetector {
 public:
  // The result of a detection.
  struct DetectedPrinter {
    // Printer information
    Printer printer;

    // Additional metadata used to find a driver.
    PpdProvider::PrinterSearchData ppd_search_data;
  };

  class Observer {
   public:
    virtual ~Observer() = default;

    // Called with a collection of printers as they are discovered.  On each
    // call |printers| is the full set of known printers; it is not
    // incremental; printers may be added or removed.
    //
    // To avoid race conditions of printer state changes, you should register
    // your Observer and call PrinterDetector::GetPrinters() to populate the
    // initial state in the same sequenced atom.
    virtual void OnPrintersFound(
        const std::vector<DetectedPrinter>& printers) = 0;
  };

  virtual ~PrinterDetector() = default;

  // Observer management.  Observer callbacks will be performed on the calling
  // sequence, but observers do not need to be on the same sequence as each
  // other or the detector.
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Get the current list of known printers.
  virtual std::vector<DetectedPrinter> GetPrinters() = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_DETECTOR_H_
