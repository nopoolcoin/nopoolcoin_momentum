from distutils.core import setup, Extension

nopoolcoin_momentum_module = Extension('nopoolcoin_momentum',
                               libraries = ['ssl', 'crypto'],
                               sources = ['momentummodule.c',
                                          'groestl.c',
                                          'keccak.c',
                                          'momentum.c'],
                               include_dirs=['.'])

setup (name = 'nopoolcoin_momentum',
       version = '1.0',
       description = 'Bindings for AES-NI momentum proof of work used by nopoolcoin',
       ext_modules = [nopoolcoin_momentum_module])
