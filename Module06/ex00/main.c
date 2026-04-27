#include <avr/io.h>
#include <util/twi.h>

#ifndef F_CPU
# define F_CPU 16000000UL
#endif

#define BAUD 115200
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

/* AHT20 I2C address (7-bit): 0x38
 * In write mode: (0x38 << 1) | 0 = 0x70
 * In read mode:  (0x38 << 1) | 1 = 0x71
 */
#define AHT20_ADDR 0x38

/* ========== UART functions (from your previous work) ========== */

void uart_init(void)
{
	/* Datasheet section 20.11 (p.179-183): USART Register Description
	 * UBRR0H/L: USART Baud Rate Register
	 * UCSR0B: Enable RX and TX (RXEN0, TXEN0)
	 * UCSR0C: 8-bit data, no parity, 1 stop bit (UCSZ01, UCSZ00)
	 */
	UBRR0H = (unsigned char)(UBRR_VALUE >> 8);
	UBRR0L = (unsigned char)UBRR_VALUE;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_tx(char c)
{
	/* Wait for empty transmit buffer (UDRE0 flag in UCSR0A) */
	while (!(UCSR0A & (1 << UDRE0)))
		;
	UDR0 = c;
}

void uart_printstr(const char *str)
{
	while (*str)
	{
		uart_tx(*str);
		str++;
	}
}

/* Print a byte as 2 hex characters */
void print_hex_value(unsigned char c)
{
	const char hex[] = "0123456789ABCDEF";
	uart_tx(hex[(c >> 4) & 0x0F]);
	uart_tx(hex[c & 0x0F]);
}

/* ========== I2C (TWI) functions ========== */

void i2c_init(void)
{
	/* Datasheet section 22.5.2 (p.221-222): Bit Rate Generator Unit
	 * Formula: SCL_freq = F_CPU / (16 + 2 * TWBR * PrescalerValue)
	 *
	 * For SCL = 100 kHz, F_CPU = 16 MHz, Prescaler = 1:
	 *   100000 = 16000000 / (16 + 2 * TWBR * 1)
	 *   16 + 2*TWBR = 160
	 *   TWBR = 72
	 *
	 * Datasheet section 22.9.1 (p.239): TWBR register (0xB8)
	 *   Set bit rate to 72
	 *
	 * Datasheet section 22.9.3 (p.240-241): TWSR register (0xB9)
	 *   Table 22-7: TWPS1=0, TWPS0=0 -> Prescaler = 1
	 */
	TWBR = 72;
	TWSR = 0;  /* Prescaler = 1 (TWPS1:0 = 00) */
}

void i2c_start(void)
{
	/* Datasheet section 22.6 (p.223-224): Using the TWI
	 * Step 1: Write TWCR to initiate START condition
	 *   - TWINT = 1 : Clear the interrupt flag (starts operation)
	 *   - TWSTA = 1 : Generate START condition
	 *   - TWEN  = 1 : Enable TWI module
	 *
	 * Datasheet section 22.9.2 (p.239-240): TWCR register (0xBC)
	 *   Bit 7 (TWINT): Writing 1 clears the flag
	 *   Bit 5 (TWSTA): START condition bit
	 *   Bit 2 (TWEN):  TWI Enable bit
	 */
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	/* Step 2: Wait for TWINT flag to be set
	 * This indicates the START condition has been transmitted
	 * (p.223: "When the START condition has been transmitted,
	 *  the TWINT Flag in TWCR is set")
	 */
	while (!(TWCR & (1 << TWINT)))
		;

	/* Step 3: Print the status code for debugging
	 * Datasheet section 22.9.3 (p.240): TWSR bits 7:3 contain
	 * the status code. Mask with 0xF8 to ignore prescaler bits.
	 *
	 * Expected status codes (Table 22-2, p.226):
	 *   0x08 = START condition transmitted
	 *   0x10 = Repeated START condition transmitted
	 */
	uart_printstr("START status: 0x");
	print_hex_value(TWSR & 0xF8);
	uart_printstr("\r\n");
}

void i2c_stop(void)
{
	/* Datasheet section 22.9.2 (p.240): TWCR register
	 *   Bit 4 (TWSTO): "Writing the TWSTO bit to one in Master mode
	 *     will generate a STOP condition on the 2-wire Serial Bus.
	 *     When the STOP condition is executed on the bus, the TWSTO
	 *     bit is cleared automatically."
	 *
	 * Note (p.224): "TWINT is NOT set after a STOP condition has
	 * been sent" — so we don't wait for TWINT here.
	 */
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

/* ========== Main ========== */

int main(void)
{
	uart_init();
	i2c_init();

	uart_printstr("I2C initialized at 100kHz\r\n");

	/* Send START condition */
	i2c_start();

	/* Load SLA+W into TWDR and transmit
	 * Datasheet section 22.6 step 3 (p.223):
	 *   "the application must load SLA+W into TWDR"
	 * AHT20 address = 0x38, write mode -> (0x38 << 1) | 0 = 0x70
	 *
	 * Datasheet section 22.9.4 (p.241): TWDR register (0xBB)
	 *   Load the slave address + W bit
	 *
	 * Then clear TWINT and keep TWEN to start transmission
	 */
	TWDR = (AHT20_ADDR << 1);  /* SLA+W = 0x70 */
	TWCR = (1 << TWINT) | (1 << TWEN);

	/* Wait for transmission to complete */
	while (!(TWCR & (1 << TWINT)))
		;

	/* Check status: expecting 0x18 (SLA+W sent, ACK received)
	 * Datasheet Table 22-2 (p.226):
	 *   0x18 = SLA+W has been transmitted; ACK has been received
	 *   0x20 = SLA+W has been transmitted; NOT ACK has been received
	 */
	uart_printstr("SLA+W status: 0x");
	print_hex_value(TWSR & 0xF8);
	uart_printstr("\r\n");

	/* Send STOP condition */
	i2c_stop();
	uart_printstr("STOP sent\r\n");

	while (1)
		;

	return 0;
}
