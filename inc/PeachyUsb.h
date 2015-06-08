#ifndef PEACHY_USB_H
#define PEACHY_USB_H

typedef void(*usb_callback_t)(unsigned char*, uint32_t);

class PeachyUsb {
public:
	PeachyUsb(uint32_t buffer_size);
	~PeachyUsb();
	void set_read_callback(usb_callback_t callback);
	int write(const unsigned char* buf, uint32_t length);
};

class UsbWriter {
public:
	UsbWriter(uint32_t capacity, libusb_device_handle* dev);
	~UsbWriter();
	int write(const unsigned char* buf, uint32_t length);
};


class UsbReader {
public:
	UsbReader(libusb_device_handle* dev);
	~UsbReader();
	void set_read_callback(usb_callback_t callback);
};

#endif