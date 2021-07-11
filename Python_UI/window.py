import tkinter
import time
import numpy as np
import pygatt
from binascii import hexlify

WINDOW_WIDTH = 1250
WINDOW_HEIGHT = 600

MAX_DIGITS_LEN = 20

DIGITS = []
CURSOR_POS = 0

COMMUNICATION_STARTED = False

NEW_DIGIT = 0
NEW_DIGIT_ARRIVED = False

NONE_CODE = 1
DIGIT_ACK_CODE = 2

class Example(tkinter.Frame):

    def __init__(self):
        super().__init__()
        self.initUI()

    def initUI(self):
        self.master.title("Colours")
        self.pack(fill = tkinter.BOTH, expand = 1)

        canvas = tkinter.Canvas(self)
        canvas.create_rectangle(0, WINDOW_HEIGHT / 5, WINDOW_WIDTH / 2, WINDOW_HEIGHT * 4 / 5, outline = "#fb0", fill = "#fb0")
        canvas.pack(fill = tkinter.BOTH, expand = 1)

window = tkinter.Tk()
ex = Example()
window.title("DigitBoard")
window.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}+10+20")

heading1 = tkinter.Label(window, text = "DigitBoard User Interface", fg = 'red', font = ("Helvetica", 16))
heading1.place(x = WINDOW_WIDTH * 2 / 5, y = 0)

v0 = tkinter.IntVar()
v0.set(1)
r1 = tkinter.Radiobutton(window, text = "Text Mode", variable = v0, value = 1)
r1.place(x = WINDOW_WIDTH * 2 / 3, y = WINDOW_HEIGHT / 3)

window.update()

digit_output = tkinter.Label(window, text = "", fg = 'black', bg = '#fb0', font = ('Helvetica', 16))
digit_output.place(x = 0, y = WINDOW_HEIGHT / 5)

def add_new_digit(digit, digits, cursor_pos):
    if((cursor_pos > MAX_DIGITS_LEN) | (cursor_pos < 0)):
        print("Not possible!")
        return digits, cursor_pos
    
    if(len(digits) == MAX_DIGITS_LEN):
        del(digits[0])
        cursor_pos = cursor_pos - 1
    digits.insert(cursor_pos, digit)
    cursor_pos = cursor_pos + 1
    return digits, cursor_pos

def convert_digits_to_str(digits, cursor_pos):
    digits_str = []
    for digit_idx, digit in enumerate(digits):
        if(digit_idx == cursor_pos):
            digits_str.append("|")
        digits_str.append(str(digit))
    if(len(digits) == cursor_pos):
        digits_str.append("|")
    return digits_str

adapter = pygatt.GATTToolBackend()

def handle_data(handle, value):
    global COMMUNICATION_STARTED
    if(COMMUNICATION_STARTED == True):
        global NEW_DIGIT
        global NEW_DIGIT_ARRIVED
        if(NEW_DIGIT_ARRIVED == False):
            new_digit = hexlify(value).decode("utf-8")[1]
            print(f"Received digit: {new_digit}")
            NEW_DIGIT = new_digit
            NEW_DIGIT_ARRIVED = True

try:
    adapter.start()
    device = adapter.connect('6c:5a:51:5f:5d:63')

    print("Initializing ")
    device.char_write("a1e8f5b1-696b-4e4c-87c6-69dfe0b0093b", bytearray([DIGIT_ACK_CODE]), wait_for_response = True)
    
    COMMUNICATION_STARTED = True
    print("Connection Initiating...")

    device.subscribe("936b6a25-e503-4f7c-9349-bcc76c22b8c3", callback = handle_data)
    while True:
        if(NEW_DIGIT_ARRIVED == True):
            (DIGITS, CURSOR_POS) = add_new_digit(NEW_DIGIT, DIGITS, CURSOR_POS)
            digits_text = convert_digits_to_str(DIGITS, CURSOR_POS)
            digit_output[ "text" ] = ' '.join(digits_text)
            window.update()
            NEW_DIGIT_ARRIVED = False
            time.sleep(1)
finally:
    adapter.stop()