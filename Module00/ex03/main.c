#include <avr/io.h>
#include <util/delay.h>

#define LED_B PB0
#define BUTTON_D PD2

// int main(void){
// 
// 	DDRB |= (1 << LED_B); // -> OUT (devient une sortie)
// 	// PORTB &= ~(1 << LED_B); // -> IN (2nd bit a 0)
// 	DDRD &= ~(1 << BUTTON_D); // -> IN (2nd bit a 0)
// 	PORTD |= (1 << BUTTON_D); // -> Pull up (2nd bit a 1)
// 	while(1){
// 		if (PIND & (1 << BUTTON_D)){ // Relache
// 			// PORTB ^= (1 << LED_B);
// 			PORTB &= ~(1 << LED_B); // -> Met a 0 - Eteind
// 			// _delay_ms(20);
// 		}
// 		else {
// 			// _delay_ms(20);
// 			PORTB ^= (1 << LED_B);
// 			// PORTB |= (1 << LED_B); // -> Met a 1 - Allume
// 		}
// 	}
// 	return (0);
// }


// [ OU ]
int main(){
	DDRB |= (1 << LED_B);
	PORTB &= ~(1 << LED_B);
	DDRD &= ~(1 << BUTTON_D);
	PORTD |= (1 << BUTTON_D);
	while(1){
		if ( !(PIND & (1 << BUTTON_D)) ){
			_delay_ms(20); //rebond au Press
			PORTB ^= (1 << LED_B);
			while( !(PIND & (1 << BUTTON_D)) );
			_delay_ms(20); //rebond relache du button
		}
	}
	return(0);
}
