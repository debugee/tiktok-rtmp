cmake_minimum_required(VERSION 3.31)

project(untitled1 VERSION 0.1 LANGUAGES C CXX RC)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install" CACHE PATH "install path" FORCE)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Concurrent Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Concurrent Sql)

set(TS_FILES untitled1_zh_CN.ts untitled1_en_US.ts)

set(DecoderFiles
    decoder/fetchdecode32.cc
    decoder/fetchdecode64.cc
)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    ${InjectFiles}
    mainwindow.ui
    ${TS_FILES}
    $<$<PLATFORM_ID:Windows>:inject.cpp;${DecoderFiles}>
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(untitled1
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

    # Define target properties for Android with Qt 6 as:
    # set_property(TARGET untitled1 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
    # ${CMAKE_CURRENT_SOURCE_DIR}/android)
    # For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(untitled1 SHARED
            ${PROJECT_SOURCES}
        )

    # Define properties for Android with Qt 5 after find_package() calls as:
    # set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(untitled1
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(untitled1 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Sql)

if(WIN32)
    target_compile_definitions(untitled1 PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        UNICODE
        _UNICODE
        BX_STANDALONE_DECODER
        BX_INSTR_STORE_OPCODE_BYTES
    )

    if(MINGW)
        target_sources(untitled1 PRIVATE untitled.rc)
    else()
        set_target_properties(untitled1 PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
    endif()

    target_link_libraries(untitled1 PRIVATE shlwapi.lib)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.untitled1)
endif()

set_target_properties(untitled1 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS untitled1
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(untitled1)
endif()

qt_add_resources(untitled1 "ts"
    PREFIX "/i18n"
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES ${QM_FILES})

target_compile_definitions(untitled1 PRIVATE $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>)

# set(CODE_SIGN_IDENTITY "Apple Development: Wen Cheng (Y9UU5S8ZW7)")
# find_program(DEPLOY macdeployqt6)
# add_custom_target(postBuild ALL
# COMMAND
# ${DEPLOY} $<TARGET_BUNDLE_DIR:untitled1> -sign-for-notarization=${CODE_SIGN_IDENTITY} -dmg
# VERBATIM
# )

# target_link_options(untitled1 PRIVATE -ld_classic)

# ################tiktok################
set(_unicode TRUE)
set(_console FALSE)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
add_library(tiktok SHARED tiktok.cpp hooklib.cpp ${DecoderFiles})
set_target_properties(tiktok PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(tiktok PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    _USRDLL
    BX_STANDALONE_DECODER
    BX_INSTR_STORE_OPCODE_BYTES
)

if(MINGW)
    target_link_libraries(tiktok PRIVATE libwinpthread.a libc++.a libunwind.a)
    target_link_options(tiktok PRIVATE -Wl,--pdb=)
else()
    set_target_properties(tiktok PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(_unicode)
    target_compile_definitions(tiktok PRIVATE UNICODE _UNICODE)

    if(MINGW)
        target_link_options(tiktok PRIVATE -municode)
    endif()
endif()

if(_console)
    target_compile_definitions(tiktok PRIVATE _CONSOLE)
else()
    target_compile_definitions(tiktok PRIVATE _WINDOWS)

    if(MINGW)
        target_link_options(tiktok PRIVATE -mwindows)
    else()
        target_link_options(tiktok PRIVATE /SUBSYSTEM:WINDOWS)
    endif()
endif()

set_target_properties(tiktok PROPERTIES PREFIX "")

target_compile_options(tiktok PRIVATE "$<$<C_COMPILER_ID:MSVC>:/source-charset:utf-8>")

target_compile_options(tiktok PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8>")

add_library(aaaa SHARED antidebug.cpp hooklib.cpp ${DecoderFiles})
set_target_properties(aaaa PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(aaaa PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    _USRDLL
    BX_STANDALONE_DECODER
    BX_INSTR_STORE_OPCODE_BYTES
)

if(MINGW)
    target_link_libraries(aaaa PRIVATE libwinpthread.a libc++.a libunwind.a)
    target_link_options(aaaa PRIVATE -Wl,--pdb=)
else()
    set_target_properties(aaaa PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(_unicode)
    target_compile_definitions(aaaa PRIVATE UNICODE _UNICODE)

    if(MINGW)
        target_link_options(aaaa PRIVATE -municode)
    endif()
endif()

if(_console)
    target_compile_definitions(aaaa PRIVATE _CONSOLE)
else()
    target_compile_definitions(aaaa PRIVATE _WINDOWS)

    if(MINGW)
        target_link_options(aaaa PRIVATE -mwindows)
    else()
        target_link_options(aaaa PRIVATE /SUBSYSTEM:WINDOWS)
    endif()
endif()

set_target_properties(aaaa PROPERTIES PREFIX "")

target_compile_options(aaaa PRIVATE "$<$<C_COMPILER_ID:MSVC>:/source-charset:utf-8>")

target_compile_options(aaaa PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8>")

add_executable(epinject
    epinject.cpp
    charset.cpp
    winprocess.cpp
    process.cpp
    injectlib.cpp
    ${DecoderFiles}
)

target_compile_definitions(epinject PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    UNICODE
    _UNICODE
    BX_STANDALONE_DECODER
    BX_INSTR_STORE_OPCODE_BYTES
)

if(MINGW)
    target_link_libraries(epinject PRIVATE libwinpthread.a libc++.a libunwind.a)
    target_link_options(epinject PRIVATE -Wl,--pdb=)
else()
    set_target_properties(epinject PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(_unicode)
    target_compile_definitions(epinject PRIVATE UNICODE _UNICODE)

    if(MINGW)
        target_link_options(epinject PRIVATE -municode)
    endif()
endif()

target_compile_definitions(epinject PRIVATE _CONSOLE)

# ##################################install dependencies##############################
if("${CMAKE_SYSTEM_NAME}x" STREQUAL "${CMAKE_HOST_SYSTEM_NAME}x")
    # set(deploy_tool_options_arg "")
    # if(APPLE)
    # set(deploy_tool_options_arg --hardened-runtime)
    # elseif(WIN32)
    # set(deploy_tool_options_arg --no-compiler-runtime)
    # endif()
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(deploy_tool_options_arg --no-compiler-runtime)
    endif()

    # Generate a deployment script to be executed at install time
    qt_generate_deploy_script(
        TARGET untitled1
        OUTPUT_SCRIPT deploy_script
        CONTENT "
qt_deploy_runtime_dependencies(
    EXECUTABLE \"$<TARGET_FILE:untitled1>\"
    DEPLOY_TOOL_OPTIONS \"${deploy_tool_options_arg}\"
    GENERATE_QT_CONF
    VERBOSE
)")

    install(SCRIPT ${deploy_script})
endif()

install(TARGETS tiktok aaaa epinject)

# install(TARGETS tiktok
# RUNTIME_DEPENDENCIES
# PRE_EXCLUDE_REGEXES "^ext.+\\.dll" "^api.+\\.dll"
# POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" ".*syswow64/.*\\.dll"
# RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/librtmp.dll DESTINATION ${CMAKE_INSTALL_BINDIR})