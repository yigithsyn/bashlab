cmake_minimum_required(VERSION 3.16.3)
project(freq2wavelen VERSION 0.1.0)


if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)

include_directories("include")
include_directories("libs/argtable3/dist/")
file(GLOB SOURCES "src/impl/*.c" "libs/argtable3/dist/*.c")

add_executable(freq2wavelen src/freq2wavelen.c ${SOURCES})
if(UNIX)
  target_link_libraries(freq2wavelen m)
endif(UNIX)

install(TARGETS freq2wavelen CONFIGURATIONS Release)