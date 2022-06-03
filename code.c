#define F_CPU 16000000UL //frequ�ncia de trabalho
#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <util/delay.h> 
#define set_bit(y,bit) (y|=(1<<bit))//coloca em 1 o bit x da vari�vel Y
#define clr_bit(y,bit) (y&=~(1<<bit))//coloca em 0 o bit x da vari�vel Y
#define cpl_bit(y,bit) (y^=(1<<bit))//troca o estado l�gico do bit x da vari�vel Y
#define tst_bit(y,bit) (y&(1<<bit))//retorna 0 ou 1 conforme leitura do bit
#define LED0 PD3
#define LED1 PD2
#define LED2 PB3
#define LED3 PC4
#define Bcima PB5
#define Bbaixo PB4
#define LCD_Port PORTD			//Define LCD Port (PORTA, PORTB, PORTC, PORTD)
#define LCD_DPin  DDRD			//Define 4-Bit Pins (PD4-PD7 at PORT D)
#define RSPIN PD0			//RS Pin
#define ENPIN PD1 			//E Pin
#define vcc_lcd PD2
int runtime;
ISR(PCINT0_vect);
ISR(PCINT1_vect);




void LCD_Init (void)
{
	LCD_DPin = 0xFF;		//Control LCD Pins (D4-D7)
	_delay_ms(15);		//Wait before LCD activation
	LCD_Action(0x02);	//4-Bit Control
	LCD_Action(0x28);       //Control Matrix @ 4-Bit
	LCD_Action(0x0c);       //Disable Cursor
	LCD_Action(0x06);       //Move Cursor
	LCD_Action(0x01);       //Clean LCD
	_delay_ms(2);
}

void LCD_Action( unsigned char cmnd )
{
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0);
	LCD_Port &= ~ (1<<RSPIN);
	LCD_Port |= (1<<ENPIN);
	_delay_us(1);
	LCD_Port &= ~ (1<<ENPIN);
	_delay_us(200);
	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);
	LCD_Port |= (1<<ENPIN);
	_delay_us(1);
	LCD_Port &= ~ (1<<ENPIN);
	_delay_ms(2);
}

void LCD_Clear()
{
	LCD_Action (0x01);		//Clear LCD
	_delay_ms(2);			//Wait to clean LCD
	LCD_Action (0x80);		//Move to Position Line 1, Position 1
}


void LCD_Print (char *str)
{
	int i;
	for(i=0; str[i]!=0; i++)
	{
		LCD_Port = (LCD_Port & 0x0F) | (str[i] & 0xF0);
		LCD_Port |= (1<<RSPIN);
		LCD_Port|= (1<<ENPIN);
		_delay_us(1);
		LCD_Port &= ~ (1<<ENPIN);
		_delay_us(200);
		LCD_Port = (LCD_Port & 0x0F) | (str[i] << 4);
		LCD_Port |= (1<<ENPIN);
		_delay_us(1);
		LCD_Port &= ~ (1<<ENPIN);
		_delay_ms(2);
	}
}
//Write on a specific location
void LCD_Printpos (char row, char pos, char *str)
{
	if (row == 0 && pos<16)
	LCD_Action((pos & 0x0F)|0x80);
	else if (row == 1 && pos<16)
	LCD_Action((pos & 0x0F)|0xC0);
	LCD_Print(str);
}

void inprimeVel(int v){
	if (v==1){
		LCD_Clear();
		LCD_Print("set Curtain vel:");
		LCD_Action(0xC0);
		LCD_Print("baixa");
		}else if (v==2){
		LCD_Clear();
		LCD_Print("set Curtain vel:");
		LCD_Action(0xC0);
		LCD_Print("media");
		}else if(v==3){
		LCD_Clear();
		LCD_Print("set Curtain vel:");
		LCD_Action(0xC0);
		LCD_Print("alta");
	}
	
}

int confTenperatura(){
	int i=18;
	char temperatura[16];
	
	LCD_Clear();
	LCD_Print("set AR Temp:");
	while(1){
		itoa(i, temperatura, 10);
		LCD_Action(0xC0);
		LCD_Print(temperatura);
		if(!tst_bit(PINB,Bcima)){
			i++;
			if (i>27){
				i=18;
			}
			_delay_ms(300);
		}
		if(!tst_bit(PINB,Bbaixo)){
			i--;
			if (i<18){
				i=27;
			}
			_delay_ms(300);
		}
		if (!tst_bit(PINC,PB3)){
			break;
		}
	}
	
	return i;
}

