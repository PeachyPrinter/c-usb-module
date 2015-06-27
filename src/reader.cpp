#include "PeachyUsb.h"

UsbReader::UsbReader(libusb_device_handle* dev) {
	this->usb_handle = dev;
	this->run_reader = true;
	this->reader = std::thread(reader_func, this);
}

UsbReader::~UsbReader() {
	this->run_reader = false;
	if (this->reader.joinable()) {
		this->reader.join();
	}
}

void UsbReader::set_read_callback(usb_callback_t callback) {
	this->read_callback = callback;
}

void UsbReader::reader_func(UsbReader* ctx) {
	unsigned char buf[64] = { 0 };
	int packet_size = 64;
	int transferred;

	while (ctx->run_reader) {
		transferred = 0;
		int res = libusb_bulk_transfer(ctx->usb_handle, 0x83, buf, packet_size, &transferred, 2000);
		usb_callback_t callback = ctx->read_callback;
		if (callback && transferred) {
			callback(buf, transferred);
		}
	}
}
