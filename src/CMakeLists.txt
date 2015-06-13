include (GenerateExportHeader)

add_library (PeachyUSB SHARED main.cpp reader.cpp writer.cpp interface.cpp)
GENERATE_EXPORT_HEADER ( libPeachyUSB )
target_link_libraries (libPeachyUSB ${LIBUSB_LIBRARY})
