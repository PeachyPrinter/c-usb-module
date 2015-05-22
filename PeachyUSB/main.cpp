#include <stdio.h>
#include <stdlib.h>
#include <condition_variable>
#include <mutex>

#include <cmath>

typedef struct {
	unsigned char data[64];
	uint32_t length;
} packet_t;

class PeachyUsb {
	std::condition_variable write_avail;
	std::condition_variable read_avail;
	std::mutex mtx;
	uint32_t write_r_index; // index to read from
	uint32_t write_count; // number of packets ready to read (written and unread)
	uint32_t write_capacity; // total capacity
	packet_t* write_packets;

public:
	PeachyUsb(uint32_t buffer_size) {
		printf("In the constructor\n");
		this->write_capacity = buffer_size;
		this->write_r_index = 0;
		this->write_count = 0;
		this->write_packets = (packet_t*)malloc(sizeof(packet_t) * buffer_size);
	}

	void read(unsigned char* buf, int length, int* transferred) {
		std::unique_lock<std::mutex> lck(mtx);
		while (this->write_count == 0) {
			read_avail.wait(lck);
		}
		packet_t* pkt = &this->write_packets[this->write_r_index];
		int read_count = (pkt->length > length ? length : pkt->length);
		memcpy(buf, pkt->data, read_count);
		this->write_count--;
		this->write_r_index = (this->write_r_index + 1) % this->write_capacity;
		this->write_avail.notify_one();
	}

	void write(unsigned char* buf, int length) {
		std::unique_lock<std::mutex> lck(mtx);
		while (this->write_count == this->write_capacity) {
			this->write_avail.wait(lck);
		}
		uint32_t write_w_index = (write_r_index + write_count) % write_capacity;
		this->write_packets[write_w_index].length = length;
		memcpy(this->write_packets[write_w_index].data, buf, length);
		this->write_count++;
		this->read_avail.notify_one();
	}
};

extern "C" {
	__declspec(dllexport) PeachyUsb *peachyusb_init(uint32_t capacity) {
		PeachyUsb* ctx = new PeachyUsb(capacity);
		printf("Mah I'm in a DLL\n");
		return ctx;
	}

	__declspec(dllexport) int peachyusb_read(PeachyUsb* ctx, unsigned char* buf, uint32_t length) {
		int transferred;
		ctx->read(buf, length, &transferred);
		return transferred;
	}

	__declspec(dllexport) void peachyusb_write(PeachyUsb* ctx, unsigned char* buf, uint32_t length) {
		ctx->write(buf, length);
	}

	__declspec(dllexport) void peachyusb_shutdown(PeachyUsb* ctx) {
		delete ctx;
	}
}