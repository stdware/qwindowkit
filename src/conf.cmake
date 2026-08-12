# ----------------------------------
# Project Constants
# ----------------------------------
# Install the CMake package files, the public headers and the import libraries alongside the
# binaries. The generic helpers gate all of those on <proj>_DEVEL, which defaults to off.
set(QWINDOWKIT_DEVEL ON)

# The helpers read both halves of the pair, while the project has only ever had the static one as
# an option. Left alone they would fall back to `BUILD_SHARED_LIBS`, and a plain configure would
# quietly turn into a static build.
if(QWINDOWKIT_BUILD_STATIC)
    set(QWINDOWKIT_BUILD_SHARED off)
else()
    set(QWINDOWKIT_BUILD_SHARED on)
endif()

# Generated from the definitions added in `CMakeLists.txt`, and installed with the other headers.
set(QWINDOWKIT_CONFIG_HEADER_PATH QWKCore/qwkconfig.h)

set(QWINDOWKIT_INSTALL_CONFIG_TEMPLATE
    "${CMAKE_CURRENT_LIST_DIR}/${QWINDOWKIT_INSTALL_NAME}Config.cmake.in"
)

# The forwarding headers and the configuration header are written at configure time, so they can
# go under `QMSETUP_BUILD_DIR` only while it holds a plain path. The root sets one, and this is the
# fallback for a parent project that left the `$<CONFIG>` carrying default in place.
string(GENEX_STRIP "${QMSETUP_BUILD_DIR}" _stripped_build_dir)

if(NOT _stripped_build_dir STREQUAL "${QMSETUP_BUILD_DIR}")
    set(QWINDOWKIT_BUILD_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)
endif()

if(QWINDOWKIT_INSTALL)
    # The headers go one directory deeper than the helpers would put them, under a directory named
    # after the package, so that an include reads `<QWKCore/qwkglobal.h>`.
    set(QWINDOWKIT_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/${QWINDOWKIT_INSTALL_NAME})

    # The helpers hardcode bin and lib. `share/install.cmake` fills the generated qmake and MSBuild
    # files in from CMAKE_INSTALL_LIBDIR, so leaving these out puts the libraries in one directory
    # and sends every consumer that is not using CMake to another. Both build system tests go red
    # under -DCMAKE_INSTALL_LIBDIR=lib64 without them.
    set(QWINDOWKIT_INSTALL_RUNTIME_DIR ${CMAKE_INSTALL_BINDIR})
    set(QWINDOWKIT_INSTALL_LIBRARY_DIR ${CMAKE_INSTALL_LIBDIR})
    set(QWINDOWKIT_INSTALL_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${QWINDOWKIT_INSTALL_NAME})
endif()

function(_qwk_common_configure_target _target)
    qm_add_win_rc(${_target}
        NAME ${QWINDOWKIT_INSTALL_NAME}
        DESCRIPTION "${QWINDOWKIT_DESCRIPTION}"
        COPYRIGHT "${QWINDOWKIT_COPYRIGHT}"
    )

    get_target_property(_type ${_target} TYPE)

    if(NOT _type MATCHES "_LIBRARY$")
        return()
    endif()

    # `QWKCore` is `QWindowKit::Core` to whoever links it, from this build tree as much as from the
    # installed package.
    if(${_target} MATCHES "^QWK(.+)")
        set(_name ${CMAKE_MATCH_1})
        set_target_properties(${_target} PROPERTIES EXPORT_NAME ${_name})
    else()
        set(_name ${_target})
    endif()

    add_library(${QWINDOWKIT_INSTALL_NAMESPACE}::${_name} ALIAS ${_target})
endfunction()

set(QWINDOWKIT_POST_CONFIGURE_COMMANDS _qwk_common_configure_target)

# ----------------------------------
# Include Build Helpers
# ----------------------------------
qm_import(private/BuildSystem)
qm_setup_build_repo_helpers(qwk)

# Found here, at directory scope. The helpers add their targets from inside a function, where a
# `find_package` leaves the targets behind but takes its variables with it, and Qt 5 sets
# `Qt5Gui_PRIVATE_INCLUDE_DIRS` only on the call that creates the targets.
qm_find_qt(Core Gui)

if(QWINDOWKIT_BUILD_WIDGETS)
    qm_find_qt(Widgets)
endif()

if(QWINDOWKIT_BUILD_QUICK)
    qm_find_qt(Quick)
endif()
