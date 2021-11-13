cmake_minimum_required(VERSION 3.16.3)
project(freq2wavelen VERSION 0.1.0)


if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)

# commons
include_directories("include")
include_directories("libs/argtable3/dist/")
include_directories("libs/jansson-2.14/build/include/")
link_directories("libs/jansson-2.14/build/lib")
file(GLOB COMMONS "libs/argtable3/dist/*.c")

file(GLOB SOURCES "src/impl/freq2wavelen_impl.c" "src/impl/wavelen2freq_impl.c")
add_executable(freq2wavelen src/freq2wavelen.c ${SOURCES} ${COMMONS})
add_executable(wavelen2freq src/wavelen2freq.c ${SOURCES} ${COMMONS})
if(UNIX)
  target_link_libraries(freq2wavelen m jansson)
  target_link_libraries(wavelen2freq m)
endif(UNIX)

install(TARGETS freq2wavelen wavelen2freq CONFIGURATIONS Release)