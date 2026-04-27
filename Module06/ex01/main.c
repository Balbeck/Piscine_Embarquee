#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>

#ifndef F_CPU
# define F_CPU 16000000UL
#endif

#define BAUD 115200
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

/*
** AHT20 I2C address (7-bit): 0x38
** Write mode: (0x38 << 1) | 0 = 0x70
** Read mode:  (0x38 << 1) | 1 = 0x71
**
** AHT20 measurement command: 0xAC with params 0x33, 0x00
** After sending command, wait >= 80ms before reading
** Response: 7 bytes (status + humidity + temp + CRC)
*/
#define AHT20_ADDR 0x38

/* ========== UART functions ========== */

void uart_init(void)
{
	/*
	** ATmega328P datasheet section 20.11 (p.179-183)
	** UBRR0H/L : Baud rate register
	** UCSR0B   : Enable RX (RXEN0) and TX (TXEN0)
	** UCSR0C   : 8-bit data (UCSZ01 + UCSZ00), no parity, 1 stop bit
	*/
	UBRR0H = (unsigned char)(UBRR_VALUE >> 8);
	UBRR0L = (unsigned char)UBRR_VALUE;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_tx(char c)
{
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

/*
** print_hex_value:
** Converts a byte to its 2-character hexadecimal representation
** and sends it via UART.
** Example: 0x4E -> "4E"
*/
void print_hex_value(char c)
{
	const char hex[] = "0123456789ABCDEF";
	/*
	** High nibble: shift right 4 bits, mask with 0x0F
	** Low nibble: mask with 0x0F
	*/
	uart_tx(hex[((unsigned char)c >> 4) & 0x0F]);
	uart_tx(hex[(unsigned char)c & 0x0F]);
}

/* ========== I2C (TWI) functions ========== */

void i2c_init(void)
{
	/*
	** ATmega328P datasheet section 22.5.2 (p.221-222):
	** SCL frequency formula:
	**   SCL_freq = F_CPU / (16 + 2 * TWBR * PrescalerValue)
	**
	** For 100 kHz with F_CPU = 16 MHz:
	**   100000 = 16000000 / (16 + 2 * TWBR * 1)
	**   16 + 2 * TWBR = 160
	**   TWBR = 72
	**
	** Section 22.9.1 (p.239): TWBR register at address 0xB8
	** Section 22.9.3 (p.240): TWSR register at address 0xB9
	**   Table 22-7 (p.241): TWPS1=0, TWPS0=0 -> Prescaler = 1
	*/
	TWBR = 72;
	TWSR = 0;  /* Prescaler = 1 */
}

void i2c_start(void)
{
	/*
	** ATmega328P datasheet section 22.6, step 1 (p.223):
	** "transmit a START condition... by writing a specific value into TWCR"
	**
	** Section 22.9.2 (p.239-240): TWCR register at address 0xBC
	**   Bit 7 - TWINT : Writing 1 clears the flag -> starts the operation
	**   Bit 5 - TWSTA : Set to 1 to generate START condition
	**   Bit 2 - TWEN  : Set to 1 to enable TWI module
	**
	** After START is transmitted, TWINT is set by hardware (step 2, p.223)
	** We poll TWINT to know when the START has been sent.
	**
	** Expected status (Table 22-2, p.226):
	**   0x08 = A START condition has been transmitted
	**   0x10 = A repeated START condition has been transmitted
	*/
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
}

void i2c_stop(void)
{
	/*
	** ATmega328P datasheet section 22.9.2 (p.240):
	** Bit 4 - TWSTO: "Writing the TWSTO bit to one in Master mode
	**   will generate a STOP condition on the 2-wire Serial Bus.
	**   When the STOP condition is executed on the bus, the TWSTO
	**   bit is cleared automatically."
	**
	** IMPORTANT (section 22.6, step 7, p.224):
	** "Note that TWINT is NOT set after a STOP condition has been sent."
	** -> We do NOT wait for TWINT after STOP.
	*/
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void i2c_write(unsigned char data)
{
	/*
	** ATmega328P datasheet section 22.6, step 5 (p.224):
	** "the application must load a data packet into TWDR. Subsequently,
	**  a specific value must be written to TWCR, instructing the TWI
	**  hardware to transmit the data packet present in TWDR."
	**
	** Section 22.9.4 (p.241): TWDR register at address 0xBB
	**   "In Transmit mode, TWDR contains the next byte to be transmitted."
	**   "It is writable while the TWI is not in the process of shifting
	**    a byte. This occurs when the TWI Interrupt Flag (TWINT) is set."
	**
	** After loading data into TWDR, we clear TWINT (by writing 1)
	** and keep TWEN to start transmission.
	**
	** Then we wait for TWINT to be set again, meaning the byte
	** has been transmitted.
	**
	** Status codes (Table 22-2, p.226-228):
	**   For SLA+W: 0x18 = ACK received, 0x20 = NACK received
	**   For data:  0x28 = ACK received, 0x30 = NACK received
	*/
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
}

unsigned char i2c_read(int ack)
{
	/*
	** ATmega328P datasheet section 22.7.2 (p.229-230):
	** Master Receiver mode — reading data from a slave.
	**
	** Section 22.9.2 (p.240): TWCR register
	**   Bit 6 - TWEA: TWI Enable Acknowledge Bit
	**   "If the TWEA bit is written to one, the ACK pulse is generated
	**    on the TWI bus if [...] a data byte has been received in
	**    Master Receiver [...] mode."
	**
	** Two cases:
	**   1) ack = 1 (ACK) : We want MORE data from the slave
	**      -> Set TWEA bit (bit 6) to generate ACK
	**      -> Status expected: 0x50 (Table 22-3, p.230)
	**         "Data byte has been received; ACK has been returned"
	**
	**   2) ack = 0 (NACK) : This is the LAST byte we want
	**      -> Do NOT set TWEA -> slave knows to stop sending
	**      -> Status expected: 0x58 (Table 22-3, p.230)
	**         "Data byte has been received; NOT ACK has been returned"
	**
	** After TWINT is set, the received byte is in TWDR (p.241):
	** "In Receive mode, the TWDR contains the last byte received."
	*/
	if (ack)
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	else
		TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
	return TWDR;
}

/* ========== AHT20 sensor functions ========== */

/*
** AHT20 measurement sequence (from AHT20 datasheet):
**
** 1. Send trigger measurement command:
**    START -> SLA+W (0x70) -> 0xAC -> 0x33 -> 0x00 -> STOP
**
** 2. Wait >= 80ms for measurement to complete
**    (The AHT20 datasheet specifies this delay.
**     The exercise warns: "Soyez attentif au délai demandé entre
**     la fin de la commande d'écriture et la lecture des trames
**     de données.")
**
** 3. Read 7 bytes of response:
**    START -> SLA+R (0x71) -> read byte0 (ACK) -> read byte1 (ACK)
**    -> ... -> read byte5 (ACK) -> read byte6 (NACK) -> STOP
**
** The 7 bytes are:
**    Byte 0: Status byte
**            Bit 7 = BUSY (1 = measurement in progress)
**            Bit 3 = CAL  (1 = calibrated)
**    Byte 1: Humidity [19:12]
**    Byte 2: Humidity [11:4]
**    Byte 3: Humidity [3:0] (high nibble) | Temperature [19:16] (low nibble)
**    Byte 4: Temperature [15:8]
**    Byte 5: Temperature [7:0]
**    Byte 6: CRC
*/
void aht20_read_raw(unsigned char *buffer)
{
	/* --- Phase 1: Trigger measurement --- */
	i2c_start();
	i2c_write((AHT20_ADDR << 1) | 0);  /* SLA+W = 0x70 */
	i2c_write(0xAC);                     /* Trigger measurement command */
	i2c_write(0x33);                     /* Parameter byte 1 */
	i2c_write(0x00);                     /* Parameter byte 2 */
	i2c_stop();

	/* --- Phase 2: Wait for measurement to complete --- */
	/*
	** AHT20 datasheet: wait at least 80ms after trigger command.
	** The exercise specifically warns about this delay.
	*/
	_delay_ms(80);

	/* --- Phase 3: Read 7 bytes of measurement data --- */
	/*
	** ATmega328P datasheet section 22.7.2 (p.229):
	** "In order to enter a Master mode, a START condition must be
	**  transmitted. [...] if SLA+R is transmitted, MR mode is entered."
	**
	** We read 7 bytes:
	**   - Bytes 0-5: read with ACK (TWEA=1) to tell slave "send more"
	**   - Byte 6:    read with NACK (TWEA=0) to tell slave "last byte"
	**
	** Section 22.7.2 (p.229):
	** "After the last byte has been received, the MR should inform
	**  the ST by sending a NACK after the last received data byte."
	*/
	i2c_start();
	i2c_write((AHT20_ADDR << 1) | 1);  /* SLA+R = 0x71 */
	for (int i = 0; i < 6; i++)
		buffer[i] = i2c_read(1);         /* Read with ACK */
	buffer[6] = i2c_read(0);             /* Read last byte with NACK */
	i2c_stop();
}

/* ========== Main ========== */

int main(void)
{
	unsigned char buffer[7];

	uart_init();
	i2c_init();

	/*
	** AHT20 datasheet: wait 40ms after power-on before first communication
	*/
	_delay_ms(40);

	/*
	** Main loop: read and display raw data continuously.
	**
	** The AHT20 has a default data refresh rate of ~0.5 Hz
	** (approximately 1 measurement every 2 seconds).
	** We use a 2-second delay between readings to respect
	** the manufacturer's recommendation.
	*/
	while (1)
	{
		aht20_read_raw(buffer);

		/* Print the 7 bytes in hexadecimal, space-separated */
		for (int i = 0; i < 7; i++)
		{
			if (i > 0)
				uart_tx(' ');
			print_hex_value(buffer[i]);
		}
		uart_printstr("\r\n");

		/*
		** Wait ~2 seconds before next measurement
		** (AHT20 refresh rate ~0.5 Hz)
		*/
		_delay_ms(2000);
	}

	return 0;
}
