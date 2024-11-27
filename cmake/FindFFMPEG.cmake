macro(_find_component arg_component arg_header)
  if (NOT WIN32)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
      pkg_check_modules(PC_${arg_component} QUIET IMPORTED_TARGET lib${arg_component})
    endif ()
  endif (NOT WIN32)

  find_path("FFMPEG_${arg_component}_INCLUDE_DIR"
      NAMES
      "lib${arg_component}/${arg_header}"
      PATHS
      ${PC_${arg_component}_INCLUDEDIR}
      ${PC_${arg_component}_INCLUDE_DIRS}
      "${FFMPEG_ROOT}/include"
      PATH_SUFFIXES
      ffmpeg
      DOC "FFMPEG's ${arg_component} include directory"
  )
  mark_as_advanced("FFMPEG_${arg_component}_INCLUDE_DIR")

  find_library("FFMPEG_${arg_component}_LIBRARY"
      NAMES
      "${arg_component}"
      PATHS
      ${PC_${arg_component}_LIBDIR}
      ${PC_${arg_component}_LIBRARY_DIRS}
      "${FFMPEG_ROOT}/lib"
      PATH_SUFFIXES
      ffmpeg
      DOC "FFMPEG's ${arg_component} library"
  )
  mark_as_advanced("FFMPEG_${arg_component}_LIBRARY")

  if (FFMPEG_${arg_component}_LIBRARY AND FFMPEG_${arg_component}_INCLUDE_DIR)
    set(_deps_found TRUE)
    set(_deps_link)
    foreach (_ffmpeg_dep IN LISTS ARGN)
      if (TARGET "FFMPEG::${_ffmpeg_dep}")
        list(APPEND _deps_link "FFMPEG::${_ffmpeg_dep}")
      else ()
        set(_deps_found FALSE)
      endif ()
    endforeach ()

    if (_deps_found)
      if (NOT TARGET "FFMPEG::${arg_component}")
        add_library("FFMPEG::${arg_component}" UNKNOWN IMPORTED)
        set_target_properties("FFMPEG::${arg_component}" PROPERTIES
            IMPORTED_LOCATION "${FFMPEG_${arg_component}_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_${arg_component}_INCLUDE_DIR}"
            IMPORTED_LINK_INTERFACE_LIBRARIES "${_deps_link}")
      endif ()
      set("FFMPEG_${arg_component}_FOUND" 1)

      set(version_header_path "${FFMPEG_${arg_component}_INCLUDE_DIR}/lib${arg_component}/version.h")
      if (EXISTS "${version_header_path}")
        string(TOUPPER "${arg_component}" component_upper)
        file(STRINGS "${version_header_path}" version
            REGEX "#define  *LIB${component_upper}_VERSION_(MAJOR|MINOR|MICRO) ")
        string(REGEX REPLACE ".*_MAJOR *\([0-9]*\).*" "\\1" major "${version}")
        string(REGEX REPLACE ".*_MINOR *\([0-9]*\).*" "\\1" minor "${version}")
        string(REGEX REPLACE ".*_MICRO *\([0-9]*\).*" "\\1" micro "${version}")
        if (NOT major STREQUAL "" AND
            NOT minor STREQUAL "" AND
            NOT micro STREQUAL "")
          set("FFMPEG_${arg_component}_VERSION" "${major}.${minor}.${micro}")
        endif ()
      endif ()
    else ()
      set("FFMPEG_${arg_component}_FOUND" 0)
      set(what)
      if (NOT FFMPEG_${arg_component}_LIBRARY)
        set(what "library")
      endif ()
      if (NOT FFMPEG_${arg_component}_INCLUDE_DIR)
        if (what)
          string(APPEND what " or headers")
        else ()
          set(what "headers")
        endif ()
      endif ()
      set("FFMPEG_${arg_component}_NOT_FOUND_MESSAGE"
          "Could not find the ${what} for ${arg_component}.")
    endif ()
  endif ()
endmacro()

_find_component(avutil avutil.h)
_find_component(swresample swresample.h
    avutil)
_find_component(swscale swscale.h
    avutil)
_find_component(avcodec avcodec.h
    avutil)
_find_component(avformat avformat.h
    avcodec avutil)
_find_component(avfilter avfilter.h
    avutil)
_find_component(avdevice avdevice.h
    avformat avutil)

if (TARGET FFMPEG::avutil)
  set(_ffmpeg_version_header_path "${FFMPEG_avutil_INCLUDE_DIR}/libavutil/ffversion.h")
  if (EXISTS "${_ffmpeg_version_header_path}")
    file(STRINGS "${_ffmpeg_version_header_path}" _ffmpeg_version
        REGEX "FFMPEG_VERSION")
    string(REGEX REPLACE ".*\"n?\(.*\)\"" "\\1" FFMPEG_VERSION "${_ffmpeg_version}")
    unset(_ffmpeg_version)
  else ()
    set(FFMPEG_VERSION FFMPEG_VERSION-NOTFOUND)
  endif ()
  unset(_ffmpeg_version_header_path)
endif ()

set(FFMPEG_INCLUDE_DIRS)
set(FFMPEG_LIBRARIES)

set(_ffmpeg_required_vars)
foreach (_ffmpeg_component IN LISTS FFMPEG_FIND_COMPONENTS)
  if (TARGET "FFMPEG::${_ffmpeg_component}")
    set(FFMPEG_${_ffmpeg_component}_INCLUDE_DIRS
        "${FFMPEG_${_ffmpeg_component}_INCLUDE_DIR}")
    set(FFMPEG_${_ffmpeg_component}_LIBRARIES
        "${FFMPEG_${_ffmpeg_component}_LIBRARY}")
    list(APPEND FFMPEG_INCLUDE_DIRS
        "${FFMPEG_${_ffmpeg_component}_INCLUDE_DIRS}")
    list(APPEND FFMPEG_LIBRARIES
        "${FFMPEG_${_ffmpeg_component}_LIBRARIES}")
    if (FFMEG_FIND_REQUIRED_${_ffmpeg_component})
      list(APPEND _ffmpeg_required_vars
          "FFMPEG_${_ffmpeg_required_vars}_INCLUDE_DIRS"
          "FFMPEG_${_ffmpeg_required_vars}_LIBRARIES")
    endif ()
  endif ()
endforeach ()
unset(_ffmpeg_component)

if (FFMPEG_INCLUDE_DIRS)
  list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFMPEG
    REQUIRED_VARS FFMPEG_INCLUDE_DIRS FFMPEG_LIBRARIES ${_ffmpeg_required_vars}
    VERSION_VAR FFMPEG_VERSION
    HANDLE_COMPONENTS)
unset(_ffmpeg_required_vars)

