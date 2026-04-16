#include <avr/io.h>
#include <util/delay.h>

#define LED_B PB0
#define BUTTON_D PD2

// Allumer la LED lorsque Appuie sur BUTTON ! 
int main(void){

	DDRB |= (1 << LED_B);

	DDRD &= ~(1 << BUTTON_D);   // PD2 IN bit a 0!
	PORTD |= (1 << BUTTON_D);   // active resistance pull-up interne
	
	while(1){
		if (!(PIND & (1 << BUTTON_D))) { // Broche a LOW
			PORTB |= (1 << LED_B);
		} 
		else { // Broche a HIGH (Button relache Si pull up Active!)
			PORTB &= ~(1 << LED_B);
		}
	}
	
	return (0);
}
