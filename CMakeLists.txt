cmake_minimum_required(VERSION 3.16.0)
project(freq2wavelen VERSION 0.1.0)

add_definitions(-DREADME="${freq2wavelen_SOURCE_DIR}/README")

if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)


# include_directories(include)



# file(GLOB SOURCES "src/*.cpp" "src/*.c")
add_executable(freq2wavelen main.c)


