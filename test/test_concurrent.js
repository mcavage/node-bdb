// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var assert = require('assert');
var Buffer = require('buffer').Buffer;
var exec  = require('child_process').exec;
var fs = require('fs');
var bdb = require('bdb');
var helper = require('./helper');

// setup
var ITERATIONS = 10;

var env = new bdb.DbEnv();
var db = new bdb.Db();

var env_location = "/tmp/" + helper.uuid();
var stat = env.setTxnMax(ITERATIONS * 2);
assert.equal(0, stat.code, stat.message);

fs.mkdirSync(env_location, 0750);
stat = env.openSync({home:env_location});
assert.equal(0, stat.code, stat.message);
stat = db.openSync({env: env, file: helper.uuid()});
assert.equal(0, stat.code, stat.message);

var run = function(callback) {
  var key = new Buffer(helper.uuid());
  var val = new Buffer(helper.uuid());
  var txn = new bdb.DbTxn();
  stat = env.txnBegin({txn: txn});
  assert.equal(0, stat.code, stat.message);

  db.put({key: key, val: val, txn:txn}, function(res) {
    assert.equal(0, res.code, res.message);
    db.get({key: key, txn: txn}, function(res, data) {
      assert.equal(0, res.code, res.message);
      assert.ok(data, "no data from get");
      assert.equal(val, data.toString(encoding='utf8'), 'Data mismatch');
      db.del({key: key, txn: txn}, function(res) {
	assert.equal(0, res.code, res.message);
	db.get({key: key, txn: txn}, function(res, data) {
	  assert.equal(bdb.FLAGS.DB_NOTFOUND, res.code, res.message);
	  txn.commit(function(res) {
	    assert.equal(0, res.code, res.message);
	    callback();
	  });
	});
      });
    });
  });
};

// Make sure that multiple threads are kicking in for put/get/del
var finished = 0;
for(var i = 0; i < ITERATIONS; i++) {
  run(function() {
    if(++finished >= ITERATIONS) {
      console.log('test_concurrent: PASSED');
      exec("rm -fr " + env_location, function(err, stdout, stderr) {});
    }
  });
}
