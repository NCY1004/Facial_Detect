/*
* My_Project.c
*
* Created: 2020-03-07 오후 7:29:23
* Author : nacha
*/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>


#define LCD_DB_PORT   PORTA            // LCD_DATABUS
#define LCD_DB_DDR   DDRA
#define LCD_CMD_PORT   PORTC          // LCD_COMMAND
#define LCD_CMD_DDR   DDRC

#define YES 12            // 숫자는 의미 없음. 분별을 위한 임의의 값
#define NO 34

unsigned char char_address = 0xC0; 
unsigned char keypad_press = NO;    
unsigned char pwinput = NO;           // 비밀번호 입력이 완료되었는지의 여부
unsigned char userinput_mode = NO;           // 새로운 비밀번호 입력 모드

char key_buffer;                // 키 값 버퍼

char val;	//디바운싱 키패드함수에서 데이터 받는 변수
char value;	//UART에서 데이터 받는용도

unsigned char operand[4] = {'0','0','0','0'};		//초기 비밀번호
unsigned char pw[4]={'0','0','0','0'};				//입력할때 비밀번호	
	
	
enum MODE_SELECT{
	facial_mode,
	keypad_mode
	};
enum MODE_SELECT mode=3;

////////////////////////LCD용 출력함수///////////////
void LCD_cmd(unsigned char cmd)        // LCD에 command를 입력함수
{
	LCD_CMD_PORT = 0x00;               // Instruction register 선택, Write Disable
	LCD_DB_PORT = cmd;                 // LCD data bus에 command 출력
	LCD_CMD_PORT = 0x01;               // Write Enable
	_delay_us(1);                           // LCD 명령처리 지연시간
	LCD_CMD_PORT = 0x00;               // Write Disable
	_delay_us(100);                        // LCD 명령처리 지연시간
}


void LCD_init(void)                     // LCD 초기화 함수(Initialize)
{
	LCD_cmd(0x38);                    // Function set (data length-8bit, 2행, 5x8dote)
	LCD_cmd(0x0F);                    // Display on/off control(화면출력 ON, 커서표시 & 커서깜빡임 OFF)
	LCD_cmd(0x06);                    // Entry mode set(커서 오른쪽 이동, 화면이동 OFF)
	LCD_cmd(0x01);                    // Clear display
	_delay_ms(2);
}



void LCD_data(unsigned char data)     // LCD Data 전송 함수
{
	LCD_CMD_PORT = 0x02;             // Data register 선택, Write Disable
	LCD_DB_PORT = data;               // LCD data bus에 Data 출력
	LCD_CMD_PORT = 0x03;             // Write Enable
	_delay_us(1);
	LCD_CMD_PORT = 0x02;             // Write Disable
	_delay_us(100);
	
}


void LCD_print(unsigned char cmd, char *str)   // LCD 문자열 출력 함수
{
	LCD_cmd(cmd);                    // 출력위치 지정
	while(*str != 0)                    // 문자열의 끝이 올때까지
	{ LCD_data(*str);
		str++;
	}
}


void LCD_char_print(unsigned char cmd, char *str)   // LCD 문자 출력 함수
{
	LCD_cmd(cmd);                    // 출력위치 지정
	LCD_data(*str);
}

void init_uart0()	//UART0 초기화 함수
{
	UCSR0B=0x18;	//수신및 송신 활성화
	UCSR0C=0x06;	//UART MODE
	UBRR0H=0;
	UBRR0L=103;		//보드레이트 세팅 (9600)
}

void putchar0(char c)		//송신
{
	while(!(UCSR0A & (1<<UDRE0)))
	;
	UDR0=c;
}

char getchar0()		//수신
{
	while(!(UCSR0A & (1<<RXC0)))
	;
	return(UDR0);
}


char Keypad_input(void)                // 키패드 입력받는 함수
{
	unsigned char input=' ';            // 키 패드의 입력값, 초기값은 공백문자
	
	PORTD = 0xFF;                     // pin 0,1,2,3 은 키패드의 Row(출력)
	DDRD = 0x0F;                      // pin 4,5,6 은 키패드의 Column(입력)
	

	PORTD = 0b11110111;                // 1번 Row 선택
	_delay_us(1);                         // 명령처리 딜레이 타임
	if ((PIND & 0x10) == 0) input = '1';
	else if ((PIND & 0x20) == 0) input = '2';
	else if ((PIND & 0x40) == 0) input = '3';
	
	
	PORTD = 0b11111011;                // 2번 Row 선택
	_delay_us(1);
	if ((PIND & 0x10) == 0) input = '4';
	else if ((PIND & 0x20) == 0) input = '5';
	else if ((PIND & 0x40) == 0) input = '6';
	
	PORTD = 0b11111101;                // 3번 Row 선택
	_delay_us(1);
	if ((PIND & 0x10) == 0) input = '7';
	else if ((PIND & 0x20) == 0) input = '8';
	else if ((PIND & 0x40) == 0) input = '9';
	
	PORTD = 0b11111110;                // 4번 Row 선택
	_delay_us(1);
	if ((PIND & 0x10) == 0) input = '*';
	else if ((PIND & 0x20) == 0) input = '0';
	else if ((PIND & 0x40) == 0) input = '#';
	
	key_buffer = input;
	
	return input;                    // 입력이 없으면 공백문자 리턴
}

