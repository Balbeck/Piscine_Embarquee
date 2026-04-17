#include <avr/io.h>
#include <util/delay.h>


#define RED_D PD5 //-> devrait etre 3
#define GREEN_D PD6 //-> devrait etre 5 
#define BLUE_D PD3 //-> devrait etre 6


// LED RGB D5
// Theoriquement Good ! 
// #define RED_D PD3
// #define GREEN_D PD5
// #define BLUE_D PD6

int main() {

	DDRD |= (1 << RED_D);
	DDRD |= (1 << GREEN_D);
	DDRD |= (1 << BLUE_D);

	while(1){
		PORTD |= (1 << RED_D);
		_delay_ms(1000);
		PORTD &= ~(1 << RED_D);
		PORTD |= (1 << GREEN_D);
		_delay_ms(1000);
		PORTD &= ~(1 << GREEN_D);
		PORTD |= (1 << BLUE_D);
		_delay_ms(1000);
		PORTD &= ~(1 << BLUE_D);


		// _delay_ms(500);
		// // (Les 3 couleurs)BLANC 😁
		// PORTD |= (1 << RED_D);	
		// PORTD |= (1 << GREEN_D);
		// PORTD |= (1 << BLUE_D);
		// _delay_ms(500);
		// PORTD ^= (1 << RED_D);
		// PORTD ^= (1 << GREEN_D);
		// PORTD ^= (1 << BLUE_D);

		// 		// (Green + Red) 😁 JAUNE
		// PORTD ^= (1 << GREEN_D);
		// PORTD ^= (1 << RED_D);
		// _delay_ms(500);
		// PORTD ^= (1 << GREEN_D);
		// PORTD ^= (1 << RED_D);

		// 		// (Red + Blue) 😁 VIOLET
		// PORTD ^= (1 << RED_D);
		// PORTD ^= (1 << BLUE_D);
		// _delay_ms(500);
		// PORTD ^= (1 << RED_D);
		// PORTD ^= (1 << BLUE_D);

		// 		// (Green + Blue) 😁 BLEU CIEL
		// PORTD ^= (1 << GREEN_D);
		// PORTD ^= (1 << BLUE_D);
		// _delay_ms(500);
		// PORTD ^= (1 << GREEN_D);
		// PORTD ^= (1 << BLUE_D);


	}

	return(0);

}
