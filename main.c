#include "lpc111x.h"

#define INPUT_SIZE 256

char hex[] = "0123456789ABCDEF";
short x = 0;
char input[INPUT_SIZE];
short count;

void send(char c) {
    while ((U0LSR & BIT5) == 0);
    U0THR = c;
}

void sends(char* s) {
    while (*s) {
        send(*(s++));
    }
}

void sendi(int x, int d) {
    while (d-- > 0) {
        send(hex[(x >> (d * 4)) & 0xF]);
    }
}

int main() {
    int n;
    
    SYSAHBCLKCTRL |= BIT16; // clock for IOCON
    IOCON_PIO1_6 = 0x01;
    IOCON_PIO1_7 = 0x01;
    IOCON_R_PIO0_11 = 0x02;
    SYSAHBCLKCTRL |= BIT6 | BIT7 | BIT12 | BIT13; // clock for TC16-0, UART and ADC
    GPIO0DIR |= BIT4; // PIO0_4 as output
    
    PDRUNCFG &= ~BIT4; // ADC power on
    
    int divisor = 12000000 / (16 * 57600);
    UARTCLKDIV = 1;
    U0LCR = 0x83;
    U0DLM = divisor >> 8;
    U0DLL = divisor & 0xFF;
    U0LCR = 0x03;
    U0FCR = 0x07;
    
    TMR16B0MCR = 3;
    TMR16B0MR0 = 1087;
    TMR16B0TCR = 1;
    
    ISER = BIT16 | BIT24; // interrupts from TC16-0 and ADC
    AD0INTEN = 0x100;
    asm("CPSIE i");
    
    count = 0;
    while(1) {
        //AD0CR = 0x1000003;
        asm("WFI");
    }    
}

void process() {
    int i, a, s, x;
    a = 0;
    for (i = 0; i < INPUT_SIZE; i++) {
        a += input[i];
    }
    a >>= 8;
    sendi(a, 5);
    send(' ');
    s = 0;
    for (i = 0; i < INPUT_SIZE; i++) {
        x = input[i] - a;
        s += x * x;
    }
    s >>= 8;
    sendi(s, 5);
    send(13);
}

void Tc160_Handler() {
    if (count < INPUT_SIZE) {
        AD0CR = 0x1000003;
    } else if (count == INPUT_SIZE) {
        process();
        count++;
    } else if (count < INPUT_SIZE * 5) {
        count++;
    } else {
        GPIO0DATA ^= BIT4;
        count = 0;
    }
    TMR16B0IR = 1;
}

void Adc_Handler() {
    input[count++] = (AD0GDR >> 8) & 0xFF;
}
