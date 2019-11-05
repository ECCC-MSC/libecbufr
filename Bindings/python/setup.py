#
# build script for 'ecbufr' - Python libecbufr wrapper
#
import sys, os
from distutils.core import setup
from distutils.extension import Extension

libecbufrDir = os.path.dirname(os.getcwd())
libecbufrDir = os.path.dirname(libecbufrDir)
libecbufrIncludeDir = libecbufrDir + "/API/Headers"
libecbufrLibDir = libecbufrDir + "/API/Sources/.libs"

# we'd better have Cython installed, or it's a no-go
try:
    from Cython.Distutils import build_ext
except:
    print("libecbufr directory:", libecbufrDir )
    print("You don't seem to have Cython installed. Please get a")
    print("copy from www.cython.org and install it")
    sys.exit(1)


# scan the 'ecbufr' directory for extension files, converting
# them to extension names in dotted notation
def scandir(dir, files=[]):
    for file in os.listdir(dir):
        path = os.path.join(dir, file)
        if os.path.isfile(path) and path.endswith(".pyx"):
            files.append(path.replace(os.path.sep, ".")[:-4])
        elif os.path.isdir(path):
            scandir(path, files)
    return files


# generate an Extension object from its dotted name
def makeExtension(extName):
    extPath = extName.replace(".", os.path.sep)+".pyx"
    return Extension(
        extName,
        [extPath],
        include_dirs = [libecbufrIncludeDir, "."],   # adding the '.' to include_dirs is CRUCIAL!!
        library_dirs = [libecbufrLibDir, "."],   # adding the '.' to include_dirs is CRUCIAL!!
        extra_compile_args = ["-fPIC", "-Wall"],
        extra_link_args = ['-g'],
        libraries = ["ecbufr",],
        )

# get the list of extensions
extNames = scandir("ecbufr")

# and build up the set of Extension objects
extensions = [makeExtension(name) for name in extNames]

# finally, we can pass all this to distutils
setup(
  name="ecbufr",
  packages=["ecbufr"],
  ext_modules=extensions,
  cmdclass = {'build_ext': build_ext},
)
