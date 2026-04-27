#include <avr/io.h>
#define UBRR_VALUE 8


char uart_rx(void){
    /* (p.189) Wait for data to be received */
    while ( !(UCSR0A & (1<<RXC0)) )
    ;
    /* Get and return received data from buffer */
    return UDR0;
}


void uart_tx(char c){
    // Attend que le registre UDR0 soit vide (poll via UDRE0 = 1)
	// Sinon Risque ecraser un char en cours d'envoie
	while (!(UCSR0A & (1 << UDRE0)))
    ;
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


int main(void){
    char c;

    uart_init();
    while(1){
        c = uart_rx();
        if (c == '\r') { // Enter
            uart_tx('\n');
            uart_tx('\r');
        }
        else if (c == 127) { // DEL (Backspace)
            uart_tx('\b');
            uart_tx(' ');
            uart_tx('\b');
        }
        else
            uart_tx(c);
    }
    
    return(0);
}
