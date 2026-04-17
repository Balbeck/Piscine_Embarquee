#define UART_BAUDRATE 115200
// #define UART_BAUDRATE 9600
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define UBBR_VALUE ( ( F_CPU / (16UL * UART_BAUDRATE) ) -1 )
#define RED_D PD5 //-> devrait etre 3
#define GREEN_D PD6 //-> devrait etre 5 
#define BLUE_D PD3 //-> devrait etre 6

volatile char buffer[8];
volatile uint8_t index = 0;
volatile uint8_t ready = 0;


ISR(USART_RX_vect)
{
    char c = UDR0;  // lire l'octet reçu
    if(c == '#'){
		index = 0;
	}
	if(c == '\n'){
		ready = 1;
	}
	if(index < 8){
		buffer[index] = c;
		index++;
	}
}

void	set_rgb(uint8_t r, uint8_t g, uint8_t b){
	OCR0B = r;
	OCR0A = g;
	OCR2B = b;
}

uint8_t	ft_char_to_uint8_t(char c){
	uint8_t nbr = 0;
	if(c >= '0' && c <= '9'){
		nbr = c - '0'; //48
	}
	if(c >= 'a' && c <= 'f'){
		nbr = c - 'a' + 10;
	}
	if(c >= 'A' && c <= 'F'){
		nbr = c - 'A' + 10;
	}
	return nbr;
}

/*		[ Natation Hexadecimale #RRGGBB ]
	Base 16 allant de 00 a FF (0 a 255)
Deci a Hexa:
	 - On divise nbr par 16, on garde partie entiere et le reste (%) pour la seconde partie
	 - Remplace les valeurs 10 a 15 par lettres 'A' a 'F'
Hexa a Deci:
	 - Converti chaque caractere en decimal:
	 	Le premier est Multiplie par 16, le second x1
	 - On additionne le Tout
	 - ex: C8 ->  (C==)12 x 16 + 8 = 192 + 8 = 200
*/
void	ft_from_buffer_to_rgb(){
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	// r = buffer[1] X 16 + buffer[2]
	// g = buffer[3] X 16 + buffer[4]
	// b = buffer[5] X 16 + buffer[6]
	r = 16 * ft_char_to_uint8_t(buffer[1]) + ft_char_to_uint8_t(buffer[2]);
	g = 16 * ft_char_to_uint8_t(buffer[3]) + ft_char_to_uint8_t(buffer[4]);
	b = 16 * ft_char_to_uint8_t(buffer[5]) + ft_char_to_uint8_t(buffer[6]);
	set_rgb(r, g, b);
}


/*		[ UART - Init ]
UBRR0H / UBRR0L  -> baud rate
UCSR0B           -> activer RX, TX, interruption
UCSR0C           -> format 8N1
*/
void	uart_init(void){
	// Set baud rate  (p.185)
	UBRR0H = (unsigned char)(UBBR_VALUE>>8);
	UBRR0L = (unsigned char)UBBR_VALUE;
	//Enable receiver, transmitter, Interupt */
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	// Set frame format: 8 bits, 1 stop bit
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void	init_rgb(void){
	DDRD |= (1 << RED_D) | (1 << GREEN_D) | (1 << BLUE_D); 

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

void test_fill_buffer(char* str){
	buffer[1] = str[0];
	buffer[2] = str[1];
	buffer[3] = str[2];
	buffer[4] = str[3];
	buffer[5] = str[4];
	buffer[6] = str[5];
}

int	main(void){

	init_rgb();
	uart_init();
	sei();
	while(1){

		///////////////// [ TESTS ] //////////////////
		// buffer[1]='F';
		// buffer[2]='F';
		// buffer[3]='0';
		// buffer[4]='0';
		// buffer[5]='0';
		// buffer[6]='0';
		// ft_from_buffer_to_rgb();
		// _delay_ms(2000);

		// // buffer ="#00FF00\n"; //Vert
		// buffer[1]='0';
		// buffer[2]='0';
		// buffer[3]='F';
		// buffer[4]='F';
		// buffer[5]='0';
		// buffer[6]='0';
		// ft_from_buffer_to_rgb();
		// _delay_ms(2000);

		// // buffer ="#0000FF\n"; //Bleu
		// buffer[1]='0';
		// buffer[2]='0';
		// buffer[3]='0';
		// buffer[4]='0';
		// buffer[5]='F';
		// buffer[6]='F';
		// ft_from_buffer_to_rgb();
		// _delay_ms(2000);


		// #FF00FF		# Rose fuchsia
		test_fill_buffer("FF00FF ");
		ft_from_buffer_to_rgb();
		_delay_ms(2000);

		// // #CCFF00  # Vert citron
		test_fill_buffer("CCFF00");
		ft_from_buffer_to_rgb();
		_delay_ms(2000);

		// #1A5C1A		# Vert bouteille
		test_fill_buffer("1A5C1A");
		ft_from_buffer_to_rgb();
		_delay_ms(2000);

		// #072007		# Vert bouteille très foncé
		test_fill_buffer("072007");
		ft_from_buffer_to_rgb();
		_delay_ms(2000);

		// #00008B		# Bleu foncé
		test_fill_buffer("00008B");
		ft_from_buffer_to_rgb();
		_delay_ms(2000);

		////////////////////////////////////////////////////////////

		if(ready == 1){
			// [ On Reccoit des char de 'volatile char buffer[8]' -> On veut 3 uint8_t! ]
			if(buffer[0] == '#'){
				ft_from_buffer_to_rgb();
			}
			ready =0;
		}
	}

	return (0);
}
