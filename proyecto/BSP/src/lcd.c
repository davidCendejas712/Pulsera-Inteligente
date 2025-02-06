
#include <s3c44b0x.h>
#include <lcd.h>

extern uint8 font[];
uint8 lcd_buffer[LCD_BUFFER_SIZE];

static uint8 state;

void lcd_init( void )
{
	LCDCON1 = 0x1C020;
	LCDCON2 = 0x13CEF ;
	LCDSADDR3 = 0x50;
	LCDCON3 = 0x0;
	REDLUT = 0x0;
	GREENLUT = 0x0;
	BLUELUT = 0x0;
	DITHMODE = 0x12210;
	DP1_2 = 0xA5A5;
	DP4_7 = 0xBA5DA65;
	DP3_5 = 0xA5A5F;
	DP2_3 = 0xD6B;
	DP5_7 = 0xEB7B5ED;
	DP3_4 = 0x7DBE;
	DP4_5 = 0x7EBDF;
	DP6_7 = 0x7FDFBFE;

    LCDSADDR1 = (2 << 27) | ((uint32)lcd_buffer >> 1);
    LCDSADDR2 = (1 << 29) | (((uint32)lcd_buffer + LCD_BUFFER_SIZE) & 0x3FFFFF) >> 1;
    
    lcd_off();
}

void lcd_on( void )
{
	LCDCON1 |= (1<<0);
}

void lcd_off( void )
{
	LCDCON1 &= ~(1<<0);
}

uint8 lcd_status( void )
{
	return (LCDCON1 & 0x00000001);
}

void lcd_clear( void )
{
	uint16 x, ySrc;
	for( ySrc=0; ySrc<LCD_HEIGHT; ySrc++)
	{
		for( x=0; x<LCD_WIDTH/2; x++ )
			lcd_buffer[ySrc*320/2 + x] = WHITE;
	}
}
void lcd_clear_area(uint16 x1, uint16 y1, uint16 x2, uint16 y2)
{
	uint16 x, ySrc;
	for( ySrc=y1; ySrc<y2; ySrc++)
	{
		for( x=x1; x<x2; x++ )
			lcd_buffer[ySrc*320/2  + x] = WHITE;
	}
}


void lcd_putpixel( uint16 x, uint16 y, uint8 c)
{
    uint8 byte, bit;
    uint16 i;

    i = x/2 + y*(LCD_WIDTH/2);
    bit = (1-x%2)*4;
    
    byte = lcd_buffer[i];
    byte &= ~(0xF << bit);
    byte |= c << bit;
    lcd_buffer[i] = byte;
}

uint8 lcd_getpixel( uint16 x, uint16 y )
{
	return lcd_buffer[y*320/2 + x/2];
}

void lcd_draw_hrow( uint16 xleft, uint16 xright, uint16 y, uint8 color, uint16 width )
{

	uint16 x, ySrc;
	for( ySrc=0; ySrc<width; ySrc++)
	{
		for( x=xleft; x<xright; x++ )
			lcd_putpixel(x, y + ySrc, color);
	}
}

void lcd_draw_vrow( uint16 yup, uint16 ydown, uint16 x, uint8 color, uint16 width )
{
	uint16 s, xSrc;
		for( s=yup; s<ydown; s++ )
		{
			for( xSrc=0; xSrc<width; xSrc++)
				lcd_putpixel(x + xSrc,s,color);
		}
}

void lcd_draw_box( uint16 xleft, uint16 yup, uint16 xright, uint16 ydown, uint8 color, uint16 width )
{
	lcd_draw_hrow(xleft, xright, yup, color, width);
	lcd_draw_vrow(yup, ydown, xleft, color, width);
	lcd_draw_hrow(xleft, xright, ydown - width, color, width);
	lcd_draw_vrow(yup, ydown, xright - width, color, width);

}

void lcd_putchar( uint16 x, uint16 y, uint8 color, char ch )
{
    uint8 row, col;
    uint8 *bitmap;

    bitmap = font + ch*16;
    for( row=0; row<16; row++ )
        for( col=0; col<8; col++ )                    
            if( bitmap[row] & (0x80 >> col) )
                lcd_putpixel( x+col, y+row, color );
            else
                lcd_putpixel( x+col, y+row, WHITE );
}

void lcd_puts( uint16 x, uint16 y, uint8 color, char *s )
{
	uint8 i = 0;
	uint8 j = 0;
	while(*s != '\0'){
		lcd_putchar(x + (i * 8), y + (j * 16), color, *s++);
		i++;
		if(x + (i * 8) == LCD_WIDTH){
			i = 0;
			j++;
		}

	}
}

