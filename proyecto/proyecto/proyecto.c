#include <s3c44b0x.h>
#include <s3cev40.h>
#include <common_types.h>
#include <leds.h>
#include <lcd.h>
#include <system.h>
#include <ts.h>
#include <pbs.h>
#include <iis.h>
#include <uda1341ts.h>
#include <stdlib.h>
#include <rtc.h>
#include <stdio.h>
#include "sensorsEmulator.h"
#include <keypad.h>
#include <string.h>
#include <segs.h>
#include <fix_types.h>

#define AUDIOBUFFER_SIZE   (16000*10) // 16000sps * 10s = 160000 samples
#define INICIO      ((uint8 *)0x0c210000)
#define RELOJ       ((uint8 *)0x0c220000)
#define m11    ((uint8 *)0x0c230000)
#define m12      ((uint8 *)0x0c240000)
#define m13    ((uint8 *)0x0c250000)
#define m14      ((uint8 *)0x0c260000)
#define m15  ((uint8 *)0x0c270000)
#define m21    ((uint8 *)0x0c280000)
#define m22      ((uint8 *)0x0c290000)
#define m23    ((uint8 *)0x0c2a0000)
#define m24      ((uint8 *)0x0c2b0000)
#define m25  ((uint8 *)0x0c2c0000)
#define m31    ((uint8 *)0x0c300000)
#define m32      ((uint8 *)0x0c310000)
#define m33    ((uint8 *)0x0c320000)
#define m34      ((uint8 *)0x0c330000)
#define m35  ((uint8 *)0x0c340000)
#define m41    ((uint8 *)0x0c350000)
#define m42      ((uint8 *)0x0c360000)
#define m43    ((uint8 *)0x0c370000)
#define m44      ((uint8 *)0x0c380000)
#define m45  ((uint8 *)0x0c390000)
#define m51    ((uint8 *)0x0c3a0000)
#define m52      ((uint8 *)0x0c3b0000)
#define m53    ((uint8 *)0x0c3c0000)
#define m54      ((uint8 *)0x0c400000)
#define m55  ((uint8 *)0x0c410000)
#define REC_BUFFER1     ((int16 *)0x0c700000)
#define REC_SIZE1       (64000)  /* (5,0 s) * (2 canales) * (2 B/canal) * (16000 muestras/s) = 320000 B  */
#define QM 12

#define NOKIATUNE  ((int16 *)0x0c420000)
#define TEMON  ((int16 *)0x0c480000)
#define mPorPasos (int16) 3031 //0,74*2^12

typedef struct {
	int16 ch0[AUDIOBUFFER_SIZE];
	int16 ch1[AUDIOBUFFER_SIZE];
} audiobuffer_t;
audiobuffer_t buffer;
typedef struct{
	int dec;
	int seg;
	int min;
	int hora;
	int latPerMin;
} tTiempo;




uint8 *mapaM[5][5]={{m11,m12,m13,m14,m15},
		{m21,m22,m23,m24,m25},
		{m31,m32,m33,m34,m35},
		{m41,m42,m43,m44,m45},
		{m51,m52,m53,m54,m55}};
uint8 nPasos;
uint8 nPulsos;
char tiempoAct[9];
void isr_pb( void ) __attribute__ ((interrupt ("IRQ")));
static void beatHandler( void ) __attribute__ ((interrupt ("IRQ")));
static void stepHandler( void ) __attribute__ ((interrupt ("IRQ")));
void isr_tick( void ) __attribute__ ((interrupt ("IRQ")));
void one_second_loop( void );
/*******************************************************************/
void mapa(void);
void reloj(void);
void cronometro(void);
void temporizador(void);
void alarma(void);
void pasos(void);
void pulso(void);
void alarmaSonido(void);
int sonarAlarma(void);
void compruebaAlarma(rtc_time_t rtc_time);
void compruebaTemporizador(rtc_time_t rtc_time);
void compruebaCronometro(rtc_time_t rtc_time);
volatile static boolean newBeat = FALSE;
volatile static boolean newStep = FALSE;
boolean MenuPrincipal = TRUE;
boolean Map = FALSE;
boolean Map1time = TRUE;
boolean relojMenu = FALSE;
boolean alarmaFlag = FALSE;
boolean pasosFlag = FALSE;
boolean pulsoFlag = FALSE;
boolean TempTermina;
tTiempo Alarma;
tTiempo tiempoCr;
tTiempo tiempoTemp;
boolean backP = FALSE;
boolean backPs = FALSE;
boolean backTemp = FALSE;
boolean backAlarma = FALSE;
boolean MostrarA;
boolean CronoFlag = FALSE;
boolean goCr = TRUE;
boolean inicioCr;
boolean pauseCr = FALSE;
boolean tempFlag = FALSE;
boolean confPintar;
boolean SuenaAlarma;
boolean peligro;

int32 metrosRecorridos;
int segundos;
int contB;
int contS;
int vuelta = 1;
int alt = 100;
int iCr;
int ticks;
boolean conf;
boolean confFinish;
boolean iniTemp;
boolean pintarDirectriz;
int latidosPorMinuto;
int pasosPorMinuto;
boolean reentrar;

volatile boolean flagPb;
/*******************************************************************/
void intToTwoDigitStr(int num, char *str) {
	str[0] = '0' + num / 10;
	str[1] = '0' + num % 10;
}
void intToStr(int num, char *str) {
	str[0] = '0' + num;

}

