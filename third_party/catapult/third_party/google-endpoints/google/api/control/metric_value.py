# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""metric values provides funcs using to aggregate `MetricValue`.

:func:`merge` merges two `MetricValue` instances.
:func:`update_hash` adds a `MetricValue` to a secure hash
:func:`sign` generates a signature for a `MetricValue` using a secure hash

"""

from __future__ import absolute_import

import hashlib
import logging

from apitools.base.py import encoding

from . import distribution, money, signing, timestamp, MetricKind
from google.api.gen.servicecontrol_v1_messages import MetricValue


logger = logging.getLogger(__name__)


def create(labels=None, **kw):
    """Constructs a new metric value.

    This acts as an alternate to MetricValue constructor which
    simplifies specification of labels.  Rather than having to create
    a MetricValue.Labels instance, all that's necessary to specify the
    required string.

    Args:
      labels (dict([string, [string]]):
      **kw: any other valid keyword args valid in the MetricValue constructor

    Returns
      :class:`MetricValue`: the created instance

    """
    if labels is not None:
        kw['labels'] = encoding.PyValueToMessage(MetricValue.LabelsValue,
                                                 labels)
    return MetricValue(**kw)


def merge(metric_kind, prior, latest):
    """Merges `prior` and `latest`

    Args:
       metric_kind (:class:`MetricKind`): indicates the kind of metrics
         being merged
       prior (:class:`MetricValue`): an prior instance of the metric
       latest (:class:`MetricValue`: the latest instance of the metric
    """
    prior_type, _ = _detect_value(prior)
    latest_type, _ = _detect_value(latest)
    if prior_type != latest_type:
        logger.warn('Metric values are not compatible: %s, %s',
                    prior, latest)
        raise ValueError('Incompatible delta metric values')
    if prior_type is None:
        logger.warn('Bad metric values, types not known for : %s, %s',
                    prior, latest)
        raise ValueError('Unsupported delta metric types')

    if metric_kind == MetricKind.DELTA:
        return _merge_delta_metric(prior, latest)
    else:
        return _merge_cumulative_or_gauge_metrics(prior, latest)


def update_hash(a_hash, mv):
    """Adds ``mv`` to ``a_hash``

    Args:
       a_hash (`Hash`): the secure hash, e.g created by hashlib.md5
       mv (:class:`MetricValue`): the instance to add to the hash

    """
    if mv.labels:
        signing.add_dict_to_hash(a_hash, encoding.MessageToPyValue(mv.labels))
    money_value = mv.get_assigned_value('moneyValue')
    if money_value is not None:
        a_hash.update('\x00')
        a_hash.update(money_value.currencyCode)


def sign(mv):
    """Obtains a signature for a `MetricValue`

    Args:
       mv (:class:`google.api.gen.servicecontrol_v1_messages.MetricValue`): a
         MetricValue that's part of an operation

    Returns:
       string: a unique signature for that operation
    """
    md5 = hashlib.md5()
    update_hash(md5, mv)
    return md5.digest()


def _merge_cumulative_or_gauge_metrics(prior, latest):
    if timestamp.compare(prior.endTime, latest.endTime) == -1:
        return latest
    else:
        return prior


def _merge_delta_metric(prior, latest):
    prior_type, prior_value = _detect_value(prior)
    latest_type, latest_value = _detect_value(latest)
    _merge_delta_timestamps(prior, latest)
    updated_value = _combine_delta_values(prior_type, prior_value, latest_value)
    setattr(latest, latest_type, updated_value)
    return latest


# This is derived from the oneof choices for the MetricValue message's value
# field in google/api/servicecontrol/v1/metric_value.proto, and should be kept
# in sync with that
_METRIC_VALUE_ONEOF_FIELDS = (
    'boolValue', 'distributionValue', 'doubleValue', 'int64Value',
    'moneyValue', 'stringValue')


def _detect_value(metric_value):
    for f in _METRIC_VALUE_ONEOF_FIELDS:
        value = metric_value.get_assigned_value(f)
        if value is not None:
            return f, value
    return None, None


def _merge_delta_timestamps(prior, latest):
    # Update the start time and end time in the latest metric value
    if (prior.startTime and
        (latest.startTime is None or
         timestamp.compare(prior.startTime, latest.startTime) == -1)):
        latest.startTime = prior.startTime

    if (prior.endTime and
        (latest.endTime is None or timestamp.compare(
            latest.endTime, prior.endTime) == -1)):
        latest.endTime = prior.endTime

    return latest


def _combine_delta_values(value_type, prior, latest):
    if value_type in ('int64Value', 'doubleValue'):
        return prior + latest
    elif value_type == 'moneyValue':
        return money.add(prior, latest, allow_overflow=True)
    elif value_type == 'distributionValue':
        distribution.merge(prior, latest)
        return latest
    else:
        logger.error('Unmergeable metric type %s', value_type)
        raise ValueError('Could not merge unmergeable metric type')
