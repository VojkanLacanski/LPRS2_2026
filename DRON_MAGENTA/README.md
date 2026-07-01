DRON MAGENTA

Projekat ima implemetirano poketanje motora i osnovnu PID regulaciju. Dron za cilj ima kratko odvajanje od podloge i vracanje na istu, uz regulaciju oscilacija koje se javljaju prilikom letenja.

Bezbednosne mere pre poletanja:

* Proveriti stanje propelera
* Proveriti da su svi zavrtnji dobro zategnuti (u smeru suprotnom od smera kretanja propelera)
* Proveriti napon baterije i da li je puna
* Proveriti da je memorijska kartica ukljucena u ESP32
* Testirati WiFi konekciju pre poletanja



Pokretanje projekta:

1. Build project
2. Flash project na ESP32 preko USB-a
3. Vracanje ESP-a na protoboard
4. Povezivanje napajanja (baterije)
5. Povezivanje laptopa na WiFi mrezu drona - username: DRON-magenta, password: 12345678
6. Pokretanje Python skripte za citanje komandi motora sa tastature komandom "python3 controls.py" iz direktorijuma u kom se ona nalazi
7. Upravljanje jacinom motora preko tastature (UP - povecaj brzinu, DOWN - smanji brzinu)
8. Hitno gasenje motora pritiskom na dugme SPACE
9. Klikom na dugme 'q' se izlazi iz skripte



Trenutni parametri za PID regulaciju:

1. Kp = 1.3 - 1.5  (proporcionalna korekcija)
2. Ki = 0.0002 - 0.0005  (eliminacija drifta)
3. Kd = 0.3  (prigusivanje oscilacija)



Organizacija softvera:
main.c          - Inicijalizacija sistema, WiFi AP, UDP server
mpu.c           - MPU6050 drajver, PID kontroler, motor control task
mpu.h           - Definicije i deklaracije funkcija
controls.py     - Python skripta za daljinsko upravljanje



PID regulacija i obrada senzora

Flight controller koristi MPU6050 senzor za ocitavanje akcelerometra i ziroskopa. Pre pokretanja motora vrsi se kalibracija senzora dok dron miruje, cime se odredjuju pocetni offseti i smanjuje greska ocitavanja.

Zbog vibracija motora i sasije dodat je NF/DLPF filter na MPU6050 senzoru. Filter se podesava upisom u CONFIG registar senzora:
mpu6050_write_reg(0x1A, 0x02);

Vrednost 0x02 je izabrana kao kompromis izmedju smanjenja suma i brzog odziva senzora. Jaci filter smanjuje vibracije, ali uvodi vece kasnjenje, sto moze negativno uticati na PID regulaciju.

Roll i pitch uglovi se racunaju kombinovanjem ziroskopa i akcelerometra pomocu complementary filtera. Ziroskop daje brz odziv, dok akcelerometar dugorocno koriguje drift.

PID regulator racuna korekcije za roll, pitch i yaw osu. Korekcije se zatim raspodeljuju na cetiri motora preko motor mixing formule:

m1 = base + pitch_corr + roll_corr - yaw_corr;
m2 = base - pitch_corr + roll_corr + yaw_corr;
m3 = base - pitch_corr - roll_corr - yaw_corr;
m4 = base + pitch_corr - roll_corr + yaw_corr;

Podesavanje PID parametara vrseno je postepeno: prvo samo P clan, zatim dodavanje D clana za prigusenje oscilacija, i na kraju mali I clan za korekciju sporog drifta.




OpenHD video prenos i joystick komunikacija

Za prenos slike koristi se OpenHD sistem. OpenHD omogucava bezicni video link izmedju air unit-a, koji se nalazi na dronu, i ground unit-a, koji se nalazi na racunaru korisnika.

Air unit je realizovan pomocu Raspberry Pi 4 racunara i Raspberry Camera Module 3 kamere. Kamera je povezana na Raspberry Pi, a video signal se preko OpenHD-a strimuje ka ground unit-u. Na ground unit strani se prima video prenos u realnom vremenu, sto omogucava pregled slike sa drona tokom rada sistema.

Povezivanje OpenHD sistema obuhvata:

1. Flash OpenHD AirUnit softvera na SD karticu za Raspberry Pi 4
2. Flash OpenHD GroundUnit softvera na USB/fles memoriju
3. Povezivanje Raspberry Camera Module 3 kamere na Raspberry Pi 4
4. Pokretanje air unit-a na Raspberry Pi racunaru
5. Pokretanje ground unit-a na racunaru korisnika
6. Uspostavljanje veze izmedju air unit-a i ground unit-a

Za buduce upravljanje dronom planirano je koriscenje joystick-a. Joystick je povezan sa OpenHD ground unit-om i testirano je uspesno primanje komandi sa joystick-a. Na taj nacin je provereno da ground unit moze da detektuje ulazne komande, sto predstavlja osnovu za kasnije povezivanje joystick upravljanja sa flight controller-om.

YOUTUBE SNIMAK: https://youtube.com/shorts/vHXbZhPBSl8?si=-V2OrkggPQL_lESc




