cmake_minimum_required(VERSION 3.21)

project(ndi-streamer)

set(CMAKE_C_STANDARD 17)

if (NOT WIN32)
  set(CMAKE_C_FLAGS "-O2 -Wall -Wextra")
  set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra")
else ()
  set(CMAKE_C_FLAGS "-O2 -Wall /experimental:c11atomics")
  set(CMAKE_C_FLAGS_DEBUG "-g -Wall /experimental:c11atomics")
endif ()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
if (APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
elseif (UNIX)
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
endif ()

find_package(NDI REQUIRED)
find_package(FFMPEG REQUIRED COMPONENTS avutil avformat avcodec swscale swresample)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
set(INCLUDE_DIRS ${NDI_INCLUDE_DIR})

if (WIN32)
  file(GLOB SOURCES_WIN ${CMAKE_CURRENT_SOURCE_DIR}/src/windows/*.c)
  list(APPEND SOURCES ${SOURCES_WIN})
  list(APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/windows)
endif ()

add_executable(ndi-streamer ${SOURCES})
target_include_directories(ndi-streamer PRIVATE ${INCLUDE_DIRS})
target_link_libraries(ndi-streamer PRIVATE ${NDI_LIBS}
    FFMPEG::avutil FFMPEG::avformat FFMPEG::avcodec
    FFMPEG::swscale FFMPEG::swresample)

if (WIN32)
  install(TARGETS ndi-streamer RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/ndi-streamer)
  file(GLOB DLLS "${FFMPEG_ROOT}/bin/*.dll")
  list(APPEND DLLS "${NDI_DIR}/Bin/${NDI_ARCH}/Processing.NDI.Lib.${NDI_ARCH}.dll")
  install(FILES ${DLLS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ndi-streamer)
elseif (APPLE)
  install(TARGETS ndi-streamer RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
  install(FILES "${NDI_LIBRARY_DIR}/libndi.dylib" DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
elseif (UNIX)
  file(GLOB LIBS "${NDI_LIBRARY_DIR}/libndi.so*")
  install(TARGETS ndi-streamer RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
  install(FILES ${LIBS} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif ()