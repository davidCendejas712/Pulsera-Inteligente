
#include <s3c44b0x.h>
#include <s3cev40.h>
#include <rtc.h>

#define bin2BCD(n) ((((n)/10)<<4) + (n)%10)
#define BCD2bin(n) (((n)&15)+ ((n)>>4)*10)

extern void isr_TICK_dummy( void );

void rtc_init( void )
{
    TICNT   = 0x80;
    RTCALM  = 0x0;
    RTCRST  = 0x0;
        
    RTCCON  |= (1<<0);
    
    BCDYEAR = 0x13;
    BCDMON  = 0x01;
    BCDDAY  = 0x01;
    BCDDATE = 0x03;
    BCDHOUR = 0x00;
    BCDMIN  = 0x00;
    BCDSEC  = 0x00;

    ALMYEAR = 0x0;
    ALMMON  = 0x0;
    ALMDAY  = 0x0;
    ALMHOUR = 0x0;
    ALMMIN  = 0x0;
    ALMSEC  = 0x0;

    RTCCON &= ~(1<<0);
}

void rtc_puttime( rtc_time_t *rtc_time )
{
    RTCCON |= (1<<0);
    
   BCDYEAR = bin2BCD(rtc_time->year);
   BCDMON  = bin2BCD(rtc_time->mon);
   BCDDAY  = bin2BCD(rtc_time->mday);
   BCDDATE = bin2BCD(rtc_time->wday);
   BCDHOUR = bin2BCD(rtc_time->hour);
   BCDMIN  = bin2BCD(rtc_time->min);
   BCDSEC  = bin2BCD(rtc_time->sec);

    RTCCON &= ~(1<<0);
}

void rtc_gettime( rtc_time_t *rtc_time )
{
    RTCCON |= (1<<0);
    
    rtc_time->year = BCD2bin(BCDYEAR);
    rtc_time->mon  = BCD2bin(BCDMON);
    rtc_time->mday = BCD2bin(BCDDAY);
    rtc_time->wday = BCD2bin(BCDDATE);
    rtc_time->hour = BCD2bin(BCDHOUR);
    rtc_time->min  = BCD2bin(BCDMIN);
    rtc_time->sec  = BCD2bin(BCDSEC);
    if( ! rtc_time->sec ){
    	rtc_time->year = BCD2bin(BCDYEAR);
		rtc_time->mon  = BCD2bin(BCDMON);
		rtc_time->mday = BCD2bin(BCDDAY);
		rtc_time->wday = BCD2bin(BCDDATE);
		rtc_time->hour = BCD2bin(BCDHOUR);
		rtc_time->min  = BCD2bin(BCDMIN);
		rtc_time->sec  = BCD2bin(BCDSEC);
    };

    RTCCON &= ~(1<<0);
}


void rtc_open( void (*isr)(void), uint8 tick_count )
{
    pISR_TICK =(uint32)isr;
    I_ISPC    = BIT_TICK;
    INTMSK   &= ~(BIT_GLOBAL|BIT_TICK);
    TICNT     = 0X80|tick_count;
}

void rtc_close( void )
{
    TICNT     = 0X0;
    INTMSK   |= BIT_TICK;
    pISR_TICK = isr_TICK_dummy;
}
