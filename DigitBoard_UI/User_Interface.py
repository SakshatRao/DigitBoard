####################################################################################
# Imports
####################################################################################

import tkinter
import time
import numpy as np
import serial
import threading




####################################################################################
# Parameters
####################################################################################

WINDOW_WIDTH = 1250                                     # Width of window
WINDOW_HEIGHT = 600                                     # Height of window

MAX_DIGITS_LEN = 20                                     # Max no. of digits displayed

DIGITS = []                                             # List to hold displayed digits
CURSOR_POS = 0                                          # Index at which next digit will be placed

DIGIT_COLORS = ['black', 'red', 'green', 'blue']        # List of available font colors
DIGIT_NUM_COLORS = len(DIGIT_COLORS)                    # No. of available font colors
DIGIT_COLOR_IDX = 0                                     # Index of currently chosen font color

DIGIT_SIZES = [12, 16, 20, 24, 28, 32]                  # List of available font sizes
DIGIT_NUM_SIZES = len(DIGIT_SIZES)                      # No. of available font sizes
DIGIT_SIZE_IDX = 2                                      # Index of currently chosen font size

COMMUNICATION_STARTED = False                           # Whether serial communication has started

NEW_DIGIT = 0                                           # Variable to hold newly received digit
NEW_DIGIT_ARRIVED = False                               # Whether a new digit has been received

NEW_CMD = ''                                            # Variable to hold newly received command
NEW_CMD_ARRIVED = False                                 # Whether a new command has been received




####################################################################################
# Tkinter Initialization
####################################################################################

# For main text box
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

# For title
window.title("DigitBoard")
window.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}+10+20")

# For main heading
heading1 = tkinter.Label(window, text = "DigitBoard User Interface", fg = 'black', font = ("Helvetica", 32))
heading1.place(x = 0, y = 0)

# For received digit/command info
heading2 = tkinter.Label(window, text = "", fg = 'red', font = ("Helvetica", 16))
heading2.place(x = 0, y = 50)

# For font color selection
heading_color = tkinter.Label(window, text = "Font Color", fg = 'black', font = ("Helvetica", 16))
heading_color.place(x = WINDOW_WIDTH * 2 / 3, y = WINDOW_HEIGHT / 3 - 40)
v0 = tkinter.IntVar()
v0.set(1)
r = []
for color_idx, color in enumerate(DIGIT_COLORS):
    if(color_idx == DIGIT_COLOR_IDX):
        r1 = tkinter.Radiobutton(window, text = color, variable = v0, value = 1, fg = color)
    else:
        r1 = tkinter.Radiobutton(window, text = color, variable = v0, value = 0)
    r1.place(x = WINDOW_WIDTH * 2 / 3, y = WINDOW_HEIGHT / 3 + color_idx * 20)
    r.append(r1)

# For font size selection
heading_size = tkinter.Label(window, text = "Font Size", fg = 'black', font = ("Helvetica", 16))
heading_size.place(x = WINDOW_WIDTH * 2 / 3, y = 2 * WINDOW_HEIGHT / 3 - 40)
scale_widget = tkinter.Scale(window, from_ = max(DIGIT_SIZES), to = min(DIGIT_SIZES), orient = tkinter.VERTICAL)
scale_widget.set(DIGIT_SIZES[DIGIT_SIZE_IDX])
scale_widget.place(x = WINDOW_WIDTH * 2 / 3, y = 2 * WINDOW_HEIGHT / 3)

# For actual output to be displayed
digit_output = tkinter.Label(window, text = "|", fg = DIGIT_COLORS[DIGIT_COLOR_IDX], bg = '#fb0', font = ('Helvetica', DIGIT_SIZES[DIGIT_SIZE_IDX]))
digit_output.place(x = 0, y = WINDOW_HEIGHT / 5)

# Updating all tkinter objects
window.update()





####################################################################################
# Processing Functions
####################################################################################

# To add newly received digit to display
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

# To display all received digits
def convert_digits_to_str(digits, cursor_pos):
    digits_str = []
    for digit_idx, digit in enumerate(digits):
        if(digit_idx == cursor_pos):
            digits_str.append("|")
        digits_str.append(str(digit))
    if(len(digits) == cursor_pos):
        digits_str.append("|")
    return digits_str

# To map gestures to commands
def handle_cmd(cmd, digits, cursor_pos, color_idx, size_idx, num_colors, num_sizes):
    if(cmd == 'r'):
        if(len(digits) > cursor_pos):
            cursor_pos += 1
    elif(cmd == 'l'):
        if(cursor_pos > 0):
            cursor_pos -= 1
    elif(cmd == 'b'):
        if(size_idx < num_sizes - 1):
            size_idx = size_idx + 1
    elif(cmd == 's'):
        if(size_idx > 0):
            size_idx = size_idx - 1
    elif(cmd == 'u'):
        if(color_idx == num_colors - 1):
            color_idx = 0
        else:
            color_idx = color_idx + 1
    elif(cmd == 'd'):
        if(color_idx == 0):
            color_idx = num_colors - 1
        else:
            color_idx = color_idx - 1
    else:
        print(f"Unknown Command: {cmd}")
    return cursor_pos, color_idx, size_idx

