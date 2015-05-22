
import ctypes

class peachyusb_t(ctypes.Structure):
    pass

peachyusb_t_p = ctypes.POINTER(peachyusb_t)

def _loadLibrary():
    dll = ctypes.CDLL("PeachyUSB.dll")
    dll.peachyusb_init.argtypes = [ctypes.c_uint]
    dll.peachyusb_init.restype = peachyusb_t_p
    
    dll.peachyusb_read.argtypes = [peachyusb_t_p, ctypes.c_char_p, ctypes.c_uint]
    dll.peachyusb_read.restype = ctypes.c_int

    dll.peachyusb_write.argtypes = [peachyusb_t_p, ctypes.c_char_p, ctypes.c_uint]
    dll.peachyusb_write.restype = None
    return dll

lib = _loadLibrary()

class PeachyUSB(object):
    def __init__(self, capacity):
        self.context = lib.peachyusb_init(capacity)

    def write(self, buf):
        lib.peachyusb_write(self.context, buf, len(buf))
        
    def read(self):
        buf = ctypes.create_string_buffer(64)
        count = lib.peachyusb_read(self.context, buf, 64)
        return buf.raw[:count]
        
