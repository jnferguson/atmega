#define F_CPU 20000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>

#define NULL ((void*)0)
#define SBUF_SIZE 128
#define SBUF_MASK (SBUF_SIZE-1)

unsigned int eodd = 0;

volatile struct {
	uint8_t send;
	uint8_t head;
	uint8_t tail;
	uint8_t len;
	uint8_t buf[SBUF_SIZE];
} sbuf = { 0 };

void
spi_init(void)
{
	DDRB |= _BV(DDB3) | _BV(DDB5) | _BV(DDB2); //_BV(PB3) | _BV(PB5) | _BV(PB2);
	SPSR &= ~_BV(SPI2X);
	SPCR = _BV(SPIE) | _BV(SPE) | _BV(MSTR) | _BV(SPR1) | _BV(SPR0) | ~_BV(DORD);

	return;
}

void
spi_send(void)
{
	uint8_t c;

	if (0 < sbuf.len) {

		SPSR;
		SPDR;
		sbuf.send = 1;
		c = sbuf.buf[sbuf.head++];
		sbuf.head &= SBUF_MASK;
		sbuf.len--;
		SPDR = c;

	}

	return;
}

void
spi_putc(uint8_t byte)
{
	if (SBUF_SIZE >= sbuf.len) {
		sbuf.buf[sbuf.tail] = byte;
		sbuf.tail++;
		sbuf.tail &= SBUF_MASK;

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			sbuf.len++;
		}
	}

	if (!sbuf.send)
		spi_send();

	return;
}

void
spi_xmit(uint8_t byte)
{

	SPSR;
	SPDR;
	SPDR = byte;

	while (!(SPSR)& (1 << SPIF))
		SPDR;

	SPDR;
	_delay_ms(1);
	return;
}

void
clear(void)
{
	spi_putc(0xFE);
	spi_putc(0x51);
	return;
}

void
home(void)
{
	spi_putc(0xFE);
	spi_putc(0x46);
	return;
}

void
setPos(uint8_t p)
{
	spi_putc(0xFE);
	spi_putc(0x45);
	spi_putc(p);
	return;
}

void
contrast(uint8_t b)
{
	if (50 < b)
		b = 50;

	spi_putc(0xFE);
	spi_putc(0x52);
	spi_putc(b);
	return;
}

void
brightness(uint8_t b)
{
	if (8 < b)
		b = 8;

	spi_putc(0xFE);
	spi_putc(0x53);
	spi_putc(b);
	return;
}

void
write(const char* str)
{
	unsigned int cpos = 0;

	if (NULL == str)
		return;

	clear();
	home();

	while (*str) {
		switch (cpos) {
		case 16:
			setPos(42);
			cpos = 42;
			break;
		case 58:
			home();
			cpos = 0;
			break;
		default:
			break;
		}

		spi_putc(*str++); //spi_xmit(*str++);
		cpos++;
	}

	return;

}

ISR(TIMER1_OVF_vect)
{
	if (eodd++ % 2)
		PORTB ^= _BV(PB0);
	else
		PORTB ^= _BV(PB1);
	return;
}

ISR(SPI_STC_vect)
{
	sbuf.send = 0;
	spi_send();
}

signed int
main(void)
{
	DDRB |= _BV(DDB0) | _BV(DDB1);
	TCCR1B |= _BV(CS10);
	TIMSK1 |= _BV(TOIE1);

	sei();

	spi_init();
	_delay_ms(100);

	contrast(50);
	brightness(8);

	write("  just a test.  ");

	while (1) {
		_delay_ms(100);
	}

	return 0;
}
