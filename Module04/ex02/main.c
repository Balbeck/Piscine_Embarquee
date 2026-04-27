/*
** ex02 - Compteur binaire sur LEDs D1-D4 avec boutons SW1/SW2
**
** Schéma 42Chips Devboard :
**   SW1 → PD2 (INT0)             - bouton incrémenter
**   SW2 → PD4 (PCINT20, port D)  - bouton décrémenter
**   D1  → PB0    D2 → PB1    D3 → PB2    D4 → PB4 (pas PB3 !)
**
** Stratégie de debounce (sans _delay_ms dans une ISR) :
**   - Timer0 en CTC tick à 1 kHz (1 ms par tick).
**   - Quand un bouton est pressé, l'ISR du bouton arme un compteur de
**     debounce (20 ms) ET désactive temporairement la source d'interruption.
**   - L'ISR Timer0 décrémente ces compteurs. Quand un compteur atteint 0,
**     on relit l'état du bouton : s'il est toujours appuyé, on agit, puis
**     on réactive la source d'interruption.
**   - Toutes les ISR sont courtes (pas de blocage), les autres
**     interruptions ne sont jamais retardées.
*/

#include <avr/io.h>
#include <avr/interrupt.h>

/*
** Volatile : toutes ces variables sont partagées entre ISR et/ou main.
** Le compilateur ne doit PAS les mettre en cache dans un registre.
*/
volatile uint8_t  counter        = 0;   /* valeur affichée (0-15)         */
volatile uint8_t  sw1_debounce   = 0;   /* ms restantes avant validation  */
volatile uint8_t  sw2_debounce   = 0;

#define DEBOUNCE_MS  20

/*
** Affiche les 4 bits de poids faible de val sur D1-D4.
**   bit 0 → PB0 (D1)
**   bit 1 → PB1 (D2)
**   bit 2 → PB2 (D3)
**   bit 3 → PB4 (D4)   ← décalé de +1 car D4 est sur PB4, pas PB3
**
** PB3 ne doit pas être touché (utilisé par LED_R / MOSI). Masque explicite.
*/
static void display_value(uint8_t val)
{
    /*
    ** Datasheet p.100 – section 14.4.4 PORTB
    ** On préserve PB3, PB5, PB6, PB7 ; on ne réécrit que PB0,PB1,PB2,PB4.
    */
    uint8_t portb = PORTB & ~((1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB4));

    portb |= (val & 0x07);            /* bits 0-2 → PB0, PB1, PB2          */
    portb |= ((val & 0x08) << 1);     /* bit 3 décalé → PB4 (saute PB3)    */

    PORTB = portb;
}

int main(void)
{
    /*
    ** --- Sorties LEDs ---
    ** Datasheet p.99 – section 14.4.3 DDRB
    ** PB0, PB1, PB2, PB4 en sortie. PB3 préservé (LED_R / MOSI).
    */
    DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2) | (1 << DDB4);

    /*
    ** --- Entrées boutons avec pull-up ---
    ** Datasheet p.101 – section 14.4.9 DDRD
    ** PD2 (SW1) et PD4 (SW2) en entrée. PD3 (LED_B de la RGB) intact.
    */
    DDRD  &= ~((1 << DDD2) | (1 << DDD4));

    /*
    ** Datasheet p.102 – section 14.4.10 PORTD
    ** Pull-up internes activés sur PD2 et PD4 (boutons actifs-bas).
    */
    PORTD |= (1 << PORTD2) | (1 << PORTD4);

    /*
    ** --- INT0 sur PD2 (SW1, incrément), falling edge ---
    ** Datasheet p.80 – section 13.2.1 EICRA, Table 13-2
    ** ISC01=1, ISC00=0 → falling edge (front descendant à l'appui).
    */
    EICRA |=  (1 << ISC01);
    EICRA &= ~(1 << ISC00);

    /*
    ** Datasheet p.80 – section 13.2.2 EIMSK
    ** Active INT0. INT1 (PD3 = LED bleue) reste désactivé.
    */
    EIMSK |= (1 << INT0);

    /*
    ** --- PCINT20 sur PD4 (SW2, décrément) ---
    ** Datasheet p.81 – section 13.2.4 PCICR : PCIE2 active le groupe PD.
    ** Datasheet p.82 – section 13.2.6 PCMSK2 : on n'active QUE PCINT20.
    */
    PCICR  |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT20);

    /*
    ** --- Timer0 en mode CTC, tick à 1 kHz (1 ms par tick) ---
    **
    ** Datasheet p.112 – section 15.9.1 TCCR0A
    **   WGM01=1, WGM00=0  → mode CTC (avec WGM02=0 dans TCCR0B)
    **   COM0A1:0 = 0      → pin OC0A déconnectée (on ne sort pas de PWM,
    **                       on veut juste l'interruption Compare Match)
    **
    ** Datasheet p.115 – section 15.9.2 TCCR0B
    **   WGM02 = 0
    **   CS02:0 = 011 → prescaler /64 (Table 15-9 p.116)
    **
    ** Calcul du tick :
    **   f_timer = F_CPU / prescaler = 16 000 000 / 64 = 250 000 Hz
    **   pour 1 ms : OCR0A = (250 000 / 1000) - 1 = 249
    **   période effective : (249 + 1) / 250 000 = 1 ms exactement
    */
    TCCR0A = (1 << WGM01);                              /* CTC mode        */
    TCCR0B = (1 << CS01) | (1 << CS00);                 /* prescaler /64   */
    OCR0A  = 249;                                       /* TOP → 1 ms      */

    /*
    ** Datasheet p.117 – section 15.9.6 TIMSK0
    ** OCIE0A = 1 → active l'interruption Compare Match A (vector
    ** TIMER0_COMPA_vect).
    */
    TIMSK0 |= (1 << OCIE0A);

    /*
    ** Datasheet p.79 – section 13 :
    ** "if the SREG I-flag and the corresponding interrupt mask are set"
    ** → activation des interruptions globales.
    */
    sei();

    /* Affichage initial : 0 sur les 4 LEDs */
    display_value(counter);

    /* Consigne : rien dans la boucle main, tout dans les ISR. */
    while (1)
    {
        /* rien */
    }
    return (0);
}

