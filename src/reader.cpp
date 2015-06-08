/*static void reader_func(PeachyUsb* ctx) {
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

*/