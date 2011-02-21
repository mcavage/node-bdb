var util = require('util');
var DbEnv = require('bdb').DbEnv;
var Db = require('bdb').Db;

var env = new DbEnv();

env.open("/tmp/foo", function(err) {
	console.log(util.inspect(err));
	var db = new Db();
	db.open(env, "test", function(err) {
		console.log(util.inspect(err));
	});
});
