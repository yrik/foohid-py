# Pre Installation

## foohid
https://github.com/unbit/foohid.git

1. 到 recovery mode disable kext
    a. 重開機， cmd + R 進 Recovery mode
    b. 打開 terminal: util -> terminal
    c. $ csrutil disable
    d. $ csrutil enable --without kext 

2. 下載專案，編完 foohid.text 放到 /Library/Extensions

3. $ sudo kextload /Library/Extensions/foohid.kext

## other

$ pip3 install pyobjc

# Installation

```bash
$ python3 setup.py install
```

# Update

1. 在 foohid.c 實作 subscribe
2. python 層設定 callback 到 C
3. 不要在 C 層呼叫 CFRunLoopRun, 這樣會卡住 python thread
4. 透過 pyobjc，在 python 呼叫底層 CFRunLoop 


# Test

$ python3 test_mouse.py

# foohid-py
Python wrapper for the foohid OSX driver. Compatible with Python 2 and 3.

https://github.com/unbit/foohid

Exposed functions
=================

```py
import foohid

foohid.create('your new device', report_descriptor, serial_number, vendor_id, device_id)
foohid.destroy('your new device')
foohid.send('your new device', hid_message)
foohid.list()
```

Included tests/examples
=======================

test_mouse.py -> creates a virtual mouse and randomly moves it (your cursor will move too ;)

test_joypad.py -> creates a virtual joypad and randomly moves left and right axis (run a game with joypad support to check it)

test_keyboard.py -> creates a virtual keyboard and presses the "a" key

test_list.py -> tests listing feature

Troubleshooting
===============

Currently, there is a bug in the original foohid package where the userclient is not properly terminated.
If the code is run repeatedly, this sometimes results in a "unable to open it_unbit_foohid service" error.
If this happens, unload and remove the kernel extension:

```bash
$ sudo kextunload /Library/Extensions/foohid.kext
$ sudo rm -rf /System/Library/Extensions/foohid.kext
```

Then reinstall the kernel extension: https://github.com/unbit/foohid/releases/latest (no restart required)


