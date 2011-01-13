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

void FaultRoutine(void);
void ConfigWDT(void);
void ConfigClocks(void);
void ConfigPins(void);
void ConfigADC10(void);
void ConfigTimerA2(void);
void Transmit(void);
void Erase(void);
void Write(long value);
void Read(void);
void reverse(char s[]);
void itoa(int n, char s[]);
