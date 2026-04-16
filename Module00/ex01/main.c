#include <avr/io.h>
// #include <util/delay.h>

int main(void){
	// On config PB0 en sortie -> DDRB
	// On met PB0 a High Intensity pour allumer la LED
	// DDRB = 0b00000001;
	// PORTB = 0b00000001;

	// // [ DDR always On -> Port switching ] --> Intensity On/Off 0/1
	// int i = 0;
	// DDRB |= (1 << PB0);   // met le bit PB0 à 1 dans DDRB	
	// while(i < 10 ) {
	// 	PORTB |= (1 << PB0);  // met le bit PB0 à 1 dans PORTB
	// 	_delay_ms(500);
	// 	PORTB &= ~(1 << PB0);  // met le bit PB0 à 0 dans PORTB
	// 	_delay_ms(500);
	// 	i++;
	// }


	// // [ PORT always On -> DDR switching ] --> Intensity 0.5/1 !!!! Tjr Mini Intensitee de la LED
	// PORTB |= (1 << PB0);  // met le bit PB0 à 1 dans PORTB
	// int i = 0;
	// while(i < 10 ) {
	// 	DDRB |= (1 << PB0);   // met le bit PB0 à 1 dans DDRB
	// 	_delay_ms(500);
	// 	DDRB &= ~(1 << PB0);  // met le bit PB0 à 0 dans PORTB
	// 	_delay_ms(500);
	// 	i++;
	// }

	// // [ XOR ^ -> Inversion bit ]
	// DDRB |= (1 << PB0);
	// while(1) {
	// 	PORTB ^= (1 << PB0);
	// 	_delay_ms(500);
	// }

	DDRB |= (1 << PB0);
	PORTB |= (1 << PB0);

	return (0);
}
