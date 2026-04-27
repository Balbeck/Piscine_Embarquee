
/*
** Module04 - ex01 : Timer0 interrupt + PWM Timer1
**
** Timer1 → PWM Fast 8-bit sur OC1A (PB1) — Datasheet p.171-173
** Timer0 → CTC interrupt ~2ms          — Datasheet p.132-135
*/

#include <avr/io.h>
#include <avr/interrupt.h>

/*
** ISR Timer0 Compare Match A — ~toutes les 2ms
** Datasheet p.65 Table 12-1 : vecteur TIMER0_COMPA_vect
*/
ISR(TIMER0_COMPA_vect)
{
    static uint16_t duty = 0;
    static int8_t   direction = 1;   // 1=monte, -1=descend

    /* Met à jour le duty cycle du Timer1
    ** OCR1A — Datasheet p.177 */
    OCR1A = duty;

    duty += direction;

    /* Inverse la direction aux bornes 0 et 255 */
    if (duty == 255)
        direction = -1;
    else if (duty == 0)
        direction = 1;
}

int main(void)
{
    /* PB1 (OC1A) en sortie
    ** Datasheet p.86 "Configuring the Pin" */
    DDRB |= (1 << DDB1);

    /*
    ** --- Configuration Timer1 : Fast PWM 8-bit, prescaler 8 ---
    **
    ** TCCR1A — Datasheet p.171
    **   COM1A1=1, COM1A0=0 : Clear OC1A on compare, set at BOTTOM
    **                        (mode non-inverting)
    **   WGM11=0,  WGM10=1  : Fast PWM 8-bit (mode 5, bits de poids faible)
    */
    TCCR1A = (1 << COM1A1) | (1 << WGM10);

    /*
    ** TCCR1B — Datasheet p.173
    **   WGM12=1             : Fast PWM 8-bit (mode 5, bit de poids fort)
    **   CS11=1              : Prescaler 8
    **   → F_pwm = 16MHz / (8 × 256) ≈ 7812 Hz
    */
    TCCR1B = (1 << WGM12) | (1 << CS11);

    /* Duty cycle initial à 0
    ** OCR1A — Datasheet p.177 */
    OCR1A = 0;

    /*
    ** --- Configuration Timer0 : CTC, prescaler 1024 ---
    **
    ** TCCR0A — Datasheet p.132
    **   WGM01=1 : Mode CTC (mode 2)
    */
    TCCR0A = (1 << WGM01);

    /*
    ** TCCR0B — Datasheet p.133
    **   CS02=1, CS00=1 : Prescaler 1024
    */
    TCCR0B = (1 << CS02) | (1 << CS00);

    /*
    ** OCR0A — Datasheet p.134
    **   31 → période = (31+1) × 1024 / 16MHz ≈ 2.048ms
    */
    OCR0A = 31;

    /*
    ** TIMSK0 — Datasheet p.135
    **   OCIE0A=1 : Active interrupt sur Compare Match A
    */
    TIMSK0 = (1 << OCIE0A);

    /* Active les interruptions globales
    ** Datasheet p.15 */
    sei();

    /* Boucle infinie vide — tout géré par les timers */
    while (1)
        ;

    return (0);
}