unsigned char keypad_db_input(void)        // 키패드 입력 debouncing 함수
{
	
	unsigned char input;                    // 키패드 입력값 변수
	
	input = Keypad_input();
	
	if(input == ' ')                           // 키가 아무것도 눌리지 않았을 때(혹은 눌렀다 뗐을 때)
	{
		if(keypad_press == NO) return input;    // 키가 이전에 눌린 적이 없다면
		
		else                                     // 키가 이전에 눌린 적이 있다면
		{
			
			_delay_ms(20);
			keypad_press = NO;
			return input;
		}
	}
	
	else                                  // 키가 눌렸을 때
	{
		if(keypad_press == YES) return ' ';    // 키가 이전에 눌린 적이 있다면
		
		else                                   // 키가 눌린 적이 없다면
		{
			if (key_buffer != ' ' ) 
			keypad_press = YES;
			return input;
		}
	}
}

void face_mode()	//얼굴 감지모드
{		
		value=getchar0();	//UART에서 데이터 받는용도
		
		
		if(value=='y')		//y문자 받을때
		{				
			LCD_print(0x80,"    Unlocked!   ");               
			LCD_print(0xC0,"  Welcome Home! ");
			PORTG=0x01;
			OCR3A = 35;		//-90도
		}
			
		else if(value=='n')	//n문자 받을때
		{
			LCD_print(0x80,"     Locked!    ");               
			LCD_print(0xC0," Unknown Person "); 
			PORTG=0x02;
			OCR3A = 150;	//90도
		}
		else if(value=='f')	//f문자 받을때
		{		
			LCD_print(0x80," Face Not Found ");             
			LCD_print(0xC0,"                ");
			PORTG=0x00;
			OCR3A = 150;		//-90도
		}
		
		else if((value=='f')||(value=='n')||(value=='y')&&(PINF&0x02)==0)			//단독으로 스위치만 누르면 키패드 모드가 안들어가져 이코드를 얼굴감지모드 함수에 추가
		{
			PORTB=0x0F;	
			mode=keypad_mode;
		}
		
}

