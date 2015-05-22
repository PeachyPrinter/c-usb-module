import threading
import peachyusb
import time

p = peachyusb.PeachyUSB(500)
count = int(1e7)

def read(buf, length):
    print "Got %d bytes from usb: %r" % (length, buf)

p.set_read_callback(read)

p.write("\x07")

def worker():
    while True:
        s = 0
        for x in xrange(int(1e9)):
            s += x

def writer():
    start = time.time()
    writecount = 0
    for x in xrange(count):
        if x % 10000 == 0:
            end = time.time()
            delta = end - start
            if delta == 0:
                delta = 0.0001
            print "Wrote %d bytes in %f sec (%f bytes/sec)" % (writecount, delta, writecount/delta)
            start = end
            writecount = 0
        b = "\x02\x08\xff\xd7\x01\x10\xff\xff\x01\x18\n"
        writecount += len(b)
        p.write(b)
    print "Done Writing"

t2 = threading.Thread(target=writer)
t2.daemon = True
t2.start()

# for x in xrange(2):
#     t = threading.Thread(target=worker)
#     t.start()

#t1.join()
while t2.is_alive():
    t2.join(1)
