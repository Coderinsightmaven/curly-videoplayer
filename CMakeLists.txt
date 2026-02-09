cmake_minimum_required(VERSION 3.21)

project(VideoPlayerForMe VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

option(VPFM_ENABLE_STRICT_WARNINGS "Enable strict compiler warnings for project sources." ON)
option(VPFM_ENABLE_SANITIZERS "Enable ASan/UBSan for Debug builds (Clang/GCC only)." OFF)
include(CTest)

function(vpfm_apply_quality_flags target_name)
  if(VPFM_ENABLE_STRICT_WARNINGS)
    if(MSVC)
      target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
      target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
  endif()

  if(VPFM_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${target_name} PRIVATE
      $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>
    )
    target_link_options(${target_name} PRIVATE
      $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>
    )
  endif()
endfunction()

option(VPFM_WINDOWS_SIGNING "Enable code signing for Windows packaging artifacts." OFF)
set(VPFM_SIGNTOOL_PATH "" CACHE FILEPATH "Absolute path to signtool.exe used for Windows signing.")
set(VPFM_SIGN_CERT_THUMBPRINT "" CACHE STRING "Certificate thumbprint used by signtool /sha1.")
set(VPFM_SIGN_TIMESTAMP_URL "http://timestamp.digicert.com" CACHE STRING "RFC3161 timestamp server URL.")
set(VPFM_SIGN_FILE_DIGEST "sha256" CACHE STRING "File digest algorithm passed to signtool /fd.")
set(VPFM_SIGN_TIMESTAMP_DIGEST "sha256" CACHE STRING "Timestamp digest algorithm passed to signtool /td.")

set(MPV_TARGET "")

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(MPV QUIET IMPORTED_TARGET mpv)
  if(TARGET PkgConfig::MPV)
    set(MPV_TARGET PkgConfig::MPV)
  endif()
endif()

if(NOT MPV_TARGET)
  find_path(MPV_INCLUDE_DIR mpv/client.h)
  find_library(MPV_LIBRARY NAMES mpv libmpv)

  if(NOT MPV_INCLUDE_DIR OR NOT MPV_LIBRARY)
    message(FATAL_ERROR "libmpv was not found. Install mpv dev files and retry.")
  endif()

  add_library(mpv::mpv UNKNOWN IMPORTED)
  set_target_properties(mpv::mpv PROPERTIES
    IMPORTED_LOCATION "${MPV_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${MPV_INCLUDE_DIR}"
  )
  set(MPV_TARGET mpv::mpv)
endif()

set(RTMIDI_TARGET "")
find_path(RTMIDI_INCLUDE_DIR RtMidi.h)
find_library(RTMIDI_LIBRARY NAMES rtmidi)
if(RTMIDI_INCLUDE_DIR AND RTMIDI_LIBRARY)
  add_library(rtmidi::rtmidi UNKNOWN IMPORTED)
  set_target_properties(rtmidi::rtmidi PROPERTIES
    IMPORTED_LOCATION "${RTMIDI_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${RTMIDI_INCLUDE_DIR}"
  )
  set(RTMIDI_TARGET rtmidi::rtmidi)
endif()

set(NDI_TARGET "")
find_path(NDI_INCLUDE_DIR Processing.NDI.Lib.h)
find_library(NDI_LIBRARY NAMES ndi ndi.5 ndi_shared Processing.NDI.Lib.x64)
if(NDI_INCLUDE_DIR AND NDI_LIBRARY)
  add_library(ndi::ndi UNKNOWN IMPORTED)
  set_target_properties(ndi::ndi PROPERTIES
    IMPORTED_LOCATION "${NDI_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${NDI_INCLUDE_DIR}"
  )
  set(NDI_TARGET ndi::ndi)
endif()

set(SOURCES
  src/main.cpp
  src/app/MainWindow.cpp
  src/core/CueListModel.cpp
  src/display/DisplayManager.cpp
  src/controllers/OutputRouter.cpp
  src/controllers/PlaybackController.cpp
  src/output/OutputWindow.cpp
  src/output/LayerSurface.cpp
  src/output/PreviewWindow.cpp
  src/output/EdgeBlendOverlay.cpp
  src/player/MpvPlayer.cpp
  src/project/ProjectSerializer.cpp
  src/control/OscServer.cpp
  src/control/MidiInputService.cpp
  src/ndi/NdiBridge.cpp
)

set(HEADERS
  src/app/MainWindow.h
  src/core/AppConfig.h
  src/core/Cue.h
  src/core/CueListModel.h
  src/core/Transition.h
  src/display/DisplayManager.h
  src/controllers/OutputRouter.h
  src/controllers/PlaybackController.h
  src/output/OutputWindow.h
  src/output/LayerSurface.h
  src/output/PreviewWindow.h
  src/output/EdgeBlendOverlay.h
  src/output/OutputCalibration.h
  src/player/IPlayer.h
  src/player/MpvPlayer.h
  src/project/ProjectSerializer.h
  src/control/OscServer.h
  src/control/MidiInputService.h
  src/ndi/NdiBridge.h
)

add_executable(VideoPlayerForMe MACOSX_BUNDLE WIN32
  ${SOURCES}
  ${HEADERS}
)

target_include_directories(VideoPlayerForMe PRIVATE src)

target_link_libraries(VideoPlayerForMe PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Network
  ${MPV_TARGET}
)

vpfm_apply_quality_flags(VideoPlayerForMe)

if(RTMIDI_TARGET)
  target_link_libraries(VideoPlayerForMe PRIVATE ${RTMIDI_TARGET})
  target_compile_definitions(VideoPlayerForMe PRIVATE HAVE_RTMIDI)
endif()

if(NDI_TARGET)
  target_link_libraries(VideoPlayerForMe PRIVATE ${NDI_TARGET})
  target_compile_definitions(VideoPlayerForMe PRIVATE HAVE_NDI_SDK)
endif()

if(WIN32)
  target_compile_definitions(VideoPlayerForMe PRIVATE NOMINMAX)
endif()

if(BUILD_TESTING)
  add_executable(VideoPlayerForMeSmokeTest
    tests/smoke_project_serializer.cpp
    src/project/ProjectSerializer.cpp
  )
  target_include_directories(VideoPlayerForMeSmokeTest PRIVATE src)
  target_link_libraries(VideoPlayerForMeSmokeTest PRIVATE Qt6::Core)
  vpfm_apply_quality_flags(VideoPlayerForMeSmokeTest)

  add_test(NAME project_serializer_smoke COMMAND VideoPlayerForMeSmokeTest)
endif()

include(GNUInstallDirs)
install(TARGETS VideoPlayerForMe
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
  qt_generate_deploy_app_script(
    TARGET VideoPlayerForMe
    OUTPUT_SCRIPT VPFM_QT_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
  )
  install(SCRIPT ${VPFM_QT_DEPLOY_SCRIPT})

  set(MPV_RUNTIME_DLL "" CACHE FILEPATH "Path to mpv runtime DLL (typically mpv-2.dll).")
  if(NOT MPV_RUNTIME_DLL)
    if(DEFINED MPV_LIBRARY AND MPV_LIBRARY)
      get_filename_component(_mpv_library_dir "${MPV_LIBRARY}" DIRECTORY)
      find_file(
        MPV_RUNTIME_DLL
        NAMES mpv-2.dll libmpv-2.dll mpv.dll
        HINTS
          "${_mpv_library_dir}"
          "${_mpv_library_dir}/../bin"
          "${_mpv_library_dir}/../../bin"
      )
    endif()
  endif()

  if(MPV_RUNTIME_DLL)
    message(STATUS "Using mpv runtime DLL: ${MPV_RUNTIME_DLL}")
    install(FILES "${MPV_RUNTIME_DLL}" DESTINATION ${CMAKE_INSTALL_BINDIR})
  else()
    message(WARNING "mpv runtime DLL was not found. Windows package may fail to start without mpv-2.dll.")
  endif()
endif()

set(CPACK_PACKAGE_NAME "VideoPlayerForMe")
set(CPACK_PACKAGE_VENDOR "VideoPlayerForMe")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform ProVideoPlayer-like cue and output tool")

if(APPLE)
  set(CPACK_GENERATOR "DragNDrop")
elseif(WIN32)
  set(CPACK_GENERATOR "ZIP;NSIS")
  set(CPACK_MONOLITHIC_INSTALL ON)
  set(CPACK_NSIS_PACKAGE_NAME "VideoPlayerForMe")
  set(CPACK_NSIS_DISPLAY_NAME "VideoPlayerForMe")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
  set(CPACK_NSIS_MODIFY_PATH ON)
else()
  set(CPACK_GENERATOR "ZIP")
endif()

if(WIN32 AND VPFM_WINDOWS_SIGNING)
  set(CPACK_VPFM_SIGNTOOL_PATH "${VPFM_SIGNTOOL_PATH}")
  set(CPACK_VPFM_SIGN_CERT_THUMBPRINT "${VPFM_SIGN_CERT_THUMBPRINT}")
  set(CPACK_VPFM_SIGN_TIMESTAMP_URL "${VPFM_SIGN_TIMESTAMP_URL}")
  set(CPACK_VPFM_SIGN_FILE_DIGEST "${VPFM_SIGN_FILE_DIGEST}")
  set(CPACK_VPFM_SIGN_TIMESTAMP_DIGEST "${VPFM_SIGN_TIMESTAMP_DIGEST}")
  set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_SOURCE_DIR}/cmake/CPackPostBuildSign.cmake")
endif()

include(CPack)