// Funci�n para convertir las partes de la estructura rtc_time_t a una cadena
void timeToString(const rtc_time_t *rtc_time, char *str) {
	intToTwoDigitStr(rtc_time->hour, str);
	str[2] = ':';
	intToTwoDigitStr(rtc_time->min, str + 3);
	str[5] = ':';
	intToTwoDigitStr(rtc_time->sec, str + 6);
	str[8] = '\0';
}
void timeToStringT(const  tTiempo *rtc_time, char *str) {
	intToTwoDigitStr(rtc_time->hora, str);
	str[2] = ':';
	intToTwoDigitStr(rtc_time->min, str + 3);
	str[5] = ':';
	intToTwoDigitStr(rtc_time->seg, str + 6);
	str[8] = '\0';
}
void timeToStringAla(const tTiempo *rtc_time, char *str) {
	intToTwoDigitStr(rtc_time->hora, str);
	str[2] = ':';
	intToTwoDigitStr(rtc_time->min, str + 3);
	str[5] = '\0';

}
void timeToStringCr(const tTiempo *rtc_time, char *str) {
	intToTwoDigitStr(rtc_time->min, str);
	str[2] = ':';
	intToTwoDigitStr(rtc_time->seg, str + 3);
	str[5] = '.';
	intToStr(rtc_time->dec, str+6);
	str[7] = '\0';

}
void timeToStringCrD(const tTiempo *rtc_time, char *str) {
	intToTwoDigitStr(rtc_time->dec, str + 9);
	str[11] = '\0';
}
void timeToStringCrCompleto(const tTiempo *rtc_time, char *str) {
	intToTwoDigitStr(rtc_time->hora, str);
	str[2] = ':';
	intToTwoDigitStr(rtc_time->min, str + 3);
	str[5] = ':';
	intToTwoDigitStr(rtc_time->seg, str + 6);
	str[8] = ':';
	intToTwoDigitStr(rtc_time->dec, str + 9);
	str[11] = '\0';

}