# To display command
def cmd_str(cmd):
    if(cmd == 'r'):
        return "Move cursor right"
    elif(cmd == 'l'):
        return "Move cursor left"
    elif(cmd == 'b'):
        return "Increase font size"
    elif(cmd == 's'):
        return "Decrease font size"
    elif(cmd == 'u'):
        return "Choose lower color"
    elif(cmd == 'd'):
        return "Choose upper color"
    else:
        return "Unknown command"





####################################################################################
# Thread to continuously receive new digits/commands
####################################################################################

def ReceiveThread():

    # For using & updating global variables
    global serialPort
    global COMMUNICATION_STARTED
    global NEW_DIGIT
    global NEW_DIGIT_ARRIVED
    global NEW_CMD
    global NEW_CMD_ARRIVED

    # 1. Waiting for NEW_DIGIT_ARRIVED/NEW_COMMAND_ARRIVED to be False
    # 2. Continously receiving new digits & commands
    # 3. Setting NEW_DIGIT_ARRIVED/NEW_COMMAND_ARRIVED to True
    while(serialPort):
        if(COMMUNICATION_STARTED == True):
            if(NEW_DIGIT_ARRIVED == False):
                if(serialPort.in_waiting > 0):
                    serialString = serialPort.read().strip()
                    if(len(serialString) == 1):
                        serialString = serialString.decode("ASCII")
                        if(serialString in [str(x) for x in range(10)]):
                            new_digit = int(serialString)
                            print(f"Received digit: {new_digit}")
                            NEW_DIGIT = new_digit
                            NEW_DIGIT_ARRIVED = True
                        else:
                            new_cmd = serialString
                            print(f"Received command: {serialString}")
                            NEW_CMD = new_cmd
                            NEW_CMD_ARRIVED = True






####################################################################################
# Updating display
####################################################################################

try:
    # Initializing serial communication via handshake
    serialPort = serial.Serial(port = "/dev/ttyACM0", baudrate = 115200, bytesize = 8, timeout = 2, stopbits = serial.STOPBITS_ONE)
    print("Initializing ")    
    serialPort.write(b"2")
    COMMUNICATION_STARTED = True
    print("Connection Initiating...")

    # Starting thread to continuously receive digits/commands
    threading.Thread(target = ReceiveThread).start()

    # 1. Waiting for NEW_DIGIT_ARRIVED/NEW_COMMAND_ARRIVED to be True
    # 2. Processing & displaying new digits & commands
    # 3. Setting NEW_DIGIT_ARRIVED/NEW_COMMAND_ARRIVED to False
    while True:
        if(NEW_DIGIT_ARRIVED == True):
            (DIGITS, CURSOR_POS) = add_new_digit(NEW_DIGIT, DIGITS, CURSOR_POS)
            digits_text = convert_digits_to_str(DIGITS, CURSOR_POS)
            digit_output["text"] = ' '.join(digits_text)
            heading2['text'] = f"Received Digit: {NEW_DIGIT}!"
            NEW_DIGIT_ARRIVED = False
        
        if(NEW_CMD_ARRIVED == True):
            (CURSOR_POS, DIGIT_COLOR_IDX, DIGIT_SIZE_IDX) = handle_cmd(NEW_CMD, DIGITS, CURSOR_POS, DIGIT_COLOR_IDX, DIGIT_SIZE_IDX, DIGIT_NUM_COLORS, DIGIT_NUM_SIZES)
            digits_text = convert_digits_to_str(DIGITS, CURSOR_POS)
            digit_output["text"] = ' '.join(digits_text)
            digit_output["fg"] = DIGIT_COLORS[DIGIT_COLOR_IDX]
            digit_output["font"] = ('Helvetica', DIGIT_SIZES[DIGIT_SIZE_IDX])
            heading2['text'] = f"Received Command: {cmd_str(NEW_CMD)}!"
            NEW_CMD_ARRIVED = False
        
        for color_idx, color in enumerate(DIGIT_COLORS):
            if(color_idx == DIGIT_COLOR_IDX):
                r[color_idx]['value'] = 1
                r[color_idx]['fg'] = color
            else:
                r[color_idx]['value'] = 0
                r[color_idx]['fg'] = 'black'
        scale_widget.set(DIGIT_SIZES[DIGIT_SIZE_IDX])

        window.update()
        time.sleep(1)

finally:
    serialPort.close()