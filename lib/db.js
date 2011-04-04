// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var Buffer = require('buffer').Buffer;
var BDB = require('../build/default/bdb_bindings');
var Db = BDB.Db;

/**
 * Open a database
 *
 * Required:
 *
 * - 'file'  The Database file, given as a string
 *
 * Optional:
 * - 'type'    BDB Database Type (i.e., BTREE, HASH, ...). Default DB_BTREE
 * - 'flags'   Bitwise OR'd BDB flags. Defaults are:
 *               - DB_AUTO_COMMIT
 *               - DB_CREATE
 *               - DB_THREAD
 * - 'mode'    Unix File Permissions to set. Default is 0660.
 * - 'retries' if transactional DS, retry this many times if a DB_LOCK_DEADLOCK
 *             is encountered. Default is 1.
 *
 * @param {Object} options
 * @api public
 */
Db.prototype.openSync = function(options) {
  var type = BDB.DB_BTREE;
  var flags = BDB.DB_AUTO_COMMIT | BDB.DB_CREATE | BDB.DB_THREAD;
  var mode = 0;
  var retries = 1;
  if (!options) {
    throw new Error('options required');
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
  if (options.retries) {
    retries = options.retries;
  }
  return this._openSync(options.file, type, flags, mode, retries);
};


/**
 * Close a database
 *
 * Optional:
 * - 'flags'   Bitwise OR'd BDB flags. Default is 0.
 *
 * @param {Object} options
 * @api public
 */
Db.prototype.closeSync = function(options) {
  var flags = 0;
  if (options) {
    if (options.flags) {
      flags = options.flags;
    }
  }
  return this._closeSync(flags);
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
  return this._put(options.key, options.val, flags, callback);
};


/**
 * DB PutIf wrapper
 *
 * Required:
 * - 'key'     Database key to insert (Buffer)
 * - 'val'     Corresponding value (Buffer)
 * - 'oldVal'  value to assert database has (Buffer)
 *
 *
 * Optional:
 * - 'flags'   Optional Flags: Default is 0
 *
 * Note that this API doesn't exist in core BDB. I just added it so that
 * one can work around the lack of transactions in node.js bindings.
 *
 * @param {Object} options
 * @param {Function} callback
 * @api public
 */
Db.prototype.putIf = function(options, callback) {
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
  if (!options.oldVal) {
    throw new Error('options.oldVal required');
  }
  if (options.flags) {
    flags = options.flags;
  }
  return this._putIf(options.key, options.val, options.oldVal, flags, callback);
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
  return this._putSync(options.key, options.val, flags);
};


/**
 * DB Get wrapper
 *
 * Required:
 * - 'key'     Database key to fetch (Buffer)
 *
 *
 * Optional:
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
  return this._get(options.key, flags, callback);
};

/**
 * DB GetSync wrapper
 *
 * Required:
 * - 'key'     Database key to fetch (Buffer)
 *
 *
 * Optional:
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
  return this._getSync(options.key, flags);
};



/**
 * DB CursorGetSync wrapper
 *
 * @param {Object} options
 * Optional:
 * - 'key'
 * - 'limit'
 * - 'initFlag' Optional: Default is DB_SET
 * - 'flags'    Optional: Default is DB_NEXT
 * @param {Function} callback
 * @api public
 */
Db.prototype.cursorGet = function(options, callback) {
  var key;
  var limit = 100;
  var initFlag = BDB.DB_SET;
  var flags = BDB.DB_NEXT;

  if ((typeof options) === 'function') {
    callback = options;
  }

  if (options) {
    if (options.key) {
      key = options.key;
    }
    if (options.initFlag) {
      initFlag = options.initFlag;
    }
    if (options.flags) {
      flags = options.flags;
    }
    if (options.limit) {
      limit = options.limit;
    }
  }

  if (!key) {
    key = new Buffer(0);
  }
  return this._cursorGet(key, limit, initFlag, flags, callback);
};


/**
 * DB CursorGetSync wrapper
 *
 * Optional:
 * - 'key'
 * - 'limit'
 * - 'initFlag' Optional: Default is DB_SET
 * - 'flags'    Optional: Default is DB_NEXT
 *
 * @param {Object} options
 * @api public
 */
Db.prototype.cursorGetSync = function(options) {
  var key;
  var limit = 100;
  var initFlag = BDB.DB_SET;
  var flags = BDB.DB_NEXT;

  if (options) {
    if (options.limit) {
      limit = options.limit;
    }
    if (options.key) {
      key = options.key;
    }
    if (options.initFlag) {
      initFlag = options.initFlag;
    }
    if (options.flags) {
      flags = options.flags;
    }
  }

  if (!key) {
    key = new Buffer(0);
  }
  return this._cursorGetSync(key, limit, initFlag, flags);
};


/**
 * DB Delete wrapper
 *
 * Required:
 * - 'key'     Database key to delete (Buffer)
 *
 *
 * Optional:
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
  return this._del(options.key, flags, callback);
};

/**
 * DB Delete Sync wrapper
 *
 * Required:
 * - 'key'     Database key to delete (Buffer)
 *
 *
 * Optional:
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
  return this._delSync(options.key, flags);
};

exports.Db = Db;