void main( void )
{
	peligro = FALSE;
	latidosPorMinuto=60;
	pasosPorMinuto=60;
	SuenaAlarma = FALSE;
	reentrar = FALSE;
	confPintar = FALSE;
	MostrarA = TRUE;
	pintarDirectriz= FALSE;
	iniTemp = FALSE;
	confFinish = FALSE;
	conf = FALSE;
	iCr = 0;
	ticks = 0;
	MenuPrincipal = TRUE;
	Map = FALSE;
	Map1time = TRUE;
	relojMenu = FALSE;
	alarmaFlag = FALSE;
	pasosFlag = FALSE;
	pulsoFlag = FALSE;
	CronoFlag = FALSE;
	goCr = TRUE;
	inicioCr = FALSE;
	pauseCr = FALSE;
	tempFlag = FALSE;
	TempTermina = FALSE;
	backP = FALSE;
	backPs = FALSE;
	backTemp = FALSE;
	segundos = 0;
	contB = 0;
	contS =0;
	sys_init();
	leds_init();
	pbs_init();
	lcd_init();
	lcd_clear();
	lcd_on();
	ts_init();
	uda1341ts_init();
	iis_init( IIS_DMA );

	ts_on();
	nPasos=0;
	nPulsos=0;

	tiempoTemp.hora = 0;
	tiempoTemp.min = 0;
	tiempoTemp.seg = 0;
	tiempoCr.hora = 0;
	tiempoCr.min = 0;
	tiempoCr.seg = 0;
	tiempoCr.dec = 0;
	Alarma.dec = -1;
	Alarma.hora = 0;
	Alarma.min = 0;
	Alarma.seg = 0;
	contB = 0;
	contS = 0;
	keypad_init();
	startSensorsEmulator( beatHandler, stepHandler, 60, 60 );     // Arranca el emulador de sensores, instalando beatHandler y stepHandler como RTI de los respectivos sensores
	lcd_putWallpaper( INICIO );
	lcd_draw_box( 25, 30, 295, 110, BLACK, 2 );
	static char *horaa = __TIME__;

	rtc_time_t rtc_time;
	rtc_time.hour = atoi(horaa);
	rtc_time.min = atoi(horaa+3);
	rtc_time.sec = atoi(horaa+6);
	rtc_puttime(&rtc_time);
	//char tiempoAct[9]; // "hh:mm:ss\0"


	tTiempo Temporizador;

	tTiempo Aux;

	uint16 x=0, y=0;

	while( 1 )
	{
		x = 0;
		y = 0;


		int seg = rtc_time.sec;
		rtc_gettime( &rtc_time );
		if(rtc_time.sec != seg){
			segundos++;
		}

		timeToString(&rtc_time, tiempoAct);



		//rtc_time->sec

		if(MenuPrincipal){ 							//MENU
			lcd_putWallpaper( INICIO );
			lcd_draw_box( 25, 30, 295, 110, BLACK, 2 );
			lcd_puts_x3( 35, 40, BLACK, tiempoAct);
			MenuPrincipal = FALSE;
		}

		if(!MenuPrincipal && !Map && !relojMenu && !pasosFlag && !pulsoFlag && !SuenaAlarma && !peligro && !TempTermina){	//PONE LA HORA EN PANTALLA
			lcd_puts_x3( 35, 40, BLACK, tiempoAct);
		}

		if(ts_pressed() && !Map && !relojMenu && !pasosFlag && !pulsoFlag){  // REVISA SI HAY RESPUESTA TACTIL
			ts_getpos( &x, &y);
		}

		//ts_timeout_getpos( &x, &y, 10 );

		if(((x>50 && y>30) && (x<255  && y<110)) || relojMenu)  //RELOJ
		{
			MenuPrincipal = FALSE;
			reloj();





		}

		if( (x>0 && y>167 && x<  105&& y<240) || pasosFlag)  //PASOS
		{
			MenuPrincipal = FALSE;
			pasos();
		}

		if( (x>106 && y>167 && x<  210&& y<240) || pulsoFlag )  // PULSO
		{
			MenuPrincipal = FALSE;
			 pulso();
		}
		if(( x>211 && y>167 && x< 320&& y<240 )||Map)  // MAPA
		{
			MenuPrincipal = FALSE;
			Map = TRUE;
			mapa();
		}

		//funcionesPrincipales(newBeat, newStep );

		if( newBeat )
		{
			newBeat = FALSE;
			led_toggle( LEFT_LED );
			contB++;
		}
		if( newStep )
		{
			newStep = FALSE;
			led_toggle( RIGHT_LED );
			contS++;
		}

		if(segundos == 10){

			segundos = 0;
			latidosPorMinuto = contB*6;
			pasosPorMinuto = contS*6;
			contB = 0;
			contS = 0;
		}

		compruebaAlarma(rtc_time);
		if(iniTemp){
			compruebaTemporizador(rtc_time);
		}
		if(inicioCr){
			compruebaCronometro(rtc_time);
		}


	}
}
void lcd_putdouble(uint16 x, uint16 y, uint8 color, double d) {
    char buf[256 + 1];
    char *p = buf + 256;
    uint8 c;

    *p = '\0';
    // Obtener la parte fraccionaria
       int32 fractional_part = (int32)((d-(int32)d)*1000);

        // Convertir la parte fraccionaria a tres d�gitos
        int i;
        for (i = 0; i < 3; ++i) {
            c = fractional_part % 10;
            *--p = '0' + c;

            fractional_part = fractional_part / 10;
        }
    // Manejar n�meros negativos
    if (d < 0) {
        lcd_puts(x, y, color, "-");
        d *= -1;
    }

    *--p = '.';
    // Convertir la parte entera del n�mero a una cadena de caracteres
    int32 j = (int32)d;
    do {
        c = j % 10;
        *--p = '0' + c;
        j = j / 10;
    } while (j);

    lcd_puts(x, y, color, p);
}
void lcd_putdouble2Decimales(uint16 x, uint16 y, uint8 color, double d) {
    char buf[256 + 1];
    char *p = buf + 256;
    uint8 c;

    *p = '\0';
    // Obtener la parte fraccionaria
       int32 fractional_part = (int32)((d-(int32)d)*100);

        // Convertir la parte fraccionaria a tres d�gitos
        int i;
        for (i = 0; i < 2; ++i) {
            c = fractional_part % 10;
            *--p = '0' + c;

            fractional_part = fractional_part / 10;
        }
    // Manejar n�meros negativos
    if (d < 0) {
        lcd_puts(x, y, color, "-");
        d *= -1;
    }

    *--p = '.';
    // Convertir la parte entera del n�mero a una cadena de caracteres
    int32 j = (int32)d;
    do {
        c = j % 10;
        *--p = '0' + c;
        j = j / 10;
    } while (j);

    lcd_puts(x, y, color, p);
}
/*void lcd_putQ4_12(uint16 x, uint16 y, uint8 color, int32 Q4_12_num) {
    char buf[256 + 1];
    char *p = buf + 256;
    uint8 c;

    *p = '\0';

    // Obtener la parte fraccionaria
    int32 fractional_part = Q4_12_num & 0xFFF;

    // Convertir la parte fraccionaria a tres d�gitos
    int i;
    for (i = 0; i < 3; ++i) {
        c = fractional_part % 10;
        *--p = '0' + c;

        fractional_part = fractional_part / 10;
    }

     // A�adir el punto decimal
    *--p = '.';

    // Convertir el n�mero Q4.12 a entero
    int32 integer_part = Q4_12_num >> 12;

    // Asegurar que la parte entera no sea negativa
    if (integer_part < 0) {
        integer_part = 0;
    }

    do {
        c = integer_part % 10;
        *--p = '0' + c;

        integer_part = integer_part / 10;
    } while (integer_part);
    lcd_puts(x, y, color, p);
}*/
int hexToDecimal(const char *hex) {
	int decimal = 0;
	int base = 1;
	int i = 0;

	// Encuentra la longitud de la cadena hex
	while (hex[i] != '\0') {
		i++;
	}
	i--;

	// Convierte caracteres hexadecimales a decimal
	while (i >= 0) {
		// Si el car�cter es un d�gito, convi�rtelo a decimal
		if (isdigit(hex[i])) {
			decimal += (hex[i] - '0') * base;
		}
		// Si el car�cter es una letra (A-F o a-f), convi�rtelo a decimal
		else if ((hex[i] >= 'A' && hex[i] <= 'F') || (hex[i] >= 'a' && hex[i] <= 'f')) {
			decimal += (toupper(hex[i]) - 'A' + 10) * base;
		}

		// Actualiza la base y mueve al siguiente d�gito
		base *= 16;
		i--;
	}

	return decimal;
}
char* hexadecimalAString(int32 i){
	char buf[8 + 1];
	char *p = buf + 8;
	uint8 c;

	*p = '\0';

	do {
		c = i & 0xf;
		if( c < 10 )
			*--p = '0' + c;
		else
			*--p = 'a' + c - 10;
		i = i >> 4;
	} while( i );
	return p;
}

void beatHandler( void )
{
	newBeat = TRUE;
	nPulsos = nPulsos + 1;
	I_ISPC  = BIT_BEATEMULATOR;
}

void stepHandler( void )
{

	newStep = TRUE;
	nPasos = nPasos + 1;
	I_ISPC  = BIT_STEPEMULATOR;
}

