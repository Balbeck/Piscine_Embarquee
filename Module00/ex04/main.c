#include <avr/io.h>
#include <util/delay.h>

#define LED1_B PB0
#define LED2_B PB1
#define LED3_B PB2
#define LED4_B PB4
#define BUTTON_INC_D PD2
#define BUTTON_DEC_D PD4


// // [ Clignotement ]
void ft_gling_gling(void) {
	int i = 0;
	int ms = 42;
	while(i < 5) {
			PORTB |= (1 << LED1_B);
			PORTB &= ~(1 << LED2_B);
			PORTB &= ~(1 << LED3_B);
			PORTB &= ~(1 << LED4_B);	
			_delay_ms(ms);

			PORTB |= (1 << LED2_B);
			PORTB &= ~(1 << LED1_B);
			PORTB &= ~(1 << LED3_B);
			PORTB &= ~(1 << LED4_B);	
			_delay_ms(ms);

			PORTB |= (1 << LED3_B);
			PORTB &= ~(1 << LED2_B);
			PORTB &= ~(1 << LED1_B);
			PORTB &= ~(1 << LED4_B);	
			_delay_ms(ms);

			PORTB |= (1 << LED4_B);
			PORTB &= ~(1 << LED2_B);
			PORTB &= ~(1 << LED3_B);
			PORTB &= ~(1 << LED1_B);	
			_delay_ms(ms);
			
			i++;
	}
}


int main(){

	int c = 0;
	// [ DDRx ]
	DDRB |= (1 << LED1_B);
	DDRB |= (1 << LED2_B);
	DDRB |= (1 << LED3_B);
	DDRB |= (1 << LED4_B);
	DDRD &= ~(1 << BUTTON_INC_D); //-> 0 = IN
	DDRD &= ~(1 << BUTTON_DEC_D); //-> 0 = IN
	
	// [ PORTx ]
	PORTB &= ~(1 << LED1_B);
	PORTB &= ~(1 << LED2_B);
	PORTB &= ~(1 << LED3_B);
	PORTB &= ~(1 << LED4_B);
	PORTD |= (1 << BUTTON_INC_D); //-> 1 = Pull up
	PORTD |= (1 << BUTTON_DEC_D); //-> 1 = Pull up

	// // [ Ou ]
	// uint8_t c = 0;
	// DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB4);
	// PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB4));
	// DDRD &= ~((1 << BUTTON_INC) | (1 << BUTTON_DEC));
    // PORTD |= (1 << BUTTON_INC) | (1 << BUTTON_DEC);

	while(1) {
		// [ BUTTONS ]
		if( !(PIND & (1 << BUTTON_INC_D)) ) {
			_delay_ms(20);
			if(c <= 15) {
				if(c == 15) {
					ft_gling_gling();
				} else {
					c +=1;
				}
			}
			while( !(PIND & (1 << BUTTON_INC_D)) );
			_delay_ms(20);
		}
		if( !(PIND & (1 << BUTTON_DEC_D)) ) {
			_delay_ms(20);
			if(c >= 0) {
				if(c == 0) {
					ft_gling_gling();
				} else {
					c -= 1;
				}
			}
			while( !(PIND & (1 << BUTTON_DEC_D)) );
			_delay_ms(20);
		}

		// [ LEDS ]
		if(c == 8 || c == 9 || c == 10 || c == 11 || c ==12 || c == 13 || c == 14 || c == 15) {
			PORTB |= (1 << LED4_B);
		} else {
			PORTB &= ~(1 << LED4_B);
		}

		if( c == 4 || c == 5 || c == 6|| c == 7 || c == 12 || c == 13 || c == 14 || c == 15) {
			PORTB |= (1 << LED3_B);
		} else {
			PORTB &= ~(1 << LED3_B);
		}

		if(c == 2 || c == 3 || c == 6 || c == 7 || c == 10 || c == 11 || c == 14 || c == 15) {
			PORTB |= (1 << LED2_B);
		} else {
			PORTB &= ~(1 << LED2_B);
		}

		if(c == 1 || c == 3 || c == 5 || c == 7 || c == 9 || c == 11 || c == 13 || c == 15) {
			PORTB |= (1 << LED1_B);
		} else {
			PORTB &= ~(1 << LED1_B);
		}

		// // Affichage sur les LEDs - [ Plus Simplement ]
        // PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB4));
        // PORTB |= (c & 0b00000111) | ((c & 0b00001000) << 1);

	}

	return (0);
}
