# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections

from tracing.value import histogram as histogram_module
from tracing.value.diagnostics import all_diagnostics
from tracing.value.diagnostics import diagnostic
from tracing.value.diagnostics import diagnostic_ref

class HistogramSet(object):
  def __init__(self, histograms=()):
    self._histograms_by_guid = {}
    self._shared_diagnostics_by_guid = {}
    for hist in histograms:
      self.AddHistogram(hist)

  @property
  def shared_diagnostics(self):
    return self._shared_diagnostics_by_guid.itervalues()

  def RemoveOrphanedDiagnostics(self):
    orphans = set(self._shared_diagnostics_by_guid.keys())
    for h in self._histograms_by_guid.itervalues():
      for d in h.diagnostics.itervalues():
        if d.guid in orphans:
          orphans.remove(d.guid)
    for guid in orphans:
      del self._shared_diagnostics_by_guid[guid]

  def FilterHistograms(self, discard):
    self._histograms_by_guid = dict(
        (guid, hist)
        for guid, hist in self._histograms_by_guid.iteritems()
        if not discard(hist))

  def AddHistogram(self, hist, diagnostics=None):
    if hist.guid in self._histograms_by_guid:
      raise ValueError('Cannot add same Histogram twice')

    if diagnostics:
      for name, diag in diagnostics.iteritems():
        hist.diagnostics[name] = diag

    self._histograms_by_guid[hist.guid] = hist

  def AddSharedDiagnostic(self, name, diag):
    self._shared_diagnostics_by_guid[diag.guid] = diag

    for hist in self:
      hist.diagnostics[name] = diag

  def GetFirstHistogram(self):
    for histogram in self._histograms_by_guid.itervalues():
      return histogram

  def GetHistogramsNamed(self, name):
    return [h for h in self if h.name == name]

  def GetSharedDiagnosticsOfType(self, typ):
    return [d for d in self.shared_diagnostics if isinstance(d, typ)]

  def LookupHistogram(self, guid):
    return self._histograms_by_guid.get(guid)

  def LookupDiagnostic(self, guid):
    return self._shared_diagnostics_by_guid.get(guid)

  def ResolveRelatedHistograms(self):
    histograms = self
    def HandleDiagnosticMap(dm):
      for diag in dm.itervalues():
        if isinstance(diag, histogram_module.RelatedHistogramMap):
          diag.Resolve(histograms)

    for hist in self:
      HandleDiagnosticMap(hist.diagnostics)
      for dm in hist.nan_diagnostic_maps:
        HandleDiagnosticMap(dm)
      for hbin in hist.bins:
        for dm in hbin.diagnostic_maps:
          HandleDiagnosticMap(dm)

  def __len__(self):
    return len(self._histograms_by_guid)

  def __iter__(self):
    for hist in self._histograms_by_guid.itervalues():
      yield hist

  def ImportDicts(self, dicts):
    for d in dicts:
      if d.get('type') in all_diagnostics.GetDiagnosticTypenames():
        diag = diagnostic.Diagnostic.FromDict(d)
        self._shared_diagnostics_by_guid[d['guid']] = diag
      else:
        hist = histogram_module.Histogram.FromDict(d)
        hist.diagnostics.ResolveSharedDiagnostics(self)
        self.AddHistogram(hist)

  def AsDicts(self):
    dcts = []
    for d in self._shared_diagnostics_by_guid.itervalues():
      dcts.append(d.AsDict())
    for h in self:
      dcts.append(h.AsDict())
    return dcts

  def ReplaceSharedDiagnostic(self, old_guid, new_diagnostic):
    if not isinstance(new_diagnostic, diagnostic_ref.DiagnosticRef):
      self._shared_diagnostics_by_guid[new_diagnostic.guid] = new_diagnostic

    for hist in self:
      for name, diag in hist.diagnostics.iteritems():
        if diag.has_guid and diag.guid == old_guid:
          hist.diagnostics[name] = new_diagnostic

  def DeduplicateDiagnostics(self):
    names_to_candidates = {}
    diagnostics_to_histograms = collections.defaultdict(list)

    for hist in self:
      for name, candidate in hist.diagnostics.iteritems():
        diagnostics_to_histograms[candidate].append(hist)

        if name not in names_to_candidates:
          names_to_candidates[name] = set()
        names_to_candidates[name].add(candidate)

    for name, candidates in names_to_candidates.iteritems():
      deduplicated_diagnostics = set()

      for candidate in candidates:
        found = False
        for test in deduplicated_diagnostics:
          if candidate == test:
            hists = diagnostics_to_histograms.get(candidate)
            for h in hists:
              h.diagnostics[name] = test
            found = True
            break
        if not found:
          deduplicated_diagnostics.add(candidate)

        for diag in deduplicated_diagnostics:
          self._shared_diagnostics_by_guid[diag.guid] = diag
