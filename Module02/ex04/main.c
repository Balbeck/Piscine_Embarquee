#include <avr/io.h>
#include <util/delay.h>

#define UBRR_VALUE 8
#define LED1_B PB0
#define LED2_B PB1
#define LED3_B PB2
#define LED4_B PB4



// // [ Clignotement ]
void ft_gling_gling(void) {
	int i = 0;
	int ms = 42;
	while(i < 5) {
			PORTB |= (1 << LED1_B);
			PORTB &= ~(1 << LED2_B);
			PORTB &= ~(1 << LED3_B);
			PORTB &= ~(1 << LED4_B);	
			_delay_ms(ms);

			PORTB |= (1 << LED2_B);
			PORTB &= ~(1 << LED1_B);
			PORTB &= ~(1 << LED3_B);
			PORTB &= ~(1 << LED4_B);	
			_delay_ms(ms);

			PORTB |= (1 << LED3_B);
			PORTB &= ~(1 << LED2_B);
			PORTB &= ~(1 << LED1_B);
			PORTB &= ~(1 << LED4_B);	
			_delay_ms(ms);

			PORTB |= (1 << LED4_B);
			PORTB &= ~(1 << LED2_B);
			PORTB &= ~(1 << LED3_B);
			PORTB &= ~(1 << LED1_B);	
			_delay_ms(ms);
			
			i++;
	}
    PORTB &= ~(1 << LED4_B);
}

void ft_wrong_credentials(void){
    PORTB |= (1 << LED1_B);
    PORTB |= (1 << LED2_B);
    PORTB |= (1 << LED3_B);
    PORTB |= (1 << LED4_B);
    _delay_ms(1200);
    PORTB &= ~(1 << LED1_B);
    PORTB &= ~(1 << LED2_B);
    PORTB &= ~(1 << LED3_B);
    PORTB &= ~(1 << LED4_B);	
}

int ft_strlen(const char* str){
    int i = 0;
    while(str[i] != '\0'){
        i++;
    }
    return (i);
}

void uart_printstr(const char* str){
    int len;
    len = ft_strlen(str);
    for(int i=0; i < len; i++){
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = str[i];
    }
}

char uart_rx(void){
    while ( !(UCSR0A & (1<<RXC0)) )
    ;
    return UDR0;
}


void uart_tx(char c){
	while (!(UCSR0A & (1 << UDRE0)))
    ;
	UDR0 = c;
}

void	uart_init(void) {
	UBRR0H = (unsigned char)(UBRR_VALUE>>8);
	UBRR0L = (unsigned char)UBRR_VALUE;
	UCSR0B = (1 << RXEN0)|(1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

int ft_compare_str(char* str1, char* str2){
    int i = 0;
    if (ft_strlen(str1) != ft_strlen(str2))
        return (1);
    while(str1[i] != '\0') {
        if (str1[i] != str2[i])
            return (1);
        i++;
    }
    return (0);

}


int main(void){
    char c;
    char* login = "toto";
    char* pass = "pass";
    int a = 16;
    char login_user[a];
    char pass_user[a];
    int i = 0;
    int process = 0;

    DDRB |= (1 << LED1_B);
	DDRB |= (1 << LED2_B);
	DDRB |= (1 << LED3_B);
	DDRB |= (1 << LED4_B);

    uart_init();
    while(1){
        if (process == 0) {
            c = '\0';
            i = 0;
            process = 1;
            uart_printstr("Enter your login:\n\r\tusername: ");
            while(c != '\r'){
                c = uart_rx();
                if (c == 127) { // DEL (Backspace)
                    if (i > 0) {
                        i--;
                        uart_tx('\b');
                        uart_tx(' ');
                        uart_tx('\b');
                    }
                }
                else if (c != '\r') {
                    // stocker c ... dans char* login_user;
                    if (i < a - 1) {
                        login_user[i++] = c;
                        uart_tx(c);
                    }
                }
            }
            login_user[i++] = '\0';

            uart_printstr("\n\r\tpassword: ");
            c = '\0';
            i = 0;
            while(c != '\r') {
                c = uart_rx();
                if (c == 127) { // DEL (Backspace)
                    if (i > 0) {
                        i--;
                        uart_tx('\b');
                        uart_tx(' ');
                        uart_tx('\b');
                    }
                }
                else if (c != '\r') {
                    //stocker c ... dans char* pass_user
                    if (i < a - 1) {
                        pass_user[i++] = c;
                        uart_tx('*');
                    }
                }
            }
            pass_user[i++] = '\0';

            //Ajouter condition verification si login_user et pass_user Correspondent bien a login et pass
            // Si Oui -> Welcome Bro 
            // Si Non -> uart_printstr("\n\rBad combinaison username/password!\n\r"); et process = 0;
            if (ft_compare_str(login_user, login) == 0 && ft_compare_str(pass_user, pass) == 0) {
                uart_printstr("\n\rWelcome Bro!\n\rShall we play a game?\n\r");
                ft_gling_gling();
            }
            else {
                uart_printstr("\n\rBad combinaison username/password\n\r");
                process = 0;
                ft_wrong_credentials();
            }
            
            
            
        }

    }
    
    return(0);
}

