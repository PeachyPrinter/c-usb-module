include (GenerateExportHeader)

add_library (PeachyUSB SHARED main.cpp reader.cpp writer.cpp iface.cpp)
GENERATE_EXPORT_HEADER ( PeachyUSB )
target_link_libraries (PeachyUSB ${LIBUSB_LIBRARY})
