#define F_CPU 16000000UL //frequência de trabalho
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#define set_bit(y,bit) (y|=(1<<bit))//coloca em 1 o bit x da variável Y
#define clr_bit(y,bit) (y&=~(1<<bit))//coloca em 0 o bit x da variável Y
#define cpl_bit(y,bit) (y^=(1<<bit))//troca o estado lógico do bit x da variável Y
#define tst_bit(y,bit) (y&(1<<bit))//retorna 0 ou 1 conforme leitura do bit
#define ledA PC1
#define AR PD2
#define ledB PB3
#define TV PC4
#define Bcima PB5
#define Bbaixo PB4
#define LCD_Port PORTD			//Define LCD Port (PORTA, PORTB, PORTC, PORTD)
#define LCD_DPin  DDRD			//Define 4-Bit Pins (PD4-PD7 at PORT D)
#define RSPIN PD0			//RS Pin
#define ENPIN PD1 			//E Pin
#define vcc_lcd PD2
int flagChaveGeral=0;
int tela=0;
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

int confTenperatura(){
	int t=10;
	char temperatura[16];
	tela=2;
	LCD_Clear();
	LCD_Print("set AR Temp:");
	_delay_ms(250);
	while(1){
		itoa(t, temperatura, 10);
		LCD_Action(0xC0);
		LCD_Print(temperatura);
		if(!tst_bit(PINB,Bcima)){
			t++;
			if (t>32){
				t=10;
			}
			_delay_ms(300);
		}
		if(!tst_bit(PINB,Bbaixo)){
			t--;
			if (t<10){
				t=32;
			}
			_delay_ms(300);
		}
		if (!tst_bit(PINC,PB3) || flagChaveGeral==1){
			break;
		}
	}
	
	return t;
}

int confVelocidade(){
	
	int v=0;
	tela=3;
	char velocidade[16];
	LCD_Clear();
	LCD_Print("set cortina vel:");
	_delay_ms(250);
	while(1){
		itoa(v, velocidade, 10);
		LCD_Action(0xC0);
		LCD_Print(velocidade);
		if(!tst_bit(PINB,Bcima)){
			v++;
			if (v>255){
				v=0;
			}
			LCD_Print("                ");
			_delay_ms(200);
		}
		if (!tst_bit(PINB,Bbaixo)){
			v--;
			if (v<0){
				v=255;
			}
			LCD_Print("                ");
			_delay_ms(200);
		}
		if (!tst_bit(PINC,PB3) || flagChaveGeral==1){
			break;
		}
	}
	return v;
	
}

void imprimeMenuConfig(){
	LCD_Clear();
	LCD_Print("6.config AR");
	LCD_Action(0xC0);
	LCD_Print("7.config cortina");
}

int config(){
	int tempDesejada;
	imprimeMenuConfig();
	tela=1;
	_delay_ms(500);
	
	while(1){
		if(!tst_bit(PINB,Bcima)){
			tempDesejada = confTenperatura();
			imprimeMenuConfig();
			_delay_ms(350);
		}
		if(!tst_bit(PINB,Bbaixo)){
			OCR2B = confVelocidade();
			if(!OCR2B){
				clr_bit(TCCR2A,COM2B1);
			}else{
				set_bit(TCCR2A,COM2B1);
			}
			imprimeMenuConfig();
			_delay_ms(350);
		}
		if (!tst_bit(PINC,PB3) || flagChaveGeral==1){
			break;
		}
	}
	return tempDesejada;
}

void imprimeMenu(){	
	LCD_Clear();
	LCD_Print("1.LampB 2.LampA");
	LCD_Action(0xC0);
	LCD_Print("3.AR 4.TV 5.Set");
}
//ADC biblioteca
void adcBegin(uint8_t ref, uint8_t did)
{ ADCSRA = 0;   //configuração inicial
	ADCSRB = 0;   //configuração inicial
	DIDR0  = did; //configura DIDR0
	if (ref == 0)
	{	 ADMUX &= ~((1<<REFS1) | (1<<REFS0));//Aref
	}
	if ((ref == 1) || (ref > 2))
	{ ADMUX &= ~(1<<REFS1); //Avcc
		ADMUX |=  (1<<REFS0); //Avcc
	}
	if (ref == 2)
	{ ADMUX |= (1<<REFS1) | (1<<REFS0);  //Tensão interna de ref (1.1V)
	}
	ADMUX &= ~(1<<ADLAR); //Alinhamento a direita
	ADCSRA |= (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);//habilita AD. Prescaler de 128 (clk_AD = F_CPU/128)
}

