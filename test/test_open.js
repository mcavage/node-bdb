var assert = require('assert');
var exec  = require('child_process').exec;
var fs = require('fs');
var BDB = require('bdb');
var helper = require('./helper');

var stat = undefined;

// setup
var env_location = "/tmp/" + helper.uuid();
fs.mkdirSync(env_location, 0750);

var env = new BDB.DbEnv();
stat = env.open(env_location);
assert.equal(0, stat.code, stat.message);

var db = new BDB.Db();
stat = db.open(env, helper.uuid());
assert.equal(0, stat.code, stat.message);

// cleanup
exec("rm -fr " + env_location, function(err, stdout, stderr) {});
console.log('test_open: PASSED');
