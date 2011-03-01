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

var key = new Buffer(helper.uuid());
var val = new Buffer(helper.uuid());
db.put(key, val, function(res) {
  assert.equal(0, res.code, res.message);
  db.del(key, function(res) {
    assert.equal(0, res.code, res.message);
    db.get(key, function(res, data) {
      assert.equal(BDB.DB_NOTFOUND, res.code, res.message);
      exec("rm -fr " + env_location, function(err, stdout, stderr) {});
      console.log('test_del: PASSED');
    });
  });
});
