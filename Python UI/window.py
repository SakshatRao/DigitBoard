# %%
import tkinter
import time
import numpy as np
# %%
WINDOW_WIDTH = 1250
WINDOW_HEIGHT = 600
# %%
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
# %%
window = tkinter.Tk()
ex = Example()
window.title("DigitBoard")
window.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}+10+20")
# %%
heading1 = tkinter.Label(window, text = "DigitBoard User Interface", fg = 'red', font = ("Helvetica", 16))
heading1.place(x = WINDOW_WIDTH * 2 / 5, y = 0)
# %%
v0 = tkinter.IntVar()
v0.set(1)
r1 = tkinter.Radiobutton(window, text = "Text Mode", variable = v0, value = 1)
r1.place(x = WINDOW_WIDTH * 2 / 3, y = WINDOW_HEIGHT / 3)
# %%
window.update()
# %%
digits = ['|']
digit_output = tkinter.Label(window, text = " ".join(digits))
digit_output.place(x = 0, y = WINDOW_HEIGHT / 5)
# %%
while True:
    time.sleep(1)
    digits.append(str(np.random.randint(low = 0, high = 10)))
    digit_output[ "text" ] = ' '.join(digits)
    window.update()

tkinter.mainloop()
# %%
