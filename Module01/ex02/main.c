#include <avr/io.h>

#define LED2_B PB1


int main(void)
{
    DDRB |= (1 << LED2_B); // Config PB1 en Sortie

	// Configurer le Prescaler a 256 -> Mettre CS12 a 1 du Registre TCCR1B.
	TCCR1B |= (1 << CS12);

	// Pour utiliser les Rapports cycliques (Duty Cycle) on doit utiliser le Mode Fast-PWM -> 14 ou 15
	// Fast PWM permet de faire varier le temps du signal passe a HIGH.
	// Mode 14 -> (TOP = ICR1)
	// 		ICR1 ─> TOP (fréquence)
	// 		OCR1A ─>  Duty cycle - Une sortie possible
	// 		OCR1B ─> Duty cycle - Une sortie possible
	//			-> 2 canaux PWM indépendants
	// Mode 15 -> (TOP = OCR1A)
	// 		OCR1A ─> TOP (fréquence)
	// 		OCR1B ─> Duty cycle - Une Sortie possible
	// 			-> 1 seul canal PWM utilisable
	// Mode 14 : WGM13, WGM12 et WGM11 doivent etre a 1   (TCCR1A et TCCR1B) (Table 16-4.)
	TCCR1A |= (1 << WGM11);

	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << WGM13);

	// Definir Action a la Comparaison (COM1A1:0: Compare Output Mode for Channel A), 
	// Compare Output Mode, Fast PWM (Table 16-2.)
	// COM1A1	COM1A0	Comportement de la pin OC1A
	// 0		0		Pin déconnectée — le Timer tourne mais n'affecte pas la pin
	// 0		1		Toggle — la pin bascule à chaque Compare Match
	// 1		0		Non-inversé — HIGH au départ, LOW quand compteur = OCR1A
	// 1		1		Inversé — LOW au départ, HIGH quand compteur = OCR1A
	// On a donc Besoin de Mettre COM1A1 a 1 sur TCCR1A
	TCCR1A |= (1 << COM1A1);

	// On doit Definir TOP  et la Value du Duty Cycle
	// Pour TOP on utlise donc ICR1 (Mode 14). Avec Frequence de 1Hz voulu,  Calcul 16 000 000 / 256 -1 = 62499
	unsigned int top = 62499;
	ICR1 = top;

	// On definit le Raport Cyclique a 10%
	unsigned int duty = top * 0.1;
	OCR1A = duty;


    
    while (1)
    {
    }
}
