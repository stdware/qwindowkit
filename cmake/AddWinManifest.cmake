#[[
# Embeds an application manifest into a Windows executable.
#
# qwk_add_win_manifest(<target>
#     [NAME <name>]
#     [VERSION <version>]
#     [DESCRIPTION <description>]
# )
#
# NAME defaults to the target name, VERSION to PROJECT_VERSION, and DESCRIPTION to
# PROJECT_DESCRIPTION and then to NAME. The manifest runs the executable as the invoking user,
# enables version 6 of the common controls, declares support for Windows Vista to Windows 11, and
# enables per-monitor DPI awareness, long paths and the segment heap. The function has no effect
# on other platforms and on other target types.
#
# Requires cmake/WriteIfDifferent.cmake.
#]]
function(qwk_add_win_manifest _target)
    if(NOT WIN32)
        return()
    endif()

    if(NOT TARGET ${_target})
        message(FATAL_ERROR "qwk_add_win_manifest: '${_target}' is not a target")
    endif()

    set(options)
    set(oneValueArgs NAME VERSION DESCRIPTION)
    set(multiValueArgs)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(FUNC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qwk_add_win_manifest: unknown arguments: ${FUNC_UNPARSED_ARGUMENTS}")
    endif()

    get_target_property(_type ${_target} TYPE)

    if(NOT _type STREQUAL "EXECUTABLE")
        return()
    endif()

    if(NOT FUNC_NAME)
        set(FUNC_NAME "${_target}")
    endif()

    if(NOT FUNC_VERSION)
        set(FUNC_VERSION "${PROJECT_VERSION}")
    endif()

    if(NOT FUNC_VERSION)
        set(FUNC_VERSION "0.0.0.0")
    endif()

    if(NOT FUNC_DESCRIPTION)
        set(FUNC_DESCRIPTION "${PROJECT_DESCRIPTION}")
    endif()

    if(NOT FUNC_DESCRIPTION)
        set(FUNC_DESCRIPTION "${FUNC_NAME}")
    endif()

    # An assembly version has exactly four components.
    string(REPLACE "." ";" _parts "${FUNC_VERSION}")
    list(LENGTH _parts _count)

    if(_count GREATER 4)
        message(FATAL_ERROR "qwk_add_win_manifest: VERSION has more than four components")
    endif()

    while(_count LESS 4)
        list(APPEND _parts 0)
        math(EXPR _count "${_count} + 1")
    endwhile()

    list(JOIN _parts "." _version)

    set(_manifest "${CMAKE_CURRENT_BINARY_DIR}/${_target}_manifest.exe.manifest")

    qwk_write_if_different("${_manifest}" "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>
<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">
    <description>${FUNC_DESCRIPTION}</description>
    <assemblyIdentity
        version=\"${_version}\"
        name=\"${FUNC_NAME}\"
        type=\"win32\"
    />

    <trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v3\">
        <security>
            <requestedPrivileges>
                <requestedExecutionLevel level=\"asInvoker\" uiAccess=\"false\" />
            </requestedPrivileges>
        </security>
    </trustInfo>

    <dependency>
        <dependentAssembly>
            <assemblyIdentity
                type=\"win32\"
                name=\"Microsoft.Windows.Common-Controls\"
                version=\"6.0.0.0\"
                processorArchitecture=\"*\"
                publicKeyToken=\"6595b64144ccf1df\"
                language=\"*\"
            />
        </dependentAssembly>
    </dependency>

    <compatibility xmlns=\"urn:schemas-microsoft-com:compatibility.v1\">
        <application>
            <!-- XAML Island won't work without the maxversiontested list  -->
            <!-- Windows 10 Version 1809 (October 2018 Update) -->
            <maxversiontested Id=\"10.0.17763.0\"/>
            <!-- Windows 10 Version 1903 (May 2019 Update) -->
            <maxversiontested Id=\"10.0.18362.0\"/>
            <!-- Windows 10 Version 1909 (November 2019 Update) -->
            <maxversiontested Id=\"10.0.18363.0\"/>
            <!-- Windows 10 Version 2004 (May 2020 Update) -->
            <maxversiontested Id=\"10.0.19041.0\"/>
            <!-- Windows 10 Version 20H2 (October 2020 Update) -->
            <maxversiontested Id=\"10.0.19042.0\"/>
            <!-- Windows 10 Version 21H1 (May 2021 Update) -->
            <maxversiontested Id=\"10.0.19043.0\"/>
            <!-- Windows 10 Version 21H2 (November 2021 Update) -->
            <maxversiontested Id=\"10.0.19044.0\"/>
            <!-- Windows 10 Version 22H2 -->
            <maxversiontested Id=\"10.0.19045.0\"/>
            <!-- Windows 11 Version 21H2 -->
            <maxversiontested Id=\"10.0.22000.0\"/>
            <!-- Windows 11 Version 22H2 -->
            <maxversiontested Id=\"10.0.22621.0\"/>
            <!-- Windows 11 Version 23H2 -->
            <maxversiontested Id=\"10.0.22631.0\"/>
            <!-- Windows 11 Version 24H2 -->
            <maxversiontested Id=\"10.0.26100.0\"/>
            <!-- Windows 11 Version 25H2 -->
            <maxversiontested Id=\"10.0.26200.0\"/>
            <!-- Windows Vista and Windows Server 2008 -->
            <supportedOS Id=\"{e2011457-1546-43c5-a5fe-008deee3d3f0}\"/>
            <!-- Windows 7 and Windows Server 2008 R2 -->
            <supportedOS Id=\"{35138b9a-5d96-4fbd-8e2d-a2440225f93a}\"/>
            <!-- Windows 8 and Windows Server 2012 -->
            <supportedOS Id=\"{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}\"/>
            <!-- Windows 8.1 and Windows Server 2012 R2 -->
            <supportedOS Id=\"{1f676c76-80e1-4239-95bb-83d0f6d0da78}\"/>
            <!-- Windows 10 and Windows 11 -->
            <supportedOS Id=\"{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}\"/>
        </application>
    </compatibility>

    <application xmlns=\"urn:schemas-microsoft-com:asm.v3\">
        <windowsSettings>
          <dpiAware xmlns=\"http://schemas.microsoft.com/SMI/2005/WindowsSettings\">True/PM</dpiAware>
          <printerDriverIsolation xmlns=\"http://schemas.microsoft.com/SMI/2011/WindowsSettings\">True</printerDriverIsolation>
          <disableWindowFiltering xmlns=\"http://schemas.microsoft.com/SMI/2011/WindowsSettings\">True</disableWindowFiltering>
          <dpiAwareness xmlns=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">PerMonitorV2, PerMonitor</dpiAwareness>
          <longPathAware xmlns=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">True</longPathAware>
          <heapType xmlns=\"http://schemas.microsoft.com/SMI/2020/WindowsSettings\">SegmentHeap</heapType>
        </windowsSettings>
      </application>
</assembly>")

    # clang with the GNU front end that targets *-windows-msvc sets neither MSVC nor MINGW. Its only
    # sign is the simulated compiler ID, and it links with the MSVC linker or lld-link as cl does.
    if(MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        if(CMAKE_GENERATOR MATCHES "Visual Studio")
            # The manifest is an input of the linker. /manifestuac:no prevents the linker from
            # writing a UAC fragment of its own, because the manifest already contains one.
            target_link_options(${_target} PRIVATE
                "/manifest" "/manifestinput:${_manifest}" "/manifestuac:no"
            )
        else()
            # CMake merges a .manifest source of an executable into the manifest that the linker
            # generates and embeds the result. Embedding the manifest through a resource script
            # instead would conflict with that default manifest. The option goes through LINKER:,
            # because clang with the GNU front end passes a bare /option to the linker as an input
            # file.
            target_sources(${_target} PRIVATE "${_manifest}")
            target_link_options(${_target} PRIVATE "LINKER:/manifestuac:no")
        endif()
    else()
        # Other toolchains embed no default manifest, so the manifest is embedded through a
        # resource script.
        set(_manifest_rc "${_manifest}.rc")
        qwk_write_if_different("${_manifest_rc}"
            "#include <windows.h>\n\n1 RT_MANIFEST \"${_manifest}\""
        )
        target_sources(${_target} PRIVATE "${_manifest_rc}")
    endif()
endfunction()
