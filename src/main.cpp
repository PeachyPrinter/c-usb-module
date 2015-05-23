#include <stdio.h>
#include <stdlib.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <libusb.h>
#include <cmath>
#include <string.h>

#if WIN32
#define EXPORT_BIT __declspec(dllexport)
#else
#define EXPORT_BIT
#endif

typedef struct {
  unsigned char data[64];
  uint32_t length;
} packet_t;

typedef void (*callback_t)(unsigned char*, uint32_t);

class PeachyUsb;


class PeachyUsb {
private:
  std::condition_variable write_avail;
  std::condition_variable read_avail;
  std::mutex mtx;
  std::thread usb_writer;
  std::thread usb_reader;
  
  bool run_writer;
  uint32_t write_r_index; // index to read from
  uint32_t write_count; // number of packets ready to read (written and unread)
  uint32_t write_capacity; // total capacity
  packet_t* write_packets;

  libusb_context* usb_context;
  libusb_device_handle* usb_handle;
  callback_t read_callback;
  
public:
  static void writer_func(PeachyUsb* ctx) {
    unsigned char buf[64] = { 0 };
    int packet_size;
    int transferred;

    while (ctx->run_writer) {
      ctx->get_from_write_queue(buf, 64, &packet_size);
      int res = libusb_bulk_transfer(ctx->usb_handle, 2, buf, packet_size, &transferred, 2000);
      if (res != 0) {
	printf("libusb error %d: %s\n", res, libusb_error_name(res));
      }
    }
    libusb_release_interface(ctx->usb_handle, 0);
    libusb_exit(ctx->usb_context);
    ctx->usb_handle = NULL;
    ctx->usb_context = NULL;
  }

  static void reader_func(PeachyUsb* ctx) {
    unsigned char buf[64] = { 0 };
    int packet_size = 64;
    int transferred;
    
    while (ctx->run_writer) {
      transferred = 0;
      int res = libusb_bulk_transfer(ctx->usb_handle, 0x83, buf, packet_size, &transferred, 2000);
      if (ctx->read_callback && transferred) {
	ctx->read_callback(buf, transferred);
      }
    }
  }
  
  PeachyUsb(uint32_t buffer_size) {
    this->write_capacity = buffer_size;
    this->write_r_index = 0;
    this->write_count = 0;
    this->write_packets = (packet_t*)malloc(sizeof(packet_t) * buffer_size);
  }
  ~PeachyUsb() {
    if (this->usb_context) {
      this->run_writer = false;
    }
    this->usb_writer.join();
  }
  int start() {
    libusb_init(&this->usb_context);
    this->usb_handle = libusb_open_device_with_vid_pid(this->usb_context, 0x16d0, 0x0af3);
    if (this->usb_handle == NULL) {
      return -1;
    }
    libusb_claim_interface(this->usb_handle, 0);

    this->run_writer = true;
    this->usb_writer = std::thread(writer_func, this);
    this->usb_reader = std::thread(reader_func, this);
    return 0;
  }

  void get_from_write_queue(unsigned char* buf, int length, int* transferred) {
    std::unique_lock<std::mutex> lck(mtx);
    while (this->write_count == 0) {
      read_avail.wait(lck);
    }
    packet_t* pkt = &this->write_packets[this->write_r_index];
    int read_count = (pkt->length > length ? length : pkt->length);
    memcpy(buf, pkt->data, read_count);
    *transferred = read_count;
    this->write_count--;
    this->write_r_index = (this->write_r_index + 1) % this->write_capacity;
    this->write_avail.notify_one();
  }

  void write(unsigned char* buf, int length) {
    if (!this->usb_handle) {
      return;
    }
    std::unique_lock<std::mutex> lck(mtx);
    while (this->write_count == this->write_capacity) {
      this->write_avail.wait(lck);
    }
    uint32_t write_w_index = (this->write_r_index + this->write_count) % this->write_capacity;
    this->write_packets[write_w_index].length = length;
    memcpy(this->write_packets[write_w_index].data, buf, length);
    this->write_count++;
    this->read_avail.notify_one();
  }

  void set_read_callback(callback_t callback) {
    this->read_callback = callback;
  }
};

extern "C" {
  EXPORT_BIT PeachyUsb *peachyusb_init(uint32_t capacity) {
    PeachyUsb* ctx = new PeachyUsb(capacity);
    ctx->start();
    return ctx;
  }

  EXPORT_BIT void peachyusb_set_read_callback(PeachyUsb* ctx, callback_t callback) {
    ctx->set_read_callback(callback);
  }
  
  EXPORT_BIT void peachyusb_write(PeachyUsb* ctx, unsigned char* buf, uint32_t length) {
    ctx->write(buf, length);
  }

  EXPORT_BIT void peachyusb_shutdown(PeachyUsb* ctx) {
    delete ctx;
  }
}
