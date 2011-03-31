#!/usr/bin/env node

var exec  = require('child_process').exec;

exec('rm ' + process.installPrefix + '/include/node/db.h',
     function(err, stdout, stderr) {});

