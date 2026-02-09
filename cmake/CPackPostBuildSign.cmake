if(NOT WIN32)
  return()
endif()

if(NOT DEFINED CPACK_PACKAGE_FILES OR CPACK_PACKAGE_FILES STREQUAL "")
  message(STATUS "Signing skipped: CPACK_PACKAGE_FILES is empty.")
  return()
endif()

if(NOT DEFINED CPACK_VPFM_SIGNTOOL_PATH OR CPACK_VPFM_SIGNTOOL_PATH STREQUAL "")
  message(WARNING "Signing skipped: CPACK_VPFM_SIGNTOOL_PATH is not set.")
  return()
endif()

if(NOT DEFINED CPACK_VPFM_SIGN_CERT_THUMBPRINT OR CPACK_VPFM_SIGN_CERT_THUMBPRINT STREQUAL "")
  message(WARNING "Signing skipped: CPACK_VPFM_SIGN_CERT_THUMBPRINT is not set.")
  return()
endif()

set(_timestamp_url "${CPACK_VPFM_SIGN_TIMESTAMP_URL}")
if(_timestamp_url STREQUAL "")
  set(_timestamp_url "http://timestamp.digicert.com")
endif()

set(_fd "${CPACK_VPFM_SIGN_FILE_DIGEST}")
if(_fd STREQUAL "")
  set(_fd "sha256")
endif()

set(_td "${CPACK_VPFM_SIGN_TIMESTAMP_DIGEST}")
if(_td STREQUAL "")
  set(_td "sha256")
endif()

foreach(_artifact IN LISTS CPACK_PACKAGE_FILES)
  get_filename_component(_ext "${_artifact}" EXT)
  string(TOLOWER "${_ext}" _ext_lower)

  if(_ext_lower STREQUAL ".zip")
    message(STATUS "Skipping signing for ZIP artifact: ${_artifact}")
    continue()
  endif()

  if(NOT EXISTS "${_artifact}")
    message(WARNING "Skipping signing for missing artifact: ${_artifact}")
    continue()
  endif()

  message(STATUS "Signing artifact: ${_artifact}")
  execute_process(
    COMMAND "${CPACK_VPFM_SIGNTOOL_PATH}"
      sign
      /sha1 "${CPACK_VPFM_SIGN_CERT_THUMBPRINT}"
      /fd "${_fd}"
      /td "${_td}"
      /tr "${_timestamp_url}"
      /d "VideoPlayerForMe"
      /v
      "${_artifact}"
    RESULT_VARIABLE _sign_result
    OUTPUT_VARIABLE _sign_stdout
    ERROR_VARIABLE _sign_stderr
  )

  if(NOT _sign_result EQUAL 0)
    message(FATAL_ERROR
      "Failed to sign artifact ${_artifact}\n"
      "Exit code: ${_sign_result}\n"
      "stdout:\n${_sign_stdout}\n"
      "stderr:\n${_sign_stderr}\n"
    )
  endif()

  message(STATUS "Signed artifact: ${_artifact}")
endforeach()
