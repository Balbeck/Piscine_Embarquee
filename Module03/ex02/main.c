#include <avr/io.h>
#include <util/delay.h>

#define RED_D PD5 //-> devrait etre 3
#define GREEN_D PD6 //-> devrait etre 5 
#define BLUE_D PD3 //-> devrait etre 6

/*
	1.	[ PWM - Pulse Width Modulation ] - Modulation de Largeur d'Impulsion
	Ce qui permet de gerer les Rapport Cycliques - Duty Cycle! 
	 - OCR = 0   -> pin LOW  -> 0% LED
	 - OCR = 127 -> pin LOW 50% Time -> 50% LED  
	 - OCR = 255 -> pin HIGH -> 100% LED
	‼️️ Si Anode(Entree) Commune ou Catode(Sortie) Commune la logique est inversee:
		 - OCR = 0 -> 100% LED
		 - OCR = 255 -> 0% LED
		 Donc (255 - value) ou (value simplement) -> gere ca dans le set_rgb()
	
	2.	Registres a Config pour Activer PWM:
	 - Timer0 (p.115):
	 TCCR0A – Timer/Counter Control Register A
	 	 - Mode:3, WGM00 = 0, WGM01 = 1 et WGM00 = 1
		Configurer les types values d'arret: Les COM (Compare Output Mode)
		(p.113 Table 15-3. Compare Output Mode, Fast PWM Mode)
		 - COM0B1 = 1 et COM0B0 = 0
	
	3.	Raison Risque D'Overflow Configurer Baud Rate Sachant que Timer0,
	son compteur: TCNT0 – Timer/Counter Register, est sur 8 bit. (255 Max)
	On Configure donc un prescaler avec le Registre:
		TCCR0B – Timer/Counter Control Register B (p.116)
		 - (/256)	:	 CS02 = 1, CS01 = 0 et CS00 = 0 
		 - (/1024)	:	 CS02 = 1, CS01 = 0 et CS00 = 1 

	Dapres le shema + Datasheet (p.97 Table 14.3.3 Alternate Functions of Port D)
		 - PD3 (OC2B / INT1) -> Timer3 Canal B
		 - PD5 (OC0B / T1) -> Timer0 Canal B
		 - PD6 (OC0A / AIN0) -> Timer0 Canal A
*/


void init_rgb(void){

	// Met les LED en Sortie:
	DDRD |= (1 << RED_D);
	DDRD |= (1 << GREEN_D);
	DDRD |= (1 << BLUE_D);

	// [ Timer0 - Init ] 2 colors:
	TCCR0A |= (1 << WGM01); //Fast-PWM
	TCCR0A |= (1 << WGM00); //Fast-PWM
	TCCR0A |= (1 << COM0A1); // Compare Output Non-Inverse Canal A -> [ *GREEN* ]
	TCCR0A |= (1 << COM0B1); // Compare Output Non-inverse Canal B -> [ *RED* ]
	TCCR0B |= (1 << CS02); // Prescaler 1024
	TCCR0B |= (1 << CS00); // Prescaler 1024

	// [ Timer2 - Init ] 1 color:
	TCCR2A |= (1 << WGM21); //Fast-PWM
	TCCR2A |= (1 << WGM20); //Fast-PWM
	TCCR2A |= (1 << COM2B1); // Compare Output Non-Inverse Canal B -> [ *BLUE* ]
	TCCR2B |= (1 << CS22); // Prescaler 1024
	TCCR2B |= (1 << CS20); // Prescaler 1024
}


/*
On sait que la valeur d'OCR definit le DutyCycle donc:
	 - Rouge -> OCR0B
	 - Vert  -> OCR0A
	 - Bleu  -> OCR2B
Le But est d'ecrir dans le bon registre OCR les valeurs de chaque couleur correspondante. 

*/

void set_rgb(uint8_t r, uint8_t g, uint8_t b){
	OCR0B = r;
	OCR0A = g;
	OCR2B = b;
}


void wheel(uint8_t pos) {
	pos = 255 - pos;
	
	if (pos < 85) {
		set_rgb(255 - pos * 3, 0, pos * 3);
	} else if (pos < 170) {
		pos = pos - 85;
		set_rgb(0, pos * 3, 255 - pos * 3);
	} else {
		pos = pos - 170;
		set_rgb(pos * 3, 255 - pos * 3, 0);
	}
}

int main(void){

	init_rgb();

	while(1){
		uint8_t i = 0;
		while(i < 255){
			wheel(i);
			i += 1;
			_delay_ms(20);
		}
	}

	return(0);
}
