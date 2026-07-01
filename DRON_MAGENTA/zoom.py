import cv2
import pygame
import sys

pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    print("Nije pronadjen dzojstik")
    sys.exit()

joystick = pygame.joystick.Joystick(0)
joystick.init()
print(self_name := f"Povezan dzojstik: {joystick.get_name()}")

cap = cv2.VideoCapture(0)

zoom_factor = 1.0 

print("Program radi. Pomeraj levu palicu gore/dole da zumiras. Pritisni 'q' na tastaturi za izlaz.")

try:
    while True:
        pygame.event.pump()
        
        axis_val = joystick.get_axis(1)
        
        if axis_val < -0.2:
            zoom_factor += 0.05
        elif axis_val > 0.2:
            zoom_factor -= 0.05
            
        zoom_factor = max(1.0, min(zoom_factor, 5.0))

        ret, frame = cap.read()
        if not ret:
            break

        h, w, _ = frame.shape

        if zoom_factor > 1.0:
            new_h, new_w = int(h / zoom_factor), int(w / zoom_factor)
            
            start_y = (h - new_h) // 2
            start_x = (w - new_w) // 2
     
            cropped = frame[start_y:start_y+new_h, start_x:start_x+new_w]

            frame = cv2.resize(cropped, (w, h), interpolation=cv2.INTER_LINEAR)

        cv2.putText(frame, f"Zoom: {zoom_factor:.2f}x", (10, 30), 
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

        cv2.imshow('Dzojsitk Kamera Zum', frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

finally:
    cap.release()
    cv2.destroyAllWindows()
    pygame.quit()