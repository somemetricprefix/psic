#!/usr/bin/env python

VERSION = '0.1'
APPNAME = 'psic'

top = '.'
out = 'build'

import Options

def set_options(opt):
    opt.tool_options('compiler_cc')
    opt.add_option(
        '--debug',
        action = 'store_true',
        default = False,
        help = 'build debug variant [Default: False]',
        dest = 'debug'
    )
    opt.add_option(
        '--release',
        action = 'store_true',
        default = False,
        help = 'build release variant [Default: False]',
        dest = 'release'
    )

def configure(conf):
    conf.check_tool('compiler_cc')
    conf.check_cc(lib='ev')

    debug_env = conf.env.copy()
    debug_env.set_variant('debug')
    debug_env.append_value('CCFLAGS', ['-DDEBUG', '-g', '-O0', '-Wall', '-Wextra'])
    conf.set_env_name('debug', debug_env)

    release_env = conf.env.copy()
    release_env.set_variant('release')
    release_env.append_value('CCFLAGS', ['-DNDEBUG', '-O3'])
    conf.set_env_name('release', release_env)

def build(bld):
    psic = bld(
        features = ['cc', 'cprogram'],
        target = 'psic',
        source = bld.path.ant_glob('src/*.c'),
        lib = 'ev'
    )

    if Options.options.debug:
        psic.clone('debug')
    if Options.options.release:
        psic.clone('release')

