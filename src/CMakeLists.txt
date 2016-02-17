include (GenerateExportHeader)

# linux and osx (?) automatically prepend lib, but visual studio does not
if(WIN32)
set(LIBNAME "libPeachyUSB")
else(WIN32)
set(LIBNAME "PeachyUSB")
endif(WIN32)

add_library ( ${LIBNAME} SHARED main.cpp reader.cpp writer.cpp interface.cpp)
GENERATE_EXPORT_HEADER ( ${LIBNAME} )
target_link_libraries (${LIBNAME} ${LIBUSB_LIBRARY} ${EXTRA_LIBS})
