include (GenerateExportHeader)

add_library (PeachyUSB SHARED main.cpp)
GENERATE_EXPORT_HEADER ( PeachyUSB )
target_link_libraries (PeachyUSB ${LIBUSB_LIBRARY})
