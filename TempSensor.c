#include <stdint.h>
#include "inc/tm4c1294ncpdt.h"
#include <stdio.h>

void configTimer()
{
    SYSCTL_RCGCTIMER_R |= (1 << 1); // Enables the clock for timer 1A
    while (!(SYSCTL_PRTIMER_R & 0x02))
        ; // Wait for timer 1 to activate
    TIMER1_CTL_R &= ~(1 << 0); // Stop Timer
    TIMER1_CFG_R = 0x04; // Sets into 16-bit mode
    TIMER1_TAMR_R |= 0x12; // Sets counting direction up and periodic (compare mode)
    TIMER1_TAPR_R = 245 - 1; // Sets pre-scalar value to 245
    TIMER1_TAILR_R = 229 - 1; // Sets load value to 229 to allow counter to count from the bottom to top, which takes one second (ILR)
    TIMER1_CTL_R |= (1 << 0); // Restart timer
}

void configADC()
{
    SYSCTL_RCGCGPIO_R |= (1 << 4); // PE (AIN0 to AIN2 belong to Port E)
    while (!(SYSCTL_PRGPIO_R & 0x10))
        ; // Ready?
    SYSCTL_RCGCADC_R |= 0x00000001; // Clock activation for ADC = 0 (analog to digital converter)
    while (!(SYSCTL_PRADC_R & 0x01))
        ; // Ready?
    GPIO_PORTE_AHB_AFSEL_R |= 0x08; // PE3 Alternative pin function enable
    GPIO_PORTE_AHB_DEN_R &= ~0x08; // PE3 disable digital IO
    GPIO_PORTE_AHB_AMSEL_R |= 0x08; // PE3 enable analog function
    GPIO_PORTE_AHB_DIR_R &= ~0x08; // Allow Input PE3
}

int main(void)
{

    // ADC0 � Configuration (to be implemented as a function)
    configADC();

    // Chose clock for ADC module (�magic code�)
    ADC0_ACTSS_R &= ~0x0F; // Disable all 4 sequencers of ADC0
    SYSCTL_PLLFREQ0_R |= (1 << 23); // PLL Power
    while (!(SYSCTL_PLLSTAT_R & 0x01))
        ; // Until PLL has locked
    ADC0_CC_R |= 0x01;
    // waitcycle++; // PIOSC for ADC sampling clock
    SYSCTL_PLLFREQ0_R &= ~(1 << 23); // PLL Power off

    //setting sample sequence
    ADC0_SSMUX0_R |= 0x00000000; // Adds analog input channel 0 to the sequence
    ADC0_SSEMUX0_R |= 0x00000000; // All 0 since AIN0 5th MSB is 0
    ADC0_SSCTL0_R |= 0x0000A888; // Activates TS3..0 and END3
    ADC0_ACTSS_R |= 0x01; // Enable sequencer 0 ADC0
    // Timer � Configuration (to be implemented as a function)
    configTimer();

    float sum;
    float avgTemp;
    int i;

    while (1)
    {
        //Removing any previous values
        sum = 0;
        avgTemp = 0;
        float temp[4] = {0,0,0,0};

        ADC0_PSSI_R |= 0x01; // starts sampling sequence
        // sampling a sequence of four values every second

        // checking if FIFO is not empty
        if (!(ADC0_SSFSTAT0_R & (1 << 8)))
        {
            for (i = 0; i < 4; i++)
            {
                temp[i] = 147.5 - ((75 * 3.3) * ADC0_SSFIFO0_R) / 4096;
                sum += temp[i];
            }

        }
        else
        {
            printf("fifo empty\n");
        }

        // averaging the values + outputting them in the console window
        avgTemp = sum / 4;
        printf("%.2f�C\n", avgTemp);
        while ((TIMER1_RIS_R & (1 << 0)) == 0)
            ; // Waits for 1 sec
        TIMER1_ICR_R |= (1 << 0); //clear timeout bit in RIS register
    }
}
