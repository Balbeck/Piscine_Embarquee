#include <avr/io.h>

#define LED2_B PB1


int main() {

	// volatile int i = 0;
	DDRB |= (1 << LED2_B); // Met la led en Sortie ! 

	// DDRB |= (1 << PB4);
	while (1) {

		// [ PINx Write ! ]
		// Le Registre PINx Permet en Lecture de lire l'etat actuel des broches (pins) mais il permet aussi
		// d'ecrire 1 afin de toggle.
		// Ecrire 1 dans PINB permet d'inverser le bit correspondant dans PORTB!
		// Ca permet le toggle en 1 seule Operation ! Au lieu de 3: Lire-Modifier-Ecrire
		// Lire PORTB, Le modifier, Reecrire PORTB
		PINB |= (1 << LED2_B);
		
		// [ Fake Delay ]
		// Le But est de creer un delai de 1/2 sec (500ms) a 16MHz
		// L'idee est d'executer par le CPU Sufisament de cycles pour atteindre environ 500ms.
		// A 16MHz (16Millions Hertz) qui est la Frequence ATmega328p, 1 cycle CPU = 62.5ns
		// Frequence(f) = 1 / Periode(T)
		// Periode(T) = 1 / Frequence(f)
		// Notre Periode est de 1 seconde ! 
		// Et on veut durant cette periode la LED [ 50% ON - 50% OFF ].
		// Donc 1 / 16 000 000 = 0.000 000 062 5 secondes
		// -> 62.5 nanosecondes
		// Toutes les 62.5 ns le microprocesseur "Tick"
		// Ainsi pour 500ms (500 000 000ns):
		// 500000000/62.5 = 8 000 000 de cycles!
		
		// Le Nombre de Hertz c'est le nombre de cycles complets par seconde ! 1Hz = 1 cycle par seconde !
		// 1 Cycle complet = ON 500ms puis OFF 500ms.
		// un délai logiciel (sans timer hardware) n'est pas précis à la milliseconde.
		// Le nombre de cycles CPU par itération de boucle dépend des instructions générées par le compilateur, du niveau d'optimisation, etc. 


		// ‼️ Sur ATmega ! 
		// char           →   8 bits  →  -128 à +127
		// unsigned char  →   8 bits  →  0 à 255
		// int            →  16 bits  →  -32 768 à +32 767
		// unsigned int   →  16 bits  →  0 à 65 535
		// long           →  32 bits  →  -2 147 483 648 à +2 147 483 647
		// unsigned long  →  32 bits  →  0 à 4 294 967 295
		// OU
		// /* Entiers non signés */
		// uint8_t   →  8 bits  →  0 à 255
		// uint16_t  →  16 bits →  0 à 65 535
		// uint32_t  →  32 bits →  0 à 4 294 967 295
		// uint64_t  →  64 bits →  0 à 18 446 744 073 709 551 615

		// /* Entiers signés */
		// int8_t    →  8 bits  →  -128 à +127
		// int16_t   →  16 bits →  -32 768 à +32 767
		// int32_t   →  32 bits →  -2 147 483 648 à +2 147 483 647
		// int64_t   →  64 bits →  très grand...


		uint32_t i = 0;
		uint32_t a = 200000;

		while(i < a){
			// if (i == 100000) {
			// 	PORTB |= (1 << PB4);
			// }
			i++;
		} 

		// // [ Or ] -> Mais moins d'instructions/ cycle d'execution Donc Plus rapide!
		// uint32_t a = 300000;
		// while(a--);

	}

	return (0);
}

