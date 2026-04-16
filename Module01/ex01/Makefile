MCU = atmega328p #donne au compiler (avr-gcc) pour quel microcontroleur on compile
# -> Micro Controller Unit
F_CPU = 16000000UL #La frequence du CPU en Hertz (16MHz pour ATmega328p)
BAUD = 115200 #La vitesse de communication pour Flasher
PORT = /dev/tty.usbserial-110 # resultat de la cmd `ls /dev/tty.*`, c'est le port usb ou la carte est branchee
NAME = main

all: hex flash

hex:
	avr-gcc -mmcu=$(MCU) -DF_CPU=$(F_CPU) -o $(NAME).bin $(NAME).c
	avr-objcopy -O ihex $(NAME).bin $(NAME).hex

flash:
	avrdude -c arduino -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$(NAME).hex

clean: 
	rm -rf $(NAME).bin $(NAME).hex

run: clean hex flash 