void lcd_putint( uint16 x, uint16 y, uint8 color, int32 i )
{
	boolean negativo = 0;
	char buf[256 + 1];
	char *p = buf + 256;
	uint8 c;

	*p = '\0';
	if(i<0){
		negativo = 1;
		i*=-1;
	}
	do {
	   c = i % 10;
	   *--p = '0' + c;

	   i = i / 10;
	} while( i );

	if(negativo){
		*--p = '-';
	}
	lcd_puts(x, y, color, p );
}

void lcd_puthex( uint16 x, uint16 y, uint8 color, uint32 i )
{
    char buf[8 + 1];
    char *p = buf + 8;
    uint8 c;

    *p = '\0';

    do {
        c = i & 0xf;
        if( c < 10 )
            *--p = '0' + c;
        else
            *--p = 'A' + c - 10;
        i = i >> 4;
    } while( i );

	lcd_puts(x, y, color, p );
}


void lcd_putchar_x2( uint16 x, uint16 y, uint8 color, char ch )
{
    uint8 row, col;
	uint8 *bitmap;

	bitmap = font + ch*16;
	for( row=0; row<32; row++ ){
		for( col=0; col<16; col++ ){
			if( bitmap[row/2] & (0x80 >> col/2) )
				lcd_putpixel( x+col, y+row, color );
			else
				lcd_putpixel( x+col, y+row, WHITE );
		}
	}
}
void lcd_putchar_x3( uint16 x, uint16 y, uint8 color, char ch )
{
    uint8 row, col;
	uint8 *bitmap;

	bitmap = font + ch*16;
	for( row=0; row<64; row++ ){
		for( col=0; col<32; col++ ){
			if( bitmap[row/4] & (0x80 >> col/4) )
				lcd_putpixel( x+col, y+row, color );
			else
				lcd_putpixel( x+col, y+row, WHITE );
		}
	}
}

void lcd_puts_x2( uint16 x, uint16 y, uint8 color, char *s )
{
	uint8 i = 0;
	uint8 j = 0;
	while(*s != '\0'){
		lcd_putchar_x2(x + (i * 16), y + (j * 32), color, *s++);
		i++;
		if(x + (i * 16) == LCD_WIDTH){
			i = 0;
			j++;
		}

	}
}

void lcd_puts_x3( uint16 x, uint16 y, uint8 color, char *s )
{
	uint8 i = 0;
	uint8 j = 0;
	while(*s != '\0'){
		lcd_putchar_x3(x + (i * 32), y + (j * 64), color, *s++);
		i++;
		if(x + (i * 32) == LCD_WIDTH){
			i = 0;
			j++;
		}

	}
}

void lcd_putint_x2( uint16 x, uint16 y, uint8 color, int32 i )
{
	boolean negativo = 0;
	char buf[256 + 1];
	char *p = buf + 256;
	uint8 c;

	*p = '\0';
	if(i<0){
		negativo = 1;
		i*=-1;
	}
	do {
	   c = i % 10;
	   *--p = '0' + c;

	   i = i / 10;
	} while( i );

	if(negativo){
		*--p = '-';
	}
	lcd_puts_x2(x, y, color, p );
}
void lcd_putint_x3( uint16 x, uint16 y, uint8 color, int32 i )
{
	boolean negativo = 0;
	char buf[256 + 1];
	char *p = buf + 256;
	uint8 c;

	*p = '\0';
	if(i<0){
		negativo = 1;
		i*=-1;
	}
	do {
	   c = i % 10;
	   *--p = '0' + c;

	   i = i / 10;
	} while( i );

	if(negativo){
		*--p = '-';
	}
	lcd_puts_x3(x, y, color, p );
}

void lcd_puthex_x2( uint16 x, uint16 y, uint8 color, uint32 i )
{
    char buf[8 + 1];
    char *p = buf + 8;
    uint8 c;

    *p = '\0';

    do {
        c = i & 0xf;
        if( c < 10 )
            *--p = '0' + c;
        else
            *--p = 'A' + c - 10;
        i = i >> 4;
    } while( i );

	lcd_puts_x2(x, y, color, p );
}

void lcd_putWallpaper( uint8 *bmp )
{
    uint32 headerSize;

    uint16 x, ySrc, yDst;
    uint16 offsetSrc, offsetDst;

    headerSize = bmp[10] + (bmp[11] << 8) + (bmp[12] << 16) + (bmp[13] << 24);

    bmp = bmp + headerSize;
    
    for( ySrc=0, yDst=LCD_HEIGHT-1; ySrc<LCD_HEIGHT; ySrc++, yDst-- )                                                                       
    {
        offsetDst = yDst*LCD_WIDTH/2;
        offsetSrc = ySrc*LCD_WIDTH/2;
        for( x=0; x<LCD_WIDTH/2; x++ )
            lcd_buffer[offsetDst+x] = ~bmp[offsetSrc+x];
    }

}