//---------------------------------------------------------------------------
//Seleciona canal do ADC
//0 <= channel <= 5 - Leitura dos pinos AD0 a AD5
//channel = 6 - Leitura do sensor de temperatura
//channel = 7 - 1,1V
//channel > 7 - GND
//---------------------------------------------------------------------------
void adcChannel(uint8_t channel)
{ if (channel <= 5)//seleciona um canal no multiplex
	ADMUX = (ADMUX & 0xF0) | channel;
	if (channel == 6)//seleciona sensor interno de temperatura
	ADMUX = (ADMUX & 0xF0) | 0x08;
	if (channel == 7)//seleciona 1,1 V
	ADMUX = (ADMUX & 0xF0) | 0x0E;
	if (channel > 7)//seleciona GND
	ADMUX = (ADMUX & 0xF0) | 0x0F;
}

//---------------------------------------------------------------------------
//Habilita ou desabilita interrupção do ADC
//Se x = 0, desabilita interrupção
//Caso contrário, habilita interrupção
//---------------------------------------------------------------------------
void adcIntEn(uint8_t x)
{ if (x)
	ADCSRA |= (1<<ADIE);//habilita interrupção do ADC
	else
	ADCSRA &= ~(1<<ADIE);//Desabilita interrupção do ADC
}

//---------------------------------------------------------------------------
//Inicia conversão
//---------------------------------------------------------------------------
void adcSample(void)
{ ADCSRA |= (1<<ADSC);//Inicia conversão
}

//---------------------------------------------------------------------------
//Verifica se conversão foi concluída
//Retorna valor 0 se conversão concluída. 64 se não.
//---------------------------------------------------------------------------
uint8_t adcOk(void)
{ return (ADCSRA & (1<<ADSC));
}

//---------------------------------------------------------------------------
//Ler o ADC e retorna o valor lido do ADC
//---------------------------------------------------------------------------
uint16_t adcReadOnly()
{ return (ADCL | (ADCH<<8));//retorna o valor do ADC
}

//---------------------------------------------------------------------------
//Converte, aguarda, ler e rotorna valor lido do ADC
//---------------------------------------------------------------------------
uint16_t adcRead()
{ adcSample();         //Inicia conversão
	while(adcOk());      //Aguarda fim da conversão (ADSC = 0)
	return adcReadOnly();//retorna o valor do ADC
}
//UART biblioteca
void uartBegin(uint32_t baud, uint32_t freq_cpu)
{ uint16_t myubrr = freq_cpu/16/baud-1;//calcula valor do registrador UBRR
	UBRR0H = (uint8_t)(myubrr>>8);	//ajusta a taxa de transmissão
	UBRR0L = (uint8_t)myubrr;
	UCSR0A = 0;//desabilitar velocidade dupla (no Arduino é habilitado por padrão)
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);//Habilita a transmissão e recepção. Sem interrupcao |(1<<RXCIE0)|(1<<TXCIE0)
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);//modo assíncrono, 8 bits de dados, 1 bit de parada, sem paridade
}

//---------------------------------------------------------------------------
//verifica se novo dado pode ser enviado pela UART
//---------------------------------------------------------------------------
uint8_t uartTxOk (void)
{ return (UCSR0A & (1<<UDRE0));//retorna valor 32 se novo dado pode ser enviado. Zero se não.
}

//---------------------------------------------------------------------------
//Envia um byte pela porta uart
//---------------------------------------------------------------------------
void uartTx(uint8_t dado)
{ UDR0 = dado;//envia dado
}

//---------------------------------------------------------------------------
//verifica se UART possui novo dado
//retorna valor 128 se existir novo dado recebido. Zero se não.
//---------------------------------------------------------------------------
uint8_t uartRxOk (void)
{ return (UCSR0A & (1<<RXC0));
}

//---------------------------------------------------------------------------
//Ler byte recebido na porta uart
//---------------------------------------------------------------------------
uint8_t uartRx()
{ return UDR0; //retorna o dado recebido
}

//---------------------------------------------------------------------------
//Habilita ou desabilita interrupção de recepção da usart
//x = 0, desabilita interrupção. x = 1, habilita interrupção
//---------------------------------------------------------------------------
void uartIntRx(uint8_t x)
{ if (x)
	UCSR0B |= (1<<RXCIE0);//Habilita interrupção de recepção de dados
	else
	UCSR0B &= ~(1<<RXCIE0);//Desabilita interrupção de recepção de dados
}

//---------------------------------------------------------------------------
//Habilita ou desabilita interrupção de Trasnmissão da usart
//x = 0, desabilita interrupção. x = 1, habilita interrupção
//---------------------------------------------------------------------------
void uartIntTx(uint8_t x)
{ if (x)
	UCSR0B |= (1<<TXCIE0);//Habilita interrupção de recepção de dados
	else
	UCSR0B &= ~(1<<TXCIE0);//Desabilita interrupção de recepção de dados
}