void key_mode()
{	
	PORTG=0x00;
	char number;
		
	unsigned int i=0;			//lcd비밀번호 자리수 용도
	unsigned int a=0;			//새로운 비밀번호 자리수 용도
	unsigned int cur=0;			//입력한 비밀번호 맞는지 검증용도
	
	while(pwinput != YES && userinput_mode !=YES)        // 비밀번호 입력완료가 안되고 비밀번호변경모드가 아닐시 무한루프
	{	
		LCD_print(0x80,"PassWord:       ");
		number = keypad_db_input();
		
		
	if (number == ' ') continue;// 입력이 없으면 Continue 
	
			
		
	else if(number != '*' && number != '#')        // 숫자를 누르면
	{	
		pw[i] = number;                    // 배열에 입력된 숫자 저장		     	
		LCD_char_print(char_address++, &number);
		i++;
		
		 if(i == 4)                            // 4자리까지 숫자가 입력되면
		 {		
			 pwinput = YES;
		 }		 	
			
	}
	
			
	}
	
	while(pwinput != NO && userinput_mode !=YES) 
	{		
		
		number = keypad_db_input();		
		
		if(number=='#'&& pwinput == YES)
		{
	
			for(int j=0; j<4 ;j++)
			{
				if(operand[j]==pw[j]) cur++;			
			}
			
			if(cur==4)				//위의 비밀번호 숫자가 다맞으면 CUR이 4까지 옴
			{
				LCD_print(0x80,"    Unlocked!   ");               // 1열 문자열 출력
				LCD_print(0xC0,"  Welcome Home! ");
				PORTG=0x01;
				OCR3A = 35;		//-90도
				_delay_ms(3000);
				
				LCD_cmd(0x01);                    // Clear display
				PORTG=0x00;							//LED 끄고
				pwinput = NO;				//비밀번호 다시입력 상태로 돌아가기
				char_address = 0xC0;				//LCD 글자위치 초기화
				cur=0;							
				OCR3A = 150;
			}
			
			else   //	비밀번호 틀릴시
			{
				LCD_print(0x80,"     Locked!    ");               // 1열 문자열 출력
				LCD_print(0xC0," Unknown Person ");
				PORTG=0x02;
				OCR3A = 150;	//90도
				_delay_ms(3000);
				
				LCD_cmd(0x01);                    // Clear display
				PORTG=0x00;							//LED 끄고
				pwinput = NO;				//비밀번호 다시입력 상태로 돌아가기
				char_address = 0xC0;				//LCD 글자위치 초기화
				cur=0;								//CUR는 다시 0으로
			}
			
		}
		
		else if(number == '*' && pwinput == YES)        // 별표를 숫자를 누르면(비번바꾸기) 비빌번호를 입력해야 비밀번호 변경모드로 들어옴
		{
			for(int j=0; j<4 ;j++)
			{
				if(operand[j]==pw[j]) cur++;
			}
			
			if(cur==4)				//위의 비밀번호 숫자가 다맞으면 CUR이 4까지 옴
			{	
				LCD_print(0x80,"    PASSWORD    ");               
				LCD_print(0xC0,"  CHANGE MODE ! ");
				_delay_ms(3000);
				LCD_cmd(0x01);
				char_address = 0xC0;	
				userinput_mode = YES;
				pwinput = NO;	
			}
			
			else
			{
				LCD_print(0x80,"  Wrong Number  ");               // 틀린번호를 입력하면(비밀번호 수정모드로 진입전)
				LCD_print(0xC0,"  Try Again   ! ");
				_delay_ms(3000);
				LCD_cmd(0x01);
				char_address = 0xC0;	
				pwinput = NO;									//다시 비밀번호 입력초기로 돌아옴							
			}
			
			
		}
		
			else if((PINF&0x01)==0)				//얼굴감지 모드로 변경
			{
				PORTB=0xF0;
				pwinput = NO;
				char_address = 0xC0;				//LCD 글자위치 초기화
				mode=facial_mode;
			}
		
		
	}
	
	while (userinput_mode!=NO && pwinput == NO)			//비밀번호 변경모드
	
	{	
		LCD_print(0x80,"New_PassWord:   ");	
		number = keypad_db_input();
		
		if (number == ' ') continue;// 입력이 없으면 Continue
		
		else if(number != '*' && number != '#')
		{
		operand[a] = number;                    // 새로운 비밀번호 숫자 저장
		LCD_char_print(char_address++, &number);
		a++;
		
		if(a == 4)                            // 4자리까지 숫자가 입력되면
		{	
			LCD_print(0x80,"Please Wait Now ");         
			LCD_print(0xC0,"Saving Pw_number");
			_delay_ms(5000);
			LCD_cmd(0x01);
			char_address = 0xC0;
			userinput_mode=NO;	//비밀번호 변경모드
			pwinput=NO;	//다시 잠금해제 모드로
		}
		
		
		}
	}

	
	
	
}



int main(void)
{	
	DDRF=0x00;		    //모드선택 버튼
	DDRB=0xFF;			//모드상태 LED
	PORTB=0x00;			//모드상태 LED 초기상태
	
	DDRD = 0x0F;                    // pin 0,1,2,3 은 키패드의 Row(출력)
	PORTD = 0xFF;                   // pin 4,5,6 은 키패드의 Column(입력)
	
	DDRG=0x0F;	// 도어락확인 LED
	PORTG=0x00;
	
	
	LCD_DB_DDR = 0xFF;                // LCD data bus, LCD command port 출력모드
	LCD_CMD_DDR = 0xFF;
	LCD_DB_PORT = 0x00;
	LCD_CMD_PORT = 0x00;
	
	DDRE |= (1<<3);
	TCCR3A |= (1<<COM3A1) | (1<<WGM31);
	TCCR3B |= (1<<WGM33) | (1<<WGM32) | (1<<CS32);
	ICR3 = 1250;
	OCR3A = 150;
	
	LCD_init();
	init_uart0();	//UART시작
	
	_delay_us(100);                    // 명령처리 대기시간
	
	LCD_print(0x80,"Polytech Facial ");               // 1열 문자열 출력
	LCD_print(0xC0,"Security System ");
	
	_delay_ms(3000);
	
	LCD_print(0x80," Please Select  ");               // 1열 문자열 출력
	LCD_print(0xC0," Security_Mode  ");
	
	while (1)
	{	
		
		
		if((PINF&0x01)==0)
		{	
			PORTB=0xF0;
			LCD_print(0x80,"     Facial     ");               // 1열 문자열 출력
			LCD_print(0xC0,"Recognition Mode");
			_delay_ms(3000);
			LCD_cmd(0x01);			
			mode=facial_mode;
		}
		else if((PINF&0x02)==0)
		{
			PORTB=0x0F;
			OCR3A = 150;
		    LCD_print(0x80,"     Key_Pad    ");               // 1열 문자열 출력
			LCD_print(0xC0,"      Mode      ");
			_delay_ms(3000);
			LCD_cmd(0x01);
			mode=keypad_mode;	
		}
		
		
		switch(mode)
		{
			case facial_mode:
			face_mode();
			break;
			
			case keypad_mode:
			key_mode();
			break;
			
		}
		
	}
	
}


