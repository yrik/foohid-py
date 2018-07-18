import foohid
import struct
import time

from stoppableThread import StoppableThread

report_descriptor = (
    0x06, 0xD0, 0xF1,  # Usage Page (Reserved 0xF1D0)
    0x09, 0x01,        # Usage (0x01)
    0xA1, 0x01,        # Collection (Application)
    0x09, 0x20,        #   Usage (0x20)
    0x15, 0x00,        #   Logical Minimum (0)
    0x26, 0xFF, 0x00,  #   Logical Maximum (255)
    0x75, 0x08,        #   Report Size (8)
    0x95, 0x40,        #   Report Count (64)
    0x81, 0x02,        #   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x21,        #   Usage (0x21)
    0x15, 0x00,        #   Logical Minimum (0)
    0x26, 0xFF, 0x00,  #   Logical Maximum (255)
    0x75, 0x08,        #   Report Size (8)
    0x95, 0x40,        #   Report Count (64)
    0x91, 0x02,        #   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              # End Collection
   
)
def u2fCallback(obj):
    print("u2fCallback")
    # print(obj)
    # a = struct.unpack_from("(y#)",obj)
    print(obj.hex())

import time

def u2fWorker(t):    
    import time
    while 1:        
        print("will u2fWorker")
        time.sleep(1)


def main():    
    import signal
    import sys

    def SIGINT_handler(*_, **__):
        print('\nA SIGINT (CTRL-C) signal is detected')
        foohid.destroy("FooHID simple u2f", u2fCallback)
        sys.exit(0)
        t.stop()
    signal.signal(signal.SIGINT, SIGINT_handler)

    t = StoppableThread(u2fWorker)    
    t.daemon = True
    t.start()
    print("t.start")

    try:
        foohid.destroy("FooHID simple u2f")
    except:
        pass
        
    foohid.create("FooHID simple u2f", struct.pack('{0}B'.format(len(report_descriptor)), *report_descriptor), "SN 123", 2, 3)
    foohid.subscribe("FooHID simple u2f", u2fCallback)

    from CoreFoundation import CFRunLoopRun

    runner = CFRunLoopRun()


if __name__ == '__main__':
    main()