void confVelocidade(){
	
	int v=1;
	LCD_Clear();
	LCD_Print("set Curtain vel:");
	LCD_Action(0xC0);
	_delay_ms(300);
	while(1){
		if(!tst_bit(PINB,Bcima)){
			v++;
			if (v>3){
				v=1;
			}
			inprimeVel(v);
			_delay_ms(300);
		}
		if (!tst_bit(PINB,Bbaixo)){
			v--;
			if (v<1){
				v=3;
			}
			inprimeVel(v);
			_delay_ms(300);
		}
		if (!tst_bit(PINC,PB3)){
			break;
		}
	}
	
}

int config(){
	int tempDesejada;

	while(1){
			LCD_Clear();
			LCD_Print("6.config AR");
			LCD_Action(0xC0);
			LCD_Print("7.config cortina");
			_delay_ms(600);
		if(!tst_bit(PINB,Bcima)){
			tempDesejada = confTenperatura();
			_delay_ms(350);
		}
		if(!tst_bit(PINB,Bbaixo)){
			confVelocidade();
			_delay_ms(350);
		}
		if (!tst_bit(PINC,PB3)){
			break;
		}
	}
	return tempDesejada;
}

int main(){

	DDRD = 0xff;
	PORTD = 0xff;
	PORTD = 0x00;
	DDRB = 0b00001000;
	PORTB = 0b00110111;
	DDRC = 0b00010111;
	PORTC = 0b11101111;
	
	
	
	//UCSR0B = 0x00; //necessario desabilitar RX e TX para trabalhar com os pinos da PORTD no Arduino
	//EICRA = 1<<ISC01;
	//EICRA = 1<<ISC11;
	//EIMSK = (1<<INT1) | (1<<INT0);
	_delay_ms(100);
	LCD_Init(); //Activate LCD
	LCD_Clear();
	LCD_Print("UNIFACS");	//Begin writing at Line 1, Position 1
	LCD_Action(0xC0);
	LCD_Print("Micro 2022");
	_delay_ms(1000);
	
	PCICR = (1<<PCIE1)|(1<<PCIE0);//habilita interrup��o por qualquer mudan�a de sinal no PORTB e PORTC
	PCMSK0 = (1<<PCINT2)|(1<<PCINT1)|(1<<PCINT0);//habilita os pinos PCINT8:9 para gerar interrup��o
	PCMSK1 = (1<<PCINT13);//habilita os pinos PCINT13 para gerar interrup��o
	sei();//habilita as interrup��es
	
	
	LCD_Clear();
	LCD_Print("UNIFACS");
	LCD_Action(0xC0);
	LCD_Print("Micro 2022");
	_delay_ms(100);
	int tempDesejada;
	
	while(1){
		LCD_Clear();
		LCD_Print("Bem Vindos!!");
		LCD_Action(0xC0);
		LCD_Print("1.AR 2.TV 3.Lamp A");
		_delay_ms(3000);
		if(!tst_bit(PINC,PB3)){
			tempDesejada = config();
		}
		LCD_Clear();
		LCD_Print("Bem Vindos!!");
		LCD_Action(0xC0);
		LCD_Print("4.LampB 5.Set");
		_delay_ms(3000);
		if(!tst_bit(PINC,PB3)){
			tempDesejada = config();
		}
		
	}

}
int flagAr=0,flagTV=0,flagLa=0,flagLb=0 ;

ISR(PCINT0_vect)
{

	if(!tst_bit(PINB,PB0)){
		cpl_bit(PORTD,PD3);
		LCD_Clear();
		if (flagLa == 0){
			LCD_Print("Lamp A Ligada");
			flagLa=1;
		} 
		else{
			LCD_Print("Lamp A desligada");
			flagLa=0;
		}
		
		_delay_ms(300);
	}
	else if(!tst_bit(PINB,PB1)){
		cpl_bit(PORTD,PD2);
		LCD_Clear();
		if (flagAr == 0){
			LCD_Print("Ar Ligado");
			flagAr=1;
		} 
		else{
			LCD_Print("Ar Desligado");
			flagAr=0;
		}
		
		_delay_ms(300);
	}
	else if(!tst_bit(PINB,PB2)){
		cpl_bit(PORTB,LED2);
		LCD_Clear();
			if (flagLb == 0){
				LCD_Print("Lamp B ligada");
				flagLb=1;
			}
			else{
				LCD_Print("Lamp B Desligada");
				flagLb=0;
			}
		_delay_ms(300);
	}
}
ISR(PCINT1_vect){

	if(!tst_bit(PINC,PC5)){
		cpl_bit(PORTC,LED3);
		LCD_Clear();
			if (flagTV == 0){
				LCD_Print("TV ligada");
				flagTV=1;
			}
			else{
				LCD_Print("TV Desligada");
				flagTV=0;
			}
		
		_delay_ms(300);
	}
	
}
