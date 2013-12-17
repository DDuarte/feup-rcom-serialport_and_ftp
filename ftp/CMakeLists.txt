project(download)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

set(CMAKE_C_FLAGS "-std=gnu99 -Wall -g")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(PCRE)
if (PCRE_FOUND)
  include_directories(${PCRE_INCLUDE_DIRS})
  target_link_libraries (${PROJECT_NAME} ${PCRE_LIBRARIES})
endif (PCRE_FOUND)
