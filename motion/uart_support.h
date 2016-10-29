/*
 * uart_support.h
 * Setup, transmit & receive from UART
 */

void static init_UART(long int baud)
{
    int rate = ((F_CPU >> 4) / baud) - 1;
    //
    // Set baud rate (always set H->L order)
    UBRR0H = (unsigned char)(rate >> 8);
    UBRR0L = (unsigned char)(rate);
    // disable 2x speed mode & MultiProcessor Mode
    // also clear TX complete flag
    UCSR0A = (1 << TXC0);
    // Enable receiver and transmitter
    // also receiver interrupts
    UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
    // Set frame format, 8N1
    UCSR0C = (3 << UCSZ00);
}

void static inline UART_char(unsigned char ch)
{
    while (!(UCSR0A & (1 << UDRE0))) ;
    UDR0 = ch;
}

void static UART_string(char *str)
{
    while (*str) {
        UART_char(*str);
        str++;
    }
}

void static inline UART_crlf()
{
    UART_string((char *)"\r\n");
}

void static UART_unsigned(unsigned long int value, uint8_t base)
{
    if (value >= base) {
        UART_unsigned(value / base, base);
        value = value % base;
    }
    UART_char(value + 48 + ((value > 9) ? 7 : 0));
}

void static UART_signed(long int value, uint8_t base)
{
    if (value < 0) {
        UART_char('-');
        value = -value;
    }
    UART_unsigned(value, base);
}

// MUST be a power of 2
const uint8_t RX_BUFF_SIZE = 16;
volatile uint8_t uart_rx_buff[RX_BUFF_SIZE];
volatile uint8_t uart_rx_head = 0;
volatile uint8_t uart_rx_tail = 0;

ISR(USART_RX_vect)
{
    uint8_t in = UDR0;
    // figure position of next char to store
    uint8_t next = (uart_rx_tail + 1) & (RX_BUFF_SIZE - 1);
    // store it always, overwrite last if full
    uart_rx_buff[uart_rx_tail] = in;
    // if not full, bump tail by one
    if (next != uart_rx_head) {
        uart_rx_tail = next;
    }
}

uint8_t UART_available()
{
    return (uart_rx_tail - uart_rx_head) & (RX_BUFF_SIZE - 1);
}

uint8_t UART_getchar()
{
    uint8_t retval = 0;
    if (uart_rx_head != uart_rx_tail) {
        retval = uart_rx_buff[uart_rx_head];
        uart_rx_head = (uart_rx_head + 1) & (RX_BUFF_SIZE - 1);
    }
    return retval;
}