void pasos(void){
	uint16 x, y;
	uint8 metros;
	uint8 ritmo;
	uint8 ritmoMax = 0;



	if (!pasosFlag){        //si viene por tactil
		pasosFlag = TRUE;
		lcd_clear();
		//recuadro back

		lcd_puts( 5, 8, BLACK, "back" );
		lcd_draw_box( 0, 0, 45,30, BLACK, 2 );

		//recuadro central
		lcd_draw_box( 25, 45, 300, 180, BLACK, 2 );
		lcd_puts( 35, 55, BLACK, "Pasos totales: ");
		lcd_puts( 35, 80, BLACK, "Distancia recorrida (m): ");
		lcd_puts( 35, 105, BLACK, "Velocidad (m/s): ");
		lcd_puts( 35, 130, BLACK, "Ritmo actual (pasos/min): ");
		lcd_puts( 35, 155, BLACK, "Ritmo maximo (pasos/min): ");

		//recuadro reset
		lcd_puts_x2( 120, 195, BLACK, "RESET" );
		lcd_draw_box( 115, 195, 205, 225, BLACK, 2 );
	}
	if(!backP){
		if(pasosPorMinuto < 10 && segundos == 0){
			lcd_clear_area( 278, 48,305, 177);

		}
		lcd_putint( 240, 55, BLACK, nPasos);
		metrosRecorridos = (nPasos*(int32)mPorPasos);
		lcd_putdouble2Decimales(240, 80, BLACK, TOFLT(metrosRecorridos, QM));

		//lcd_putQ4_12(240, 80, BLACK, metrosRecorridos);
		//lcd_putint( 255, 80, BLACK, metros);
		ritmo = pasosPorMinuto;
		int32 velocidad = (getStepsPerMin() * (int32)mPorPasos)/60;
		lcd_putdouble(240, 105, BLACK, TOFLT(velocidad, QM));
		//lcd_puts( 255, 105, BLACK, v);
		lcd_putint( 240, 130, BLACK, ritmo);
		if (ritmo > ritmoMax){
			ritmoMax = ritmo;
		}
		lcd_putint( 240, 155, BLACK, ritmoMax);

		if(ts_pressed()){
			ts_getpos( &x, &y);
		}
		//BACK
		if( x>0 && y>0 && x<45  && y<30){
			backP = TRUE;
		}

		//RESET
		if (x > 115 && y > 195 && x < 205 && y < 225){
			//lcd_clear_area( 235, 48,305, 182);
			//recuadro back
			lcd_clear_area( 278, 48,305, 177);
			lcd_puts( 5, 8, BLACK, "back" );
			lcd_draw_box( 0, 0, 45,30, BLACK, 2 );
			nPasos = 0;
			metrosRecorridos = 0;
			ritmoMax = ritmo;
			/*
			lcd_draw_box( 25, 45, 295, 180, BLACK, 2 );
			lcd_puts( 35, 55, BLACK, "Pasos totales: ");
			lcd_puts( 35, 80, BLACK, "Distancia recorrida (m): ");
			lcd_puts( 35, 105, BLACK, "Velocidad (m/s): ");
			lcd_puts( 35, 130, BLACK, "Ritmo actual (pasos/min): ");
			lcd_puts( 35, 155, BLACK, "Ritmo maximo (pasos/min): ");
			*/
			//nPasos = 0;
			//lcd_putint( 255, 55, BLACK, nPasos);
		//	metros = 0;
			//lcd_putint( 255, 80, BLACK, metros);
			//poner velocidad
			//lcd_putint( 255, 130, BLACK, ritmo);
			//ritmoMax = ritmo;
			//lcd_putint( 255, 155, BLACK, ritmoMax);


			//recuadro reset
			lcd_puts_x2( 120, 195, BLACK, "RESET" );
			lcd_draw_box( 115, 195, 205, 225, BLACK, 2 );
		}
	}
	else{
		lcd_clear();
		MenuPrincipal=TRUE;
		pasosFlag = FALSE;
		backP = FALSE;
	}

}

void pulso(void){
	uint16 x, y;

	uint8 reposo = 0;
	uint8 reserva = 0;
	uint8 ritmo;
	uint8 puls;
	uint8 pulsMax = 0;

	if (!pulsoFlag){
		pulsoFlag = TRUE;
		lcd_clear();
		//recuadro back

		lcd_puts( 5, 8, BLACK, "back" );
		lcd_draw_box( 0, 0, 45,30, BLACK, 2 );

		//recuadro central
		lcd_draw_box( 25, 45, 300, 180, BLACK, 2 );
		lcd_puts( 35, 55, BLACK, "Pulsaciones totales: ");
		lcd_puts( 35, 80, BLACK, "Pulso en reposo: ");
		lcd_puts( 35, 105, BLACK, "Reserva cardiaca: ");   //max - reposo
		lcd_puts( 35, 130, BLACK, "Pulso actual (puls/min): ");
		lcd_puts( 35, 155, BLACK, "Pulso maximo (puls/min): ");

		//recuadro reset
		lcd_puts_x2( 120, 195, BLACK, "RESET" );
		lcd_draw_box( 115, 195, 205, 225, BLACK, 2 );
	}
	if(!backPs){
		if(latidosPorMinuto < 10 && segundos == 0){
				lcd_clear_area( 278, 48,305, 177);
			}
		lcd_putint( 240, 55, BLACK, nPulsos);
		ritmo = pasosPorMinuto;
		puls = latidosPorMinuto;
		if (ritmo == 0){
			reposo = puls;
			lcd_putint( 240, 80, BLACK, reposo);
		}
		lcd_putint( 240, 130, BLACK, puls);
		if (puls > pulsMax){
			pulsMax = puls;
		}
		lcd_putint( 240, 155, BLACK, pulsMax);
		if (reposo != 0){
			reserva = pulsMax - reposo;
			lcd_putint( 240, 105, BLACK, reserva);
		}


		if(ts_pressed()){
			ts_getpos( &x, &y);
		}

		//BACK
		if( x>0 && y>0 && x<45  && y<30){
			backPs = TRUE;
		}
		//RESET
		if (x > 115 && y > 195 && x < 205 && y < 225){
			//lcd_clear();
			lcd_clear_area( 278, 48,305, 177);
			//lcd_clear_area( 235, 48,305, 182);
			//recuadro back
			lcd_puts( 5, 8, BLACK, "back" );
			lcd_draw_box( 0, 0, 45,30, BLACK, 2 );

			//recuadro central
			/*lcd_draw_box( 25, 45, 295, 180, BLACK, 2 );
			lcd_puts( 35, 55, BLACK, "Pulsaciones totales: ");
			lcd_puts( 35, 80, BLACK, "Pulsaciones en reposo: ");
			lcd_puts( 35, 105, BLACK, "Reserva cardiaca: ");
			lcd_puts( 35, 130, BLACK, "Pulso actual (puls/min): ");
			lcd_puts( 35, 155, BLACK, "Pulso maximo (puls/min): ");
			*/
			nPulsos = 0;
			//lcd_putint( 255, 55, BLACK, nPulsos);
			reposo = 0;
			reserva = 0;
			//lcd_putint( 255, 130, BLACK, puls);
			pulsMax = puls;
			//lcd_putint( 255, 155, BLACK, pulsMax);

			//recuadro reset
			lcd_puts_x2( 120, 195, BLACK, "RESET" );
			lcd_draw_box( 115, 195, 205, 225, BLACK, 2 );
		}
	}
	else{
		lcd_clear();
		MenuPrincipal=TRUE;
		pulsoFlag = FALSE;
		backPs = FALSE;
	}

}


