// -*- mode: c++; tab-width: 4 -*-

#include <stdint.h>
#include <libusb.h>
#include "PeachyUsb.h"
#include <condition_variable>
#include <mutex>

typedef struct usb_packet {
	unsigned char data[64];
	uint32_t length;
} packet_t;

typedef struct {
	UsbWriter* ctx;
	uint32_t packet_id;
} writer_callback_data_t;

void UsbWriter::writer_func(UsbWriter* ctx) {
	unsigned char buf[64] = { 0 };
	int packet_size;
	writer_callback_data_t* callback_data;

	while (ctx->run_writer) {
		struct libusb_transfer* transfer = libusb_alloc_transfer(0);

		ctx->get_from_write_queue(buf, 64, &packet_size);
		if (packet_size == 0) {
			continue;
		}
        printf("Dequeued %d bytes\n", packet_size);
		int packet_id = ctx->get_next_inflight_id();

		callback_data = new writer_callback_data_t();
		callback_data->ctx = ctx;
		callback_data->packet_id = packet_id;

		libusb_fill_bulk_transfer(transfer, ctx->usb_handle, 2, buf, packet_size, write_complete_callback, (void*)callback_data, 2000);
        libusb_submit_transfer(transfer);
	}
}

UsbWriter::UsbWriter(uint32_t capacity, libusb_device_handle* dev) {
	this->write_capacity = capacity;
	this->write_r_index = 0;
	this->write_count = 0;
	this->write_packets = (packet_t*)malloc(sizeof(packet_t) * capacity);
	this->max_inflight = 40; // 40 ~= 20 milliseconds worth of packets

	this->run_writer = true;
	this->writer = std::thread(writer_func, this);
}
UsbWriter::~UsbWriter() {
	this->run_writer = false;
	if (this->writer.joinable()) {
		this->writer.join();
	}
}


void LIBUSB_CALL UsbWriter::write_complete_callback(struct libusb_transfer* transfer) {
	writer_callback_data_t* callback_data = (writer_callback_data_t*)transfer->user_data;
	UsbWriter* ctx = callback_data->ctx;
	uint32_t packet_id = callback_data->packet_id;
	delete callback_data;

	// TODO do some error handling here

	ctx->remove_inflight_id(packet_id);
	libusb_free_transfer(transfer);
}


void UsbWriter::get_from_write_queue(unsigned char* buf, uint32_t length, int* transferred) {
	std::unique_lock<std::mutex> lck(mtx);
	while (this->write_count == 0) {
		data_avail.wait(lck);
	}
	packet_t* pkt = &this->write_packets[this->write_r_index];
	uint32_t read_count = (pkt->length > length ? length : pkt->length);
	memcpy(buf, pkt->data, read_count);
	*transferred = read_count;
	this->write_count--;
	this->write_r_index = (this->write_r_index + 1) % this->write_capacity;
	this->room_avail.notify_one();
}

int UsbWriter::write(const unsigned char* buf, uint32_t length) {
	if (!this->usb_handle) {
		return -1;
	}
	std::unique_lock<std::mutex> lck(mtx);
	while (this->write_count == this->write_capacity) {
		this->room_avail.wait(lck);
	}
	uint32_t write_w_index = (this->write_r_index + this->write_count) % this->write_capacity;
	this->write_packets[write_w_index].length = length;
	memcpy(this->write_packets[write_w_index].data, buf, length);
	this->write_count++;
	this->data_avail.notify_one();
	return length;
}
uint32_t UsbWriter::get_next_inflight_id(void) {
	uint32_t inflight_id;
	std::unique_lock<std::mutex> lock(this->inflight_mtx);
	while (this->inflight.size() >= this->max_inflight) {
		this->inflight_room_avail.wait(lock);
	}
	this->inflight.insert(this->packet_counter);
	inflight_id = packet_counter;
	packet_counter++;
	return inflight_id;
}
void UsbWriter::remove_inflight_id(uint32_t inflight_id) {
	std::unique_lock<std::mutex> lock(this->inflight_mtx);
	this->inflight.erase(inflight_id);
	this->inflight_room_avail.notify_one();
}
