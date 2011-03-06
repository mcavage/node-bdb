// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var assert = require('assert');
var Buffer = require('buffer').Buffer;
var exec  = require('child_process').exec;
var fs = require('fs');
var BDB = require('bdb');
var helper = require('./helper');

// setup
var env_location = "/tmp/" + helper.uuid();
var env = new BDB.DbEnv();
var db = new BDB.Db();
var stat = 0;
var key = new Buffer(helper.uuid());
var val = new Buffer(helper.uuid());

fs.mkdirSync(env_location, 0750);
stat = env.openSync({home:env_location});
assert.equal(0, stat.code, stat.message);
stat = db.openSync({env: env, file: helper.uuid()});
assert.equal(0, stat.code, stat.message);

db.put({key: key, val: val}, function(res) {
  assert.equal(0, res.code, res.message);
  exec("rm -fr " + env_location, function(err, stdout, stderr) {});
  console.log('test_put: PASSED');
});
