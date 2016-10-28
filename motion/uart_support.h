/*
 * uart_support.h
 * Setup, transmit & receive from UART
 */

void static init_UART(long int baud)
{
    int rate = ((F_CPU >> 4) / baud ) -1;
    //
    // Set baud rate
    UBRR0H = (unsigned char)(rate>>8);
    UBRR0L = (unsigned char)(rate);
    // Enable receiver and transmitter
    UCSR0B = (1<<RXEN0)|(1<<TXEN0);
    // Set frame format
    UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void static inline UART_char(unsigned char ch)
{
    while (!(UCSR0A&(1<<UDRE0))) ;
    UDR0 = ch;
}

void static UART_string(char* str)
{
    while (*str) {
        UART_char(*str);
        str++;
    }
}

void static inline UART_crlf()
{
    UART_string((char*)"\r\n");
}

void static UART_unsigned(unsigned long int value, uint8_t base)
{
    if (value>=base) {
        UART_unsigned(value / base, base);
        value = value % base;
    }
    UART_char(value+48+((value>9)?7:0));
}

void static UART_signed(long int value, uint8_t base)
{
    if (value<0) {
        UART_char('-');
        value=-value;
    }
    UART_unsigned(value, base);
}
