// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var bindings = require('../build/default/bdb_bindings');
var Db = require('./db').Db;
var DbEnv = require('./env').DbEnv;

exports.Db = Db;
exports.DbEnv = DbEnv;
exports.FLAGS = bindings;
