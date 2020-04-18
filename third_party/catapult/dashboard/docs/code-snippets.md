# Code Snippets

It is possible to directly execute code on a production instance of the performance dashboard. This is one way to directly query information about the state of the datastore, and make quick adjustments to data in the datastore.

There are two places where production code can be run (admins only):

 - https://chromeperf.appspot.com/\_ah/dev\_console/interactive
 - https://chromeperf.appspot.com/\_ah/stats/shell

## List tests frequently marked as invalid

```python
import collections
from google.appengine.ext import ndb
from speed.dashboard import utils
from dashboard.models import anomaly

sheriff = ndb.Key('Sheriff', 'Chromium Perf Sheriff')
query = anomaly.Anomaly.query(anomaly.Anomaly.bug_id == -1)
query = query.filter(anomaly.Anomaly.sheriff == sheriff)
query = query.order(-anomaly.Anomaly.timestamp)
alerts = query.fetch(limit=5000)

total_alerts = len(alerts)
print 'Fetched {} "invalid" alerts.'.format(len(alerts))

occurrences = [[], [], []]
for a in alerts:
  parts = utils.TestPath(a.test).split('/', 3)[1:]
    for i, part in enumerate(parts):
      occurrences[i].append(part)

types = ['bot', 'benchmark', 'subtest']
counters = [(type, collections.Counter(x)) for type, x in zip(types, occurrences)]
for type, counter in counters:
  print 'nTop {}s marked invalid:'.format(type)
    print ' {0:>5} {1:>13} {2}'.format('Count', '% of invalid', 'Name')
    for name, count in counter.most_common(10):
      percent = 100 * float(count) / total_alerts
        print ' {0:>5} {1:>12}% {2}'.format(count, percent, name)
```

## List unique test suite names

```python
from dashboard.models import graph_data

LIMIT = 10000

query = graph_data.Test.query(graph_data.Test.parent_test == None)
test_keys = query.fetch(limit=LIMIT, keys_only=True)
unique = sorted(set(k.string_id() for k in test_keys))
print 'Fetched %d Test keys, %d unique names.' % (len(test_keys), len(unique))
for name in unique:
  print name
```

## List deprecated test suites

```python
from dashboard import utils
from dashboard.models import graph_data

LIMIT = 10000

query = graph_data.Test.query(
    graph_data.Test.parent_test == None,
        graph_data.Test.deprecated == True)
        test_keys = query.fetch(limit=LIMIT, keys_only=True)
    print 'Fetched %d Test keys.' % len(test_keys)
for key in test_keys:
  print utils.TestPath(key)
```

## List all sub-tests of a particular test

```python
from google.appengine.ext import ndb
from dashboard import utils
from dashboard.models import graph_data

ancestor = utils.TestKey('ChromiumPerf/linux-release/sunspider')
keys = graph_data.Test.query(ancestor=ancestor).fetch(keys_only=True)

print 'Fetched %d keys.' % len(keys)
for key in keys:
  print utils.TestPath(key)
```

## Delete a particular sheriff or other entity

```python
from google.appengine.ext import ndb

key = ndb.Key('Sheriff', 'Sheriff name')
print 'Deleting: %s\n%s' % (key.string_id(), key.get())
key.delete()
```

## Clear the LastAddedRevision entities for a Test

This allows point IDs that are much higher or lower to be posted.

```python
from google.appengine.ext import ndb
from dashboard import utils
from dashboard.models import graph_data

ancestor_key = utils.TestKey('Master/bot/test')
test_query = graph_data.Test.query(ancestor=ancestor_key)
test_keys = test_query.fetch(keys_only=True)
to_delete = []
for test_key in test_keys:
  to_delete.append(ndb.Key('LastAddedRevision', utils.TestPath(test_key)))
print 'Deleting up to %d LastAddedRevision entities.' % len(to_delete)
ndb.delete_multi(to_delete)
```

## Delete a few specific points (dangerous)

```python
from google.appengine.ext import ndb
from dashboard.models import graph_data

POINTIDS = []
TEST_PATHS = []

to_delete = []
for id in IDS:
  for path in TEST_PATHS:
    to_delete.append(ndb.Key('TestContainer', path, 'Row', id))

print 'Deleting %d rows.' % len(to_delete)
ndb.delete_multi(to_delete)
```

## Delete Rows and Tests under a particular Master or Bot (dangerous)

```python
from google.appengine.ext import ndb
from dashboard import utils
from dashboard.models import graph_data

ancestor_key = utils.TestKey('ChromiumEndure')
test_keys = graph_data.Test.query(ancestor=ancestor_key).fetch(keys_only=True)
print len(test_keys)
to_delete = []
for test_key in test_keys:
  row_keys = graph_data.Row.query(
      graph_data.Row.parent_test == test_key).fetch(keys_only=True, limit=100)
  to_delete.extend(row_keys)
  if not row_keys:
    to_delete.append(test_key)
print len(to_delete)
ndb.delete_multi(to_delete[:1000])
```
