#!/usr/bin/env vpython
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities for checking for disallowed usage of non-Blink declarations.

The scanner assumes that usage of non-Blink code is always namespace qualified.
Identifiers in the global namespace are always ignored. For convenience, the
script can be run in standalone mode to check for existing violations.

Example command:

$ git ls-files third_party/blink \
    | python third_party/blink/tools/audit_non_blink_usage.py
"""

import os
import re
import sys

_CONFIG = [
    {
        'paths': ['third_party/blink/renderer/'],
        'allowed': [
            # TODO(dcheng): Should these be in a more specific config?
            'gfx::ColorSpace',
            'gfx::CubicBezier',
            'gfx::ICCProfile',
            'gfx::ScrollOffset',

            # //base constructs that are allowed everywhere
            'base::AdoptRef',
            'base::AutoReset',
            'base::GetUniqueIdForProcess',
            'base::Location',
            'base::MakeRefCounted',
            'base::Optional',
            'base::OptionalOrNullptr',
            'base::RefCountedData',
            'base::CreateSequencedTaskRunnerWithTraits',
            'base::SequencedTaskRunner',
            'base::SingleThreadTaskRunner',
            'base::ScopedFD',
            'base::SysInfo',
            'base::ThreadChecker',
            'base::Time',
            'base::TimeDelta',
            'base::TimeTicks',
            'base::UnguessableToken',
            'base::UnsafeSharedMemoryRegion',
            'base::WeakPtr',
            'base::WeakPtrFactory',
            'base::WritableSharedMemoryMapping',
            'base::in_place',
            'base::make_optional',
            'base::make_span',
            'base::nullopt',
            'base::sequence_manager::TaskTimeObserver',
            'base::size',
            'base::span',
            'logging::GetVlogLevel',

            # //base/bind_helpers.h.
            'base::DoNothing',

            # //base/callback.h is allowed, but you need to use WTF::Bind or
            # WTF::BindRepeating to create callbacks in Blink.
            'base::OnceCallback',
            'base::OnceClosure',
            'base::RepeatingCallback',
            'base::RepeatingClosure',

            # //base/memory/ptr_util.h.
            'base::WrapUnique',

            # //base/synchronization/waitable_event.h.
            'base::WaitableEvent',

            # Debugging helpers from //base/debug are allowed everywhere.
            'base::debug::.+',

            # (Cryptographic) random number generation
            'base::RandUint64',
            'base::RandInt',
            'base::RandGenerator',
            'base::RandDouble',
            'base::RandBytes',

            # Feature list checking.
            'base::Feature.*',
            'base::FEATURE_.+',

            # Chromium geometry types.
            'gfx::Point',
            'gfx::Rect',
            'gfx::RectF',
            'gfx::Size',
            'gfx::SizeF',
            'gfx::Transform',
            # Wrapper of SkRegion used in Chromium.
            'cc::Region',

            # A geometric set of TouchActions associated with areas, and only
            # depends on the geometry types above.
            'cc::TouchActionRegion',

            # cc::Layers.
            'cc::Layer',

            # cc::Layer helper data structs.
            'cc::ElementId',
            'cc::LayerPositionConstraint',
            'cc::LayerStickyPositionConstraint',
            'cc::OverscrollBehavior',
            'cc::Scrollbar',
            'cc::ScrollbarLayerInterface',
            'cc::ScrollbarOrientation',
            'cc::ScrollbarPart',

            # cc::Layer helper enums.
            'cc::HORIZONTAL',
            'cc::VERTICAL',
            'cc::THUMB',
            'cc::TICKMARKS',

            # Standalone utility libraries that only depend on //base
            'skia::.+',
            'url::.+',

            # Nested namespace under the blink namespace for CSSValue classes.
            'cssvalue::.+',

            # Scheduler code lives in the scheduler namespace for historical
            # reasons.
            'scheduler::.+',

            # Third-party libraries that don't depend on non-Blink Chrome code
            # are OK.
            'icu::.+',
            'testing::.+',  # googlemock / googletest
            'v8::.+',
            'v8_inspector::.+',

            # Inspector instrumentation and protocol
            'probe::.+',
            'protocol::.+',

            # Blink code shouldn't need to be qualified with the Blink namespace,
            # but there are exceptions.
            'blink::.+',
            # Assume that identifiers where the first qualifier is internal are
            # nested in the blink namespace.
            'internal::.+',

            # Some test helpers live in the blink::test namespace.
            'test::.+',

            # Blink uses Mojo, so it needs mojo::Binding, mojo::InterfacePtr, et
            # cetera, as well as generated Mojo bindings.
            # Note that the Mojo callback helpers are explicitly forbidden:
            # Blink already has a signal for contexts being destroyed, and
            # other types of failures should be explicitly signalled.
            'mojo::(?!WrapCallback).+',
            'mojo_base::BigBuffer.*',
            '(?:.+::)?mojom::.+',
            "service_manager::BinderRegistry",
            # TODO(dcheng): Remove this once Connector isn't needed in Blink
            # anymore.
            'service_manager::Connector',
            'service_manager::InterfaceProvider',

            # STL containers such as std::string and std::vector are discouraged
            # but still needed for interop with WebKit/common. Note that other
            # STL types such as std::unique_ptr are encouraged.
            'std::.+',

            # Blink uses UKM for logging e.g. always-on leak detection (crbug/757374)
            'ukm::.+',

            # WebRTC classes
            'webrtc::.+',
        ],
        'disallowed': ['.+'],
    },
    {
        'paths': ['third_party/blink/renderer/bindings/'],
        'allowed': ['gin::.+'],
    },
    {
        'paths': ['third_party/blink/renderer/core/css'],
        'allowed': [
            # Internal implementation details for CSS.
            'detail::.+',
        ],
    },
    {
        'paths': ['third_party/blink/renderer/core/paint'],
        'allowed': [
            # cc painting types.
            'cc::ContentLayerClient',
            'cc::DisplayItemList',
            'cc::DrawRecordOp',
            'cc::PictureLayer',
        ],
    },
    {
        'paths': ['third_party/blink/renderer/core/page/scrolling'],
        'allowed': [
            # cc painting types.
            'cc::PaintCanvas',

            # cc scrollbar layer types.
            'cc::PaintedOverlayScrollbarLayer',
            'cc::PaintedScrollbarLayer',
            'cc::SolidColorScrollbarLayer',
        ],
    },
    {
        'paths': ['third_party/blink/renderer/core/inspector/inspector_memory_agent.cc'],
        'allowed': [
            'base::SamplingHeapProfiler',
        ],
    },
    {
        'paths': ['third_party/blink/renderer/core/inspector/inspector_performance_agent.cc'],
        'allowed': [
            'base::subtle::TimeTicksNowIgnoringOverride',
        ],
    },
    {
        'paths': [
            'third_party/blink/renderer/modules/device_orientation/',
            'third_party/blink/renderer/modules/gamepad/',
            'third_party/blink/renderer/modules/sensor/',
        ],
        'allowed': ['device::.+'],
    },
    {
        'paths': [
            'third_party/blink/renderer/core/html/media/',
            'third_party/blink/renderer/modules/vr/',
            'third_party/blink/renderer/modules/webgl/',
            'third_party/blink/renderer/modules/xr/',
        ],
        # The modules listed above need access to the following GL drawing and
        # display-related types.
        'allowed': [
            'gpu::gles2::GLES2Interface',
            'gpu::MailboxHolder',
            'display::Display',
        ],
    },
    {
        'paths': [
            'third_party/blink/renderer/platform/',
        ],
        # Suppress almost all checks on platform since code in this directory
        # is meant to be a bridge between Blink and non-Blink code. However,
        # base::RefCounted should still be explicitly blocked, since
        # WTF::RefCounted should be used instead.
        'allowed': ['(?!base::RefCounted).+'],
    },
    {
        'paths': [
            'third_party/blink/renderer/core/exported/',
            'third_party/blink/renderer/modules/exported/',
        ],
        'allowed': [
            'base::Time',
            'base::TimeTicks',
            'base::TimeDelta',
        ],
    },
    {
        'paths': [
            'third_party/blink/renderer/modules/webdatabase/',
        ],
        'allowed': ['sql::.+'],
    },
    {
        'paths': [
            'third_party/blink/renderer/core/layout/layout_theme.cc',
            'third_party/blink/renderer/core/paint/fallback_theme.cc',
            'third_party/blink/renderer/core/paint/fallback_theme.h',
            'third_party/blink/renderer/core/paint/theme_painter.cc',
        ],
        'allowed': ['ui::NativeTheme.*'],
    },
    {
        'paths': [
            'third_party/blink/renderer/modules/crypto/',
        ],
        'allowed': ['crypto::.+'],
    },
]


def _precompile_config():
    """Turns the raw config into a config of compiled regex."""
    match_nothing_re = re.compile('.^')

    def compile_regexp(match_list):
        """Turns a match list into a compiled regexp.

        If match_list is None, a regexp that matches nothing is returned.
        """
        if match_list:
            return re.compile('(?:%s)$' % '|'.join(match_list))
        return match_nothing_re

    compiled_config = []
    for raw_entry in _CONFIG:
        compiled_config.append({
            'paths': raw_entry['paths'],
            'allowed': compile_regexp(raw_entry.get('allowed')),
            'disallowed': compile_regexp(raw_entry.get('disallowed')),
        })
    return compiled_config


_COMPILED_CONFIG = _precompile_config()

# Attempt to match identifiers qualified with a namespace. Since parsing C++ in
# Python is hard, this regex assumes that namespace names only contain lowercase
# letters, numbers, and underscores, matching the Google C++ style guide. This
# is intended to minimize the number of matches where :: is used to qualify a
# name with a class or enum name.
#
# As a bit of a minor hack, this regex also hardcodes a check for GURL, since
# GURL isn't namespace qualified and wouldn't match otherwise.
_IDENTIFIER_WITH_NAMESPACE_RE = re.compile(
    r'\b(?:(?:[a-z_][a-z0-9_]*::)+[A-Za-z_][A-Za-z0-9_]*|GURL)\b')


def _find_matching_entries(path):
    """Finds entries that should be used for path.

    Returns:
        A list of entries, sorted in order of relevance. Each entry is a
        dictionary with two keys:
            allowed: A regexp for identifiers that should be allowed.
            disallowed: A regexp for identifiers that should not be allowed.
    """
    entries = []
    for entry in _COMPILED_CONFIG:
        for entry_path in entry['paths']:
            if path.startswith(entry_path):
                entries.append({'sortkey': len(entry_path), 'entry': entry})
    # The path length is used as the sort key: a longer path implies more
    # relevant, since that config is a more exact match.
    entries.sort(key=lambda x: x['sortkey'], reverse=True)
    return [entry['entry'] for entry in entries]


def _check_entries_for_identifier(entries, identifier):
    for entry in entries:
        if entry['allowed'].match(identifier):
            return True
        if entry['disallowed'].match(identifier):
            return False
    # Disallow by default.
    return False


def check(path, contents):
    """Checks for disallowed usage of non-Blink classes, functions, et cetera.

    Args:
        path: The path of the file to check.
        contents: An array of line number, line tuples to check.

    Returns:
        A list of line number, disallowed identifier tuples.
    """
    results = []
    basename, ext = os.path.splitext(path)
    # Only check code. Ignore tests.
    # TODO(tkent): Remove 'Test' after the great mv.
    if (ext not in ('.cc', '.cpp', '.h', '.mm')
            or basename.endswith('Test')
            or basename.endswith('_test')
            or basename.endswith('_unittest')):
        return results
    entries = _find_matching_entries(path)
    if not entries:
        return
    for line_number, line in contents:
        idx = line.find('//')
        if idx >= 0:
            line = line[:idx]
        match = _IDENTIFIER_WITH_NAMESPACE_RE.search(line)
        if match:
            if not _check_entries_for_identifier(entries, match.group(0)):
                results.append((line_number, match.group(0)))
    return results


def main():
    for path in sys.stdin.read().splitlines():
        try:
            with open(path, 'r') as f:
                contents = f.read()
                disallowed_identifiers = check(path, [
                    (i + 1, l) for i, l in
                    enumerate(contents.splitlines())])
                if disallowed_identifiers:
                    print '%s uses disallowed identifiers:' % path
                    for i in disallowed_identifiers:
                        print i
        except IOError as e:
            print 'could not open %s: %s' % (path, e)


if __name__ == '__main__':
    sys.exit(main())
