#include <msp430.h>
#include <stdint.h>

uint8_t receivedData = 0;
volatile uint8_t packetReceived = 0;
volatile int packet = 0;
volatile uint8_t signal = 0x00;

int emergency = 0;
int count = 0;

// SPI write function
void spiWrite(uint8_t addr, uint8_t data) {
    P1OUT &= ~BIT4; // CS active (low)

    while (!(UCA0IFG & UCTXIFG)); // Wait for TX buffer to be ready
    UCA0TXBUF = addr | 0x80;      // Write command

    while (!(UCA0IFG & UCTXIFG)); // Wait for TX buffer to be ready
    UCA0TXBUF = data;             // Send data

    while (UCA0STATW & UCBUSY);   // Wait for SPI transaction to complete
    P1OUT |= BIT4;                // CS inactive (high)
}

// SPI read function
uint8_t spiRead(uint8_t addr) {
    uint8_t data;
    P1OUT &= ~BIT4;               // CS active (low)

    while (!(UCA0IFG & UCTXIFG)); // Wait for TX buffer to be ready
    UCA0TXBUF = addr & 0x7F;      // Read command

    while (!(UCA0IFG & UCRXIFG)); // Wait for RX buffer
    UCA0TXBUF = 0x00;             // Send dummy data

    while (UCA0STATW & UCBUSY);   // wait for SPI transaction to complete
    data = UCA0RXBUF;             // Read response
    P1OUT |= BIT4;                // CS inactive (high)
    return data;
}

// Configure LoRa
void configLoRa() {

    //verify version
    //if(spiRead(0x42) != 0x12){ while(1){} }
    while(spiRead(0x42)!= 0x12){}

    //put into loRa/ sleep mode
    spiWrite(0x01, 0x80);

    //set freq to 915MHz
    spiWrite(0x06, 0xE4);
    spiWrite(0x07, 0xC0);
    spiWrite(0x08, 0x00);

    //set base address of fifo to h00 (tx and rx)
    spiWrite(0x0E, 0x00);
    spiWrite(0x0F, 0x00);

    //Other random stuff
    spiWrite(0x0C, 0x23); //turn on HF LNA boost
    spiWrite(0x26, 0x04); //Set Reg Modem Config
    spiWrite(0x4D, 0x84); //set PaDac (PABoost)
    spiWrite(0x0B, 0x2B); //set overcurrent protection
    spiWrite(0x09, 0x8F); //set Pa output power

    //set to LoRa/ standby mode
    spiWrite(0x01, 0x81);

}

int packetsearch() {
    uint8_t flags = spiRead(0x12);          //check flags
    if(flags&0x40){                        //if no rxdone flag
        spiWrite(0x12,0xFF);                //clear flags
        return 1;
    }
    else if(flags&0x80){
            spiWrite(0x12,0xFF);
            __delay_cycles(50);
            spiRead(0x01);
            spiRead(0x12);
            __delay_cycles(50);
            spiWrite(0x0D, 0x00);
            __delay_cycles(50);
            spiWrite(0x01, 0x86);
            return 0;
        }
    else {
        spiWrite(0x12, 0xFF);               //clear flags
        spiWrite(0x0D, 0x00);               //set fifo pointer to 0x0
        spiWrite(0x01, 0x86);               //set to lora rx single
        return 0;
    }
}

uint8_t recieve() {
    spiWrite(0x12, 0xFF);
    spiRead(0x13);
    spiRead(0x10);
    spiWrite(0x0D, 0x00);
    spiRead(0x0D);
    spiWrite(0x01, 0x81);
    spiRead(0x12);
    return(spiRead(0x00));
}




// Main function
int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer

    // SPI configuration
    UCA0CTLW0 |= UCSWRST;
    UCA0CTLW0 |= UCSSEL__SMCLK | UCMSB | UCMST | UCSYNC;
    UCA0BRW = 0x08;
    UCA0CTLW0 &= ~UCSWRST;

    // SPI GPIO setup
    P1SEL0 |= BIT5 | BIT6 | BIT7; // P1.5: SCK, P1.6: MISO, P1.7: MOSI
    P1SEL1 &= ~(BIT5 | BIT6 | BIT7);

    // CS and reset pin setup
    P1DIR |= BIT4; // CS
    P1OUT |= BIT4;

    P2DIR |= BIT0; // Reset
    P2OUT |= BIT0;

    //on off lights
    P1DIR |= BIT2;
    P1OUT &= ~BIT2;

    P1DIR |= BIT3;
    P1OUT |= BIT3;

    //pb
    P2DIR &= ~BIT1; //config P2.0 as input
    P2REN |=  BIT1; //enable resistor
    P2OUT &= ~BIT1; //make pull down resistor


    // Unlock GPIO pins
    PM5CTL0 &= ~LOCKLPM5;

    //--setup IRQ
    P2IES &= ~BIT1; //config IRQ sensitivity L to H
    P2IFG &= ~BIT1;
    P2IE |= BIT1;
    __enable_interrupt();



    // Reset LoRa module
    P2OUT &= ~BIT0;
    __delay_cycles(1000);
    P2OUT |= BIT0;

    configLoRa();      // Configure LoRa module
    //packet = packetsearch();




    while (1) {
        __delay_cycles(100000);
        packet = packetsearch();
        //signal = recieve();
        if(emergency ==1){
            P1OUT |= BIT3;
            P1OUT &= ~BIT2;
            signal = recieve();
            count = 0;
        }
        else if (packet){
            signal = recieve();
            __delay_cycles(10);

            if(signal == 48){
                P1OUT |= BIT2;
                P1OUT &= ~BIT3;

            }

            else if(signal == 49){
                P1OUT |=BIT3;
                P1OUT &= ~BIT2;
            }
            else{
                P1OUT |= BIT2;
                P1OUT &= ~BIT3;
            }
            count = 0;

        }
        else {
            count++;
        }
        if (count == 100){
            signal = recieve();
            count = 0;
        }


        //signal = recieve();





    }

    return 0;
}

#pragma vector = PORT2_VECTOR
__interrupt void ISR_PORT2(void)
{
    if(emergency == 0){
        emergency = 1;
        P1OUT |= BIT3;
        P1OUT &= ~BIT2;
        __delay_cycles(200000);
        P1OUT &= ~BIT3;
        P1OUT &= ~BIT2;
        __delay_cycles(200000);
        P1OUT |= BIT3;
        P1OUT &= ~BIT2;
        __delay_cycles(200000);
        P1OUT &= ~BIT3;
        P1OUT &= ~BIT2;
        __delay_cycles(200000);
        P1OUT |= BIT3;
        P1OUT &= ~BIT2;

    }
    else{
        emergency = 0;
        P1OUT &= ~BIT3;
        P1OUT &= ~BIT2;
        __delay_cycles(200000);
        P1OUT |= BIT3;
        P1OUT &= ~BIT2;
    }
    __delay_cycles(200000);
    P2IFG &= ~BIT1;
}
