//***********************************************************
// Temperature Tracking for MSP430G2231
// Records at intervals of roughly 30 minutes
// Enough storage space for 1 week
//
// Usage:
// Event on P1.3 begins program
// While P1.6 is low, event to P1.3 triggers a dump & halt to UART
// While program is running, event to P1.3 triggers dump to UART
//
// UART: 2400 baud
// P1.1 = TXD, P1.2 = RXD
//
// Patrick Shyvers
// CrossStudio 2.0.9
// Completed 1/2011
//***********************************************************

#include <temptrac.h>
#include <msp430g2231.h>
#include <msp430.h>
#include <string.h> 
#include <__cross_studio_io.h>

#define TXD     BIT1                                            // TXD on P1.1
#define RXD     BIT2						// RXD on P1.2
#define Bitime 13*4                                             // 0x0D

unsigned int TXByte = 0;
unsigned char BitCnt;

volatile long tempRaw;
char read_out = 0;
char i = 0;

void main(void){
    ConfigWDT();
    ConfigClocks();
   ConfigPins();
   ConfigADC10();
   ConfigTimerA2();
  
    // Stall until button is pressed. Save flash from repeated power cycles.
    P1DIR |= BIT0; 
    P1OUT |= BIT0;
    while(!read_out){
       __bis_SR_register(LPM3_bits + GIE);		// turn on interrupts and LPM3
    }
    read_out = 0;
    P1DIR &= ~BIT0; 
    P1OUT &= ~BIT0;
    P1DIR |= BIT6; 
    P1OUT |= BIT6;
    __delay_cycles(1000000);
    ConfigPins();
    if ( read_out ) {
        Read();
        while(1){
            __bis_SR_register(LPM3_bits + GIE);		// turn on interrupts and LPM3
        }
    }
    else { 
        Erase();
    }
    
    while(1){
        __bis_SR_register(LPM3_bits + GIE);		// turn on interrupts and LPM3
        if ( i == 1512 ){
            Write(tempRaw);
          i = 0;
        } else {
          i++;
        }	
        if ( read_out == 1 ){
            Read();
        }
    }
}

void Read(void){
    int *Read_ptr = (int *)0x1000;
    long value;
    char s[6], i;
    
    while ( Read_ptr < (int *)0x10C0 ){
        value = *Read_ptr++;				// Read flash to value
        if ( value != -1 ){
            itoa(value, s);
            for ( i = 0; ( i < 5 && s[i] != '\0' ); i++ ){
                TXByte = s[i];
                Transmit();
            }
            TXByte = '\r';
            Transmit();
            TXByte = '\n';
            Transmit();
        }
    }
    read_out = 0;
}


