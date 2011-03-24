// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var assert = require('assert');
var Buffer = require('buffer').Buffer;
var exec  = require('child_process').exec;
var fs = require('fs');
var util = require('util');

var bdb = require('bdb');
var helper = require('./helper');

// setup
var ITERATIONS = 500;

var env = new bdb.DbEnv();
var db = new bdb.Db();

env.setLockDetect(bdb.FLAGS.DB_LOCK_MAXWRITE);
env.setMaxLockers(ITERATIONS * 6 + 1);
env.setMaxLockObjects(ITERATIONS * 6 + 1);

var env_location = "/tmp/" + helper.uuid();
fs.mkdirSync(env_location, 0750);
stat = env.openSync({home:env_location});
assert.equal(0, stat.code, stat.message);
stat = db.openSync({env: env, file: helper.uuid(), retries: 4});
assert.equal(0, stat.code, stat.message);

var run = function(callback) {
  var key = new Buffer(helper.uuid());
  var val = new Buffer(helper.uuid());

  db.put({key: key, val: val}, function(res) {
    assert.equal(0, res.code, res.message);
    db.get({key: key}, function(res, data) {
      assert.equal(0, res.code, res.message);
      assert.ok(data, "no data from get");
      assert.equal(val, data.toString(encoding='utf8'), 'Data mismatch');
      db.del({key: key}, function(res) {
	assert.equal(0, res.code, res.message);
	callback();
      });
    });
  });
};

// Make sure that multiple threads are kicking in for put/get/del
var finished = 0;
for(var i = 0; i < ITERATIONS; i++) {
  run(function() {
    if(++finished >= ITERATIONS) {
      stat = db.closeSync();
      assert.equal(0, stat.code, stat.message);
      stat = env.closeSync();
      assert.equal(0, stat.code, stat.message);
      console.log('test_concurrent: PASSED');
      exec("rm -fr " + env_location, function(err, stdout, stderr) {});
    }
  });
}
