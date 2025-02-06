
#include <s3c44b0x.h>
#include <leds.h>

void leds_init( void )
{
    PDATB |= ((1<<9) |(1<<10));
}

void led_on( uint8 led )
{
    if(led == LEFT_LED){
    	PDATB &= ~(1<<9); //pone a 0 el bit 9
    }else if(led == RIGHT_LED){
    	PDATB &= ~(1<<10);//pone a 0 el bit 10
    }
}

void led_off( uint8 led )
{
	if(led == LEFT_LED){
	    PDATB |= (1<<9); //pone a 1 el bit 9
	}else if(led == RIGHT_LED){
	   	PDATB |= (1<<10); //pone a 1 el bit 10
    }
}

void led_toggle( uint8 led )
{
	if(led == LEFT_LED){
		PDATB ^= (1<<9); //invierte el bit 9
	}else if(led == RIGHT_LED){
	   	PDATB ^= (1<<10); //invierte  el bit 10
	}
}

uint8 led_status( uint8 led )
{
	switch(led){
	case RIGHT_LED:
		if(PDATB &(1<<10)){
			return 0;
		}else{return 1;}

	case LEFT_LED:
			if (PDATB &(1<<9)){
				return 0;

			}else{return 1;}

	}
	return 1;


  /*  if((led == RIGHT_LED) && (PDATB &(1<<10))){//si el bit 10 vale 1 significa que
		 	 	 	 	 	 	 	 	 	   //que esta apagado y devuelve 0
        return 0;
    }
    else if((led == LEFT_LED) && (PDATB &(1<<9))){ //si el bit 9 vale 1 significa que
    										  //que esta apagado y devuelve 0
    	return 0;
    }else{return 1;}*/
}
