#!/usr/bin/env python3
"""Keep automatic CPU dependencies without requiring every optional GPU driver."""
import fnmatch
from pathlib import PurePosixPath
import subprocess
import sys


def optional_gpu_plugin(argument):
    if argument.startswith('-'):
        return False
    path = PurePosixPath(argument)
    if path.parent.as_posix() != 'usr/share/libremax/runtime/blender/lib' and not path.parent.as_posix().endswith('/usr/share/libremax/runtime/blender/lib'):
        return False
    return any(fnmatch.fnmatchcase(path.name, pattern) for pattern in (
        'libOpenImageDenoise_device_cuda.so*',
        'libOpenImageDenoise_device_hip.so*',
        'libOpenImageDenoise_device_sycl.so*',
        'libur_adapter_level_zero.so*',
    ))


def main():
    arguments = [argument for argument in sys.argv[1:] if not optional_gpu_plugin(argument)]
    # CPack's --version and --help probes also reach the real distribution tool.
    return subprocess.call(['/usr/bin/dpkg-shlibdeps', *arguments])


if __name__ == '__main__':
    sys.exit(main())
