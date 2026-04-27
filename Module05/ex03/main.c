#include <avr/io.h>
#include <util/delay.h>

// === UART ===

void uart_init(void) {
    // UBRR = 8 pour 115200 baud à 16MHz
    // Datasheet Table 19-1, p.163
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

// Affiche un int16_t en décimal (peut être négatif)
void uart_print_dec(int16_t val) {
    char buf[6];  // max "-1023\0"
    int8_t i = 0;

    // Gestion des valeurs négatives
    if (val < 0) {
        uart_tx('-');
        val = -val;
    }

    if (val == 0) {
        uart_tx('0');
        return;
    }

    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0)
        uart_tx(buf[--i]);
}

// === ADC ===

void adc_init_temp(void) {
    // Datasheet p.213 + p.217, registre ADMUX :
    //   REFS1:0 = 11 → référence interne 1.1V (OBLIGATOIRE pour le capteur temp)
    //   ADLAR   = 0  → alignement droite, résultat 10 bits
    //   MUX3:0  = 1000 (valeur 8) → canal capteur température interne
    //
    // ATTENTION : REFS1=1 ET REFS0=1, différent des ex précédents !
    // (avant on avait REFS1=0, REFS0=1 pour AVCC)
    ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << MUX3);
    //                                      ^^^^^^^
    //                              MUX3=1, MUX2:0=000 → canal 8

    // Datasheet p.218, registre ADCSRA :
    //   ADEN = 1 → active l'ADC
    //   ADPS2:0 = 111 → prescaler 128 → 125kHz
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // Datasheet p.213 :
    // "The first ADC conversion result after switching to the temperature
    //  sensor channel may be inaccurate, discard this reading."
    // On lance une conversion factice et on jette le résultat
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    (void)ADC;  // lecture pour vider, résultat ignoré
}

uint16_t adc_read_raw(void) {
    // Lance une conversion
    ADCSRA |= (1 << ADSC);
    // Attend la fin
    while (ADCSRA & (1 << ADSC));
    // Retourne la valeur 10 bits (ADCL lu en premier automatiquement)
    // Datasheet p.219
    return ADC;
}

// === MAIN ===

int main(void) {
    uart_init();
    adc_init_temp();

    while (1) {
        uint16_t raw = adc_read_raw();

        // Conversion en Celsius
        // Datasheet p.213, Table 23-2 :
        // Valeur typique à 25°C ≈ 314 LSB, coefficient ≈ 1 LSB/°C
        // Donc : T(°C) = raw - 314 + 25 = raw - 289
        // En pratique on utilise ~273 comme offset de départ et on calibre
        // La valeur exacte dépend de chaque puce (±10°C sans calibration)
        int16_t celsius = (int16_t)raw - 273;

        uart_print_dec(celsius);
        uart_tx('\r');
        uart_tx('\n');

        _delay_ms(20);
    }
}
