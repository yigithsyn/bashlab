cmake_minimum_required(VERSION 3.16.3)
project(freq2wavelen VERSION 0.1.0)


if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)

# commons
include_directories("include")
include_directories("libs/argtable-v3.2.1.52f24e5/dist/")
include_directories("libs/jansson-2.14/build/include/")
set(LIB_ARGTABLE_DIR "libs/argtable-v3.2.1.52f24e5")
set(LIB_JANSSON_DIR "libs/jansson-2.14")
if(UNIX)
  link_directories("${LIB_JANSSON_DIR}/build/lib/")
  link_directories("${LIB_ARGTABLE_DIR}/build/src/")
endif(UNIX)
if(WIN32)
  link_directories("${LIB_JANSSON_DIR}/build/lib/Release")
  link_directories("${LIB_ARGTABLE_DIR}/build/src/Release")
endif(WIN32)

if(WIN32)
  set(COMMON_LINK_LIBS jansson argtable3_static)
endif(WIN32)
if(UNIX)
  set(COMMON_LINK_LIBS m jansson argtable3_static)
endif(UNIX)

set(ALL_TARGETS freq2wavelen wavelen2freq ffdist linspace wsset wsclr wslist s11toswr)

file(GLOB SOURCES "src/impl/freq2wavelen_impl.c" "src/impl/wavelen2freq_impl.c")
add_executable(freq2wavelen src/freq2wavelen.c ${SOURCES} ${COMMONS})
add_executable(wavelen2freq src/wavelen2freq.c ${SOURCES} ${COMMONS})
target_link_libraries(freq2wavelen ${COMMON_LINK_LIBS})
target_link_libraries(wavelen2freq ${COMMON_LINK_LIBS})

file(GLOB SOURCES "src/impl/freq2wavelen_impl.c" "src/impl/ffdist_impl.c")
add_executable(ffdist src/ffdist.c ${SOURCES} ${COMMONS})
target_link_libraries(ffdist ${COMMON_LINK_LIBS})

file(GLOB SOURCES "libs/algo/linspace.c")
add_executable(linspace src/linspace.c ${SOURCES} ${COMMONS})
target_include_directories(linspace PUBLIC "libs/algo")
target_link_libraries(linspace ${COMMON_LINK_LIBS})

file(GLOB SOURCES "src/impl/no_impl*")
add_executable(wsset src/wsset.c ${SOURCES} ${COMMONS})
target_link_libraries(wsset ${COMMON_LINK_LIBS})

file(GLOB SOURCES "src/impl/no_impl*")
add_executable(wsclr src/wsclr.c ${SOURCES} ${COMMONS})
target_link_libraries(wsclr ${COMMON_LINK_LIBS})

file(GLOB SOURCES "src/impl/no_impl*")
add_executable(wslist src/wslist.c ${SOURCES} ${COMMONS})
target_link_libraries(wslist ${COMMON_LINK_LIBS})

file(GLOB SOURCES "src/impl/s11toswr_impl.c") 
add_executable(s11toswr src/s11toswr.c ${SOURCES} ${COMMONS})
target_link_libraries(s11toswr ${COMMON_LINK_LIBS})

install(TARGETS ${ALL_TARGETS} CONFIGURATIONS Release) 