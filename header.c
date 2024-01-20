#define F_CPU 16000000UL // 16MHZ clock from the debug processor
#include <avr/io.h>
#include <util/delay.h>

#define LCD_DB PORTD       // 'Data Bus (7-14)' is connected to port D pins 0-7
#define LCD_EN PB2         // 'Enable' signal is connected to port B pin 2
#define LCD_RW PB1         // 'Read/Write' signal (Read(1) / Write (0)) is connected to port B pin 1
#define LCD_RS PB0         // 'Register Select Signal' (Data Input Control (1) / Instruction Input Control(0)) is connected to port B pin 0

/* Important LCD Commands:
General AVR LCD: http://www.avr-asm-tutorial.net/avr_en/apps/lcd/lcd.html
LCD Interface Sample #1: https://youtu.be/ZTpE78zKfV4?si=noJ0Zgg_nIHj9XP0
LCD Interface Sample #2: https://www.electronicwings.com/avr-atmega/lcd-custom-character-display-using-atmega-16-32-

0x01 - Clear Screen
0x02 - Reset
0x0A - Display OFF, cursor ON
0x0C - Display ON, cursor OFF
0x0E - Display ON, cursor blinking
0X0F - Display data on cursor blinking

0x05 - Display shifts to right
0x06 - Cursor increment
0x07 - Display shifts to left
0x08 - Cursor and display OFF
0x10 - Cursor position shifts to left
0x14 - Cursor position shifts to right
0x80 - Move cursor to the beginning of first line
0xC0 - Move cursor to the beginning of second line

0X30 - For Display in one line in 8-bit mode
*/

enum {
	ADC0_VOLTAGE_PIN,
	ADC1_TEMPERATURE_PIN, // Internal temperature sensor?
	};
	
int counter = 0;

void LCD_CMD (unsigned char cmd) // LCD Commands Setup
{
	LCD_DB = cmd;             // data lines are set to send command*
	PORTB &= ~(1 << LCD_RS);  // RS sets 0
	PORTB &= ~(1 << LCD_RW);  // RW sets 0
	PORTB |= (1 << LCD_EN);   // En sets 1
	_delay_ms(100);
	PORTB &= ~(1 << LCD_EN);  // En sets 0
	return;
}

void LCD_Init(void) // LCD Initialization
{
	LCD_CMD(0x38);          // initialization in 8-bit mode of 16X2 LCD
	_delay_ms(1);
	LCD_CMD(0x0C);          // display on, cursor off
	_delay_ms(1);
	LCD_CMD(0x01);          // Clear Screen
	_delay_ms(1);
	LCD_CMD(0x06);          // Auto Cursor Increments
	_delay_ms(1);
	LCD_CMD(0x80);          // Move cursor to the beginning of first line
	_delay_ms(1);
	return;
}

void LCD_Write_Char(unsigned char data) // LCD Write One Character
{
	LCD_DB = data;              // Single Character Data Bus Assignment
	PORTB |= (1 << LCD_RS);    // RS sets 1
	PORTB &= ~(1 << LCD_RW);   // RW sets 0
	PORTB |= (1 << LCD_EN);    // En sets 1
	_delay_ms(50);
	PORTB &= ~(1 << LCD_EN);   // En sets 0
	return ;
}

void LCD_Write_String(char *string) // LCD Write One String
{
	int i;
	for(i=0;string[i]!='\0';i++)
	{
		char string_element = string[i];
		LCD_Write_Char(string_element); // Multiple Characters Data Bus Assignment
	}
}
//-------------------------------------------------------------------------------------------------------
/**
void ADC_start()
{
	// ADMUX &= ~(1 << REFS0) & ~(1 << REFS1);	// External 5V reference voltage
	
	ADCSRA |= (1 << ADEN) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);	// Pre-scaler = 128
}

uint16_t adc_convert(uint8_t pin)
{
	ADMUX = pin;	// Set ADC pin
	
	ADCSRA |= (1 << ADSC);					// Start conversion
	
	while(ADCSRA & (1 << ADSC));
	
	return ADC;
}
**/
//-------------------------------------------------------------------------------------------------------
void adc_init(void){
	ADMUX &= ~(1 << REFS0 | 1 << REFS1); // AREF PIN Voltage Referencing
	ADCSRA |= (1 << ADEN | 1 << ADIE | 1 << ADPS0 | 1<< ADPS1 | 1 << ADPS2); // ADC ENABLE | ADC Interrupt Enable | ADC Pre-scaler = 128
}

void adc_pin_enable(uint8_t pin){
	DIDR0 |= 1 << pin;
}

void adc_pin_disable(uint8_t pin){
	DIDR0 &= ~(1 << pin);
}

void adc_pin_select(uint8_t pin){
	ADMUX &= ~(1 << MUX0 | 1 << MUX1 | 1 << MUX2 | 1 << MUX3); // Clear the 4 lower bits, leave the 4 higher bits alone.
	ADMUX |= pin;
}

// Logic for Battery Discharging
// 4.2V = 100% 
// 2.5 = 0%
uint8_t Battery_Discharge_Percentage(float voltage)
{
	float min_voltage = 2.5;
	float max_voltage = 4.2;
	
	if (voltage <= min_voltage)
	{
		voltage = min_voltage;
	}
	else if (voltage >= max_voltage)
	{
		voltage = max_voltage;
	}
	
	uint8_t percent = (((voltage - min_voltage) / (max_voltage - min_voltage)) * 100);
	
	return percent;
}
