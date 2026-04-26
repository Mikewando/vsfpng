project('fpng Vapoursynth Plugin', 'cpp',
  default_options: ['buildtype=release', 'warning_level=2', 'b_ndebug=if-release', 'cpp_std=c++20'],
  license: 'LGPL-2.1-or-later',
  meson_version: '>=0.64.0',
  version: '1.2'
)

sources = [
  'src/plugin.cpp'
]

if host_machine.cpu_family().startswith('x86')
  add_project_arguments('-mfpmath=sse', '-msse2', language: 'cpp')
endif

if host_machine.system() == 'windows'
  add_project_link_arguments('-lws2_32', language: 'cpp')
endif

libs = []

libs += static_library('p2p_main',
    [
        'src/p2p_api.cpp',
        'src/v210.cpp'
    ],
    cpp_args: ['-std=c++20']
)

if host_machine.cpu_family().startswith('x86')
  fpng_cpp_args = ['-std=c++20', '-DFPNG_NO_SSE=0', '-msse4.1', '-mpclmul', '-fno-strict-aliasing']
else
  fpng_cpp_args = ['-std=c++20', '-DFPNG_NO_SSE=1', '-fno-strict-aliasing']
endif

libs += static_library('fpng',
    [
        'src/fpng.cpp',
    ],
    cpp_args: fpng_cpp_args
)

py = import('python').find_installation(pure: false)

# prefer submodule
vsinclude = 'vapoursynth/include'

# fallback to python (won't be present in isolated builds)
fs = import('fs')
if not fs.is_dir(vsinclude)
  vsinclude = run_command(py, '-c', 'import vapoursynth as vs; print(vs.get_include())', check: true).stdout().strip()
endif

shared_module('fpng',
  sources,
  include_directories: ['src', vsinclude],
  install: true,
  install_dir: py.get_install_dir() / 'vapoursynth/plugins',
  link_with: libs,
  gnu_symbol_visibility: 'hidden'
)

py.install_sources(
  'python/vsfpng.py'
)
