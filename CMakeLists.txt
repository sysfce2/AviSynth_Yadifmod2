cmake_minimum_required(VERSION 3.16)

project(libyadifmod2 LANGUAGES CXX)

find_package (Git)
if (GIT_FOUND)
    execute_process (COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        OUTPUT_VARIABLE ver
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else ()
    message (STATUS "GIT not found")
endif ()

add_library(yadifmod2 SHARED
    src/proc_filter.cpp
    src/proc_filter_avx.cpp
    src/proc_filter_avx2.cpp
    src/proc_filter_sse2.cpp
    src/proc_filter_sse41.cpp
    src/proc_filter_ssse3.cpp
    src/yadifmod2.cpp
)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

message(STATUS "Build type - ${CMAKE_BUILD_TYPE}")

set_source_files_properties(src/proc_filter_sse2.cpp PROPERTIES COMPILE_OPTIONS "-mfpmath=sse;-msse2")
set_source_files_properties(src/proc_filter_ssse3.cpp PROPERTIES COMPILE_OPTIONS "-mssse3")
set_source_files_properties(src/proc_filter_sse41.cpp PROPERTIES COMPILE_OPTIONS "-msse4.1")
set_source_files_properties(src/proc_filter_avx.cpp PROPERTIES COMPILE_OPTIONS "-mavx")
set_source_files_properties(src/proc_filter_avx2.cpp PROPERTIES COMPILE_OPTIONS "-mavx2;-mfma")

target_include_directories(yadifmod2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_INSTALL_PREFIX}/include/avisynth
    /usr/local/include/avisynth
)

set_target_properties(yadifmod2 PROPERTIES OUTPUT_NAME "yadifmod2.${ver}")

include(GNUInstallDirs)

INSTALL(TARGETS yadifmod2
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")
        
# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
