#include <avr/io.h>
#include <util/delay.h>

// === Définitions des broches ===

// LED RGB D5 (schéma section "RGB LED")
// Datasheet p.97 Table 14.3.3 - Alternate Functions of Port D :
//   PD5 = OC0B (Timer0 Canal B) → Rouge
//   PD6 = OC0A (Timer0 Canal A) → Vert
//   PD3 = OC2B (Timer2 Canal B) → Bleu
#define RED_D   PD5
#define GREEN_D PD6
#define BLUE_D  PD3

// LEDs jauge D1-D4 (schéma section "LEDs")
//   D1 → PB0 (pin 12)
//   D2 → PB1 (pin 13)
//   D3 → PB2 (pin 14)
//   D4 → PB4 (pin 16, MISO)
#define LED_D1 PB0
#define LED_D2 PB1
#define LED_D3 PB2
#define LED_D4 PB4

// === UART (pour debug si besoin) ===

void uart_init(void) {
    UBRR0H = 0;
    UBRR0L = 8;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// === ADC ===

void adc_init(void) {
    // Datasheet p.217, registre ADMUX :
    //   REFS0 = 1 → AVCC comme référence
    //   ADLAR = 1 → alignement gauche (résultat 8 bits dans ADCH)
    //   On veut 8 bits car wheel() prend un uint8_t (0-255)
    //   MUX = 0000 → canal ADC0 (potentiomètre RV1 sur PC0)
    ADMUX = (1 << REFS0) | (1 << ADLAR);

    // Datasheet p.218, registre ADCSRA :
    //   ADEN = 1, prescaler 128 → 125kHz
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint8_t adc_read(void) {
    // On reste sur le canal 0 (RV1), pas besoin de changer
    ADCSRA |= (1 << ADSC);          // Lance conversion
    while (ADCSRA & (1 << ADSC));   // Attend la fin
    return ADCH;                     // Résultat 8 bits (ADLAR=1)
}

// === PWM RGB ===
// Datasheet p.113-116 (Timer0) et p.148-151 (Timer2)

void init_rgb(void) {
    // Configure les broches RGB en sortie
    DDRD |= (1 << RED_D);
    DDRD |= (1 << GREEN_D);
    DDRD |= (1 << BLUE_D);

    // Timer0 - Fast PWM (Mode 3 : WGM01=1, WGM00=1)
    // Datasheet p.113 Table 15-3 : COM0x1=1 → non-inverting
    // Gère Rouge (OC0B = PD5) et Vert (OC0A = PD6)
    TCCR0A |= (1 << WGM01) | (1 << WGM00);   // Fast PWM
    TCCR0A |= (1 << COM0A1);                   // Non-inverting Canal A (Vert)
    TCCR0A |= (1 << COM0B1);                   // Non-inverting Canal B (Rouge)
    TCCR0B |= (1 << CS02) | (1 << CS00);       // Prescaler 1024

    // Timer2 - Fast PWM (Mode 3)
    // Datasheet p.148 Table 17-3 : COM2B1=1 → non-inverting
    // Gère Bleu (OC2B = PD3)
    TCCR2A |= (1 << WGM21) | (1 << WGM20);    // Fast PWM
    TCCR2A |= (1 << COM2B1);                   // Non-inverting Canal B (Bleu)
    TCCR2B |= (1 << CS22) | (1 << CS20);       // Prescaler 1024
}

void set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    // Datasheet p.116 (OCR0A/OCR0B) et p.151 (OCR2B)
    // La valeur OCR définit le duty cycle :
    //   OCR = 0   → 0% (LED éteinte)
    //   OCR = 255 → 100% (LED max)
    OCR0B = r;   // Rouge sur Timer0 Canal B
    OCR0A = g;   // Vert sur Timer0 Canal A
    OCR2B = b;   // Bleu sur Timer2 Canal B
}


void wheel(uint8_t pos) {
    if (pos == 255)
        pos = 252;
    pos = 255 - pos;

    if (pos < 85) {
        // Zone 1 : Rouge décroît, Bleu croît
        set_rgb(255 - pos * 3, 0, pos * 3);
    } else if (pos < 170) {
        // Zone 2 : Vert croît, Bleu décroît
        pos = pos - 85;
        set_rgb(0, pos * 3, 255 - pos * 3);
    } else {
        // Zone 3 : Rouge croît, Vert décroît
        pos = pos - 170;
        set_rgb(pos * 3, 255 - pos * 3, 0);
    }
}


void init_gauge_leds(void) {
    // Configure les 4 LEDs de jauge en sortie
    // Datasheet p.92, registre DDRB : met le bit à 1 = sortie
    DDRB |= (1 << LED_D1);  // PB0
    DDRB |= (1 << LED_D2);  // PB1
    DDRB |= (1 << LED_D3);  // PB2
    DDRB |= (1 << LED_D4);  // PB4
}

void update_gauge(uint8_t val) {
    // val = 0-255 (valeur du potentiomètre)
    // Seuils : D1 à 25%, D2 à 50%, D3 à 75%, D4 à 100%
    //
    // 25% de 255 = 64
    // 50% de 255 = 128
    // 75% de 255 = 192
    // 100% = 255 (ou on peut arrondir un peu)

    // On éteint tout d'abord
    PORTB &= ~((1 << LED_D1) | (1 << LED_D2) | (1 << LED_D3) | (1 << LED_D4));

    // Puis on allume selon le niveau
    if (val >= 64)    // >= 25%
        PORTB |= (1 << LED_D1);
    if (val >= 128)   // >= 50%
        PORTB |= (1 << LED_D2);
    if (val >= 192)   // >= 75%
        PORTB |= (1 << LED_D3);
    if (val >= 253)   // ~100% (avec un peu de marge)
        PORTB |= (1 << LED_D4);


    // Test 1/3 R - G - B
    // 255 / 3 = 85
    if (val == 0)
        PORTB |= (1 << LED_D4);
    if (val >= 84 && val <= 86)
        PORTB |= (1 << LED_D4);
    if (val >= 169 && val <= 171)
        PORTB |= (1 << LED_D4);

}

// === MAIN ===

int main(void) {
    adc_init();
    init_rgb();
    init_gauge_leds();

    while (1) {
        // 1. Lire le potentiomètre (ADC0, 8 bits → 0-255)
        uint8_t pot_val = adc_read();

        // 2. Mettre à jour la couleur de D5 via wheel()
        //    pot_val va de 0 à 255, wheel() attend 0-255 → parfait
        wheel(pot_val);

        // 3. Mettre à jour la jauge D1-D4
        update_gauge(pot_val);

        _delay_ms(20);
    }

    return 0;
}
