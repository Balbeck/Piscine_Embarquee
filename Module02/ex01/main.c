/*
\r  ->  Déplacer le curseur à la colonne zéro.

Contraintes: 
    - uart_printstr() doit etre appele toutes 2s pour afficher 'Hello World!\n'
    - L'UART du MCU doit etre configure en 115200 8N1
    - while(1){} doit rester vide

Etapes: 
    1) Init UART Emeteur \ Recepteur
    2) Configurer en 115200 8N1
    3) Utiliser les Timers pour call uart_printstr() toutes 2 sec 
*/

// #include <stdio.h>


#define UART_BAUDRATE 115200
#define F_CPU 16000000UL
#define UBRR_VALUE (F_CPU / (16UL * UART_BAUDRATE)) // -> [ 8 ]

#include <avr/io.h>
#include <avr/interrupt.h>

int ft_strlen(const char* str){
    int i = 0;
    while(str[i] != '\0'){
        i++;
    }
    return (i);
}

void uart_printstr(const char* str){
    // printf("UBRR [%ld]", UBRR_VALUE);
    int len;
    len = ft_strlen(str);
    // printf("%d\n", len);
    for(int i=0; i < len; i++){
        // Attend que le registre UDR0 soit vide (poll via UDRE0 = 1)
        // Sinon Risque ecraser un char en cours d'envoie
        while (!(UCSR0A & (1 << UDRE0)));
        // Ecrit chaque char sur le registre Emiter UDR0
        UDR0 = str[i];
    }
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = '\r';
}


void uart_init(void) {
    /* 
    [ UART Configuration ] (Datasheet - p.179 section: 20. USART0)
		1:	Configurer le BAUDRATE via le Registre UBBR (shema20.1 p.80)
			-> Registre: UBBR USART Baud Rate Register
				 - UBRR and the down-counter connected to it function 
				 - as a programmable prescaler or baud rate generator
				- sur 16 bit: Donc sur 2 registres 8 bits !
			 - UBRR0H (USART Baud Rate Register HIGH)
			 - UBRR0L (USART Baud Rate Register LOW)
		2: Activer l'emetteur TXEN0 
			 - sur Registre UCSR0B
		3: Activer le recepteur RXEN0
			 - sur Registre UCR0B
		4: Definir le format 8N1
			- Registre UCSR0C:
				UCSZ01 + UCSZ00 = 1 1	-> Bit
				UPM01  + UPM00  = 0 0	-> Pas Parite
				USBS0           = 0		-> Nbr 'stop bit': 1
	*/
	// Set baud rate  (p.185)
	UBRR0H = (unsigned char)(UBRR_VALUE>>8);
	UBRR0L = (unsigned char)UBRR_VALUE;
	//Enable receiver and transmitter */
	UCSR0B = (1 << RXEN0)|(1 << TXEN0);
	// Set frame format: 8 bits, 1 stop bit
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}


void timer_init(void){
    /*
    [ Timer1 Config ]
     - Timer1 sur 16bit -> Value Max [ 65 535 ] ! 
     - Frequence 1Hz = 1sec -->  2sec = Frequence 0.5Hz
     - ATmega328p Tourne a frequence 16 000 000 Hz -> on veut un delay equivalent 32 000 00 Hz
        -> 32 000 000 / 65535 = 488 
        -> 256 est donc un prescaler trop petit On a besoin de [ 1024 ] !
    
    */

    // [ Prescaler 1024 Timer1 p.143 Registre TCCR1B ]
    TCCR1B |= (1 << CS12) | (1 << CS10);

    // [ Compare Output Mode, non-PWM P.140 ]
    TCCR1A |= (1 << COM1A0);

    // [ Mode 4 - CTC (Clear Timer on Compare Match) table 15.1 p.140 Registre TCCR1A -> TOP = OCR1A (p.132)]
    TCCR1B |= (1 << WGM12);
    
    /* [ Calcul OCR1A ]
    32 000 000 /1024 = 31 250 (Value atteinte au bout de 2 sec) 
    (le -1 est pour le fait que le copmpteur start a 0 !)
    */
    OCR1A = 31249;
}

