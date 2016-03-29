// -*- mode: c++; tab-width: 4 -*-
#include "PeachyUsb.h"
#include "version.h"

#if WIN32
#define EXPORT_BIT __declspec(dllexport)
#else
#define EXPORT_BIT
#endif

/* States for PeachyUSB object
- uninitialized (new!)
- device not found
- device active
- device removed
- shutting down
*/

PeachyUsb::PeachyUsb(uint32_t buffer_size) {
	libusb_init(&this->usb_context);
	this->usb_handle = libusb_open_device_with_vid_pid(this->usb_context, 0x16d0, 0x0af3);
	if (this->usb_handle == NULL) {
		throw std::runtime_error("Failed to get device handle");
	}
	if (libusb_claim_interface(this->usb_handle, 0) != 0) {
		throw std::runtime_error("Failed to claim interface");
	}
	this->reader = std::unique_ptr<UsbReader>(new UsbReader(this->usb_handle));
	this->writer = std::unique_ptr<UsbWriter>(new UsbWriter(buffer_size, this->usb_handle));
}
PeachyUsb::~PeachyUsb() {
	this->writer.reset();

    // printf("Writer finished\n");
    // Kill the writer first. Once the reader is done, no more libusb
    // callbacks get called. 
    this->reader->set_read_callback(NULL);
	this->reader.reset();

	if (this->usb_handle) {
		libusb_release_interface(this->usb_handle, 0);
	}
	libusb_close(this->usb_handle);
	libusb_exit(this->usb_context);
    // printf("End of PeachyUsb shutdown\n");
    fflush(stdout);
}

void PeachyUsb::set_read_callback(usb_callback_t callback) {
	this->reader->set_read_callback(callback);
}

int PeachyUsb::write(const unsigned char* buf, uint32_t length) {
  return this->writer->write(buf, length);
}


extern "C" {
	EXPORT_BIT PeachyUsb *peachyusb_init(uint32_t capacity) {
		try {
			PeachyUsb* ctx = new PeachyUsb(capacity);
			return ctx;
		}
		catch (std::runtime_error e) {
		  fprintf(stderr, "Runtime exception creating printer: %s\n", e.what());
			return NULL;
		}
	}

	EXPORT_BIT const char * peachyusb_version() {
		return PEACHY_USB_VERSION;
	}

	EXPORT_BIT void peachyusb_set_read_callback(PeachyUsb* ctx, usb_callback_t callback) {
		ctx->set_read_callback(callback);
	}

	EXPORT_BIT void peachyusb_write(PeachyUsb* ctx, unsigned char* buf, uint32_t length) {
		ctx->write(buf, length);
	}

	EXPORT_BIT void peachyusb_shutdown(PeachyUsb* ctx) {
		delete ctx;
	}
}
