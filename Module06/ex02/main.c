#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>
#include <stdlib.h>   /* dtostrf() */

#ifndef F_CPU
# define F_CPU 16000000UL
#endif

#define BAUD 115200
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

/*
** AHT20 sensor address and commands
** -----------------------------------
** I2C 7-bit address: 0x38
**   Write mode (SLA+W): (0x38 << 1) | 0 = 0x70
**   Read mode  (SLA+R): (0x38 << 1) | 1 = 0x71
**
** Trigger measurement command: 0xAC + params 0x33, 0x00
** Wait >= 80ms after trigger before reading
** Response: 7 bytes (status, hum[19:12], hum[11:4], hum[3:0]|temp[19:16],
**                    temp[15:8], temp[7:0], CRC)
**
** Conversion formulas (from AHT20 datasheet):
**   Humidity (%RH)    = (raw_hum  / 2^20) * 100
**   Temperature (°C)  = (raw_temp / 2^20) * 200 - 50
**
** Sensor accuracy (from AHT20 datasheet):
**   Temperature: ±0.3°C  -> display 1 decimal (0.1°C)
**   Humidity:    ±2%RH   -> display 1 decimal (0.1%)
*/
#define AHT20_ADDR 0x38

/* Number of measurements averaged for the displayed value */
#define AVG_WINDOW 3

/* ========== UART functions (from your previous work) ========== */

