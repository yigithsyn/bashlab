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

if(WIN32)
  include_directories($ENV{USERPROFILE}/AppData/Local/include) 
  include_directories($ENV{USERPROFILE}/AppData/Local/include/libmongoc-1.0) 
  include_directories($ENV{USERPROFILE}/AppData/Local/include/libbson-1.0)
  link_directories($ENV{USERPROFILE}/AppData/Local/lib)

  # set(COMMON_LINK_LIBS_DEBUG jansson_d argtable3d)
  set(COMMON_LINK_LIBS_OPTIM jansson argtable3 mongoc-1.0 bson-1.0)
endif(WIN32)

if(UNIX)
  include_directories("/usr/local/include/")
  include_directories("/usr/local/include/libmongoc-1.0")
  include_directories("/usr/local/include/libbson-1.0")
 
  # set(COMMON_LINK_LIBS_DEBUG jansson argtable3 m)
  set(COMMON_LINK_LIBS_OPTIM jansson argtable3 mongoc-1.0 bson-1.0 m) 
endif(UNIX)

file(GLOB SOURCES "libs/algo/linspace.c")
add_executable(linspace src/linspace.c ${SOURCES} ${COMMONS})
target_include_directories(linspace PUBLIC "libs/algo")
target_link_libraries(linspace ${COMMON_LINK_LIBS_OPTIM})
install(TARGETS linspace CONFIGURATIONS Release) 

file(GLOB SOURCES "src/impl/no_impl*")
add_executable(ws.set src/ws.set.c ${SOURCES} ${COMMONS})
target_link_libraries(ws.set ${COMMON_LINK_LIBS_OPTIM})
install(TARGETS ws.set CONFIGURATIONS Release) 

file(GLOB SOURCES "src/impl/no_impl*")
add_executable(ws.list src/ws.list.c ${SOURCES} ${COMMONS})
target_link_libraries(ws.list ${COMMON_LINK_LIBS_OPTIM})
install(TARGETS ws.list CONFIGURATIONS Release) 

file(GLOB SOURCES "src/impl/no_impl*")
add_executable(ws.clr src/ws.clr.c ${SOURCES} ${COMMONS})
target_link_libraries(ws.clr ${COMMON_LINK_LIBS_OPTIM})
install(TARGETS ws.clr CONFIGURATIONS Release) 

file(GLOB SOURCES "src/impl/no_impl*")
add_executable(ams2nsi src/ams2nsi.c ${SOURCES} ${COMMONS})
target_link_libraries(ams2nsi ${COMMON_LINK_LIBS_OPTIM})
install(TARGETS ams2nsi CONFIGURATIONS Release) 

add_executable(freq2wavelen src/freq2wavelen.c ${COMMONS})
target_link_libraries(freq2wavelen ${COMMON_LINK_LIBS_OPTIM})
install(TARGETS freq2wavelen CONFIGURATIONS Release) 

add_executable(wavelen2freq src/wavelen2freq.c ${COMMONS})
target_link_libraries(wavelen2freq ${COMMON_LINK_LIBS_OPTIM})
install(TARGETS wavelen2freq CONFIGURATIONS Release)

add_executable(s11toswr src/s11toswr.c ${COMMONS})
target_link_libraries(s11toswr ${COMMON_LINK_LIBS_OPTIM})
install(TARGETS s11toswr CONFIGURATIONS Release) 

add_executable(ffdist src/ffdist.c ${COMMONS})
target_link_libraries(ffdist ${COMMON_LINK_LIBS_OPTIM})
install(TARGETS ffdist CONFIGURATIONS Release) 

# MongoDB as workspace
add_executable(serialport.list src/serialport.list.c ${COMMONS})
target_link_libraries(serialport.list ${COMMON_LINK_LIBS_OPTIM} serialport)
install(TARGETS serialport.list CONFIGURATIONS Release) 