/*
** ISR INT0 – appui sur SW1 (PD2, falling edge).
** Datasheet p.79 (vector INT0_vect).
**
** ISR très courte : on arme juste le compteur de debounce et on désactive
** INT0 le temps que le rebond se calme. Le Timer0 réactivera INT0 dans
** 20 ms si le bouton est toujours appuyé.
*/
ISR(INT0_vect)
{
    EIMSK &= ~(1 << INT0);          /* masque INT0 le temps du debounce  */
    sw1_debounce = DEBOUNCE_MS;     /* le timer décrémentera à chaque ms */
}

/*
** ISR PCINT2 – tout changement sur PD4.
** Datasheet p.81 (vector PCINT2_vect).
**
** Idem : on arme le debounce et on masque PCINT20. Le Timer0 finalisera.
** Note : PCINT déclenche sur tout changement (montant ET descendant), donc
** on filtrera le sens du front à la fin du debounce dans l'ISR Timer0.
*/
ISR(PCINT2_vect)
{
    PCMSK2 &= ~(1 << PCINT20);      /* masque PCINT20 le temps du debounce */
    sw2_debounce = DEBOUNCE_MS;
}

/*
** ISR Timer0 Compare Match A – appelée toutes les 1 ms.
** Datasheet p.117 (vector TIMER0_COMPA_vect).
**
** Décrémente les compteurs de debounce. Quand un compteur atteint 0 :
**   - on relit la pin pour vérifier que le bouton est toujours appuyé
**     (sinon c'était un parasite ou un relâchement → on ignore)
**   - on agit sur le compteur si l'appui est confirmé
**   - on réactive la source d'interruption du bouton
**
** Lecture de PIND : ici c'est légitime, on filtre pour valider un appui
** stable APRÈS détection par interruption — pas pour détecter l'appui.
*/
ISR(TIMER0_COMPA_vect)
{
    /* --- Debounce SW1 --- */
    if (sw1_debounce != 0)
    {
        sw1_debounce--;
        if (sw1_debounce == 0)
        {
            /* Bouton actif-bas : appuyé = pin à 0 */
            if (!(PIND & (1 << PIND2)))
            {
                counter = (counter + 1) & 0x0F;     /* wrap 0-15 */
                display_value(counter);
            }
            EIMSK |= (1 << INT0);   /* réactive INT0                     */
        }
    }

    /* --- Debounce SW2 --- */
    if (sw2_debounce != 0)
    {
        sw2_debounce--;
        if (sw2_debounce == 0)
        {
            if (!(PIND & (1 << PIND4)))
            {
                counter = (counter - 1) & 0x0F;     /* wrap : 0-1 → 15   */
                display_value(counter);
            }
            PCMSK2 |= (1 << PCINT20);   /* réactive PCINT20              */
        }
    }
}
