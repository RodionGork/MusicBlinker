#include "lpc111x.h"

#define INPUT_SIZE 1024
#define BUFFER_SIZE 4
#define FILTERS_N 5

char hex[] = "0123456789ABCDEF";
short x = 0;
int input[BUFFER_SIZE];
int buffers[FILTERS_N][BUFFER_SIZE];
int lasttime;
int filters[FILTERS_N][3] = {
    // a1, a2, b0; b1=0, b2=-b0, a0=65536
    {-380584, 249019, 6562}, // F5 698
    {-347514, 247791, 7177}, // G5 784
    {-307562, 246559, 7792}, // A5 880
    {-259257, 245364, 8390}, // B5 988
    {-201610, 244276, 8934}, // C#6 1109
};
int y2s[FILTERS_N];
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
    IOCON_SWCLK_PIO0_10 = 0x01;
    SYSAHBCLKCTRL |= BIT6 | BIT7 | BIT12 | BIT13; // clock for TC16-0, UART and ADC
    GPIO0DIR |= BIT4 | BIT8 | BIT9 | BIT10; // PIO0_4, 8, 9, 10 as output
    
    PDRUNCFG &= ~BIT4; // ADC power on
    
    int divisor = 12000000 / (16 * 57600);
    UARTCLKDIV = 1;
    U0LCR = 0x83;
    U0DLM = divisor >> 8;
    U0DLL = divisor & 0xFF;
    U0LCR = 0x03;
    U0FCR = 0x07;
    
    TMR16B0MCR = 3;
    TMR16B0MR0 = 1999;
    TMR16B0TCR = 1;
    
    ISER = BIT16 | BIT24; // interrupts from TC16-0 and ADC
    AD0INTEN = 0x100;
    asm("CPSIE i");
    
    count = 0;
    while(1) {
        asm("WFI");
    }    
}

void process() {
    int i;
    for (i = 0; i < FILTERS_N; i++) {
        y2s[i] /= (INPUT_SIZE / 2);
        //sendi(y2s[i], 5);
        //send(' ');
    }
    send(13);
    if (y2s[1] > (y2s[0] + y2s[2])) {
        GPIO0DATA |= BIT8;
    } else {
        GPIO0DATA &= ~BIT8;
    }
    if (y2s[2] > (y2s[1] + y2s[3])) {
        GPIO0DATA |= BIT9;
    } else {
        GPIO0DATA &= ~BIT9;
    }
    if (y2s[3] > (y2s[2] + y2s[4])) {
        GPIO0DATA |= BIT10;
    } else {
        GPIO0DATA &= ~BIT10;
    }
}

void resetValues() {
    int i;
    for (i = 0; i < FILTERS_N; i++) {
        y2s[i] = 0;
    }
    count = 0;
}

void Tc160_Handler() {
    if (count < INPUT_SIZE) {
        AD0CR = 0x1000003;
    } else if (count == INPUT_SIZE) {
        process();
        GPIO0DATA ^= BIT4;
        resetValues();
    }
    TMR16B0IR = 1;
}

void Adc_Handler() {
    int i, x, a, a1, a2, y;
    int* f, *b;
    x = (AD0GDR >> 6) & 0x3FF;
    a = BUFFER_SIZE - count % BUFFER_SIZE - 1;
    a1 = (a + 1) % BUFFER_SIZE;
    a2 = (a + 2) % BUFFER_SIZE;
    input[a] = x;
    for (i = 0; i < FILTERS_N; i++) {
        b = buffers[i];
        if (count < 2) {
            y = x;
        } else {
            f = filters[i];
            y = (f[2] * (x - input[a2])
                    - f[1] * b[a2]
                    - f[0] * b[a1]) / 262144;
        }
        b[a] = y;
        if (count >= INPUT_SIZE / 2) {
            y2s[i] += y * y;
        }
    }
    count++;
}

