// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var BDB = require('../build/default/bdb_bindings');
var DbEnv = BDB.DbEnv;

/**
 * Open a database environment
 *
 * Required:
 *
 * - 'home'  The Database home, given as a string
 *
 * Optional:
 *
 * - 'flags'   Bitwise OR'd BDB flags. Defaults are:
 *               - DB_CREATE
 *               - DB_INIT_LOCK
 *               - DB_INIT_LOG
 *               - DB_INIT_MPOOL
 *               - DB_INIT_TXN
 *               - DB_RECOVER
 *               - DB_THREAD
 * - 'mode'    Unix File Permissions to set. Default is 0660.
 *
 * @param {Object} options
 * @api public
 */
DbEnv.prototype.openSync = function(options) {
  var flags =
    BDB.DB_CREATE     |
    BDB.DB_INIT_LOCK  |
    BDB.DB_INIT_LOG   |
    BDB.DB_INIT_MPOOL |
    BDB.DB_INIT_TXN   |
    BDB.DB_RECOVER    |
    BDB.DB_THREAD;
  var mode = 0;
  if (!options) {
    throw new Error('options required');
  }
  if (!options.home) {
    throw new Error('options.home required');
  }
  if (options.flags) {
    flags = options.flags;
  }
  if (options.mode) {
    mode = options.mode;
  }
  return this._openSync(options.home, flags, mode);
};

/**
 * Close a database environment
 *
 * Optional:
 *
 * - 'flags'   Bitwise OR'd BDB flags. Defaults is DB_FORCESYNC
 *
 * @param {Object} options
 * @api public
 */
DbEnv.prototype.closeSync = function(options) {
  var flags = BDB.DB_FORCESYNC;
  if (options) {
    if (options.flags) {
      flags = options.flags;
    }
  }
  return this._closeSync(flags);
};

/**
 * TXN Checkpoint wrapper
 *
 * Optional:
 * - 'kbyte'   Perform checkpoint if MORE than kbyte kilbytes have been written
 * - 'min'     Perform checkpoint if MORE than min minutes have passed
 *
 * @param {Object} options
 * @param {Function} callback
 * @api public
 */
DbEnv.prototype.txnCheckpoint = function(options, callback) {
  var kbyte = 0;
  var min = 0;
  var flags = BDB.DB_FORCE;
  if (options) {
    if (options.kbyte) {
      kbyte = options.kbyte;
    }
    if (options.min) {
      min = options.min;
    }
    if (kbyte || min) {
      flags = 0;
    }
  }
  return this._txnCheckpoint(kbyte, min, flags, callback);
};

exports.DbEnv = DbEnv;
