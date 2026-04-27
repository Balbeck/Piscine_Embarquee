#include <avr/io.h>
// #include <avr/interrupt.h>

#define UBRR_VALUE 8


void uart_tx(char c){
    // Attend que le registre UDR0 soit vide (poll via UDRE0 = 1)
	// Sinon Risque ecraser un char en cours d'envoie
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c; // Ecrit le char sur le registre pour que UART le transmette
}

void	uart_init(void) {
	// Set baud rate  (p.185)
	UBRR0H = (unsigned char)(UBRR_VALUE>>8);
	UBRR0L = (unsigned char)UBRR_VALUE;
	//Enable receiver and transmitter
	UCSR0B = (1 << RXEN0)|(1 << TXEN0);
	// Set frame format: 8 bits, 1 stop bit
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// ISR(USART_RX_vect) { ... } // Equivalent !
void __attribute__((signal, used)) USART_RX_vect ( void ) {
/* [ Interrupt Vectors in ATmega328 and ATmega328P ] (p.74)
Prototypage de la Fct USART_RX_vect --> __attribute__((signal, used)): extension GCC (pas du C standard) qui attache des attributs spéciaux à la fonction.
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

    char c;
    char reverse;

    // c = uart_rx();
    c = UDR0;
    if (c == '\r') { // Enter
        uart_tx('\r');
        uart_tx('\n');
    }
    else if (c == 127) { // DEL (Backspace)
        uart_tx('\b');
        uart_tx(' ');
        uart_tx('\b');
    }
    else if (('a' <= c) && (c <= 'z')) {
        reverse = c - 'a' + 'A';
        uart_tx(reverse);
    }
    else if (('A' <= c) && (c <= 'Z')) {
        reverse = c - 'A' + 'a';
        uart_tx(reverse);
    }
    else
        uart_tx(c);
}


int main(void){

    uart_init();
    SREG |= (1 << SREG_I); //Global Interrupt Enable p.20
    UCSR0B |= (1 << RXCIE0); //RX Complete Interrupt Enable p.201
    
    while(1){

    }
    
    return(0);
}
