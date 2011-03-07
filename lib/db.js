// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var BDB = require('../build/default/bdb_bindings');
var Db = BDB.Db;

/**
 * Open a database
 *
 * Required:
 *
 * - 'env'   The Database environment (Object)
 * - 'file'  The Database file, given as a string
 *
 * Optional:
 * - 'type'    BDB Database Type (i.e., BTREE, HASH, ...). Default DB_BTREE
 * - 'flags'   Bitwise OR'd BDB flags. Defaults are:
 *               - DB_AUTO_COMMIT
 *               - DB_CREATE
 *               - DB_THREAD
 * - 'mode'    Unix File Permissions to set. Default is 0660.
 *
 * @param {Object} options
 * @api public
 */
Db.prototype.openSync = function(options) {
  var type = BDB.DB_BTREE;
  var flags = BDB.DB_AUTO_COMMIT | BDB.DB_CREATE | BDB.DB_THREAD;
  var mode = 0;
  if (!options) {
    throw new Error('options required');
  }
  if (!options.env) {
    throw new Error('options.env required');
  }
  if (!options.file) {
    throw new Error('options.file required');
  }
  if (options.type) {
    type = options.type;
  }
  if (options.flags) {
    flags = options.flags;
  }
  if (options.mode) {
    mode = options.mode;
  }
  return this._openSync(options.env, options.file, type, flags, mode);
};


/**
 * DB Put wrapper
 *
 * Required:
 * - 'key'     Database key to insert (Buffer)
 * - 'val'     Corresponding value (Buffer)
 *
 *
 * Optional:
 * - 'txn'     Transaction handle
 * - 'flags'   Optional Flags: Default is 0
 *
 * @param {Object} options
 * @param {Function} callback
 * @api public
 */
Db.prototype.put = function(options, callback) {
  var flags = 0;
  if (!options) {
    throw new Error('options required');
  }
  if (!options.key) {
    throw new Error('options.key required');
  }
  if (!options.val) {
    throw new Error('options.val required');
  }
  if (options.flags) {
    flags = options.flags;
  }
  return this._put(options.txn, options.key, options.val, flags, callback);
};


/**
 * DB Put Sync wrapper
 *
 * Required:
 * - 'key'     Database key to insert (Buffer)
 * - 'val'     Corresponding value (Buffer)
 *
 *
 * Optional:
 * - 'txn'     Transaction handle
 * - 'flags'   Optional Flags: Default is 0
 *
 * @param {Object} options
 * @param {Function} callback
 * @api public
 */
Db.prototype.putSync = function(options) {
  var flags = 0;
  if (!options) {
    throw new Error('options required');
  }
  if (!options.key) {
    throw new Error('options.key required');
  }
  if (!options.val) {
    throw new Error('options.val required');
  }
  if (options.flags) {
    flags = options.flags;
  }
  return this._putSync(options.txn, options.key, options.val, flags);
};


/**
 * DB Get wrapper
 *
 * Required:
 * - 'key'     Database key to fetch (Buffer)
 *
 *
 * Optional:
 * - 'txn'     Transaction handle
 * - 'flags'   Optional Flags: Default is 0
 *
 * @param {Object} options
 * @param {Function} callback
 * @api public
 */
Db.prototype.get = function(options, callback) {
  var flags = 0;
  if (!options) {
    throw new Error('options required');
  }
  if (!options.key) {
    throw new Error('options.key required');
  }
  if (options.flags) {
    flags = options.flags;
  }
  return this._get(options.txn, options.key, flags, callback);
};

/**
 * DB GetSync wrapper
 *
 * Required:
 * - 'key'     Database key to fetch (Buffer)
 *
 *
 * Optional:
 * - 'txn'     Transaction handle
 * - 'flags'   Optional Flags: Default is 0
 *
 * @param {Object} options
 * @api public
 */
Db.prototype.getSync = function(options) {
  var flags = 0;
  if (!options) {
    throw new Error('options required');
  }
  if (!options.key) {
    throw new Error('options.key required');
  }
  if (options.flags) {
    flags = options.flags;
  }
  return this._getSync(options.txn, options.key, flags);
};

/**
 * DB Delete wrapper
 *
 * Required:
 * - 'key'     Database key to delete (Buffer)
 *
 *
 * Optional:
 * - 'txn'     Transaction handle
 * - 'flags'   Optional Flags: Default is 0
 *
 * @param {Object} options
 * @param {Function} callback
 * @api public
 */
Db.prototype.del = function(options, callback) {
  var flags = 0;
  if (!options) {
    throw new Error('options required');
  }
  if (!options.key) {
    throw new Error('options.key required');
  }
  if (options.flags) {
    flags = options.flags;
  }
  return this._del(options.txn, options.key, flags, callback);
};

/**
 * DB Delete Sync wrapper
 *
 * Required:
 * - 'key'     Database key to delete (Buffer)
 *
 *
 * Optional:
 * - 'txn'     Transaction handle
 * - 'flags'   Optional Flags: Default is 0
 *
 * @param {Object} options
 * @api public
 */
Db.prototype.delSync = function(options) {
  var flags = 0;
  if (!options) {
    throw new Error('options required');
  }
  if (!options.key) {
    throw new Error('options.key required');
  }
  if (options.flags) {
    flags = options.flags;
  }
  return this._delSync(options.txn, options.key, flags);
};


/**
 * DB Cursor wrapper
 *
 * Required:
 * - 'cursor'  Database cursor (Object)
 *
 *
 * Optional:
 * - 'txn'     Transaction handle
 * - 'flags'   Optional Flags: Default is 0
 *
 * @param {Object} options
 * @api public
 */
Db.prototype.cursor = function(options) {
  var flags = 0;
  if (!options) {
    throw new Error('options required');
  }
  if (!options.cursor) {
    throw new Error('options.cursor required');
  }
  if (options.flags) {
    flags = options.flags;
  }
  return this._cursor(options.txn, options.cursor, flags);
};


exports.Db = Db;
