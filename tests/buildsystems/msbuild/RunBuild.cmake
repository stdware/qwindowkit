# Builds the MSBuild consumer project against a staged install tree.
#
# cmake -D MSBUILD_EXECUTABLE=... -D PROJECT_FILE=... -D INSTALL_PREFIX=... -D WORK_DIR=...
#       -D BUILD_CONFIG=Debug|Release -D PLATFORM=x64 -D QT_PREFIX=... -D QT_MAJOR_VERSION=6
#       -D USE_WIDGETS=true|false -P msbuild/RunBuild.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../testing/ConsumerBuildCommon.cmake")

qwk_require_variables(MSBUILD_EXECUTABLE PROJECT_FILE INSTALL_PREFIX WORK_DIR BUILD_CONFIG
    PLATFORM QT_PREFIX QT_MAJOR_VERSION USE_WIDGETS
)

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

# The property sheet joins its own paths with backslashes, and so does the project file where it
# reaches under the prefix, so both of those go over in native form. OutDir and IntDir keep their
# forward slashes: MSBuild passes them on unchanged, and a value ending in a backslash is the one
# shape its command line escaping cannot carry.
file(TO_NATIVE_PATH "${INSTALL_PREFIX}" _native_prefix)
file(TO_NATIVE_PATH "${QT_PREFIX}" _native_qt_prefix)

qwk_run_step("msbuild" "${MSBUILD_EXECUTABLE}" "${PROJECT_FILE}"
    /nologo
    /verbosity:minimal
    "/p:Configuration=${BUILD_CONFIG}"
    "/p:Platform=${PLATFORM}"
    "/p:QWK_PREFIX=${_native_prefix}"
    "/p:QTDIR=${_native_qt_prefix}"
    "/p:QtVersionMajor=${QT_MAJOR_VERSION}"
    "/p:QwkUseWidgets=${USE_WIDGETS}"
    "/p:OutDir=${WORK_DIR}/bin/"
    "/p:IntDir=${WORK_DIR}/obj/"
)
