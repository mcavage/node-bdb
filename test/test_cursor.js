var assert = require('assert');
var Buffer = require('buffer').Buffer;
var exec  = require('child_process').exec;
var fs = require('fs');
var BDB = require('bdb');
var helper = require('./helper');

// setup
var env_location = "/tmp/" + helper.uuid();
fs.mkdirSync(env_location, 0750);

var env = new BDB.DbEnv();
stat = env.open(env_location);
assert.equal(0, stat.code, stat.message);

var db = new BDB.Db();
stat = db.open(env, helper.uuid());
assert.equal(0, stat.code, stat.message);

function testPut(collection, callback) {
  var coll = collection.slice(0);
  var txn = new BDB.DbTxn();
  stat = env.txnBegin(txn);
  assert.equal(0, stat.code, stat.message);
  var cursor = new BDB.DbCursor();
  stat = db.cursor(cursor, txn);
  assert.equal(0, stat.code, stat.message);
  (function insertOne() {
    var record = coll.splice(0, 1)[0];
    cursor.put(new Buffer(record), new Buffer(record), function(res) {
      assert.equal(0, res.code, res.message);
      if (coll.length == 0) {
	txn.commit(function(res) {
	  assert.equal(0, res.code, res.message);
          callback();
	});
      } else {
        insertOne();
      }
    });
  })();
}

function testGet(collection, callback) {
  var coll = collection.slice(0);
  var txn = new BDB.DbTxn();
  stat = env.txnBegin(txn);
  assert.equal(0, stat.code, stat.message);
  var cursor = new BDB.DbCursor();
  stat = db.cursor(cursor, txn);
  assert.equal(0, stat.code, stat.message);
  (function getOne() {
    var record = coll.splice(0, 1)[0];
    cursor.get(function(res, key, val) {
      assert.equal(0, res.code, res.message);
      assert.equal(record, key.toString(encoding='utf8'), "get key mismatch");
      assert.equal(record, val.toString(encoding='utf8'), "get val mismatch");
      if (coll.length == 0) {
	txn.commit(function(res) {
	  assert.equal(0, res.code, res.message);
          callback();
	});
      } else {
        getOne();
      }
    });
  })();
}

function testDel(collection, callback) {
  var coll = collection.slice(0);
  var txn = new BDB.DbTxn();
  stat = env.txnBegin(txn);
  assert.equal(0, stat.code, stat.message);
  var cursor = new BDB.DbCursor();
  stat = db.cursor(cursor, txn);
  assert.equal(0, stat.code, stat.message);
  (function delOne() {
    var record = coll.splice(0, 1)[0];
    cursor.get(function(res, key, val) {
      assert.equal(0, res.code, res.message);
      assert.equal(record, key.toString(encoding='utf8'), "get key mismatch");
      assert.equal(record, val.toString(encoding='utf8'), "get val mismatch");
      cursor.del(function(res) {
	assert.equal(0, res.code, res.message);
	if (coll.length == 0) {
	  txn.commit(function(res) {
	    assert.equal(0, res.code, res.message);
            callback();
	  });
	} else {
          delOne();
	}
      });
    });
  })();
}

// First create our records
var records = [];
for(var i = 0; i < 10; i++) {
  records.push(helper.uuid());
}

// Now load
testPut(records, function() {
  records.sort();
  testGet(records, function() {
    testDel(records, function() {
      console.log("test_cursor: PASSED");
    });
  });
});

