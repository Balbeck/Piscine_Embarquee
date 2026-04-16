#include <avr/io.h>
#include <util/delay.h>

#define LED2_B PB1
#define BUTTON_INC_D PD2
#define BUTTON_DEC_D PD4


int main() {

	DDRB |= (1 << LED2_B);
	DDRD &= ~(1 << BUTTON_INC_D); //-> 0 = IN
	DDRD &= ~(1 << BUTTON_DEC_D); //-> 0 = IN
	PORTD |= (1 << BUTTON_INC_D); //-> 1 = Pull up
	PORTD |= (1 << BUTTON_DEC_D); //-> 1 = Pull up

	// [ Mode 14 Fast-PWM ]
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << WGM13);

	// [ Prescaler / 256 ]
	TCCR1B |= (1 << CS12);

	// [ Compare Output Mode for Channel A - HIGH then LOW TCNT1(Compteur) = OCR1A ]
	TCCR1A |= (1 << COM1A1);
	
	// Utilise un long a cause risque Doverflow d'un Unsigned int > 65535 ! (top + decil > 65535 !)
	unsigned long top = 62499;
	ICR1 = top;

	// On definit le Raport Cyclique a 10%
	unsigned long decil = top * 0.1;
	unsigned long duty = decil;
	OCR1A = duty;


	while (1) {
		if( !(PIND & (1 << BUTTON_INC_D)) ) {
			_delay_ms(20);
			unsigned long new_value = duty + decil;
			if (new_value >= top) {
				OCR1A = top;
				duty = top;
			} else {
				OCR1A = new_value;
				duty = new_value;
			}
			while( !(PIND & (1 << BUTTON_INC_D)) );
			_delay_ms(20);
		}
		if( !(PIND & (1 << BUTTON_DEC_D)) ) {
			_delay_ms(20);
			unsigned long new_value = duty - decil;
			if (new_value >= decil ) {
				OCR1A = new_value;
				duty = new_value;
			} else {
				OCR1A = decil;
				duty = decil;
			}
			while( !(PIND & (1 << BUTTON_DEC_D)) );
			_delay_ms(20);
		}
	}

	return(0);

}
