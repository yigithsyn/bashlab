cmake_minimum_required(VERSION 3.16.0)
project(freq2wavelen VERSION 0.1.0)

# file(READ "${freq2wavelen_SOURCE_DIR}/HELP.txt" HELP HEX)
# add_definitions(-DHELP="${HELP}")

if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)






include_directories("libs/argtable3/dist/")
file(GLOB SOURCES "libs/argtable3/dist/*.c")
add_executable(freq2wavelen main.c ${SOURCES})
target_link_libraries(freq2wavelen m)


