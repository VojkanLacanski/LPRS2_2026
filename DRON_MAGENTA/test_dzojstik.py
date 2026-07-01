import pygame
import sys

pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    print("Dzojstik nije prepoznat")
    sys.exit()

joystick = pygame.joystick.Joystick(0)
joystick.init()

print("Pomeraj bilo sta na dzojstiku...")

try:
    while True:
        for event in pygame.event.get():
            if event.type == pygame.JOYAXISMOTION:
                if abs(event.value) > 0.1 and event.axis not in [2, 5]:
                    print(f"[PALICA] Osa {event.axis} se pomerila na: {event.value:.2f}")
            
            elif event.type == pygame.JOYBUTTONDOWN:
                print(f"[DUGME] Pritisnuto dugme broj: {event.button}")
                
        pygame.time.wait(10)
except KeyboardInterrupt:
    print("\nKraj testa.")
    pygame.quit()