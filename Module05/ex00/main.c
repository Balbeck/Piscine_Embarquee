#include <avr/io.h>
#include <util/delay.h>

// Initialisation UART (115200 baud, 16MHz)
void uart_init(void) {
    // UBRR = 16000000/(16*115200) - 1 = 8
    // Voir datasheet Table 19-1, page ~163
    UBRR0H = 0;
    UBRR0L = 8;
    UCSR0B = (1 << TXEN0);           // Active l'émission
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 bits, pas de parité, 1 stop
}

void uart_tx(char c) {
    while (!(UCSR0A & (1 << UDRE0))); // Attend que le buffer soit vide
    UDR0 = c;
}

void uart_printstr(const char *s) {
    while (*s)
        uart_tx(*s++);
}

// Affiche un octet en hexadécimal (2 caractères)
void uart_print_hex(uint8_t val) {
    const char hex[] = "0123456789abcdef";
    uart_tx(hex[val >> 4]);    // nibble haut
    uart_tx(hex[val & 0x0F]); // nibble bas
}

// Initialisation ADC
// Datasheet section 23.9 (ADMUX p.217, ADCSRA p.218)
void adc_init(void) {
    // REFS0=1 : AVCC comme référence
    // ADLAR=1 : alignement gauche (résultat 8 bits dans ADCH)
    // MUX=0000 : canal ADC0 (potentiomètre RV1 sur PC0)
    ADMUX = (1 << REFS0) | (1 << ADLAR);

    // ADEN=1 : active l'ADC
    // ADPS2:0=111 : prescaler 128 → 16MHz/128 = 125kHz
    // (doit être entre 50-200kHz, voir p.208)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// Lecture ADC 8 bits
uint8_t adc_read(void) {
    ADCSRA |= (1 << ADSC);           // Lance la conversion
    while (ADCSRA & (1 << ADSC));    // Attend la fin (ADSC repasse à 0)
    return ADCH;                      // Résultat 8 bits (ADLAR=1)
}

int main(void) {
    uart_init();
    adc_init();

    while (1) {
        uint8_t val = adc_read();
        uart_print_hex(val);
        uart_tx('\r');
        uart_tx('\n');
        _delay_ms(20);
    }
}