void reloj(void){
	//pbs_init();
	//pbs_open(isr_pb);
	uint16 x = -1, y = -1;
	//lcd_clear();


	if(!relojMenu || reentrar){
		lcd_clear();
		relojMenu = TRUE;
		lcd_putWallpaper( RELOJ );
		reentrar = FALSE;
	}

	if(pb_scan()!=PB_LEFT){
		if(ts_pressed() && !CronoFlag && !tempFlag && !alarmaFlag){
			ts_getpos( &x, &y );
		}
		if( (x>0 && y>40 && x<106  && y<210) || CronoFlag){
			cronometro();
		}
		if( (x>106 && y>40 && x<214  && y<210) || tempFlag){
			temporizador();
		}
		if((x>215 && y>40 && x<319  && y<210) || alarmaFlag){
			alarma();

		}

	}else{
		lcd_clear();
		MenuPrincipal=TRUE;
		relojMenu = FALSE;
		CronoFlag = FALSE;
	}

}

void cronometro(void){

	uint16 x=-1, y=-1;


		static char* horaCr = "00:00.0";


		if(!CronoFlag){
			lcd_clear();
			CronoFlag = TRUE;
			lcd_puts_x2(110, 40, BLACK, horaCr);
			lcd_draw_box( 175, 180, 315,230, BLACK, 2 );
			if(inicioCr){
				lcd_puts_x2(200, 190, BLACK, "PAUSAR");


			}else{
				lcd_puts_x2(180, 190, BLACK, "INICIAR");
			}

			lcd_draw_box( 10, 180, 145,230, BLACK, 2 );
			lcd_puts_x2(35, 190, BLACK, "RESET");
		}
		if(pauseCr){



			lcd_puts_x2(110, 40, BLACK, horaCr);
			lcd_draw_box( 175, 180, 315,230, BLACK, 2 );

				lcd_puts_x2(180, 190, BLACK, "REANUDAR");




			lcd_draw_box( 10, 180, 145,230, BLACK, 2 );
			lcd_puts_x2(35, 190, BLACK, "RESET");

		}

		if(pb_scan()!=PB_LEFT){

			if(ts_pressed()){

				ts_getpos(&x, &y);

				if( x>10 && y>180 && x<125  && y<230 ){

					tiempoCr.hora = 0;
					tiempoCr.min = 0;
					tiempoCr.seg = 0;
					tiempoCr.dec = 0;
					lcd_clear();
					vuelta = 1;
					alt = 100;
					timeToStringCr(&tiempoCr, horaCr);
					lcd_puts_x2(110, 40, BLACK, horaCr);
					lcd_draw_box( 175, 180, 315,230, BLACK, 2 );

					lcd_puts_x2(200, 190, BLACK, "PAUSAR");
					lcd_draw_box( 10, 180, 145,230, BLACK, 2 );
					lcd_puts_x2(35, 190, BLACK, "RESET");

				}


				if( x>175 && y>180 && x<315  && y<230 && goCr){
					if(inicioCr){
						pauseCr = TRUE;
						inicioCr= FALSE;

					}else{
						pauseCr = FALSE;
						inicioCr = TRUE;
						lcd_clear_area(100, 177, 318,233);
						lcd_draw_box( 175, 180, 315,230, BLACK, 2 );

						lcd_puts_x2(180, 190, BLACK, "PAUSAR");
						lcd_draw_box( 10, 180, 145,230, BLACK, 2 );
						lcd_puts_x2(35, 190, BLACK, "RESET");


					}
					//lcd_clear_area(0, 160, 320, 240);
					//int vuelta = 1;
					//int alt = 100;



				}


			 }else if(inicioCr){

				if(pb_scan()!=PB_LEFT && !pauseCr){


						if(iCr<10 && pb_scan()!=PB_LEFT){
							timeToStringCr(&tiempoCr, horaCr);
							lcd_puts_x2(110, 40, BLACK, horaCr);
							tiempoCr.dec++;
							one_s_loop();

							if(pb_scan()==PB_RIGHT && vuelta <5){
								char *str = "vuelta";
								lcd_puts(5, alt, BLACK, str);
								lcd_putint(60, alt, BLACK, vuelta);

								lcd_puts(75, alt, BLACK,horaCr);


								vuelta++;
								alt+=15;


							}
							iCr++;
						}
						if( iCr == 10){
							iCr = 0;
							tiempoCr.dec = 0;
							//tiempoCr.seg++;

									/*if (tiempoCr.seg > 60) {
										tiempoCr.seg = 0;
										tiempoCr.min++;

										if (tiempoCr.min > 60) {
										tiempoCr.min = 0;
										tiempoCr.hora++;
										}
									}
									*/

						}






					}else {
					pauseCr = TRUE;
					goCr = FALSE;
					}




			}


	}else{
		lcd_clear();
		CronoFlag = FALSE;
		relojMenu = TRUE;
		reentrar = TRUE;


	}
}

