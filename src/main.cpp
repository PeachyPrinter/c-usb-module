#include <stdio.h>
#include <stdlib.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <libusb.h>
#include <cmath>
#include <set>
#include <string.h>

#if WIN32
#define EXPORT_BIT __declspec(dllexport)
#else
#define EXPORT_BIT
#endif

class PeachyUsb;

/* States for PeachyUSB object 
- uninitialized (new!)
- device not found
- device active
- device removed
- shutting down
*/

typedef struct {
  unsigned char data[64];
  uint32_t length;
} packet_t;

typedef void (*callback_t)(unsigned char*, uint32_t);
typedef struct {
  PeachyUsb* ctx;
  uint32_t packet_id;
} writer_callback_data_t;

class PeachyUsb {
private:
  std::condition_variable write_avail;
  std::condition_variable read_avail;
  std::mutex mtx;

  std::condition_variable async_room_avail;
  std::mutex inflight_mtx;

  std::thread usb_writer;
  std::thread usb_reader;

  std::set<int> inflight;
  uint32_t max_inflight;
  uint32_t packet_counter; // perpetually incrementing packet id
  
  bool run_writer;

  libusb_context* usb_context;
  libusb_device_handle* usb_handle;
  callback_t read_callback;
  
public:

  PeachyUsb(uint32_t buffer_size) {
    libusb_init(&this->usb_context);
    this->usb_handle = libusb_open_device_with_vid_pid(this->usb_context, 0x16d0, 0x0af3);
    if (this->usb_handle == NULL) {
      throw std::runtime_error("Failed to get device handle");
    }
    if (libusb_claim_interface(this->usb_handle, 0) != 0) {
      throw std::runtime_error("Failed to claim interface");
    }

  }
  ~PeachyUsb() {
    if (this->usb_context) {
      this->run_writer = false;
    }
    this->usb_writer.join();
  }
  int start() {
    if (this->state != UNINITIALIZED) {
      return;
    }

    this->run_writer = true;
    this->usb_writer = std::thread(writer_func, this);
    this->usb_reader = std::thread(reader_func, this);
    return 0;
  }


  void set_read_callback(callback_t callback) {
    this->read_callback = callback;
  }

private:
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
