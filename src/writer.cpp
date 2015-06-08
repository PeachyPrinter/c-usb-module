#include <stdint.h>
#include <libusb.h>

class UsbWriter {
  uint32_t write_r_index; // index to read from
  uint32_t write_count; // number of packets ready to read (written and unread)
  uint32_t write_capacity; // total capacity
  packet_t* write_packets;

public:
  UsbWriter(uint32_t capacity, libusb_device_handle* dev) {
    this->write_capacity = buffer_size;
    this->write_r_index = 0;
    this->write_count = 0;
    this->write_packets = (packet_t*)malloc(sizeof(packet_t) * buffer_size);
    this->max_inflight = 40; // 40 ~= 20 milliseconds worth of packets

  }
private:

  static void LIBUSB_CALL write_complete_callback(struct libusb_transfer* transfer) {
    writer_callback_data_t* callback_data = (writer_callback_data_t*)transfer->user_data;
    PeachyUsb* ctx = callback_data->ctx;
    uint32_t packet_id = callback_data->packet_id;
    delete callback_data;

    // TODO do some error handling here

    ctx->remove_inflight_id(packet_id);
    libusb_free_transfer(transfer);
  }

  static void writer_func(PeachyUsb* ctx) {
    unsigned char buf[64] = { 0 };
    int packet_size;
    writer_callback_data_t* callback_data;

    while (ctx->run_writer) {
      struct libusb_transfer* transfer = libusb_alloc_transfer(0);
      ctx->get_from_write_queue(buf, 64, &packet_size);

      int packet_id = ctx->get_next_inflight_id();

      callback_data = new writer_callback_data_t();
      callback_data->ctx = ctx;
      callback_data->packet_id = packet_id;

      libusb_fill_bulk_transfer(transfer, ctx->usb_handle, 2, buf, packet_size, write_complete_callback, (void*)callback_data, 2000);


    }
    libusb_release_interface(ctx->usb_handle, 0);
    libusb_exit(ctx->usb_context);
    ctx->usb_handle = NULL;
    ctx->usb_context = NULL;
  }

  void get_from_write_queue(unsigned char* buf, uint32_t length, int* transferred) {
    std::unique_lock<std::mutex> lck(mtx);
    while (this->write_count == 0) {
      read_avail.wait(lck);
    }
    packet_t* pkt = &this->write_packets[this->write_r_index];
    uint32_t read_count = (pkt->length > length ? length : pkt->length);
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
  uint32_t get_next_inflight_id(void) {
    uint32_t inflight_id;
    std::unique_lock<std::mutex> lock(this->inflight_mtx);
    while (this->inflight.size() >= this->max_inflight) {
      this->async_room_avail.wait(lock);
    }
    this->inflight.insert(this->packet_counter);
    inflight_id = packet_counter;
    packet_counter++;
    return inflight_id;
  }
  void remove_inflight_id(uint32_t inflight_id) {
    std::unique_lock<std::mutex> lock(this->inflight_mtx);
    this->inflight.erase(inflight_id);
    this->async_room_avail.notify_one();
  }
}