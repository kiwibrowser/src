readonly transaction should see the result of a previous readwrite transaction

On success, you will see a series of "PASS" messages, followed by "TEST COMPLETE".


dbname = "transaction-coordination-ro-waits-for-rw.html"
indexedDB.deleteDatabase(dbname)
indexedDB.open(dbname)

prepareDatabase():
db = event.target.result
store = db.createObjectStore('store')
store.put('original value', 'key')

runTransactions():
db = event.target.result
transaction1 = db.transaction('store', 'readwrite')
transaction2 = db.transaction('store', 'readonly')
request = transaction1.objectStore('store').put('new value', 'key')
request2 = transaction2.objectStore('store').get('key')

checkResult():
PASS request2.result is "new value"
PASS successfullyParsed is true

TEST COMPLETE

