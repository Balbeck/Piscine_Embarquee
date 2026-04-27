#include <avr/io.h>
#include <util/delay.h>

// === UART ===

void uart_init(void) {
    // Baud 115200 avec quartz 16MHz
    // UBRR = 16000000 / (16 * 115200) - 1 = 8
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

// Affiche un uint16_t en décimal
// Valeur max possible : 1023 (10 bits), donc max 4 chiffres
void uart_print_dec(uint16_t val) {
    char buf[5];       // max "1023" + '\0'
    int8_t i = 0;

    // Cas spécial : valeur 0
    if (val == 0) {
        uart_tx('0');
        return;
    }

    // Extraction des chiffres (ils sortent à l'envers)
    // Ex: 128 → buf = ['8', '2', '1']
    while (val > 0) {
        buf[i++] = '0' + (val % 10);  // chiffre des unités
        val /= 10;                     // on décale
    }

    // Affichage dans le bon ordre (du dernier au premier)
    while (i > 0) {
        uart_tx(buf[--i]);
    }
}

// === ADC ===

void adc_init(void) {
    // Datasheet p.217, registre ADMUX :
    //   REFS1:0 = 01 → AVCC comme référence de tension
    //   ADLAR   = 0  → alignement à DROITE (résultat 10 bits)
    //                   Différence clé avec ex00/ex01 !
    //   MUX3:0  = 0000 → canal ADC0 par défaut
    ADMUX = (1 << REFS0);
    // Pas de (1 << ADLAR) ici ! C'est la seule différence de config.

    // Datasheet p.218, registre ADCSRA :
    //   ADEN = 1 → active l'ADC
    //   ADPS2:0 = 111 → prescaler 128
    //   16MHz / 128 = 125kHz (dans la plage 50-200kHz, p.208)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// Lecture ADC 10 bits sur un canal donné
uint16_t adc_read(uint8_t channel) {
    // Datasheet p.217, Table 23-4 :
    //   MUX3:0 = 0000 pour ADC0, 0001 pour ADC1, 0010 pour ADC2
    // On masque les 4 bits hauts pour garder REFS et ADLAR intacts
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    // Lance la conversion en mettant ADSC à 1 (p.218)
    ADCSRA |= (1 << ADSC);

    // Attend que la conversion se termine
    // ADSC repasse à 0 automatiquement quand c'est fini (p.218)
    while (ADCSRA & (1 << ADSC));

    // Datasheet p.219 :
    // Avec ADLAR=0, le résultat 10 bits est aligné à droite :
    //   ADCH contient les 2 bits de poids fort [0 0 0 0 0 0 D9 D8]
    //   ADCL contient les 8 bits de poids faible [D7 D6 D5 D4 D3 D2 D1 D0]
    // Le registre "ADC" combine les deux automatiquement (ADCL lu en premier)
    return ADC;  // uint16_t, valeur de 0 à 1023
}

// === MAIN ===

int main(void) {
    uart_init();
    adc_init();

    while (1) {
        // Lecture des 3 capteurs sur leurs canaux respectifs
        // (schéma : PC0=ADC0=RV1, PC1=ADC1=LDR, PC2=ADC2=NTC)
        uint16_t pot = adc_read(0);  // RV1 potentiomètre
        uint16_t ldr = adc_read(1);  // R14 LDR (photorésistance)
        uint16_t ntc = adc_read(2);  // R20 NTC (thermistance)

        // Affichage au format "0, 128, 1023\r\n"
        uart_print_dec(pot);
        uart_printstr(", ");
        uart_print_dec(ldr);
        uart_printstr(", ");
        uart_print_dec(ntc);
        uart_tx('\r');
        uart_tx('\n');

        _delay_ms(20);
    }
}