//---------------------------------------------------------------------------
//Envia uma string pela porta uart. Ultimo valor da string deve ser 0.
//---------------------------------------------------------------------------
void uartString(char *c)
{ for (; *c!=0; c++)
	{ while (!uartTxOk());	//aguarda último dado ser enviado
		uartTx(*c);
	}
}

//---------------------------------------------------------------------------
//Envia pela uart variavel de 1 byte (8 bits) com digitos em decimal
//---------------------------------------------------------------------------
void uartDec1B(uint8_t valor)
{ int8_t disp;
	char digitos[3];
	int8_t conta = 0;
	do //converte o valor armazenando os algarismos no vetor digitos
	{ disp = (valor%10) + 48;//armazena o resto da divisao por 10 e soma com 48
		valor /= 10;
		digitos[conta]=disp;
		conta++;
	} while (valor!=0);
	for (disp=conta-1; disp>=0; disp-- )//envia valores do vetor digitos
	{ while (!uartTxOk());  //aguarda último dado ser enviado
		uartTx(digitos[disp]);//envia algarismo
	}
}

//---------------------------------------------------------------------------
//Envia pela uart variavel de 2 bytes (16 bits) com digitos em decimal
//---------------------------------------------------------------------------
void uartDec2B(uint16_t valor)
{ int8_t disp;
	char digitos[5];
	int8_t conta = 0;
	do //converte o valor armazenando os algarismos no vetor digitos
	{ disp = (valor%10) + 48;//armazena o resto da divisao por 10 e soma com 48
		valor /= 10;
		digitos[conta]=disp;
		conta++;
	} while (valor!=0);
	for (disp=conta-1; disp>=0; disp-- )//envia valores do vetor digitos
	{ while (!uartTxOk());	 //aguarda último dado ser enviado
		uartTx(digitos[disp]);//envia algarismo
	}
}

//---------------------------------------------------------------------------
//Envia pela uart variavel de 4 bytes (32 bits) com digitos em decimal
//---------------------------------------------------------------------------
void uartDec4B(uint32_t valor)
{ int8_t disp;
	char digitos[10];
	int8_t conta = 0;
	do //converte o valor armazenando os algarismos no vetor digitos
	{ disp = (valor%10) + 48;//armazena o resto da divisao por 10 e soma com 48
		valor /= 10;
		digitos[conta]=disp;
		conta++;
	} while (valor!=0);
	for (disp=conta-1; disp>=0; disp-- )//envia valores do vetor digitos
	{ while (!uartTxOk());	 //aguarda último dado ser enviado
		uartTx(digitos[disp]);//envia algarismo
	}
}

//---------------------------------------------------------------------------
//Envia pela uart variavel de 1 byte (8 bits) com digitos em hexadecimal
//---------------------------------------------------------------------------
void uartHex1B(uint8_t valor)
{ uint8_t disp;
	disp = (valor%16) + 48;    //armazena o resto da divisao por 16 e soma com 48
	if (disp > 57) disp += 7;  //soma 7 se algarismo for uma letra devido tabela ascii
	valor = (valor / 16) + 48; //armazena o resto da divisao por 16 e soma com 48
	if (valor > 57) valor += 7;//soma 7 se algarismo for uma letra devido tabela ascii
	while (!uartTxOk());	     //aguarda último dado ser enviado
	uartTx(valor);             //envia digito mais significativo
	while (!uartTxOk());	     //aguarda último dado ser enviado
	uartTx(disp);             //envia digito menos significativo
}

//---------------------------------------------------------------------------
//Envia pela uart variavel de 2 bytes (16 bits) com digitos em hexadecimal
//---------------------------------------------------------------------------
void uartHex2B(uint16_t valor)
{ uint8_t disp0;
	uint8_t disp1;
	disp0 = (uint8_t) (valor & 0x00FF);//armazena byte menos significativo
	disp1 = (uint8_t) (valor >> 8);    //armazena byte mais significativo
	uartHex1B(disp1);               //envia byte mais significativo
	uartHex1B(disp0);               //envia byte menos significativo
}

//---------------------------------------------------------------------------
//Envia pela uart variavel de 4 bytes (32 bits) com digitos em hexadecimal
//---------------------------------------------------------------------------
void uartHex4B(uint32_t valor)
{ uint16_t disp0;
	uint16_t disp1;
	disp0 = (uint16_t) (valor & 0x0000FFFF);//armazena dois bytes menos significativos
	disp1 = (uint16_t) (valor >> 16);       //armazena dois bytes mais significativos
	uartHex2B(disp1);                    //envia dois bytes mais significativos
	uartHex2B(disp0);                    //envia dois bytes menos significativos
}

int flagAr=0,flagTV=0,flagLa=0,flagLb=0;

