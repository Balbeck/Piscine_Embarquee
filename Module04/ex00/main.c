#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define LED1_B PB0
#define BUTTON1_D PD2

volatile uint8_t flag_button = 0;

// Interrupt Service Routine 
// INT0_vect=InteruptionExterneSurPd2
// INT1_vect=InteruptionExterneSurPd3
ISR(INT0_vect){ 
	// _delay_ms(20);
	// PORTB ^= (1 << LED1_B);
	flag_button = 1;
	// EIMSK &= ~(1 << INT0); //Desactive temporaire temps traitement signal button
}

int	main(void){

	DDRB |= (1 << LED1_B); // Met D1 en Sortie
	DDRD &= ~(1 << BUTTON1_D); // Met SW1 En Entree (IN) - Mais deja a 0 par default !
	PORTD |= (1 << BUTTON1_D);// Configure SW1 en pull-up(Entree)

	// Configure declenchement ISCx1 ISCx0 sur le Registre EICRA - External Interrupt Control Register A
	// Button en general '(10)Font descendant(appui 5v -> 0v)' ou '(01)tout changement'
	EICRA |= (1 << ISC01);
	EICRA &= ~(1 << ISC00);
	EIMSK |= (1 << INT0);  // Active INT0
	// Maintenant Configures On active les Interrupts !
	// SREG - Status Register -> Registre D'etat du CPU, bit7: flag global Interupt
	// SREG |= (1 << 7);
	SREG |= (1 << 7);// Equivalent sei() -> active les Interuption!

	while(1){

		if (flag_button) {
			_delay_ms(100);
			PORTB ^= (1 << LED1_B);
			// EIMSK |= (1 << INT0); //reactive INT 0
			flag_button = 0;
		}
	}

	return(0);
}
