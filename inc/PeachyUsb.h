#ifndef PEACHY_USB_H
#define PEACHY_USB_H

#include <stdio.h>
#include <stdlib.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <libusb.h>
#include <cmath>
#include <set>
#include <map>
#include <string.h>
#include <memory>

typedef void(*usb_callback_t)(unsigned char*, uint32_t);


struct usb_packet;
struct writer_callback;

class UsbWriter {
public:
	UsbWriter(uint32_t capacity, libusb_device_handle* dev);
	~UsbWriter();
	int write(const unsigned char* buf, uint32_t length);
private:
	volatile bool run_writer;
	libusb_device_handle* usb_handle;

	uint32_t write_r_index; // index to read from
	uint32_t write_count; // number of packets ready to read (written and unread)
	uint32_t write_capacity; // total capacity
	struct usb_packet* write_packets;

  std::set<int> inflight;
  std::map<int, struct libusb_transfer*> inflight_packets;
	uint32_t max_inflight;
	uint32_t packet_counter; // perpetually incrementing packet id

	std::condition_variable room_avail;
	std::condition_variable data_avail;
	std::mutex mtx;

	std::condition_variable inflight_room_avail;
	std::mutex inflight_mtx;

	std::thread writer;

	static void writer_func(UsbWriter* ctx);
	static void LIBUSB_CALL write_complete_callback(struct libusb_transfer* transfer);
	void get_from_write_queue(unsigned char* buf, uint32_t length, int* transferred);
	uint32_t get_next_inflight_id(void);
	void remove_inflight_id(uint32_t inflight_id);
};

class UsbReader {
public:
	UsbReader(libusb_device_handle* dev);
	~UsbReader();
	void set_read_callback(usb_callback_t callback);
private:
	libusb_device_handle* usb_handle;
	std::thread reader;
	volatile bool run_reader;
	usb_callback_t read_callback;
	static void reader_func(UsbReader* ctx);
};

class PeachyUsb {
public:
	PeachyUsb(uint32_t buffer_size);
	~PeachyUsb();
	void set_read_callback(usb_callback_t callback);
	int write(const unsigned char* buf, uint32_t length);
private:
	libusb_context* usb_context;
	libusb_device_handle* usb_handle;
	std::unique_ptr<UsbReader> reader;
	std::unique_ptr<UsbWriter> writer;

};


#endif