void itoa(int n, char s[]){
    int i, sign;
    
    if ((sign = n) < 0)
        n = -n;
    i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

void reverse(char s[]){
    int i, j;
    char c;
    
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}
 
void Write(long value){
    static int *Flash_ptr = (int *)0x1000;

    WDTCTL = WDTPW + WDTHOLD;				// Disable WDT
    FCTL2 = FWKEY + FSSEL_2 + FN1 + FN0;	// SMCLK/3
    FCTL3 = FWKEY;
    FCTL1 = FWKEY + WRT;					// Set WRT bit for write operation
    
    if ( Flash_ptr == (int *)0x1100 ){
        // Uncomment to start flash over
        // Erase();
        //Flash_ptr = (int *)0x1000;
        //*Flash_ptr++ = value;				// Write value to flash
    } else {
        *Flash_ptr++ = value;	
    }
    
    FCTL1 = FWKEY;							// Clear WRT bit
    FCTL3 = FWKEY + LOCK;					// Lock
    ConfigWDT();
}

void Erase(void){
    char *Flash_ptr = (char *)0x1000;
    
    WDTCTL = WDTPW + WDTHOLD;
    while ( Flash_ptr != (char *)0x10C0 ){
        FCTL2 = FWKEY + FSSEL_2 + FN1 + FN0;
        FCTL3 = FWKEY;
        FCTL1 = FWKEY + ERASE;
        *Flash_ptr = 0x00;				// Trigger erase
        Flash_ptr += 0x40;
        FCTL1 = FWKEY;					// Clear WRT bit
        FCTL3 = FWKEY + LOCK;			// Lock
    }
    ConfigWDT();
}

void ConfigWDT(void){
    WDTCTL = WDT_ADLY_1000;				// 4 sec WDT interval
    IE1 |= WDTIE;					// Enable WDT interrupt
}

void ConfigClocks(void){
    if (CALBC1_1MHZ ==0xFF || CALDCO_1MHZ == 0xFF)				       
        FaultRoutine();				// If calibration data is erased
                                                // run FaultRoutine()
    BCSCTL1 = CALBC1_1MHZ;					// Set range
    DCOCTL = CALDCO_1MHZ;					// Set DCO step + modulation 
    BCSCTL3 |= LFXT1S_2;		    // LFXT1 = VLO
    IFG1 &= ~OFIFG;			 // Clear OSCFault flag
    BCSCTL2 = 0;					// MCLK = DCO = SMCLK	
}
 
void FaultRoutine(void){
    P1OUT = BIT0;			    // P1.0 on (red LED)
    while(1);					    // TRAP
}
 
void ConfigPins(void){
    P1SEL |= TXD + RXD;						// P1.1 & 2 TA0, rest GPIO
    P1DIR = ~(BIT3 + RXD);					// P1.3 input, other outputs
    P1OUT = BIT3;					// clear output pins
    P1REN |= BIT3;							// Enable interrupts on P1.3
    P1IES |= BIT3;
    P1IFG &= ~BIT3;
    P1IE |= BIT3;		
    P2SEL = (char)~(BIT6 + BIT7);				// P2.6 and 2.7 GPIO
    P2DIR |= BIT6 + BIT7;					// P2.6 and 2.7 outputs
    P2OUT = 0;								// clear output pins
}
 
void ConfigADC10(void){
    ADC10CTL1 = INCH_10 + ADC10DIV_0;	// Temp Sensor ADC10CLK	
}

void ConfigTimerA2(void){
    CCTL0 = OUT;			      // TXD Idle as Mark
    TACTL = TASSEL_2 + MC_2 + ID_3;	   // SMCLK/8, continuos mode
}

// WDT interrupt service routine
#pragma vector=WDT_VECTOR
__interrupt void WDT(void){
    if ( i == 1512 ){
        ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON;
        __delay_cycles(500);		     // Wait for ADC Ref to settle  
        ADC10CTL0 |= ENC + ADC10SC;	     // Sampling and conversion start
        __delay_cycles(100);
        ADC10CTL0 &= ~ENC;					// Disable ADC conversion
        ADC10CTL0 &= ~(REFON + ADC10ON);	// Ref and ADC10 off
        tempRaw = ADC10MEM;						// Read conversion value
    }
    __bic_SR_register_on_exit(LPM3_bits);	// Clr LPM3 bits from SR on exit
}

// Function Transmits Character from TXByte 
void Transmit(void){ 
    BitCnt = 0xA;		     // Load Bit counter, 8data + ST/SP
    CCR0 = TAR + Bitime;	      // Current state of TA counter + Some time till first bit
    TXByte |= 0x100;		  // Add mark stop bit to TXByte
    TXByte = TXByte << 1;	     // Add space start bit
    CCTL0 =  CCIS0 + OUTMOD0 + CCIE;  // TXD = mark = idle
    while ( CCTL0 & CCIE );	   // Wait for TX completion
}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void){
    CCR0 += Bitime;		   // Add Offset to CCR0  
    if (CCTL0 & CCIS0){	       // TX on CCI0B?
        if (BitCnt == 0){	 
          CCTL0 &= ~ CCIE ;	 // All bits TXed, disable interrupt
        } else {
            CCTL0 |=  OUTMOD2;	// TX Space
            if (TXByte & 0x01)
            CCTL0 &= ~ OUTMOD2;   // TX Mark
            TXByte = TXByte >> 1;
            BitCnt --;
        }
    }
}

// P1.3 Interrupt Handler
#pragma vector=PORT1_VECTOR
__interrupt void Port1_ISR(void){
    read_out = 1;
    P1IFG = 0;  
}
