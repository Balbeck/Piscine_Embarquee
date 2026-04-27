#include <avr/io.h>
#include <util/delay.h>

void uart_init(void) {
    UBRR0H = 0;
    UBRR0L = 8;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_tx(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart_printstr(const char *s) {
    while (*s)
        uart_tx(*s++);
}

void uart_print_hex(uint8_t val) {
    const char hex[] = "0123456789abcdef";
    uart_tx(hex[val >> 4]);
    uart_tx(hex[val & 0x0F]);
}

void adc_init(void) {
    // REFS0=1 : AVCC comme référence
    // ADLAR=1 : résultat 8 bits dans ADCH
    // MUX=0000 : canal initial ADC0
    // Datasheet p.217 (ADMUX)
    ADMUX = (1 << REFS0) | (1 << ADLAR);

    // Prescaler 128 → 125kHz (p.218)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// Lecture ADC 8 bits sur un canal donné
// Datasheet p.217 Table 23-4 : MUX3:0 sélectionne le canal
uint8_t adc_read(uint8_t channel) {
    // Sélection du canal : on garde les 4 bits hauts (REFS + ADLAR)
    // et on écrit le numéro de canal dans les 4 bits bas
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    // Lance la conversion (ADSC=1, p.218)
    ADCSRA |= (1 << ADSC);

    // Attend la fin de conversion (ADSC repasse à 0)
    while (ADCSRA & (1 << ADSC));

    return ADCH; // Résultat 8 bits grâce à ADLAR=1
}

int main(void) {
    uart_init();
    adc_init();

    while (1) {
        // Canal 0 = RV1 (potentiomètre sur PC0/ADC0)
        uint8_t pot = adc_read(0);
        // Canal 1 = R14 (LDR sur PC1/ADC1)
        uint8_t ldr = adc_read(1);
        // Canal 2 = R20 (NTC sur PC2/ADC2)
        uint8_t ntc = adc_read(2);

        uart_print_hex(pot);
        uart_printstr(", ");
        uart_print_hex(ldr);
        uart_printstr(", ");
        uart_print_hex(ntc);
        uart_tx('\r');
        uart_tx('\n');

        _delay_ms(20);
    }
}