void temporizador(void){




	static char* hora = "00:00:00";
	timeToStringT(&tiempoTemp, hora);

	boolean pintarIni = FALSE;
	if(iniTemp && pintarDirectriz ){
		lcd_clear_area(70, 160, 250, 235);
		lcd_puts_x2(20, 150, BLACK, "Pulsa el boton drc");
		lcd_puts_x2(20, 180, BLACK, "para detener");
		pintarDirectriz = FALSE;



	}
	if(!iniTemp && pintarDirectriz ){
		lcd_clear_area(70, 160, 250, 235);
		lcd_puts_x2(20, 150, BLACK, "Pulsa el boton drc");
		lcd_puts_x2(20, 180, BLACK, "para iniciar");
		pintarDirectriz = FALSE;

	}
	if (!tempFlag){
		lcd_clear();
		tempFlag = TRUE;
	}
	if(!backTemp){
		uint16 x = -1, y = -1;
		lcd_puts( 3, 8, BLACK, "back" );
		lcd_draw_box( 0, 0, 45,30, BLACK, 2 );
		lcd_puts_x3( 40, 40, BLACK, hora );

		if(!conf){
			lcd_draw_box(77, 170, 255, 230,BLACK,2);
			lcd_puts_x2(85, 180, BLACK, "Configurar");


		}

		if(ts_pressed()){
			ts_getpos( &x, &y);
		}

		if( x>0 && y>0 && x<45  && y<30){
			backTemp = TRUE;
		}
		if(!confFinish){
			if( (x>77 && y>170 && x<245  && y<230) || (conf)){
				conf = TRUE;
				lcd_clear_area(70, 160, 250, 235);
				uint16 x = -1, y = -1;
				uint8 scancode;

				lcd_puts_x3( 40, 40, BLACK, hora );

				if(ticks  < 2) {

					if(keypad_pressed()){
						scancode = keypad_getchar();
						hora[6+ticks]= hexadecimalAString(scancode)[0];
						tiempoTemp.seg = atoi(hora+6);


						timeToStringT(&tiempoTemp, hora);
						ticks++;
					}



				}else if(ticks < 4 && ticks >= 2){
					if(keypad_pressed()){
						scancode = keypad_getchar();
						hora[1+ticks]= hexadecimalAString(scancode)[0];
						tiempoTemp.min = atoi(hora+3);
						timeToStringT(&tiempoTemp, hora);
						ticks++;

					}



				}else if(ticks<6 && ticks >= 4){

					if(keypad_pressed()){
						scancode = keypad_getchar();
						hora[ticks-4]= hexadecimalAString(scancode)[0];
						tiempoTemp.hora = atoi(hora);
						timeToStringT(&tiempoTemp, hora);
						ticks++;
					}


				}else {
					confFinish = TRUE;

					pintarDirectriz = TRUE;

				}

			}
		}else{

				if(pb_scan()==PB_RIGHT){
					iniTemp =  (!iniTemp);
					pintarDirectriz = (!pintarDirectriz);

				}


				if(ts_pressed()){
					ts_getpos( &x, &y);

					if( (x>77 && y>170 && x<245  && y<230)||iniTemp){
					iniTemp = TRUE;
					}
				}





					/*if ((tiempoTemp.hora > 0 ||  tiempoTemp.min > 0  || tiempoTemp.seg >0 )&& (pb_scan()!=PB_LEFT)){

						if (tiempoTemp.seg > 0) {
							tiempoTemp.seg--;
						} else {
							if (tiempoTemp.min > 0) {
								tiempoTemp.min--;
								tiempoTemp.seg = 59;
							} else {
								if (tiempoTemp.hora > 0) {
									tiempoTemp.hora--;
									tiempoTemp.min = 59;
									tiempoTemp.seg = 59;
								}
							}
						}
						timeToStringT(&tiempoTemp, hora);

						if(tiempoTemp.hora==0 || tiempoTemp.min == 0||tiempoTemp.seg < 10){
							segs_putchar( tiempoTemp.seg );
						}
					}*/



			}

			lcd_puts_x3( 40, 40, BLACK, hora );


			/*lcd_puts_x3( 40, 40, BLACK, hora );
			tiempoT.hora = atoi(horas);
			tiempoT.min = atoi(min);
			tiempoT.seg = atoi(segundos);
			timeToStringT(&tiempoT, hora);
			lcd_draw_box(77, 170, 245, 230,BLACK,2);
			lcd_puts_x2(100, 180, BLACK, "INICIAR");
			ts_getpos(&x, &y);
			if( x>77 && y>170 && x<245  && y<230){
				lcd_clear_area(70, 160, 250, 235);
				lcd_puts_x2(20, 150, BLACK, "Pulsa el boton izq");
				lcd_puts_x2(20, 180, BLACK, "para detener");
				while ((tiempoT.hora > 0 ||  tiempoT.min > 0  || tiempoT.seg >0 )&& (pb_scan()!=PB_LEFT)){
					one_second_loop();
					if (tiempoT.seg > 0) {
						tiempoT.seg--;
					} else {
						if (tiempoT.min > 0) {
							tiempoT.min--;
							tiempoT.seg = 59;
						} else {
							if (tiempoT.hora > 0) {
								tiempoT.hora--;
								tiempoT.min = 59;
								tiempoT.seg = 59;
							}
						}
					}
					timeToStringT(&tiempoT, hora);
					lcd_puts_x3( 40, 40, BLACK, hora);
					if(tiempoT.hora==0 || tiempoT.min ||tiempoT.seg < 10){
						segs_putchar( tiempoT.seg );
					}
				}
				alarmaSonido();
				lcd_clear();
			}*/
		}else{

		lcd_clear();

		relojMenu = TRUE;
		tempFlag = FALSE;
		backTemp = FALSE;
		conf = TRUE;
		ticks = 0;
		reentrar = TRUE;
	}




}



