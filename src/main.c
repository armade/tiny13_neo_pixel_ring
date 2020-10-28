/**
 * Rainbow on NEO PIXEL ring 12 LEDS WS2812.
	                           _____
               (RESET) PB5 1 -|  u  |- 8 VCC
    (PCINT3/CLKI/ADC3) PB3 2 -|     |- 7 PB2 (SCK/ADC1/T0/PCINT2)
         (PCINT4/ADC2) PB4 3 -|     |- 6 PB1 (MISO/AIN1/OC0B/INT0/PCINT1)
                       GND 4 -|_____|- 5 PB0 (MOSI/AIN0/OC0A/PCINT0)  -> WS2812 data
								
*/

#include <avr/io.h>
#include "light_ws2812.h"
#include <avr/sleep.h>

#define NR_OF_LEDS 12

uint8_t delay = 5;
register uint8_t s       asm("r5"); // lcg var
register uint8_t a       asm("r6"); // lcg var

struct pixel {
	uint8_t g;
	uint8_t r;
	uint8_t b;
}pixels[NR_OF_LEDS];


inline static uint8_t gamma_corr(uint8_t input)
{
	uint32_t accu;

	accu = (uint32_t)input*input*input + (1UL<<15); // 27 bit max
	accu>>=16;
	
	return accu;
}



static inline int Neo_Pixel_Ring_set(struct pixel *LED, uint8_t nr, uint16_t brightness) {
	if (nr < NR_OF_LEDS) {

		LED->r = ((uint16_t)gamma_corr(LED->r)*brightness)>>8;
		LED->g = ((uint16_t)gamma_corr(LED->g)*brightness)>>8;
		LED->b = ((uint16_t)gamma_corr(LED->b)*brightness)>>8;
	}
	return 0;
}

static inline char color_wheel(uint8_t pos, struct pixel *color)
{
    if (pos < 85) {
    	color->r = pos * 3;
    	color->g = 255 - pos * 3;
    	color->b = 0;
    } else if (pos < 170) {
        pos -= 85;
        color->r = 255 - pos * 3;
        color->g = 0;
        color->b = pos * 3;
    } else {
        pos -= 170;
        color->r  = 0;
        color->g = pos * 3;
        color->b = 255 - pos * 3;
    }
	
	delay = 35;
	return 0;
}

static uint8_t rand(void) {
	s ^= s << 3;
	s ^= s >> 5;
	s ^= a++ >> 2;
	return s;
}


static void RGB_fire_run(struct pixel *color1,struct pixel *color2)
{
	uint8_t brightness = (rand());
	
	if(brightness<64)
		brightness = 64;

	delay = rand()&127;

	if(delay < 5)
		delay = 5;
	
	*color1=*color2;
	Neo_Pixel_Ring_set(color1,0,brightness);
	
	return;
}

ISR(TIM0_COMPA_vect)
{
	// Clear irq flag
	TCNT0 = 0;
}

static inline void Init_wakeup_timer(void)
{
	// 9.6MHz/(256*37) = 1013HZ ~ 1ms
	//TCCR0B = (1 << CS02);   // timer prescaler == 256
	TCCR0A = (1 << WGM01);  // timer set to CTC mode
	TIMSK0 = (1 << OCIE0A); // enable CTC timer interrupt
	OCR0A = 37; 
	
	set_sleep_mode(SLEEP_MODE_IDLE);    // only disable cpu and flash clk while sleeping
}

int main(void)
{
	struct pixel p;
	static char cou = 0;
	static char index = 0;
	uint8_t i;
	uint8_t j;
	uint16_t bright;
	
	
	Init_wakeup_timer();
	sei();

	/* loop */
	while (1) {
		/////////////////////////////////////////////////////////////
		// R A I N B O W
		/////////////////////////////////////////////////////////////
		/*for (i = 0; i < NR_OF_LEDS; i++) {
			color_wheel(
				(((i * 256) / NR_OF_LEDS) + cou) & 0xff,
				&pixels[i]);
			Neo_Pixel_Ring_set(&pixels[i],0,255);
		}
		cou++;
		delay = 5;*/
		
		/////////////////////////////////////////////////////////////
		// S C A N N E R
		/////////////////////////////////////////////////////////////

		j = index;
		for (i = 0; i < (NR_OF_LEDS-1); i++) {
			color_wheel(cou,&pixels[j]);
			bright = i*42;
			if(bright>255)
				bright = 0;

			Neo_Pixel_Ring_set(&pixels[j],0,bright);

			if(++j>(NR_OF_LEDS-1))
				j=0;
		}
	
		if(++index>(NR_OF_LEDS-1))
			index=0;
		cou++;
		delay = 85;
		
		
		/////////////////////////////////////////////////////////////
		// F I R E
		/////////////////////////////////////////////////////////////
		//RGB_fire_run(&pixels[0],&p);
		
		
		
		/////////////////////////////////////////////////////////////
		// U P D A T E
		/////////////////////////////////////////////////////////////
		ws2812_setleds((struct cRGB *)pixels, NR_OF_LEDS);
		
		TCCR0B = (1 << CS02); // timer prescaler == 256
		while(delay--){
			sleep_mode();// sleep until timer interrupt is triggered
		}
		TCCR0B &= ~(1<<CS02); //Stop Timer0
		TCNT0 = 0;
	}
	
	return 0;
}