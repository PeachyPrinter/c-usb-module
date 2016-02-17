cmake_minimum_required (VERSION 2.6)

set(CMAKE_USER_MAKE_RULES_OVERRIDE
   ${CMAKE_CURRENT_SOURCE_DIR}/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
   ${CMAKE_CURRENT_SOURCE_DIR}/cxx_flag_overrides.cmake)


# Maps to a solution file (Tutorial.sln). The solution will 
# have all targets (exe, lib, dll) as projects (.vcproj)
project (PeachyUSB)

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set compiler flags and options. 
# Here it is setting the Visual Studio warning level to 4
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

# Command to output information to the console
# Useful for displaying errors, warnings, and debugging
message ("cxx Flags: " ${CMAKE_CXX_FLAGS})

set(EXTRA_LIBS "")

if(WIN32)
  include_directories("deps")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(LIBUSB_LIBRARY libusb-1.0 deps/win64)
  else(CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(LIBUSB_LIBRARY libusb-1.0 deps/win32)
  endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
elseif(APPLE)
  set(CMAKE_CXX_FLAGS "-std=c++11")
  include_directories("/usr/include/libusb-1.0" "/usr/local/include/libusb-1.0")
  find_library(LIBUSB_LIBRARY
    NAMES
    libusb-1.0.a libusb.a
    PATHS
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/local/lib
    /opt/local/lib
    /sw/lib
    )
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(IOKIT_LIBRARY IOKit)
  set(EXTRA_LIBS ${EXTRA_LIBS} ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY} objc)
elseif(UNIX)
  set(CMAKE_CXX_FLAGS "-std=c++11")
  include_directories("/usr/include/libusb-1.0" "/usr/local/include/libusb-1.0")
  find_library(LIBUSB_LIBRARY
    NAMES
    libusb-1.0.a libusb.a
    PATHS
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/local/lib
    /opt/local/lib
    /sw/lib
    )
endif(WIN32)

include_directories(inc)
# Sub-directories where more CMakeLists.txt exist
add_subdirectory(src)