void alarma(void){
	//uda1341ts_init();
	//pbs_init();

	static char* hora = "00:00";
	timeToStringAla(&Alarma, hora);
	if (!alarmaFlag){
		lcd_clear();
		alarmaFlag = TRUE;
	}


	if (!alarmaFlag && Alarma.dec == 0){
		lcd_clear();
		alarmaFlag = TRUE;
		lcd_puts_x2(20, 50, BLACK, "Tienes una alarma: ");
		lcd_puts_x2(102, 125, BLACK, hora);
		lcd_draw_box( 75, 180, 243,225, BLACK, 2 );
		lcd_puts_x2(96, 188, BLACK, "ELIMINAR");



	}
	if(Alarma.dec == 0 && MostrarA ){
		lcd_clear();
		lcd_puts_x2(20, 50, BLACK, "Tienes una alarma: ");
		lcd_puts_x2(120, 125, BLACK, hora);
		lcd_draw_box( 75, 180, 243,225, BLACK, 2 );
		lcd_puts_x2(96, 188, BLACK, "ELIMINAR");

		MostrarA = FALSE;
	}
	if(Alarma.dec == -1){

		lcd_puts_x2(20, 150, BLACK, "Configurando...");
		if( Alarma.dec == -1 && Alarma.hora !=0 && Alarma.min !=0){
			lcd_puts(20, 190, BLACK, "Pulsa el boton dch para activar");
			lcd_puts(20, 210, BLACK, "la alarma por defecto");
		}

		confPintar = TRUE;

	}
	if(pb_scan()==PB_RIGHT && Alarma.dec == -1 && Alarma.hora !=0 && Alarma.min !=0){
		Alarma.dec = 0;
	}
	if(!backAlarma){

		uint16 x, y;
		timeToStringAla(&Alarma ,hora);
		lcd_puts( 3, 8, BLACK, "back" );
		lcd_draw_box( 0, 0, 45,30, BLACK, 2 );

		if(ts_pressed()){
			ts_getpos( &x, &y );
		}


		if( x>0 && y>0 && x<45  && y<30){
			backAlarma = TRUE;
		}

		if(Alarma.dec == 0){



			if( x>75&& y>180 && x<248  && y<225){
				Alarma.hora = 0;
				Alarma.min = 0;
				Alarma.dec = -1;
				timeToStringAla(&Alarma, hora);
				confPintar = FALSE;
				lcd_clear();
			}

		}
		else{


			uint8 scancode;

			if(Alarma.hora < 24 && Alarma.min < 59){
				if(ticks  < 2)
				{

					if(keypad_pressed()){
						scancode = keypad_getchar();
						hora[3+ticks]= hexadecimalAString(scancode)[0];
						Alarma.min = atoi(hora+3);
						timeToStringAla(&Alarma, hora);
						ticks++;

					}
				}

				else if(ticks >= 2 && ticks < 4){


					if(keypad_pressed()){
						scancode = keypad_getchar();
						hora[ticks-2]= hexadecimalAString(scancode)[0];
						Alarma.hora = atoi(hora);
						timeToStringAla(&Alarma, hora);
						ticks++;

					}
				}


				if(ticks == 4){
					Alarma.dec = 0;
					ticks = 0;
					MostrarA = TRUE;

				}



			}else{
				ticks = 0;
				Alarma.hora = 0;
				Alarma.min = 0;
			}
		}
	}
	else{
		lcd_clear();
		relojMenu = TRUE;
		alarmaFlag = FALSE;
		backAlarma = FALSE;
		MostrarA = TRUE;
		reentrar = TRUE;
		ticks = 0;
	}
	if(Alarma.dec == -1){
		lcd_puts_x3( 80, 40, BLACK, hora );
	}



}

void mapa(void){


	// pbs_init();
	//pbs_open(isr_pb);
	//lcd_clear();
	uint16 x = -1, y = -1;

	int i =2, j = 2;
	//lcd_putWallpaper(mapaM[j][i]);
	//lcd_putWallpaper(mapaM[2][2]); //el centro
	if(Map1time){
		Map1time = FALSE;
		lcd_putWallpaper(mapaM[j][i]);
	}

	if(pb_scan()!=PB_LEFT){
		//funcionesPrincipales(newBeat, newStep );


		if(ts_pressed()){
			ts_getpos( &x, &y);
			if( x>0 && y>0 && x<105  && y<80 && i>0 && j > 0 && i<=4 && j<=4){    //arriba izq
				i--;
				j--;

			}if(x>105 && y>0 && x<210  && y<80 && i>=0 && j >0 && i<=4 && j<=4){ //arriba
				j--;


			}if(x>210 && y>0 && x<320  && y<80 && i>=0 && j >0 && i<4 && j<=4){  //arriba dch
				i++;
				j--;

			}if(x>0 && y>80 && x<105  && y<160 && i>0 && j >=0 && i<=4&&j<=4){ //izquierda
				i--;


			}if(x>0 && y>160 && x<105  && y<240 && i>0 && j >=0 && i<=4 && j<4){  //abajo izq
				i--;
				j++;


			}if(x>105 && y>160 && x<210  && y<240 && i>=0 && j >=0 && i<=4&&j<4){ //abajo
				j++;


			}if(x>210 && y>160 && x<320  && y<240 && i>=0 && j >=0 && i<4&&j<4){ //abajo dcha
				i++;
				j++;

			}if(x>210 && y>80 && x<320  && y<160 && i>=0 && j >=0 && i<4&&j<=4){ //derecha
				i++;
			}
			lcd_putWallpaper(mapaM[j][i]);

		}



	}else{
		lcd_clear();
		MenuPrincipal=TRUE;
		Map = FALSE;
		Map1time = TRUE;

	}


}

