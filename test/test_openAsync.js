var assert = require('assert');
var exec  = require('child_process').exec;
var fs = require('fs');
var BDB = require('bdb');
var helper = require('./helper');

// setup
var env_location = "/tmp/" + helper.uuid();
fs.mkdirSync(env_location, 0750);

var env = new BDB.DbEnv();
var stat = env.open(env_location);
assert.equal(0, stat.code, 'env.open: ' + stat.message);
var db = new BDB.Db();
db.open(env, helper.uuid(), function(stat) {
    assert.equal(0, stat.code, 'db..open: ' + stat.message);
    exec("rm -fr " + env_location, function(err, stdout, stderr) {});
    console.log('test_openAsync: PASSED');
});
