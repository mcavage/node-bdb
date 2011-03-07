// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var assert = require('assert');
var Buffer = require('buffer').Buffer;
var exec  = require('child_process').exec;
var fs = require('fs');
var bdb = require('bdb');
var helper = require('./helper');

// setup
var env_location = "/tmp/" + helper.uuid();
var env = new bdb.DbEnv();
var db = new bdb.Db();
var stat = 0;

fs.mkdirSync(env_location, 0750);
stat = env.openSync({home:env_location});
assert.equal(0, stat.code, stat.message);
stat = db.openSync({env: env, file: helper.uuid()});
assert.equal(0, stat.code, stat.message);

function testPut(collection, callback) {
  var coll = collection.slice(0);
  var txn = new bdb.DbTxn();
  stat = env.txnBeginSync({txn: txn});
  assert.equal(0, stat.code, stat.message);
  var cursor = new bdb.DbCursor();
  stat = db.cursor({cursor: cursor, txn: txn});
  assert.equal(0, stat.code, stat.message);
  (function insertOne() {
    var record = coll.splice(0, 1)[0];
    cursor.put({key: new Buffer(record),
		val: new Buffer(record)}, function(res) {
      assert.equal(0, res.code, res.message);
      if (coll.length === 0) {
	stat = txn.commitSync();
	assert.equal(0, stat.code, stat.message);
        callback();
      } else {
        insertOne();
      }
    });
  })();
}

function testGet(collection, callback) {
  var coll = collection.slice(0);
  var txn = new bdb.DbTxn();
  stat = env.txnBeginSync({txn: txn});
  assert.equal(0, stat.code, stat.message);
  var cursor = new bdb.DbCursor();
  stat = db.cursor({cursor: cursor, txn: txn});
  assert.equal(0, stat.code, stat.message);
  (function getOne() {
    var record = coll.splice(0, 1)[0];
    cursor.get(function(res, key, val) {
      assert.equal(0, res.code, res.message);
      assert.equal(record, key.toString(encoding='utf8'), "get key mismatch");
      assert.equal(record, val.toString(encoding='utf8'), "get val mismatch");
      if (coll.length === 0) {
	stat = txn.commitSync();
	assert.equal(0, stat.code, stat.message);
        callback();
      } else {
        getOne();
      }
    });
  })();
}

function testDel(collection, callback) {
  var coll = collection.slice(0);
  var txn = new bdb.DbTxn();
  stat = env.txnBeginSync({txn: txn});
  assert.equal(0, stat.code, stat.message);
  var cursor = new bdb.DbCursor();
  stat = db.cursor({cursor: cursor, txn: txn});
  assert.equal(0, stat.code, stat.message);
  (function delOne() {
    var record = coll.splice(0, 1)[0];
    cursor.get(function(res, key, val) {
      assert.equal(0, res.code, res.message);
      assert.equal(record, key.toString(encoding='utf8'), "get key mismatch");
      assert.equal(record, val.toString(encoding='utf8'), "get val mismatch");
      cursor.del(function(res) {
	assert.equal(0, res.code, res.message);
	if (coll.length === 0) {
	  stat = txn.commitSync();
	  assert.equal(0, stat.code, stat.message);
          callback();
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
      exec("rm -fr " + env_location, function(err, stdout, stderr) {});
      console.log("test_cursor: PASSED");
    });
  });
});
