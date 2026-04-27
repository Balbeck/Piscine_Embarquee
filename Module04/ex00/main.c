/*
 - SW1  → PD2 (INT0)  — Datasheet p.80
 - LED D1 → PB0

Debounce :
	INT0 détecte le front → désactive INT0 + arme Timer0
	Timer0 overflow (×2 ≈ 32ms) → vérifie bouton + toggle LED + réactive INT0
*/
#include <avr/io.h>
#include <avr/interrupt.h>

/* Compteur d'overflows pour atteindre ~32ms de debounce
** volatile car partagé entre deux ISR
** Datasheet p.15 "Reset and Interrupt Handling" */
volatile uint8_t overflow_count = 0;

/*
** ISR INT0 — front descendant détecté sur SW1
** Vecteur INT0_vect — Datasheet p.65 Table 12-1
*/
ISR(INT0_vect)
{
    /* Désactive INT0 pour ignorer les rebonds
    ** EIMSK — Datasheet p.81 */
    EIMSK &= ~(1 << INT0);

    /* Remet le compteur Timer0 à 0 pour partir d'un état propre
    ** TCNT0 — Datasheet p.134 "TCNT0 – Timer/Counter Register" */
    TCNT0 = 0;
    overflow_count = 0;

    /* Arme Timer0 en mode Normal, prescaler 1024
    ** TCCR0B — Datasheet p.133
    ** CS02=1, CS00=1 → prescaler 1024 */
    TCCR0B = (1 << CS02) | (1 << CS00);

    /* Active l'interruption overflow du Timer0
    ** TIMSK0 — Datasheet p.136
    ** TOIE0=1 → interrupt on overflow */
    TIMSK0 = (1 << TOIE0);
}

/*
** ISR Timer0 Overflow — appelée toutes les ~16.4ms
** Vecteur TIMER0_OVF_vect — Datasheet p.65 Table 12-1
**
** On attend 2 overflows (~32ms) pour être sûr que
** les rebonds sont terminés
*/
ISR(TIMER0_OVF_vect)
{
    overflow_count++;

    if (overflow_count >= 2)
    {
        /* Arrête le Timer0
        ** TCCR0B — Datasheet p.133 : CS0x=000 → no clock source */
        TCCR0B = 0;

        /* Désactive l'interruption overflow
        ** TIMSK0 — Datasheet p.136 */
        TIMSK0 = 0;

        /* Vérifie que le bouton est TOUJOURS enfoncé
        ** (évite un toggle parasite si c'était juste du bruit)
        ** PIND — Datasheet p.100 "PIN Register" 
        ** SW1 actif bas → PD2 = 0 si appuyé */
        if (!(PIND & (1 << PIND2)))
        {
            /* Toggle LED D1
            ** PORTB — Datasheet p.86 */
            PORTB ^= (1 << PB0);
        }

        /* Réarme le flag INT0 en le vidant avant de réactiver
        ** EIFR — Datasheet p.82 "EIFR – External Interrupt Flag Register"
        ** Écrire 1 dans INTF0 efface le flag (évite déclenchement immédiat)
        ** car un front a pu être enregistré pendant le debounce */
        EIFR |= (1 << INTF0);

        /* Réactive INT0
        ** EIMSK — Datasheet p.81 */
        EIMSK |= (1 << INT0);

        overflow_count = 0;
    }
}

int main(void)
{
    /* PB0 (LED D1) en sortie
    ** DDRB — Datasheet p.86 "Configuring the Pin" */
    DDRB |= (1 << DDB0);

    /* PD2 (SW1) en entrée avec pull-up interne
    ** DDRD=0 + PORTD=1 → pull-up activé
    ** Datasheet p.86 */
    DDRD  &= ~(1 << DDD2);
    PORTD |=  (1 << PD2);

    /* Configure INT0 sur front descendant (falling edge)
    ** EICRA — Datasheet p.80 Table 13-1
    ** ISC01=1, ISC00=0 → falling edge */
    EICRA |=  (1 << ISC01);
    EICRA &= ~(1 << ISC00);

    /* Active INT0
    ** EIMSK — Datasheet p.81 */
    EIMSK |= (1 << INT0);

    /* Active les interruptions globales
    ** Datasheet p.15 */
    sei();

    /* Boucle principale vide */
    while (1)
        ;

    return (0);
}