int main(){
	
	int tempDesejada;
	uint16_t temp;

	DDRD = 0xff;
	PORTD = 0xff;
	PORTD = 0x00;
	DDRB = 0b00001000;
	PORTB = 0b00110111;
	DDRC = 0b00010010;
	PORTC = 0b11101100;
	
	
	_delay_ms(100);
	LCD_Init(); //Activate LCD
	LCD_Clear();
	LCD_Print("UNIFACS");	//Begin writing at Line 1, Position 1
	LCD_Action(0xC0);
	LCD_Print("Micro 2022");
	_delay_ms(1000);
	
	PCICR = (1<<PCIE1)|(1<<PCIE0);//habilita interrupção por qualquer mudança de sinal no PORTB e PORTC
	PCMSK0 = (1<<PCINT2)|(1<<PCINT0);//habilita os pinos PCINT0:2 para gerar interrupção
	PCMSK1 = (1<<PCINT13)|(1<<PCINT10);//habilita os pinos PCINT13 e PCINT10  para gerar interrupção
	sei();//habilita as interrupções
	
	TCCR2A=0b00100011;//habilita o pino PD3 e desabilita o PB3, liga o modo rápido
	TCCR2B=0b00000001;//fpwm = f2sc/(256*prescaler) = 16MHz/(256*64) = 976Hz
	
	
	
	LCD_Clear();
	LCD_Print("Bem Vindos!!");
	_delay_ms(4000);
	imprimeMenu();
	
	
	adcBegin(1, 0x01);//Avcc como referencia e habilita AD. Prescaler de 128 (clk_AD = F_CPU/128)
	adcChannel(0);//pino PC0
	
	while(1){
		
		temp=(adcRead() * 0.0048828125 * 100);
		
		if(!flagChaveGeral){
			if(tempDesejada<temp){
				//liga ar
				set_bit(PORTD,PD2);
			}else{
				//desliga ar
				clr_bit(PORTD,PD2);
			}
		
			tela=0;
			if(!tst_bit(PINC,PB3)){
				tempDesejada = config();
				imprimeMenu();
				_delay_ms(300);
			}
		}else{
			LCD_Clear();
			LCD_Print("EMERGENCIA!");
			clr_bit(PORTC,ledA);
			clr_bit(PORTD,PD2);
			clr_bit(PORTB,ledB);
			clr_bit(PORTC,TV);
			clr_bit(TCCR2A,COM2B1);
			flagAr=0;
			flagTV=0;
			flagLa=0;
			flagLb=0;
			_delay_ms(350);
			
		}
		
	}

}

ISR(PCINT0_vect)
{

	if(!tst_bit(PINB,PB0) && !flagChaveGeral){
		cpl_bit(PORTC,ledA);
		LCD_Clear();
		if (flagLa == 0){
			LCD_Print("Lamp A Ligada");
			flagLa=1;
		}
		else{
			LCD_Print("Lamp A Desligada");
			flagLa=0;
		}
		
		_delay_ms(600);
	}
	else if(!tst_bit(PINB,PB2) && !flagChaveGeral){
		cpl_bit(PORTB,ledB);
		LCD_Clear();
		if (flagLb == 0){
			LCD_Print("Lamp B Ligada");
			flagLb=1;
		}
		else{
			LCD_Print("Lamp B Desligada");
			flagLb=0;
		}
		_delay_ms(600);
	}
	if (tela==0){
		imprimeMenu();
	} 
	else if(tela==1){
		imprimeMenuConfig();
	}else if(tela==2){
		LCD_Clear();
		LCD_Print("set AR Temp:");
		_delay_ms(50);
		
	}else{
		LCD_Clear();
		LCD_Print("set cortina vel:");
		_delay_ms(50);
	}
}

ISR(PCINT1_vect){

	if(!tst_bit(PINC,PC5) && !flagChaveGeral){
		cpl_bit(PORTC,TV);
		LCD_Clear();
		if (flagTV == 0){
			LCD_Print("TV Ligada");
			flagTV=1;
		}
		else{
			LCD_Print("TV Desligada");
			flagTV=0;
		}
		_delay_ms(600);
	}else if(!tst_bit(PINC,PC2)){
		LCD_Clear();
		if (flagChaveGeral == 0){
			flagChaveGeral=1;
		}
		else{
			flagChaveGeral=0;
			tela=0;
		}
		_delay_ms(250);
	}
	if (tela==0){
		imprimeMenu();
	} 
	else if(tela==1){
		imprimeMenuConfig();
	}else if(tela==2){
		LCD_Clear();
		LCD_Print("set AR Temp:");
		_delay_ms(50);
		
	}else{
		LCD_Clear();
		LCD_Print("set cortina vel:");
		_delay_ms(50);
	}
}
