cmake_minimum_required (VERSION 3.12)

project (Polypartition)

set(ROOT_DIR ${CMAKE_SOURCE_DIR})

#list(append CMAKE_MODULE_PATH "${ROOT_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
add_definitions("-DUNICODE -D_UNICODE") #定义后,字符串使用wchar_t类型


if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(CMAKE_DEBUG_POSTFIX "D")
    set(XCG_MODULE_SUFFIX ${CMAKE_DEBUG_POSTFIX})
    add_definitions("-D_DEBUG")
else()
    set(CMAKE_DEBUG_POSTFIX "")
    set(XCG_MODULE_SUFFIX "")
endif()

if(UNIX)
    add_definitions("-DUNIX64")

    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ROOT_DIR}/linuxbin64/")
    #SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY     "${XCG_ROOT_DIR}/lib/linux_x64/")
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ROOT_DIR}/linuxbin64/")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ROOT_DIR}/bin64")
    #set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ROOT_DIR}/lib/")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ROOT_DIR}/bin64/")
    add_definitions("-DWIN64")
endif()

#Filter
SET(XCG_HEADER_FILES "HeaderFiles")
SET(XCG_SOURCE_FILES "SourceFiles")

add_subdirectory ("test")


