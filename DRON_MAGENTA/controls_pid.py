import socket
import tkinter as tk

#konfiguracija mreze
ESP_IP = "192.168.4.1"
ESP_PORT = 3333
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

#inicijalne vrednosti i koraci
pid_values = {"P": 6.0, "I": 0.0, "D": 0.8, "YP": 0.0, "YI": 0.0, "YD": 0.0}
steps = {"P": 0.1, "I": 0.005, "D": 0.05, "YP": 0.1, "YI": 0.0005, "YD": 0.01}

def send_update(param):
    message = f"{param}{pid_values[param]:.4f}"            #u fstring formatu: P1.250  (param, vrednost u 3 decimale)
    sock.sendto(message.encode(), (ESP_IP, ESP_PORT))     # pretvara u bajtove
    label_vars[param].set(f"{param}: {pid_values[param]:.4f}")      #azurira vrednost na interfejsu
    print(f"Poslato: {message}")

def change(param, direction):         #param je promenljiva, direction je smer promene (1 -> +, -1 -> -)
    pid_values[param] += steps[param] * direction         #trenutna vrednost += ili -= korak
    if pid_values[param] < 0: 
        pid_values[param] = 0
    send_update(param)               #zapravo salje promenu vrednosti dronu

# GUI podesavanje
root = tk.Tk()    #ugradjena funkcija iz tkinder, pravi glavni prozor interfejsa
root.title("Drone PID Tuner")
#lepsi izgled interfejsa
root.geometry("400x600")
root.configure(bg = "#2e3c4b")
header = tk.Label(root, text = "PID PARAMETERS", font=("Helvetca",16,"bold"),bg = "#2c3e50", fg = "#ecf0f1", pady = 20)

header.pack()

label_vars = {}   #incijalizacija recnika, promenljive koje kontrolisu tekst na ekranu

for param in ["P", "I", "D", "YP", "YI", "YD"]:
    frame = tk.Frame(root, bg="#34495e", bd=2, relief = "groove", padx = 20, pady=10)   #ugradjena funkc iz tkinter, dodaje piksele za razmak i grupise el. u red
    frame.pack(fill = "x", padx = 30,  pady = 10)                        #tip izgleda interfejsa, jedan ispod drugog
    
    label_vars[param] = tk.StringVar(value=f"{param}: {pid_values[param]:.4f}")   #string promenljiva, menjamo automatski vrednost P I D bez izcrtavanja novog prozora
    
    lbl = tk.Label(frame, textvariable=label_vars[param], font = ("Courier", 14, "bold"),bg = "#34495e", fg = "#f1c40f",width = 10)
    lbl.pack(side=tk.LEFT)

    btn_minus = tk.Button(frame, text = "-", command = lambda p = param: change(p,-1), font =("Arial", 12, "bold"),
                          width = 4, bg = "#e74c3c", fg = "white", activebackground="#c0392b")
    
    btn_minus.pack(side=tk.LEFT, padx = 5)

    btn_plus = tk.Button(frame, text="+", command=lambda p=param: change(p, 1),
                         font=("Arial", 12, "bold"), width=4, bg="#27ae60", fg="white", 
                         activebackground="#219150")
    btn_plus.pack(side=tk.LEFT, padx=5)
root.mainloop()    #pokretanje sistema