void one_second_loop( void )
{
	uint32 i;

	for( i=591436; i; i-- );
}
void one_s_loop( void )
{
	uint32 i;

	for( i=13560; i; i-- );
}
void isr_pb( void )
{
	switch( pb_scan() )
	{
	case PB_FAILURE:
		EXTINTPND = BIT_LEFTPB | BIT_RIGHTPB;
		break;
	case PB_LEFT:
		EXTINTPND = BIT_LEFTPB;
		break;
	case PB_RIGHT:
		EXTINTPND = BIT_RIGHTPB;
		break;
	}
	flagPb = TRUE;
	I_ISPC = BIT_PB;
}
void alarmaSonido(void){

	uda1341ts_init();
	iis_init( IIS_DMA );

	uda1341ts_setvol( VOL_MED );
	iis_playWawFile( TEMON, TRUE );
	while(pb_scan()!=PB_LEFT)
	{


	}
	iis_pause();

}
void isr_tick( void )
{

	I_ISPC = BIT_TICK;
}

void compruebaAlarma(rtc_time_t rtc_time){
	rtc_gettime( &rtc_time );

	if(latidosPorMinuto>180 && !peligro){
		lcd_clear();
		lcd_draw_box(0, 0 , 320, 240, BLACK, 5);
		lcd_puts_x3(25, 50, BLACK, "REDUZCA");
		lcd_puts_x3(25, 100, BLACK, "EL");
		lcd_puts_x3(25, 150, BLACK, "PULSO");
		pasosFlag = FALSE;
		pulsoFlag = FALSE;
		peligro = TRUE;
		MenuPrincipal = FALSE;
	}
	if(latidosPorMinuto<180 && peligro){
		MenuPrincipal = TRUE;
		peligro= FALSE;

	}
	if( SuenaAlarma ||(rtc_time.hour == Alarma.hora && rtc_time.min == Alarma.min && rtc_time.sec== Alarma.seg  && Alarma.dec != -1 )){



		MenuPrincipal = FALSE;

		if(!SuenaAlarma){
			lcd_clear();


			lcd_draw_box(0, 0 , 320, 240, BLACK, 3);
			lcd_puts_x3(13, 50, BLACK, "DESPIERTA");
			lcd_puts_x2(6, 140, BLACK, "izq para detener");
			lcd_puts_x2(6, 170, BLACK, "drch posponer 2 min");
			SuenaAlarma = TRUE;
			uda1341ts_setvol( VOL_MED );
			iis_playWawFile( NOKIATUNE, TRUE );

			Map = FALSE;

			relojMenu = FALSE;
			alarmaFlag = FALSE;
			pasosFlag = FALSE;
			pulsoFlag = FALSE;

		}

		if(pb_scan()==PB_LEFT){
			Alarma.dec = -1;
			iis_pause();
			SuenaAlarma = FALSE;
			MenuPrincipal = TRUE;
			confPintar = FALSE;

		}
		if(pb_scan()==PB_RIGHT){
			Alarma.min += 2;
			iis_pause();
			SuenaAlarma = FALSE;
			MenuPrincipal = TRUE;
		}



	}


}
void compruebaCronometro(rtc_time_t rtc_time){
	int seg = rtc_time.sec;
	rtc_gettime( &rtc_time );
	if(rtc_time.sec != seg){
		tiempoCr.seg++;
		if (tiempoCr.seg > 60) {
			tiempoCr.seg = 0;
			tiempoCr.min++;


		}
	}


}
void compruebaTemporizador(rtc_time_t rtc_time){
	int seg = rtc_time.sec;
	rtc_gettime( &rtc_time );
	if(rtc_time.sec != seg){
		if (tiempoTemp.seg > 0) {
				tiempoTemp.seg--;
			} else {
				if (tiempoTemp.min > 0) {
					tiempoTemp.min--;
					tiempoTemp.seg = 59;
				} else {
					if (tiempoTemp.hora > 0) {
						tiempoTemp.hora--;
						tiempoTemp.min = 59;
						tiempoTemp.seg = 59;
					}
				}
			}
	}
	if(tiempoTemp.hora==0 && tiempoTemp.min == 0&&tiempoTemp.seg < 10){
		segs_putchar( tiempoTemp.seg );
	}
	if(TempTermina || (tiempoTemp.hora==0 && tiempoTemp.min == 0&&tiempoTemp.seg == 0 && iniTemp)){
		segs_off();
		if(!TempTermina){
			lcd_clear();


			lcd_draw_box(0, 0 , 320, 240, BLACK, 3);
			lcd_puts_x3(13, 50, BLACK, "TEMP");
			lcd_puts_x2(6, 140, BLACK, "FINALIZADO");
			TempTermina=TRUE;
			relojMenu = FALSE;
			alarmaFlag = FALSE;
			pasosFlag = FALSE;
			pulsoFlag = FALSE;
			iis_playWawFile( NOKIATUNE, TRUE );
		}

		if(pb_scan()==PB_LEFT){

			iis_pause();
			TempTermina = FALSE;
			MenuPrincipal = TRUE;
			iniTemp = FALSE;


		}

	}
}
