/* [ Screen sur le terminal ]
trouver le port : 'ls/dev/tty.*'
		--> /dev/tty.usbserial-110
Ouvrir le port serie avec 'screen' avec un baudrate de 115200
	 - 'screen /dev/tty.usbserial-110 115200'
	 ou 'screen /dev/cu.usbserial-110 115200'
Exit screen:
	 - Ctrl+A and K
*/



#define UART_BAUDRATE 115200
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

#define UBBR_VALUE ( ( F_CPU / (16UL * UART_BAUDRATE) ) -1 )
// #define UBBR_VALUE ( ( F_CPU / (16 * UART_BAUDRATE) ) -1 )
/*
	[ BAUDRATE Formula ]
	(p.182 Table 20-1. Equations for Calculating Baud Rate Register Setting)
Asynchronous Normal Mode (U2Xn = 0):
	fosc: System clock frequency -> F_CPU (16MHz)
	Equation for Calculating UBRRn VALUE:
			UBRRn = (fosc / 16BAUD) - 1
	Equation for Calculating Baud Rate:
			BAUD = fosc / 16(UBRRn + 1)

*/

void	uart_tx(char c) {
	// Attend que le registre UDR0 soit vide (poll via UDRE0 = 1)
	// Sinon Risque ecraser un char en cours d'envoie
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c; // Ecrit le char sur le registre pour que UART le transmette
}

void	uart_init(void) {
	/* [ UART Configuration ] (Datasheet - p.179 section: 20. USART0)
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
	UBRR0H = (unsigned char)(UBBR_VALUE>>8);
	UBRR0L = (unsigned char)UBBR_VALUE;
	//Enable receiver and transmitter */
	UCSR0B = (1 << RXEN0)|(1 << TXEN0);
	// Set frame format: 8 bits, 1 stop bit
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

}

int	main(void){

	char c = 'Z';
	DDRB |= (1 << PB1);

	uart_init();
	while(1){
		uart_tx(c);
		PORTB ^= (1 << PB1);
		_delay_ms(1000); //1 sec = Frequence 1Hz 😁
	}

	return(0);

}
