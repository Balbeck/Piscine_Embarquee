MCU = atmega328p
FREQUENCE_CPU = 16000000UL
BAUD_RATE = 115200 # Pour la synchronisation des cycles d'envoie/reception
#PORT_PHYSIQUE = /dev/tty.usbserial-110
PORT_PHYSIQUE = /dev/ttyUSB0
NAME = main

all: hex flash

hex:
	avr-gcc -mmcu=$(MCU) -DF_CPU=$(FREQUENCE_CPU) -o $(NAME).bin $(NAME).c
	avr-objcopy -O ihex $(NAME).bin $(NAME).hex

flash:
	avrdude -c arduino -p $(MCU) -P $(PORT_PHYSIQUE) -b $(BAUD_RATE) -U flash:w:$(NAME).hex

clean:
	rm -rf $(NAME).bin

fclean:
	rm -rf $(NAME).hex $(NAME).bin

screen:
	screen $(PORT_PHYSIQUE) $(BAUD_RATE)

run: fclean hex flash
	@echo "Lets go Baby, Gi'me the Lighttt ⚡️ 💡 📟 ✨ !"
