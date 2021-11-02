cmake_minimum_required(VERSION 3.16.3)
project(freq2wavelen VERSION 0.1.0)


if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)

include_directories("libs/argtable3/dist/")
file(GLOB SOURCES "libs/argtable3/dist/*.c")
add_executable(freq2wavelen main.c ${SOURCES})
if(UNIX)
  target_link_libraries(freq2wavelen m)
endif(UNIX)

# if (WIN32)
#   install(TARGETS freq2wavelen CONFIGURATIONS Release DESTINATION "$ENV{USERPROFILE}\\AppData\\Local\\bin")
# endif (WIN32)
# if (UNIX)
  install(TARGETS freq2wavelen CONFIGURATIONS MinSizeRel)
# endif (UNIX)