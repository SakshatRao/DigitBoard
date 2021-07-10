# %%
import pygatt
from binascii import hexlify
import time
adapter = pygatt.backends.GATTToolBackend()
# %%
try:
    adapter.start()
    device = adapter.connect('01:23:45:67:89:ab')
    value = device.char_read("a1e8f5b1-696b-4e4c-87c6-69dfe0b0093b")
finally:
    adapter.stop()
# %%
