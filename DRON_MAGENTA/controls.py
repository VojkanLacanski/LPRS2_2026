import socket
import time
import threading
from pynput import keyboard

ESP_IP = "192.168.4.1"
ESP_PORT = 3333

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

throttle = 1000
running = True

def send_data():
    global throttle, running
    while running:
        message = str(throttle)
        sock.sendto(message.encode(), (ESP_IP, ESP_PORT))
        time.sleep(0.05)

def on_press(key):
    global throttle, running
    if key == keyboard.Key.up:
        if throttle < 2000:
            throttle += 3
            print(f"Throttle UP: {throttle}")
    elif key == keyboard.Key.down:
        if throttle > 1000:
            throttle -= 3
            print(f"Throttle DOWN: {throttle}")
    elif key == keyboard.Key.space:
        while throttle > 1000:
            throttle -= 3
            if throttle < 1000:
                throttle = 1000
            time.sleep(0.02)
        print("EMERGENCY STOP")
    try:
        if key.char == 'q':
            print("Exiting the programme...")
            running = False
            return False
    except AttributeError:
        pass

t = threading.Thread(target = send_data)
t.start()

print(f"Connecting to {ESP_IP}:{ESP_PORT}...")
print("Controls:")
print(" UP - Accelerate(+3)")
print(" DOWN - Decelerate(-3)")
print(" SPACE - EMERGENCY STOP")
print(" Q - Exit")

with keyboard.Listener(on_press = on_press, suppress=True) as listener:
    listener.join()

running = False
t.join()
sock.close()
print("Disconnected")