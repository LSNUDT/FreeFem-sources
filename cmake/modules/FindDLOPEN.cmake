INCLUDE(FindPackageHandleStandardArgs)
INCLUDE(PackageManagerPaths)

FIND_PATH(DLOPEN_INCLUDES NAMES dlfcn.h 
                          PATHS ${PACKMAN_INCLUDE_PATHS}) 

IF(DLOPEN_INCLUDES)
  SET(DLOPEN_FOUND True)
ENDIF(DLOPEN_INCLUDES)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(DLOPEN DEFAULT_MSG DLOPEN_INCLUDES)



