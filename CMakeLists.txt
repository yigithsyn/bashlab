cmake_minimum_required(VERSION 3.16.3)
project(bashlab VERSION 0.1.0)


if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)

if(WIN32)
  message(STATUS "User home: $ENV{USERPROFILE}") 
endif(WIN32)

# commons
include_directories("include")

set(LIB_ARGTABLE_DIR "${bashlab_SOURCE_DIR}/libs/argtable-v3.2.1.52f24e5")
set(LIB_JANSSON_DIR "${bashlab_SOURCE_DIR}/libs/jansson-2.14") 

if(WIN32)
  include_directories($ENV{USERPROFILE}/AppData/Local/include) 
  link_directories($ENV{USERPROFILE}/AppData/Local/lib)

  set(COMMON_LINK_LIBS_DEBUG jansson_d argtable3d)
  set(COMMON_LINK_LIBS_OPTIM jansson argtable3)
endif(WIN32)

if(UNIX)
  include_directories("/usr/local/include/")
 
  # set(COMMON_LINK_LIBS_DEBUG jansson argtable3 m)
  set(COMMON_LINK_LIBS_DEBUG ${LIB_JANSSON_DIR}/build/lib/jansson ${LIB_ARGTABLE_DIR}/build/src/argtable3 m)
  set(COMMON_LINK_LIBS_OPTIM jansson argtable3 m) 
endif(UNIX)
message(STATUS ${COMMON_LINK_LIBS_DEBUG})
set(ALL_TARGETS freq2wavelen wavelen2freq ffdist linspace wsset wsclr ws.list s11toswr)

# file(GLOB SOURCES "src/impl/freq2wavelen_impl.c" "src/impl/wavelen2freq_impl.c")
# add_executable(freq2wavelen src/freq2wavelen.c ${SOURCES} ${COMMONS})
# add_executable(wavelen2freq src/wavelen2freq.c ${SOURCES} ${COMMONS})
# target_link_libraries(freq2wavelen debug ${COMMON_LINK_LIBS_DEBUG} optimized ${COMMON_LINK_LIBS_OPTIM})
# target_link_libraries(wavelen2freq debug ${COMMON_LINK_LIBS_DEBUG} optimized ${COMMON_LINK_LIBS_OPTIM})

file(GLOB SOURCES "src/impl/freq2wavelen_impl.c" "src/impl/ffdist_impl.c")
add_executable(ffdist src/ffdist.c ${SOURCES} ${COMMONS})
target_link_libraries(ffdist debug ${COMMON_LINK_LIBS_DEBUG} optimized ${COMMON_LINK_LIBS_OPTIM})

# file(GLOB SOURCES "libs/algo/linspace.c")
# add_executable(linspace src/linspace.c ${SOURCES} ${COMMONS})
# target_include_directories(linspace PUBLIC "libs/algo")
# target_link_libraries(linspace debug ${COMMON_LINK_LIBS_DEBUG} optimized ${COMMON_LINK_LIBS_OPTIM})

# file(GLOB SOURCES "src/impl/no_impl*")
# add_executable(wsset src/wsset.c ${SOURCES} ${COMMONS})
# target_link_libraries(wsset debug ${COMMON_LINK_LIBS_DEBUG} optimized ${COMMON_LINK_LIBS_OPTIM})

# file(GLOB SOURCES "src/impl/no_impl*")
# add_executable(wsclr src/wsclr.c ${SOURCES} ${COMMONS})
# target_link_libraries(wsclr debug ${COMMON_LINK_LIBS_DEBUG} optimized ${COMMON_LINK_LIBS_OPTIM})

# file(GLOB SOURCES "src/impl/no_impl*")
# add_executable(ws.list src/ws.list.c ${SOURCES} ${COMMONS})
# target_link_libraries(ws.list debug ${COMMON_LINK_LIBS_DEBUG} optimized ${COMMON_LINK_LIBS_OPTIM})

# file(GLOB SOURCES "src/impl/s11toswr_impl.c") 
# add_executable(s11toswr src/s11toswr.c ${SOURCES} ${COMMONS})
# target_link_libraries(s11toswr debug ${COMMON_LINK_LIBS_DEBUG} optimized ${COMMON_LINK_LIBS_OPTIM})

# install(TARGETS ${ALL_TARGETS} CONFIGURATIONS Release) 