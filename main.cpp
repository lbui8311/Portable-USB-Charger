#define F_CPU 16000000UL // 16MHZ clock from the debug processor
#include <avr/io.h>
#include <math.h>
#include <util/delay.h>
#include <stdlib.h> // itoa
#include <avr/interrupt.h>
#include <string.h>	//strcat
#include "header.c"
#define ADC_voltage(x) (x * (5.161/1024.0))

void LCD_CMD(unsigned char cmd);

void LCD_Init(void);

void LCD_Write_Char(unsigned char data);

void LCD_Write_String(char *string);

void adc_init(void);

void adc_pin_enable(uint8_t pin);

void adc_pin_disable(uint8_t pin);

void adc_pin_select(uint8_t pin);

uint8_t Battery_Discharge_Percentage(float voltage);

uint8_t Button_Pressed(uint8_t counter);

volatile static uint8_t adc_done_flag = 1;

ISR(ADC_vect){
	adc_done_flag = 1;
}

uint16_t adc_convert(void){
	uint8_t adcl = 0;
	uint8_t adch = 0;
	
	adc_done_flag = 0;
	
	ADCSRA |= 1 << ADSC; // Start the conversion
	
	while(adc_done_flag == 0); // Wait until conversion is done
	
	adcl = ADCL;
	adch = ADCH;
	
	return (adch << 8 | adcl); // returning 16 bit data
}

int main(void)
{
	int percent = 0;
	char buffer_voltage[20];
	char buffer_temperature[20];
	char buffer_percent[20];
	char status;
	
	//--------Used to acquire and calculate voltage----------------
	float adc_volt = 0.0;
	float volts = 0.0;
	float temp_volts = 0.0;
	
	//--------Used to acquire and calculate temperature------------
	float adc_temperature = 0.0;
	float temperature_voltage = 0.0;
	float resistance = 0.0;
	float temperature = 0.0;
	
	DDRD |= (1 << DDD0 | 1 << DDD1 | 1 << DDD2 | 1 << DDD3 | 1 << DDD4 | 1 << DDD5 | 1 << DDD6 | 1 << DDD7); // Setting LCD data ports (7-14) as output
	DDRB |= (1 << DDB0 | 1 << DDB1 | 1 << DDB2); // Setting LCD control signals (RS, RW, E) as output

	DDRC &= ~(1 << DDC2 | 1 << DDC5); // Set physical push button & BQ Stat as inputs
	DDRC |= (1 << DDC3 | 1 << DDC4); // Set Port C3 as output to internal switch #1
	PORTB |= (1 << PORTB3); // Button input
	PORTC &= ~(1 << PORTC5); // BQ stat input
	PORTC |= (1 << PORTC3 | 1 << PORTC4); // Internal switches control: high => off => open circuit
	
	adc_init();
	sei();
	
	while(1)
	{
		PORTC &= ~(1 << PORTC4); // Internal USB switch control: low => on => closed circuit
		if(!(PINB & (1 << PINB3))) { // Button pressed (0)
			PORTC &= ~(1 << PORTC3); //Internal switch #1 control: low => on => closed circuit

			// First screen
			LCD_Init();             // initialize LCD
			LCD_Write_String("SDSU ECE-492 T#4");
			_delay_ms(10);
			LCD_CMD(0xC0);          // move cursor to the start of 2nd line
			_delay_ms(10);
			LCD_Write_String("Portable Charger");
			
			_delay_ms(1000);
			
			LCD_CMD(0x01);				// Clear screen

			adc_pin_enable(ADC0_VOLTAGE_PIN);
			adc_pin_select(ADC0_VOLTAGE_PIN);
			
			adc_volt = adc_convert();
			volts = ADC_voltage(adc_volt);
			percent = Battery_Discharge_Percentage(volts);		// Calculate Battery Percentage
			adc_pin_disable(ADC0_VOLTAGE_PIN);


			dtostrf(volts, 4, 2, buffer_voltage);
			dtostrf(percent, 4, 0, buffer_percent);
			
			// SECOND SCREEN
			LCD_CMD(0x80);				// move cursor to the start of 1st line
			LCD_Write_String("BATTERY:");
			if((PINC & (1 << PINC5))) {	// If BQ Status Pin High
				LCD_Write_String(" ...");
				LCD_CMD(0xC0);
				_delay_ms(10);
				LCD_Write_String("STATUS: ");
			}
			else {
				LCD_Write_String(buffer_percent);
				LCD_Write_String(" %");
				LCD_CMD(0xC0);				// move cursor to the start of 2nd line
				_delay_ms(10);
				LCD_Write_String("STATUS: ");
			}

			if((PINC & (1 << PINC5))) {	// If BQ Status Pin High
				if (percent >= 100) {
					LCD_Write_String("FULL");
				}
				else {
					LCD_Write_String("CHARGING");
				}
			}
			else {
				if (percent <= 10) {
					LCD_Write_String("RE-CHARGE");
				}
				else {
					LCD_Write_String("READY");
				}
			}
			_delay_ms(1000);

			LCD_CMD(0x01);				// Clear screen

			adc_pin_enable(PINC2);
			adc_pin_select(PINC2);
			
			adc_temperature = adc_convert();
			temperature_voltage = ADC_voltage(adc_temperature);
			temperature_voltage = (((temperature_voltage) + .010) * 1000) - 55;
			adc_pin_disable(PINC2);
			
			// resistance = (temperature_voltage * 8660) / (5 - temperature_voltage); // Calculate thermistor resistance, Reference resistor = 1kOhm

			//temperature = (3.35 * 0.01) + ((2.57 * 0.001) * log(resistance)) + ((2.62 * 0.0000001) * log(resistance)* log(resistance)* log(resistance));
			//temperature = (1 / temperature) * (9 / 5) + 32;	// Temperature in F
			
			// temp_volts = ADC_voltage(temperature_voltage) - 0.058;
			
			//dtostrf(temperature, 2, 0, buffer_temperature);
			dtostrf((temperature_voltage), 4, 2, buffer_temperature);
			
			LCD_CMD(0x80);				// move cursor to the start of 1st line
			LCD_Write_String("VOLTAGE: ");
			LCD_Write_String(buffer_voltage);
			LCD_Write_String(" V");
			_delay_ms(10);
			LCD_CMD(0xC0);				// move cursor to the start of 2nd line
			LCD_Write_String("TEMP: ");
			LCD_Write_String(buffer_temperature);
			LCD_Write_String(" F");
			_delay_ms(1000);
			LCD_CMD(0x01);				// Clear screen
			PORTC |= (1 << PORTC3); // Internal switch #1 control: high => off => open circuit

			} else {
			// do nothing
		}
	}
	return 0;
}
