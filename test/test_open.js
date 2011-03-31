// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var assert = require('assert');
var exec  = require('child_process').exec;
var fs = require('fs');
var BDB = require('bdb');
var helper = require('./helper');

var stat = undefined;

// setup
var env_location = "/tmp/" + helper.uuid();
var env = new BDB.DbEnv();

fs.mkdirSync(env_location, 0750);
stat = env.openSync({home:env_location});
assert.equal(0, stat.code, stat.message);

var db = new BDB.Db(env);
stat = db.openSync({file: helper.uuid()});
assert.equal(0, stat.code, stat.message);
exec("rm -fr " + env_location, function(err, stdout, stderr) {});

console.log('test_open: PASSED');
