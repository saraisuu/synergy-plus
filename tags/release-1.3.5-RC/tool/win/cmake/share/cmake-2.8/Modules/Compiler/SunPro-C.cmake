SET(CMAKE_SHARED_LIBRARY_C_FLAGS "-KPIC")
SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-G")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-R")
SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")
SET(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-h")

SET(CMAKE_C_VERBOSE_FLAG "-#")

SET(CMAKE_C_FLAGS_INIT "")
SET(CMAKE_C_FLAGS_DEBUG_INIT "-g")
SET(CMAKE_C_FLAGS_MINSIZEREL_INIT "-xO2 -xspace -DNDEBUG")
SET(CMAKE_C_FLAGS_RELEASE_INIT "-xO3 -DNDEBUG")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-g -xO2")

# Initialize C link type selection flags.  These flags are used when
# building a shared library, shared module, or executable that links
# to other libraries to select whether to use the static or shared
# versions of the libraries.
FOREACH(type SHARED_LIBRARY SHARED_MODULE EXE)
  SET(CMAKE_${type}_LINK_STATIC_C_FLAGS "-Bstatic")
  SET(CMAKE_${type}_LINK_DYNAMIC_C_FLAGS "-Bdynamic")
ENDFOREACH(type)

SET(CMAKE_C_CREATE_PREPROCESSED_SOURCE "<CMAKE_C_COMPILER> <DEFINES> <FLAGS> -E <SOURCE> > <PREPROCESSED_SOURCE>")
SET(CMAKE_C_CREATE_ASSEMBLY_SOURCE "<CMAKE_C_COMPILER> <DEFINES> <FLAGS> -S <SOURCE> -o <ASSEMBLY_SOURCE>")
