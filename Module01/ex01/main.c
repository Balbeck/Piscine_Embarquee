#include <avr/io.h>

#define LED2_B PB1


int main(void)
{
    /*
	On Doit ON + OFF la LED a Frequence 1 Hertz -> ON 500ms + OFF 500ms
    On doit Utiliser Timer1 -> Voir Registres Timer1
	While(1){} Dans le prgm -> Le CPU ne gere aucune action !
	Ne DOIT PAS Utiliser PORTX ! -> DDRB: active la LED -> Timer send
	
	Il Faut donc:
	1) Set une Value representant 500ms pour le Toogle -> Registry OCR1A ou ICR1 !
	2) Definir une action quand cette value est reached -> toogle PB1
	3) Re set a 0 le compteur a 0 pour demarer un new cycle
    */

	/*
	 [ Counter TOP ] (p.121) 
	The counter reaches the TOP when it becomes equal to the highest value in the count
	sequence. The TOP value can be assigned to be one of the fixed values: 0x00FF, 0x01FF,
	or 0x03FF, or to the value stored in the OCR1A or ICR1 Register. The assignment is
	dependent of the mode of operation.
	*/

    DDRB |= (1 << LED2_B); // PB1 en Sortie ! 

	/* 
			[ Registres Timer1 a utiliser ]
	TCCR1A – Timer/Counter1 Control Register A
		Permet de definir Action quand Timer atteint Value cible. 

					Bit   7 	  6 	 5 	    4 	3 2   1 	0
						COM1A1 COM1A0 COM1B1 COM1B0 – – WGM11 WGM10 
						R/W 	R/W 	R/W   R/W   R R  R/W   R/W
					Initial Value 0 0 0 0 0 0 0 0
 
		On veut : "Toggle OC1A/OC1B on Compare Match" (Table 16.1)
		Donc bit 7 -> 0 (COM1A1)
			 bit 6 -> 1 (COM1A0)
		MAIS cela depend du Mode -> Compare Output Mode, non-PWM OR fast-PWM
		Table 16-4 Waveform Generation Mode Bit Description 
		Modes of operation supported by the Timer/Counter unit are: Normal mode (counter),
		Clear Timer on Compare match (CTC) mode,
		and three types of Pulse Width Modulation (PWM) modes

		--> Donc Mode 4 - Mode CTC (TOP = OCR1A)
		4 0 1 0 0 CTC OCR1A Immediate MAX
		12 1 1 0 0 CTC ICR1 Immediate MAX
			->> Mettre WGM12 (CTC1) a 1, WGM12 Present dans registre TCCR1B  bit 3 !
	*/
    TCCR1A |= (1 << COM1A0);
	TCCR1B |= (1 << WGM12);

    /* [ Prescaler ] - TCCR1B (Table 16-5 p.143)
	ATmega328p tourne a 16MHz, il envoit ainsi 16 millions ticks au compteur par seconde ! 
	Timer1 est de 16-bit, Il peut ainsi comparer une nbr < 65 535 ! 
	Le Prescaler permet de disiver la frequence par 8, 64, 256 ou 1024.
	16 000 000 / 256 = 62 500
	16 000 000 / 1024 = 16 625
	On choisit le Presacler de 256 ! (62500 plus facile a diviser par 2 que 16625)
	Pour cela il faut Mettre CS12 a 1 (CS12 et CS10 a 1 pour 1024)
    */
	TCCR1B |= (1 << CS12);

    /* [ OCR1A ] (p.132 -16.6) :
    OCR1A = (F_CPU / (Prescaler × F_cible)) - 1
	OCR1A = (16 000 000 / (256 × 0.5)) - 1 = 31 249
    Toggle toutes les 0.5s → 1 Hz
    */
    OCR1A = 31249;

    /*
    Boucle infinie vide.
    Le Timer gère PB1 entièrement en hardware.
    Le CPU ne fait rien.
    */
    while (1)
    {
    }

	return(0);
}
