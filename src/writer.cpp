// -*- mode: c++; tab-width: 4 -*-

#include <stdint.h>
#include <libusb.h>
#include "PeachyUsb.h"
#include <condition_variable>
#include <mutex>
#include <chrono>

typedef struct __attribute__ ((__packed__)) _encoded_packet {
  uint32_t magic;
  uint8_t data_bytes;
  uint16_t sequence;  
} encoded_packet_t;

typedef struct usb_packet {
	unsigned char data[64];
	uint32_t length;
} packet_t;

typedef struct {
	UsbWriter* ctx;
	uint32_t packet_id;
} writer_callback_data_t;

void UsbWriter::writer_func(UsbWriter* ctx) {
	unsigned char buf[192] = { 0 };
	int packet_size;
	writer_callback_data_t* callback_data;
    uint16_t seq = 1;

	while (ctx->run_writer) {
		struct libusb_transfer* transfer = libusb_alloc_transfer(0);
        if (transfer == NULL) {
          printf("Failed to allocate transfer\n");
          break;
        }
        int buf_idx = 0;
        int data_size = 64 - sizeof(encoded_packet_t);
        while(buf_idx < sizeof(buf)) {
          encoded_packet_t* hdr = (encoded_packet_t*)(&buf[buf_idx]);
          buf_idx += sizeof(encoded_packet_t);
          ctx->get_from_write_queue(&buf[buf_idx], data_size, &packet_size);
          if (packet_size == 0) {
            buf_idx -= sizeof(encoded_packet_t);
			break;
          }
          buf_idx += data_size;
          hdr->magic = 0xdeadbeef;
          hdr->data_bytes = packet_size;
          hdr->sequence = seq;
          seq++;
        }
        if (buf_idx == 0) {
          continue;
        }
		int packet_id = ctx->get_next_inflight_id();
        if (packet_id == 0) {
          continue;
        }
		callback_data = new writer_callback_data_t();
		callback_data->ctx = ctx;
		callback_data->packet_id = packet_id;

        libusb_fill_bulk_transfer(transfer, ctx->usb_handle, 2, buf, buf_idx, write_complete_callback, (void*)callback_data, 5000);

        int res = libusb_submit_transfer(transfer);
        if (res != 0) {
          break;
        }
	}
}

UsbWriter::UsbWriter(uint32_t capacity, libusb_device_handle* dev) {
	this->write_capacity = capacity;
	this->write_r_index = 0;
	this->write_count = 0;
	this->write_packets = (packet_t*)malloc(sizeof(packet_t) * capacity);
	this->max_inflight = 40; // 40 ~= 20 milliseconds worth of packets
    this->usb_handle = dev;
    this->packet_counter = 1; // 0 is special

	this->run_writer = true;
	this->writer = std::thread(writer_func, this);
}
UsbWriter::~UsbWriter() {
	this->run_writer = false;

	if (this->writer.joinable()) {
      this->writer.join();
	}

// wait for any inflight packets to finish
    while(this->inflight.size() > 0) {
      std::unique_lock<std::mutex> lock(this->inflight_mtx);
      this->inflight_room_avail.wait_for(lock, std::chrono::milliseconds(10000));
    }
}


void LIBUSB_CALL UsbWriter::write_complete_callback(struct libusb_transfer* transfer) {
	writer_callback_data_t* callback_data = (writer_callback_data_t*)transfer->user_data;
	UsbWriter* ctx = callback_data->ctx;
	uint32_t packet_id = callback_data->packet_id;
	delete callback_data;

    if(transfer->status != LIBUSB_TRANSFER_COMPLETED) {
      printf("libusb error (packet %d) status: %d\n", packet_id, transfer->status);
    }
	ctx->remove_inflight_id(packet_id);
	libusb_free_transfer(transfer);
}


void UsbWriter::get_from_write_queue(unsigned char* buf, uint32_t length, int* transferred) {
	std::unique_lock<std::mutex> lck(mtx);
    *transferred = 0;

	while (this->write_count == 0) {
      this->data_avail.wait_for(lck, std::chrono::milliseconds(50));
      if (!this->run_writer || !this->write_count) {
        return;
      }
	}
    *transferred = 0;
    unsigned char* dest = buf;
    while(this->write_count) {
      packet_t* pkt = &this->write_packets[this->write_r_index];
      uint32_t read_count = (pkt->length > length ? length : pkt->length);
      if ((*transferred + read_count) >= length) {
        break;
      }
      memcpy(dest, pkt->data, read_count);
      dest += read_count;
      *transferred += read_count;
      this->write_count--;
      this->write_r_index = (this->write_r_index + 1) % this->write_capacity;
    }
	this->room_avail.notify_one();
    
}

int UsbWriter::write(const unsigned char* buf, uint32_t length) {
	if (!this->usb_handle) {
		return -1;
	}
    if (!this->run_writer) {
      return 0;
    }
	std::unique_lock<std::mutex> lck(mtx);
	while (this->write_count == this->write_capacity) {
      this->room_avail.wait_for(lck, std::chrono::milliseconds(50));
      if (!this->run_writer) {
        return 0;
      }
	}
	uint32_t write_w_index = (this->write_r_index + this->write_count) % this->write_capacity;
	this->write_packets[write_w_index].length = length;
	memcpy(this->write_packets[write_w_index].data, buf, length);
	this->write_count++;
	this->data_avail.notify_one();
	return length;
}
uint32_t UsbWriter::get_next_inflight_id(void) {
	std::unique_lock<std::mutex> lock(this->inflight_mtx);
    if (this->packet_counter == 0) {
      this->packet_counter = 1;
    }
	uint32_t inflight_id;
	while (this->inflight.size() >= this->max_inflight) {
      this->inflight_room_avail.wait_for(lock, std::chrono::milliseconds(50));
      if (!this->run_writer) {
        return 0;
      }
	}
	this->inflight.insert(this->packet_counter);
	inflight_id = packet_counter;
	this->packet_counter++;
	return inflight_id;
}
void UsbWriter::remove_inflight_id(uint32_t inflight_id) {
	std::unique_lock<std::mutex> lock(this->inflight_mtx);
	this->inflight.erase(inflight_id);
	this->inflight_room_avail.notify_one();
}
