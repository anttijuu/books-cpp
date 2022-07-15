cmake_minimum_required(VERSION 3.21)
project(cpp-books-threaded VERSION 1.0.0 LANGUAGES CXX)

set(APP_NAME books)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Boost 1.76.0 REQUIRED COMPONENTS system)

if (WIN32)
    macro(get_WIN32_WINNT version)
        if (CMAKE_SYSTEM_VERSION)
            set(ver ${CMAKE_SYSTEM_VERSION})
            string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
            string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
            # Check for Windows 10, b/c we'll need to convert to hex 'A'.
            if ("${verMajor}" MATCHES "10")
                set(verMajor "A")
                string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
            endif ("${verMajor}" MATCHES "10")
            # Remove all remaining '.' characters.
            string(REPLACE "." "" ver ${ver})
            # Prepend each digit with a zero.
            string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
            set(${version} "0x${ver}")
        endif(CMAKE_SYSTEM_VERSION)
    endmacro(get_WIN32_WINNT)

    get_WIN32_WINNT(ver)
    add_definitions(-D_WIN32_WINNT=${ver})
endif(WIN32)

if (Boost_FOUND)
   add_executable(${APP_NAME} main.cpp)

   set_target_properties(${APP_NAME} PROPERTIES CXX_STANDARD 17)

   target_include_directories(${APP_NAME} PRIVATE ${Boost_INCLUDE_DIRS} "../")

endif()