void interupt_init(void){
    /*
    interup sur timer1 p.137:
    The Timer/Counter Overflow Flag (TOV1) is set at the same timer clock cycle as the OCR1x Registers are
updated with the double buffer value (at BOTTOM). When either OCR1A or ICR1 is used for defining the TOP
value, the OC1A or ICF1 Flag set when TCNT1 has reached TOP. The Interrupt Flags can then be used to
generate an interrupt each time the counter reaches the TOP or BOTTOM value.
    */

    /*[ 7.3.1 SREG – AVR Status Register p.20]
    Bit 7 – I: Global Interrupt Enable
The Global Interrupt Enable bit must be set for the interrupts to be enabled. The individual interrupt enable
control is then performed in separate control registers. If the Global Interrupt Enable Register is cleared, none of
the interrupts are enabled independent of the individual interrupt enable settings.
    */
    SREG |= (1 << SREG_I);

    /* [ Timer1 Interupt p.144 - Interupt triggered Par Timer1 ]
    [16.11.8 TIMSK1 – Timer/Counter1 Interrupt Mask Register]
    • Bit 1 – OCIE1A: Timer/Counter1, Output Compare A Match Interrupt Enable
When this bit is written to one, and the I-flag in the Status Register is set (interrupts globally enabled), the
Timer/Counter1 Output Compare A Match interrupt is enabled. The corresponding Interrupt Vector (see
“Interrupts” on page 66) is executed when the OCF1A Flag, located in TIFR1, is set.
    */
    TIMSK1 |= (1 << OCIE1A);

}

// [ Interrupt Vectors in ATmega328 and ATmega328P ] p.74
// Table 12-6. Reset and Interrupt Vectors in ATmega328 and ATmega328P

/*macro ISR() est fourni par avr/interrupt.h qui genere exactement les attributs __attribute__((signal, used))
Ce que fait __attribute__((signal, used))
signal dit au compilateur GCC (avr-gcc) que cette fonction est un gestionnaire d'interruption. Concrètement, ça change deux choses par rapport à une fonction normale :

Le compilateur génère automatiquement du code pour sauvegarder et restaurer tous les registres (SREG, r0-r31) utilisés 
dans la fonction — c'est le "prologue" et "épilogue" de l'ISR.
Une fonction normale ne sauvegarde que les registres "callee-saved", 
mais une interruption peut arriver n'importe quand, donc il faut tout protéger.
La fonction se termine par l'instruction assembleur RETI (Return from Interrupt) au lieu de RET. 
RETI réactive automatiquement les interruptions globales (remet le bit I du SREG à 1), ce qui est crucial car le hardware désactive les interruptions quand il entre dans l'ISR.

used empêche le compilateur de supprimer la fonction lors de l'optimisation. 
Comme personne n'appelle directement une ISR dans le code (c'est le hardware qui y saute via la table des vecteurs), 
le compilateur pourrait croire qu'elle est inutile et l'éliminer.
*/

// ISR(TIMER1_COMPA_vect) {
//     uart_printstr("Hello World!\n");
// }

void TIMER1_COMPA_vect(void) __attribute__((signal, used));
/* [ Interrupt Vectors in ATmega328 and ATmega328P ] (p.74)
void TIMER1_COMPA_vect(void): Prototypage de la FCT TIMER1_COMPA_vect, par convention vecteur d'interruption "Timer 1 Compare Match A"
__attribute__((signal, used)): extension GCC (pas du C standard) qui attache des attributs spéciaux à la fonction.
 - signal: Attribut clé pour les interruptions AVR.
    Dit au compilo que la fct est un gestionnaire d'interruption (ISR).
    Concrètement, compilo genere un prologue et un epilogue spéciaux : sauvegarder tous les registres utilisés en début de fonction, les restaurer à la fin, et terminer par l'instruction assembleur RETI (return from interrupt).
    Il n'y a pas de désactivation globale des interruptions à l'entrée (contrairement à l'attribut interrupt). Cela signifie que d'autres interruptions de priorité supérieure peuvent interrompre celle-ci!
 - used: Attribut de securite qui empeche compilo de supprimer la fct lors de l'optimisation a la compilation.
    Normalement, si compilo voit aucun appel explicite à une fct, il peut décider la supprimer car il la considere comme "code mort".
    Un gestionnaire d'interruption n'est jamais appele directement dans le code : c'est le matériel qui saute à son adresse via la table des vecteurs. used force le compilateur à conserver la fonction dans le binaire final, même sans appel visible.
En Gros ce prototypage permet l'invocation automatique quand Timer1 atteint OCR1A, avec les garanties que les registres seront sauvegardés/restaurés correctement et que le code ne sera pas éliminé à la compilation.
*/


void TIMER1_COMPA_vect(void){
	uart_printstr("Hello World!\n");
}


int main(void){

    // char* str = "Hello World!\n";

    uart_init();
    timer_init();
    interupt_init();
    // uart_printstr(str);
    while(1){
        
    }
    return(0);
}
