#!/usr/bin/env python
## -*- Mode: python; py-indent-offset: 2; indent-tabs-mode: nil; coding: utf-8; -*-

import os, subprocess, sys
import Options, Utils
from os import unlink, symlink, chdir, popen, system
from os.path import exists, join

VERSION = '0.1'
REVISION = popen("git log | head -n1 | awk '{printf \"%s\", $2}'").readline()

cwd = os.getcwd()
bdb_root = cwd + '/deps/db-5.1.25'
bdb_bld_dir = bdb_root + '/build_unix'
srcdir = '.'
blddir = 'build'
sys.path.append(cwd + '/tools')

def set_options(opt):
  opt.tool_options("compiler_cxx")
  opt.tool_options("compiler_cc")
  opt.tool_options('misc')
  opt.add_option('--debug',
                 action='store',
                 default=False,
                 help='Enable debug variant [Default: False]',
                 dest='debug')
  opt.add_option('--shared-bdb-includes',
                 action='store',
                 default=False,
                 help='Directory containing bdb header files',
                 dest='shared_bdb_includes')
  opt.add_option('--shared-bdb-libpath',
                 action='store',
                 default=False,
                 help='A directory to search for the shared BDB DLL',
                 dest='shared_bdb_libpath')

def configure(conf):
  conf.check_tool('compiler_cxx')
  if not conf.env.CXX: conf.fatal('c++ compiler not found')
  conf.check_tool("compiler_cc")
  if not conf.env.CC: conf.fatal('c compiler not found')
  conf.check_tool('node_addon')

  o = Options.options

  conf.env['USE_DEBUG'] = o.debug
  conf.env['USE_SHARED_BDB'] = o.shared_bdb_includes or o.shared_bdb_libpath

  if conf.env['USE_SHARED_BDB']:
    bdb_includes = []
    bdb_libpath = []
    if o.shared_bdb_includes: bdb_includes.append(o.shared_bdb_includes)
    if o.shared_bdb_libpath: bdb_libpath.append(o.shared_bdb_libpath)
    conf.check_cxx(lib='db-5',
                   header_name="db.h",
                   use_libstore='BDB',
                   includes=bdb_includes,
                   libpath=bdb_libpath,
                   mandatory=True)
    conf.env.append_value('CPPPATH', o.shared_bdb_includes)
    conf.env.append_value('LIBPATH', o.shared_bdb_libpath)
  else:
    os.chdir(bdb_bld_dir)
    subprocess.check_call(['../dist/configure',
                           '--enable-dtrace',
                           '--enable-perfmon-statistics',
                           '--with-cryptography=yes',
                           '--disable-shared',
                           '--disable-replication',
                           '--enable-cxx=no',
                           '--enable-java=no',
                           '--enable-sql=no',
                           '--enable-tcl=no'])
    conf.env.append_value('CPPPATH', bdb_bld_dir)
    conf.env.append_value('LIBPATH', bdb_bld_dir)
    os.chdir(cwd)

  conf.env.append_value('CXXFLAGS', ['-D_FILE_OFFSET_BITS=64',
                                     '-D_LARGEFILE_SOURCE',
                                     '-Wall',
                                     '-Werror'])
  if o.debug:
    conf.env.append_value('CXXFLAGS', ["-g"])
  else:
    conf.env.append_value('CXXFLAGS', ['-O3'])

def lint(ctx):
  dirname = cwd + '/src'
  for f in os.listdir(dirname):
    subprocess.check_call(['./tools/cpplint.py',
                           '--filter=-build/include,-build/header_guard,-runtime/rtti',
                           os.path.join(dirname, f)])
  dirname = cwd + '/test'
  for f in os.listdir(dirname):
    print 'jslint: ' + f
    subprocess.call(['jslint', os.path.join(dirname, f)])

def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.target = 'bdb_bindings'
  obj.source = './src/bdb_object.cc ./src/bdb_bindings.cc '
  obj.source += './src/bdb_env.cc ./src/bdb_db.cc ./src/bdb_cursor.cc '
  obj.source += './src/bdb_txn.cc '
  obj.name = "node-bdb"
  obj.defines = ['NODE_BDB_REVISION="' + REVISION + '"']

  if not bld.env['USE_SHARED_BDB']:
    os.chdir(bdb_bld_dir)
    subprocess.check_call(['make', '-j', '3'])
    os.chdir(cwd)
    obj.staticlib = "db-5.1"
  else:
    obj.lib = "db-5"

def tests(ctx):
  system('node test/test_open.js')
  system('node test/test_put.js')
  system('node test/test_get.js')
  system('node test/test_del.js')
  system('node test/test_cursor.js')
  system('node test/test_concurrent.js')

def distclean(ctx):
  os.chdir(bdb_bld_dir)
  os.popen('make distclean 2>&1 > /dev/null')
  os.chdir(cwd)
  os.popen('rm -rf .lock-wscript build')

def shutdown():
  t = 'bdb_bindings.node';
  if exists('build/default/' + t) and not exists(t):
    symlink('build/default/' + t, t)