void uart_init(void)
{
	/*
	** ATmega328P datasheet section 20.11 (p.179-183): USART register description
	**   UBRR0H/L (p.190): Baud rate register
	**   UCSR0B   (p.190): RXEN0 + TXEN0 to enable receiver and transmitter
	**   UCSR0C   (p.190-191): UCSZ01 + UCSZ00 = 8-bit data, no parity, 1 stop bit
	*/
	UBRR0H = (unsigned char)(UBRR_VALUE >> 8);
	UBRR0L = (unsigned char)UBRR_VALUE;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_tx(char c)
{
	/*
	** Datasheet section 20.6.1 (p.169) and 20.11.2 (p.190):
	** UDRE0 (Data Register Empty) flag in UCSR0A indicates buffer is ready
	*/
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

/* ========== I2C (TWI) functions ========== */

void i2c_init(void)
{
	/*
	** ATmega328P datasheet section 22.5.2 (p.221-222):
	** SCL frequency = F_CPU / (16 + 2 * TWBR * PrescalerValue)
	** For 100 kHz @ 16 MHz with prescaler=1: TWBR = 72
	**
	** Section 22.9.1 (p.239): TWBR register at 0xB8
	** Section 22.9.3 (p.240): TWSR register at 0xB9
	**   Table 22-7 (p.241): TWPS1=0, TWPS0=0 -> Prescaler = 1
	*/
	TWBR = 72;
	TWSR = 0;
}

void i2c_start(void)
{
	/*
	** Datasheet section 22.6 (p.223-224) + Table 22-2 (p.226):
	**   TWCR: TWINT + TWSTA + TWEN -> generate START
	**   Wait for TWINT to be set (operation done)
	**   Expected status: 0x08 (START) or 0x10 (REPEATED START)
	*/
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
}

void i2c_stop(void)
{
	/*
	** Datasheet section 22.9.2 (p.240): TWSTO bit generates STOP
	** Note (p.224): "TWINT is NOT set after a STOP condition has been sent"
	*/
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void i2c_write(unsigned char data)
{
	/*
	** Datasheet section 22.6 step 5 (p.224) + section 22.9.4 (p.241):
	**   Load data into TWDR (register 0xBB)
	**   Clear TWINT + TWEN -> start byte transmission
	**   Wait for TWINT
	** Status codes (Table 22-2, p.226-228):
	**   0x18 = SLA+W transmitted, ACK received
	**   0x28 = data byte transmitted, ACK received
	*/
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
}

unsigned char i2c_read(int ack)
{
	/*
	** Datasheet section 22.7.2 (p.229-230) + Table 22-3 (p.230):
	**   Master Receiver mode.
	**   ack = 1 (TWEA=1) -> ACK after byte received -> "send more"
	**     Status 0x50: data byte received, ACK returned
	**   ack = 0 (TWEA=0) -> NACK after byte received -> "this was the last"
	**     Status 0x58: data byte received, NOT ACK returned
	** After TWINT is set, received byte is in TWDR (p.241).
	*/
	if (ack)
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	else
		TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
		;
	return TWDR;
}

/* ========== AHT20 sensor ========== */

/*
** Read 7 raw bytes from AHT20.
** Sequence (AHT20 datasheet):
**   1. Send measurement command:  START -> SLA+W -> 0xAC -> 0x33 -> 0x00 -> STOP
**   2. Wait 80 ms
**   3. Read response:             START -> SLA+R -> 6 bytes (ACK) -> 1 byte (NACK) -> STOP
*/
void aht20_read_raw(unsigned char *buffer)
{
	/* Trigger measurement */
	i2c_start();
	i2c_write((AHT20_ADDR << 1) | 0);
	i2c_write(0xAC);
	i2c_write(0x33);
	i2c_write(0x00);
	i2c_stop();

	/* Wait for measurement to complete (>= 80ms per AHT20 datasheet) */
	_delay_ms(80);

	/* Read 7 bytes back */
	i2c_start();
	i2c_write((AHT20_ADDR << 1) | 1);
	for (int i = 0; i < 6; i++)
		buffer[i] = i2c_read(1);
	buffer[6] = i2c_read(0);
	i2c_stop();
}

/*
** Convert 7 raw bytes from AHT20 into temperature (°C) and humidity (%RH).
**
** Layout of the 6 useful bytes (raw[0] is status, raw[6] is CRC):
**
**   byte 1     byte 2     byte 3                   byte 4     byte 5
** [HHHHHHHH][HHHHHHHH][HHHH|TTTT][TTTTTTTT][TTTTTTTT]
**  hum high  hum mid  hum low |  temp high   temp mid    temp low
**                    temp top
**
** raw_hum  = (byte1 << 12) | (byte2 << 4) | (byte3 >> 4)        [20 bits]
** raw_temp = ((byte3 & 0x0F) << 16) | (byte4 << 8) | byte5      [20 bits]
**
** Conversion (from AHT20 datasheet):
**   humidity_pct = (raw_hum  / 2^20) * 100
**   temperature  = (raw_temp / 2^20) * 200 - 50
**
** 2^20 = 1048576
*/
void aht20_compute(const unsigned char *raw, float *temperature, float *humidity)
{
	uint32_t raw_hum;
	uint32_t raw_temp;

	raw_hum = ((uint32_t)raw[1] << 12)
	        | ((uint32_t)raw[2] << 4)
	        | ((uint32_t)raw[3] >> 4);

	raw_temp = ((uint32_t)(raw[3] & 0x0F) << 16)
	         | ((uint32_t)raw[4] << 8)
	         | (uint32_t)raw[5];

	*humidity    = ((float)raw_hum  / 1048576.0f) * 100.0f;
	*temperature = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;
}

/* ========== Helpers ========== */

/*
** Print a float using dtostrf (avr-libc).
** dtostrf(value, width, precision, buffer):
**   - width:     minimum number of characters (negative = left-aligned)
**   - precision: number of digits after the decimal point
**   - buffer:    must be large enough (at least width+1 chars + sign + dot)
*/
void uart_print_float(float value, int precision)
{
	char buffer[16];

	dtostrf(value, 0, precision, buffer);
	uart_printstr(buffer);
}

/* ========== Main ========== */

int main(void)
{
	unsigned char raw[7];
	float        temp;
	float        hum;

	/*
	** Sliding window for averaging the last 3 measurements.
	**
	** Behavior required by the exercise:
	**   "la mesure affichée doit être la moyenne des 3 dernières mesures
	**    prises par le programme. Le comportement pour les deux premières
	**    mesures devra être cohérent avec cette règle."
	**
	** -> 1st measure shown: average of just that 1 measure
	** -> 2nd measure shown: average of the 2 measures so far
	** -> from 3rd onward:   sliding average over the last 3 measures
	*/
	float temp_buf[AVG_WINDOW] = {0};
	float hum_buf[AVG_WINDOW]  = {0};
	int   count = 0;     /* total number of measures taken (capped to AVG_WINDOW) */
	int   index = 0;     /* circular buffer index */

	uart_init();
	i2c_init();

	/* AHT20 datasheet: wait 40ms after power-on before first communication */
	_delay_ms(40);

	while (1)
	{
		/* 1. Read raw data and convert */
		aht20_read_raw(raw);
		aht20_compute(raw, &temp, &hum);

		/* 2. Store in circular buffer */
		temp_buf[index] = temp;
		hum_buf[index]  = hum;
		index = (index + 1) % AVG_WINDOW;
		if (count < AVG_WINDOW)
			count++;

		/* 3. Compute the average over the last `count` measurements */
		float temp_avg = 0;
		float hum_avg  = 0;
		for (int i = 0; i < count; i++)
		{
			temp_avg += temp_buf[i];
			hum_avg  += hum_buf[i];
		}
		temp_avg /= count;
		hum_avg  /= count;

		/* 4. Display
		**    AHT20 accuracy: ±0.3°C, ±2%RH
		**    -> show 1 decimal for both (rounded to upper "tenth")
		*/
		uart_printstr("Temperature: ");
		uart_print_float(temp_avg, 1);
		uart_printstr("\xC2\xB0""C, Humidity: ");  /* UTF-8 °C */
		uart_print_float(hum_avg, 1);
		uart_printstr("%\r\n");

		/*
		** AHT20 datasheet: "it is recommended to measure data every 2 seconds"
		** to keep self-heating below 0.1°C
		*/
		_delay_ms(2000);
	}

	return 0;
}
