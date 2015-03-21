//Rf Lab. all rights reserved.

#include <htc.h>
#include <math.h>
//----------------- 
// コンパイルオプション
//-----------------

// コンフィグレーションレジスタをデバッグ用に変更する
//#define USE_DEBUGGER

//---------------
// コンフィグ
//---------------
// コンフィグレーションレジスタ
#ifdef USE_DEBUGGER
	__CONFIG(
		CP_OFF
		& CPD_OFF
		& FOSC_INTRC_NOCLKOUT
		& DEBUG_ON // インサーキットデバッグ有効
		& BOREN_ON
		& IESO_OFF	// Internal/External Switchover mode
		& MCLRE_ON	// RE3/MCLR pin function is digital input, MCLR internally tied to VDD
		& PWRTE_OFF
		& LVP_OFF
		& FCMEN_ON
		& WDTE_OFF
		& WRT_OFF);
#else
	__CONFIG(
		CP_OFF
		& CPD_OFF
		& FOSC_INTRC_NOCLKOUT
		& DEBUG_OFF	  // インサーキットデバッグ無効
		& BOREN_ON
		& IESO_OFF	 // Internal/External Switchover mode
		& MCLRE_ON // RE3/MCLR pin function is digital input, MCLR internally tied to VDD
		& PWRTE_ON
		& LVP_OFF
		& FCMEN_ON
		& WDTE_OFF
		& WRT_OFF);
#endif

// PICのクロックをHzで設定(waitとかで使われる)
#define _XTAL_FREQ 8000000UL 

//---------------
// 定数＆defines
//---------------
#define NIL 0

// EEPROM関連
#define EEPROM_ADR_CONFIG (0x0)
#define EEPROM_ADR_MAX_BRIGHTNESS (0x1)

#define CONFIG_ALARM_ON (1<<7)
#define CONFIG_AUTO_OFF (1<<6)
#define CONFIG_AUTO_BRIGHTNESS (1<<4)

// I2C関連
#define I2C_SLAVE_ADDRESS_RTC 0xa2
#define I2C_READ 1
#define I2C_WRITE 0
// I2Cのクロック分周
// 伝送レート = (Fosc/4)/(設定値-1)
// ∴設定値=((Fosc(8Mhz)/4)/伝送レート(100kbps))-1 = (8,000,000/4)/(100,000)-1 = 19
#define I2C_CLOCK_DEVIDE_VAL 19

// AD関連
#define MAX_AD_CHANNELS 13

// AD変換(resolution256) → LM35(10mV/度)→ アンプ(7.8倍) → 温度 → 100倍値の変換
// adval * (5 / 256)　* 1000 / 10  / 7.8 * 100	= 25.0400641 をかければ100倍された温度が得られる
#define ADVAL_2_TMPARATURE 25

// ポートステート
// PORTA 0,1はアナログ入力（入力にする必要がある）
// PORTB ボタン入力がひとつ
// PORTC 3・4はI2CのSCL/SDA（入力にする必要がある）、0、1はボタン入力
// PORTD は欠番
// PORTE MCLRは入力
// AN 4-2はデジタル,0、1はアナログ
// AN 13-8ピンはデジタルで使用
#define PIO_A 0b00000011
#define PIO_B 0b00100000
#define PIO_C 0b00011011
#define PIO_D 0b00000000
#define PIO_E 0b00001000
#define PIO_ANSEL  0b00000011;
#define PIO_ANSELH 0b00000000;

// ポート割り当て
#define PIN_LINDECODER_0 RA4
#define PIN_LINDECODER_1 RA5
#define PIN_LINDECODER_2 RA6
#define PIN_LINDECODER_3 RA7
#define PIN_DIGIT_0 RB4
#define PIN_DIGIT_1 RB3
#define PIN_DIGIT_2 RB1
#define PIN_DIGIT_3 RB0
#define PIN_DIGIT_4 RC6
#define PIN_DIGIT_5 RC5
#define PIN_COLON_L RB2
#define PIN_COLON_R RC7
#define PIN_TEMPARATURE_SENSOR RA0
#define PIN_CDS RA1
#define PIN_TRANSISTOR_0 RA2
#define PIN_TRANSISTOR_1 RA3
#define PIN_BUTTON_0 RB5
#define PIN_ICSPDAT RB6
#define PIN_ICSPCLK RB7
#define PIN_BUTTON_1 RC0
#define PIN_BUTTON_2 RC1
#define PIN_SPEAKER RC2
#define PIN_SCL RC3
#define PIN_SDA RC4
#define PIN_NMCLK RE3

//タイマー0プリスケーラ
//	000		  | 1 : 2		| 1 : 1
//	001		  | 1 : 4		| 1 : 2
//	010		  | 1 : 8		| 1 : 4
//	011		  | 1 : 16		| 1 : 8
//	100		  | 1 : 32		| 1 : 16
//	101		  | 1 : 64		| 1 : 32
//	110		  | 1 : 128		| 1 : 64
//	111		  | 1 : 256		| 1 : 128
#define TIMER0_PRESCALER 0b10
#define TMR0_OFFSET 125

//タイマー1プリスケーラ
//	11 = 1:8 Prescale Value
//	10 = 1:4 Prescale Value
//	01 = 1:2 Prescale Value
//	00 = 1:1 Prescale Value
#define TIMER1_PRESCALER (0b01)

//#define TMR1L_1SEC_OFFSET_DEFAULT_L (0x34)
//#define TMR1H_1SEC_OFFSET_DEFAULT_L (0x85)
#define TMR1L_1SEC_OFFSET_DEFAULT_DOWN (0x00)
#define TMR1H_1SEC_OFFSET_DEFAULT_DOWN (0x85)

#define TMR1L_1SEC_OFFSET_DEFAULT_UP (0x00)
#define TMR1H_1SEC_OFFSET_DEFAULT_UP (0x86)

//#define TMR1L_1SEC_OFFSET_DEFAULT (200)
//#define TMR1H_1SEC_OFFSET_DEFAULT (88)
#define TMR1_SOFT_PRESCALER (32)

// 数字からラインデコーダ用出力へ変換するテーブル
const unsigned char NUM_2_LINE[] = 
{
	0,
	1,
	2,
	3,
	4,
	6,
	5,
	7,
	8,
	9,
	10,
};

// 輝度の下限
#define BRIGHTNESS_HIGHER_MAX (200)
#define BRIGHTNESS_LOWER_THRESHOLD_NIXIE (50)
#define BRIGHTNESS_LOWER_THRESHOLD_NEON  (180)
#define AUTO_OFF_THRESHOLD (30)
#define TRANSITION_RATE (0x3)
#define CLOCK_COLON_BLINK_OFFSET (10)
#define DYNAMIC_OFF_TIMER (0xff - 30)

//---------------
// マクロ
//---------------
#define STATIC_ASSERT(exp) {char is_size[(exp)?1:0]; (void)is_size;}
#define LO(c) ((c)&0x0f)
#define HI(c) (((c)>>4)&0x0f)

//---------------
// 型定義
//---------------

// bool
typedef enum _e_bool
{
	FALSE,
	TRUE
}e_bool;

// コールバックタイプ
typedef e_bool (*t_callback)(unsigned char arg_);

// 桁
typedef enum _e_digit
{
	DIGIT_0 = 0,
	DIGIT_1,
	DIGIT_COLON_L,
	DIGIT_2,
	DIGIT_3,
	DIGIT_COLON_R,
	DIGIT_4,
	DIGIT_5,
	DIGIT_OFF
}e_digit;

// ステートマシン用ステート
typedef struct _STATE
{
	t_callback on_button0;
	t_callback on_button1;
	t_callback on_button2;
}STATE;


//---------------
// グローバル変数
//---------------

// コンフィグ
volatile static unsigned char g_config_alarm_enable = 1;
volatile static unsigned char g_config_auto_off_enable = 1;
volatile static unsigned char g_config_auto_brightness = 1;
volatile static unsigned char g_config_max_brightness = BRIGHTNESS_HIGHER_MAX;
volatile static unsigned char g_auto_off_count = 0;

// 現在のコールバック関数
//volatile static t_callback g_timer0_callback[3] = {NIL};
volatile static t_callback g_timer1_callback[3] = {NIL};
volatile static t_callback g_mainloop_callback[3] = {NIL};

// 表示記憶領域
volatile static unsigned char g_disp_flag = 0b11111111;
volatile static unsigned char g_disp[8] = {0, 0, DIGIT_OFF, 0, 0, DIGIT_OFF, 0, 0};
volatile static unsigned char g_disp_prev[8] = {0, 0, DIGIT_OFF, 0, 0, DIGIT_OFF, 0, 0};
volatile static unsigned char g_disp_transition[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
volatile static unsigned char g_disp_capture[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
volatile static unsigned char g_disp_colon[8] = {0, 0, 3, 0, 0, 3, 0, 0};
volatile static unsigned char g_disp_colon_prev[8] = {0, 0, 3, 0, 0, 3, 0, 0};
volatile static unsigned char g_disp_colon_capture[8] = {0, 0, 3, 0, 0, 3, 0, 0};

// 時間記憶領域
volatile static unsigned char g_hour	= 0x00;
volatile static unsigned char g_minute	= 0x00;
volatile static unsigned char g_second	= 0x00;
volatile static unsigned char g_year	= 0x00;
volatile static unsigned char g_month	= 0x00;
volatile static unsigned char g_date	= 0x00;
volatile static unsigned char g_week	= 0x00;
volatile static unsigned char g_alarm_hour	  = 0x12;
volatile static unsigned char g_alarm_minute  = 0x00;

// 温度記憶領域
volatile static unsigned char g_temparature_integer = 0;
volatile static unsigned char g_temparature_fractional = 0;

// 点滅桁設定
volatile unsigned char g_blink_digits = 0b00000000;

// CdS明るさ記憶領域
volatile unsigned char g_cds_adval = 0;
volatile unsigned char g_cds_100th_place = 0;
volatile unsigned char g_cds_10th_place = 0;
volatile unsigned char g_cds_1th_place = 0;

// TargetBcdUpDown
volatile unsigned char* g_target_bcd_ptr = NIL;
volatile unsigned char g_target_max = 59;
volatile unsigned char g_target_min = 0;

// ボタン
volatile static unsigned char g_button0_push = 0;
volatile static unsigned char g_button1_push = 0;
volatile static unsigned char g_button2_push = 0;
volatile static unsigned char g_button0_press = 0;
volatile static unsigned char g_button1_press = 0;
volatile static unsigned char g_button2_press = 0;

// アラーム
volatile static unsigned char g_alarm_trigger = 0;
volatile static unsigned char g_alarm_beep_on = 0;

// ステートマシン
static STATE* g_state_current;
static STATE g_state_clock			= {NIL, NIL, NIL};
static STATE g_state_alarm			= {NIL, NIL, NIL};
static STATE g_state_temparature	= {NIL, NIL, NIL};
static STATE g_state_config			= {NIL, NIL, NIL};
static STATE g_state_calendar		= {NIL, NIL, NIL};
static STATE g_state_dispoff		= {NIL, NIL, NIL};
static STATE g_state_set_clock		= {NIL, NIL, NIL};
static STATE g_state_set_calendar	= {NIL, NIL, NIL};
static STATE g_state_set_alarm		= {NIL, NIL, NIL};
static STATE g_state_set_config		= {NIL, NIL, NIL};

// 暫定時計更新速度調整、本来はRTCからのクロック供給を行うべき
volatile unsigned char timer1h_1sec_offset = TMR1H_1SEC_OFFSET_DEFAULT_DOWN;
volatile unsigned char timer1l_1sec_offset = TMR1L_1SEC_OFFSET_DEFAULT_DOWN;
volatile unsigned char g_next_timer1_ajustment = 0;

// とりあえずプロトタイプ宣言
e_bool State_ChangeConfig(unsigned char button_no_);
e_bool State_CallbackBlink(unsigned char button_no_);


//---------------
// 汎用関数
// 注意:wait計の関数は比較とかを無視しているのであまり正確ではない
//---------------
// 100usのwait/
void Wait100us(unsigned char us100_)
{
	unsigned char count = us100_;
	while (count)
	{
		__delay_us(100);
		count--;
	}
}
// 100usのwait
void Wait10us(unsigned char us10_)
{
	unsigned char count = us10_;
	while (count)
	{
		__delay_us(10);
		count--;
	}
}

// 100usのwait
void Wait1us(unsigned char us100_)
{
	unsigned char count = us100_;
	while (count)
	{
		__delay_us(1);
		count--;
	}
}

// 100ms単位のwait
void Wait1ms(unsigned char ms_)
{
	unsigned char count = ms_;
	while (count)
	{
		__delay_ms(1);
		count--;
	}
}

// 10ms単位のwait
void Wait10ms(unsigned char ms_)
{
	unsigned char count = ms_;
	while (count)
	{
		__delay_ms(10);
		count--;
	}
}

// 100ms単位のwait
void Wait100ms(unsigned char ms_)
{
	unsigned char count = ms_;
	while (count)
	{
		__delay_ms(100);
		count--;
	}
}

#if 0
	void Beep(unsigned char loop_)
	{
		unsigned char i;
		for (i=0; i<loop_; ++i)
		{
			PIN_SPEAKER = 1;
			Wait10ms(10);
			PIN_SPEAKER = 0;
			Wait10ms(10);
		}
	}
#else
	#define Beep(loop_) \
	{\
		PIN_SPEAKER = 1;\
		Wait10ms(10);\
		PIN_SPEAKER = 0;\
	}
#endif 

// EEPROM 読み込み
unsigned char Eeprom_Read( unsigned char adr )
{
	EEADR = adr;
	RD = 1;				// Set read bit
	return( EEDATA );		// Return read data
}

// EEPROM書き込み
void Eeprom_Write( unsigned char adr, unsigned char data )
{
	EEADR = adr;
	EEDATA = data;

	WREN = 1;  			// Set write enable bit
	EECON2 = 0x55;      //おまじない
	EECON2 = 0xaa;      //おまじない

	WR = 1;  				// Set programming bit
	while( EEIF == 0 )		// Wait till write operation complete
	{
		NOP();
	}
	EEIF = 0;				// Clear EEPROM write complete bit
	WREN = 0;  			// Clear write enable bit
}

// I2Cスタートコンディション
void I2c_Start()
{
	unsigned char timeout;
	// スタートコンディション発行
	SSPIF = 0;
	SEN = 1;
	
	// SENがリセットされるか
	// SSPIFがセットされ割り込みが発生するかを待つ。
	// //SSPIFはほかにも要因が立つ可能性があるので、ここではSENを使用する
	timeout = 0;
	while (SEN)
	//while (!SSPIF)
	{
		timeout++;
		if (timeout == 0xff)
		{
			Beep(2);
			return;
		}
	}

	// 一応クリアしておかないと割り込み有効化時に割り込みでまくるはず
	//SSPIF = 0;
}

// I2Cリピートスタートコンディション
void I2c_RepeatStart()
{
	unsigned char timeout;
	// リピートスタートコンディション発行
	SSPIF = 0;
	RSEN = 1;
	
	// RSENがリセットされるか
	// SSPIFがセットされ割り込みが発生するかを待つ。
	// //SSPIFはほかにも要因が立つ可能性があるので、ここではSENを使用する
	timeout = 0;
	while (RSEN)
	// while (!SSPIF)
	{
		timeout++;
		if (timeout == 0xff)
		{		
			Beep(3);
			return;
		}
	}

	// 一応クリアしておかないと割り込み有効化時に割り込みでまくるはず
	SSPIF = 0;
}

// I2Cストップコンディション
void I2c_Stop()
{
	unsigned char timeout;
	// ストップコンディション発行
	SSPIF = 0;
	PEN = 1;

	// RSENがリセットされるか
	// SSPIFがセットされ割り込みが発生するかを待つ。
	// // SSPIFはほかにも要因が立つ可能性があるので、ここではPENを使用する
	timeout = 0;
	while (PEN)
	//while (!SSPIF)
	{
		timeout++;
		if (timeout == 0xff)
		{
			Beep(4);
			return;
		}
	}
	
	// 一応クリアしておかないと割り込み有効化時に割り込みでまくるはず
	//SSPIF = 0;
}

// I2C送信
e_bool I2c_Write(unsigned char data_)
{
	unsigned char timeout;
	// 割り込みクリアしてデータを書き込むとデータ送信開始
	SSPIF = 0;
	SSPBUF = data_;
	
	// 送信終了待ち
	timeout = 0;
	while (!SSPIF)
	{
		timeout++;
		if (timeout == 0xff)
		{
			Beep(5);
			return FALSE;
		}
	}

	// 一応クリアしておかないと割り込み有効化時に割り込みでまくるはず
	//SSPIF = 0;

	return TRUE;
}

// I2C受信
unsigned char I2c_Read(unsigned char ack_)
{
	unsigned char timeout;
	
	// I2C受信開始
	SSPIF = 0;
	RCEN = 1;

	// 受信待ち
	timeout = 0;
	while (RCEN)
	//while (!SSPIF)
	{
		timeout++;
		if (timeout == 0xff)
		{
			Beep(6);
			return 0;
		}
	}

	// アクノリッジシーケンス開始
	if (ack_)
	{
		// ackは0
		ACKDT = 0;
	}
	else
	{
		// nackは1
		ACKDT = 1;
	}
	ACKEN = 1;
	SSPIF = 0;
	 
	// アクノリッジシーケンス終了待ち
	// ACKENクリアもしくは
	// // SSPIFがセットされ割り込みが発生するかを待つ。	
	timeout = 0;
	while (ACKEN)
	//while (!SSPIF)
	{
		timeout++;
		if (timeout == 0xff)
		{
			Beep(7);
			return 0;
		}
	}

	// 取得した値
	return SSPBUF;
}

// ADコンバータの値を取得する
unsigned char Pic_GetAd(unsigned char ch_)
{
	if (ch_ > MAX_AD_CHANNELS)
	{
		return 0xff;
	}

	// GO
	ADCON0 = (ADCON0 & 0b11000011) | (ch_ << 2);
	ADCON0 |= 0b00000010;

	// AD変換終了まで待つ
	while (ADCON0 & 0b00000010)
	{
		Wait10us(1);
	}
	
	// 結果を読み込み
	return ADRESH;
};

//---------------
// アプリケーション
//---------------
/// レジスタ初期設定
void InitDevice()
{
	// 内部クロック設定
	// bit 7 Unimplemented: Read as ‘0’
	// bit 6-4 IRCF<2:0>: Internal Oscillator Frequency Select bits
	//	111 = 8 MHz
	//	110 = 4 MHz (default)
	//	101 = 2 MHz
	//	100 = 1 MHz
	//	011 = 500 kHz
	//	010 = 250 kHz
	//	001 = 125 kHz
	//	000 = 31 kHz (LFINTOSC)
	// bit 3 OSTS: Oscillator Start-up Time-out Status bit(1)
	//	1 = Device is running from the clock defined by FOSC<2:0> of the CONFIG1 register
	//	0 = Device is running from the internal oscillator (HFINTOSC or LFINTOSC)
	// bit 2 HTS: HFINTOSC Status bit (High Frequency 	 8 MHz to 125 kHz)
	//	1 = HFINTOSC is stable
	//	0 = HFINTOSC is not stable
	// bit 1 LTS: LFINTOSC Stable bit (Low Frequency 	31 kHz)
	//	1 = LFINTOSC is stable
	//	0 = LFINTOSC is not stable
	// bit 0 SCS: System Clock Select bit
	//	1 = Internal oscillator is used for system clock
	//	0 = Clock source defined by FOSC<2:0> of the CONFIG1 register
	OSCCON = 0b01110001; // 内臓8Mhz

	// ADコンバータコントロール0
	// bit 7-6 ADCS<1:0>: A/D Conversion Clock Select bits
	//　 00 = FOSC/2
	//　 01 = FOSC/8
	//　 10 = FOSC/32
	// 　11 = FRC (clock derived from a dedicated internal oscillator = 500 kHz max)
	// bit 5-2 CHS<3:0>: Analog Channel Select bits
	// 　0000 = AN0
	//　　　・・・
	// 　1101 = AN13
	// 　1110 = CVREF
	// 　1111 = Fixed Ref (0.6V fixed voltage reference)
	// bit 1 GO/DONE: A/D Conversion Status bit
	// 　1 = A/D conversion cycle in progress. Setting this bit starts an A/D conversion cycle.
	//		This bit is automatically cleared by hardware when the A/D conversion has completed.
	// 　0 = A/D conversion completed/not in progress
	// bit 0 ADON: ADC Enable bit
	// 　1 = ADC is enabled
	// 　0 = ADC is disabled and consumes no operating current
	ADCON0 = 0b10000101; // AD0, FOSC/32, Enable

	// ADコンバータコントロール1
	// bit 7 ADFM: A/D Conversion Result Format Select bit
	//	1 = Right justified
	//	0 = Left justified
	// bit 6 Unimplemented: Read as ‘0’
	// bit 5 VCFG1: Voltage Reference bit
	//	1 = VREF- pin
	//	0 = VSS
	// bit 4 VCFG0: Voltage Reference bit
	//	1 = VREF+ pin
	//	0 = VDD
	// bit 3-0 Unimplemented: Read as ‘0’
	ADCON1 = 0b00000000; // left justified, Vref=Vdd,Vss
		
	// SSPコントロール0
	// R/W-0 R/W-0 R/W-0 R/W-0 R/W-0 R/W-0 R/W-0 R/W-0
	// WCOL	 SSPOV SSPEN CKP   SSPM3 SSPM2 SSPM1 SSPM0
	// 
	// bit 7 WCOL: Write Collision Detect bit
	//	Master mode:
	//	 1 = A write to the SSPBUF register was attempted while the I2C conditions were not valid for a transmission to be started
	//	 0 = No collision
	//	Slave mode:
	//	 1 = The SSPBUF register is written while it is still transmitting the previous word (must be cleared in software)
	//	 0 = No collision
	// bit 6 SSPOV: Receive Overflow Indicator bit
	//	In SPI mode:
	//	 1 = A new byte is received while the SSPBUF register is still holding the previous data. In case of overflow, the data in SSPSR
	//		 is lost. Overflow can only occur in Slave mode. In Slave mode, the user must read the SSPBUF, even if only transmitting
	//		 data, to avoid setting overflow. In Master mode, the overflow bit is not set since each new reception (and transmission) is
	//		 initiated by writing to the SSPBUF register (must be cleared in software).
	//	 0 = No overflow
	//	In I2 C mode:
	//	 1 = A byte is received while the SSPBUF register is still holding the previous byte. SSPOV is a “don’t care” in Transmit
	//		 mode (must be cleared in software).
	//	 0 = No overflow
	// bit 5 SSPEN: Synchronous Serial Port Enable bit
	//	In both modes, when enabled, these pins must be properly configured as input or output
	//	In SPI mode:
	//	 1 = Enables serial port and configures SCK, SDO, SDI and SS as the source of the serial port pins
	//	 0 = Disables serial port and configures these pins as I/O port pins
	//	In I2 C mode:
	//	 1 = Enables the serial port and configures the SDA and SCL pins as the source of the serial port pins
	//	 0 = Disables serial port and configures these pins as I/O port pins
	// bit 4 CKP: Clock Polarity Select bit
	//	In SPI mode:
	//	 1 = Idle state for clock is a high level
	//	 0 = Idle state for clock is a low level
	//	In I2 C Slave mode:
	//	 SCK release control
	//	 1 = Enable clock
	//	 0 = Holds clock low (clock stretch). (Used to ensure data setup time.)
	//	In I2 C Master mode:
	//	 Unused in this mode
	// bit 3-0 SSPM<3:0>: Synchronous Serial Port Mode Select bits
	//	0000 = SPI Master mode, clock = FOSC/4
	//	0001 = SPI Master mode, clock = FOSC/16
	//	0010 = SPI Master mode, clock = FOSC/64
	//	0011 = SPI Master mode, clock = TMR2 output/2
	//	0100 = SPI Slave mode, clock = SCK pin, SS pin control enabled
	//	0101 = SPI Slave mode, clock = SCK pin, SS pin control disabled, SS can be used as I/O pin
	//	0110 = I2C Slave mode, 7-bit address
	//	0111 = I2C Slave mode, 10-bit address
	//	1000 = I2C Master mode, clock = FOSC / (4 * (SSPADD+1))
	//	1001 = Load Mask function
	//	1010 = Reserved
	//	1011 = I2C firmware controlled Master mode (Slave idle)
	//	1100 = Reserved
	//	1101 = Reserved
	//	1110 = I2C Slave mode, 7-bit address with Start and Stop bit interrupts enabled
	//	1111 = I2C Slave mode, 10-bit address with Start and Stop bit interrupts enabled
	SSPCON = 0b00101000; // SSPポートとして利用、I2Cマスターとして利用

	// SSPコントロール2
	// R/W-0 R-0	 R/W-0 R/W-0 R/W-0 R/W-0 R/W-0 R/W-0
	// GCEN	 ACKSTAT ACKDT ACKEN RCEN  PEN	 RSEN  SEN
	//
	// bit 7 GCEN: General Call Enable bit (in I2C Slave mode only)
	//	1 = Enable interrupt when a general call address (0000h) is received in the SSPSR
	//	0 = General call address disabled
	// bit 6 ACKSTAT: Acknowledge Status bit (in I2C Master mode only)
	//	In Master Transmit mode:
	//	 1 = Acknowledge was not received from slave
	//	 0 = Acknowledge was received from slave
	// bit 5 ACKDT: Acknowledge Data bit (in I2C Master mode only)
	//	In Master Receive mode:
	//	Value transmitted when the user initiates an Acknowledge sequence at the end of a receive
	//	 1 = Not Acknowledge
	//	 0 = Acknowledge
	// bit 4 ACKEN: Acknowledge Sequence Enable bit (in I2C Master mode only)
	//	In Master Receive mode:
	//	 1 = Initiate Acknowledge sequence on SDA and SCL pins, and transmit ACKDT data bit.
	// Automatically cleared by hardware.
	//	0 = Acknowledge sequence idle
	// bit 3 RCEN: Receive Enable bit (in I2C Master mode only)
	//	1 = Enables Receive mode for I2C
	//	0 = Receive idle
	// bit 2 PEN: Stop Condition Enable bit (in I2C Master mode only)
	//	SCK Release Control:
	//	 1 = Initiate Stop condition on SDA and SCL pins. Automatically cleared by hardware.
	//	 0 = Stop condition Idle
	// bit 1 RSEN: Repeated Start Condition Enabled bit (in I2C Master mode only)
	//	1 = Initiate Repeated Start condition on SDA and SCL pins. Automatically cleared by hardware.
	//	0 = Repeated Start condition Idle
	// bit 0 SEN: Start Condition Enabled bit (in I2C Master mode only)
	//	In Master mode:
	//	 1 = Initiate Start condition on SDA and SCL pins. Automatically cleared by hardware.
	//	 0 = Start condition Idle
	//	In Slave mode:
	//	 1 = Clock stretching is enabled forboth slave transmit and slave receive (stretch enabled)
	//	 0 = Clock stretching is disabled
	SSPCON2 = 0b00000000; // とりあえずIdle状態を保つ

	// SSPステータス
	// R/W-0 R/W-0 R-0 R-0 R-0 R-0 R-0 R-0
	// SMP	 CKE   D/A P   S   R/W UA  BF
	//
	// bit 7 SMP: Sample bit
	//	SPI Master mode:
	//	 1 = Input data sampled at end of data output time
	//	 0 = Input data sampled at middle of data output time
	//	SPI Slave mode:
	//	 SMP must be cleared when SPI is used in Slave mode
	//	In I2 C Master or Slave mode:
	//	 1 = Slew rate control disabled for standard speed mode (100 kHz and 1 MHz)
	//	 0 = Slew rate control enabled for high speed mode (400 kHz)
	// bit 6 CKE: SPI Clock Edge Select bit
	//	CKP = 0:
	//	 1 = Data transmitted on rising edge of SCK
	//	 0 = Data transmitted on falling edge of SCK
	//	CKP = 1:
	//	 1 = Data transmitted on falling edge of SCK
	//	 0 = Data transmitted on rising edge of SCK
	// bit 5 D/A: Data/Address bit (I2C mode only)
	//	1 = Indicates that the last byte received or transmitted was data
	//	0 = Indicates that the last byte received or transmitted was address
	// bit 4 P: Stop bit
	//	(I2C mode only. This bit is cleared when the MSSP module is disabled, SSPEN is cleared.)
	//	1 = Indicates that a Stop bit has been detected last (this bit is ‘0’ on Reset)
	//	0 = Stop bit was not detected last
	// bit 3 S: Start bit
	//	(I2C mode only. This bit is cleared when the MSSP module is disabled, SSPEN is cleared.)
	//	1 = Indicates that a Start bit has been detected last (this bit is ‘0’ on Reset)
	//	0 = Start bit was not detected last
	// bit 2 R/W: Read/Write bit information (I2C mode only)
	//	This bit holds the R/W bit information following the last address match. This bit is only valid from the address match to
	//	the next Start bit, Stop bit, or not ACK bit.
	//	In I2 C Slave mode:
	//	 1 = Read
	//	 0 = Write
	//	In I2 C Master mode:
	//	 1 = Transmit is in progress
	//	 0 = Transmit is not in progress
	//	OR-ing this bit with SEN, RSEN, PEN, RCEN, or ACKEN will indicate if the MSSP is in Idle mode.
	// bit 1 UA: Update Address bit (10-bit I2C mode only)
	//	1 = Indicates that the user needs to update the address in the SSPADD register
	//	0 = Address does not need to be updated
	// bit 0 BF: Buffer Full Status bit
	//	Receive (SPI and I2 C modes):
	//	 1 = Receive complete, SSPBUF is full
	//	 0 = Receive not complete, SSPBUF is empty
	//	Transmit (I2 C mode only):
	//	 1 = Data transmit in progress (does not include the ACK and Stop bits), SSPBUF is full
	//	 0 = Data transmit complete (does not include the ACK and Stop bits), SSPBUF is empty
	SSPSTAT = 0b10000000; 

	// SSPマスク
	// bit 7-1 MSK<7:1>: Mask bits
	//	1 = The received address bit n is compared to SSPADD<n> to detect I2C address match
	//	0 = The received address bit n is not used to detect I2C address match
	// bit 0 MSK<0>: Mask bit for I2C Slave mode, 10-bit Address(2)
	//	I2C Slave mode, 10-bit Address (SSPM<3:0> = 0111):
	//	 1 = The received address bit 0 is compared to SSPADD<0> to detect I2C address match
	//	 0 = The received address bit 0 is not used to detect I2C address match
	//
	// Note 1: When SSPCON bits SSPM<3:0> = 1001, any reads or writes to the SSPADD SFR address are accessed
	// through the SSPMSK register.
	// 2: In all other SSP modes, this bit has no effect.
	SSPMSK = 0b00000000;
		
	// その他SSP関連
	SSPADD = I2C_CLOCK_DEVIDE_VAL; // I2Cマスターではクロック分周の値となる

	SSPBUF = 0b00000000; // クリア

	// ポート入出力設定
	TRISA  = PIO_A;
	TRISB  = PIO_B;
	TRISC  = PIO_C;
	TRISE  = PIO_E;
	PORTA  = 0b00000000; // PORTA クリア
	PORTB  = 0b00000000; // PORTB クリア   
	PORTC  = 0b00000000; // PORTC クリア 
	PORTE  = 0b00001000; // PORTE クリア 
	ANSEL  = PIO_ANSEL;	 // AN4-2はデジタル,AN0-1はアナログ
	ANSELH = PIO_ANSELH; // AN13-8ピンはデジタルで使用
	//ULPWUE = 0; //そもそも初期値０ﾞﾌｫ路とよくわからないけどAD用に設定しないとだめ？
	//CM1CON0 = 1;

	// オプション機能設定
	// bit 7 RBPU: PORTB Pull-up Enable bit
	//	1 = PORTB pull-ups are disabled
	//	0 = PORTB pull-ups are enabled by individual PORT latch values
	// bit 6 INTEDG: Interrupt Edge Select bit
	//	1 = Interrupt on rising edge of INT pin
	//	0 = Interrupt on falling edge of INT pin
	// bit 5 T0CS: TMR0 Clock Source Select bit
	//	1 = Transition on T0CKI pin
	//	0 = Internal instruction cycle clock (FOSC/4)
	// bit 4 T0SE: TMR0 Source Edge Select bit
	//	1 = Increment on high-to-low transition on T0CKI pin
	//	0 = Increment on low-to-high transition on T0CKI pin
	// bit 3 PSA: Prescaler Assignment bit
	//	1 = Prescaler is assigned to the WDT
	//	0 = Prescaler is assigned to the Timer0 module
	// bit 2-0 PS<2:0>: Prescaler Rate Select bits
	//	Bit Value | Timer0 Rate | WDT Rate
	//	000		  | 1 : 2		| 1 : 1
	//	001		  | 1 : 4		| 1 : 2
	//	010		  | 1 : 8		| 1 : 4
	//	011		  | 1 : 16		| 1 : 8
	//	100		  | 1 : 32		| 1 : 16
	//	101		  | 1 : 64		| 1 : 32
	//	110		  | 1 : 128		| 1 : 64
	//	111		  | 1 : 256		| 1 : 128
	TMR0 = TMR0_OFFSET; // とりあえずタイマーをクリアしておく
	OPTION_REG = 0b10000000 | TIMER0_PRESCALER; // タイマー0をFOSC/4、分周器を2分周設定
	
	// タイマー1設定
	// bit 7 T1GINV: Timer1 Gate Invert bit(1)
	//	1 = Timer1 gate is active-high (Timer1 counts when gate is high)
	//	0 = Timer1 gate is active-low (Timer1 counts when gate is low)
	// bit 6 TMR1GE: Timer1 Gate Enable bit(2)
	//	If TMR1ON = 0:
	//	 This bit is ignored
	//	If TMR1ON = 1:
	//	 1 = Timer1 counting is controlled by the Timer1 Gate function
	//	 0 = Timer1 is always counting
	// bit 5-4 T1CKPS<1:0>: Timer1 Input Clock Prescale Select bits
	//	11 = 1:8 Prescale Value
	//	10 = 1:4 Prescale Value
	//	01 = 1:2 Prescale Value
	//	00 = 1:1 Prescale Value
	// bit 3 T1OSCEN: LP Oscillator Enable Control bit
	//	1 = LP oscillator is enabled for Timer1 clock
	//	0 = LP oscillator is off
	// bit 2 T1SYNC: Timer1 External Clock Input Synchronization Control bit
	//	TMR1CS = 1:
	//	　1 = Do not synchronize external clock input
	//	　0 = Synchronize external clock input
	// 　TMR1CS = 0:
	// 　　This bit is ignored. Timer1 uses the internal clock
	// bit 1 TMR1CS: Timer1 Clock Source Select bit
	//	1 = External clock from T1CKI pin (on the rising edge)
	//	0 = Internal clock (FOSC/4)
	// bit 0 TMR1ON: Timer1 On bit
	//	1 = Enables Timer1
	//	0 = Stops Timer1
	T1CON = 0b00000001 | (TIMER1_PRESCALER << 4); // ゲートは使用しない、内臓クロックFOSC/4、8分周、起動
	TMR1L = 0;
	TMR1H = 0;

	// ペリフェラルインタラプトイネーブル設定
	// bit 7 Unimplemented: Read as ‘0’
	// bit 6 ADIE: A/D Converter (ADC) Interrupt Enable bit
	//	1 = Enables the ADC interrupt
	//	0 = Disables the ADC interrupt
	// bit 5 RCIE: EUSART Receive Interrupt Enable bit
	//	1 = Enables the EUSART receive interrupt
	//	0 = Disables the EUSART receive interrupt
	// bit 4 TXIE: EUSART Transmit Interrupt Enable bit
	//	1 = Enables the EUSART transmit interrupt
	//	0 = Disables the EUSART transmit interrupt
	// bit 3 SSPIE: Master Synchronous Serial Port (MSSP) Interrupt Enable bit
	//	1 = Enables the MSSP interrupt
	//	0 = Disables the MSSP interrupt
	// bit 2 CCP1IE: CCP1 Interrupt Enable bit
	//	1 = Enables the CCP1 interrupt
	//	0 = Disables the CCP1 interrupt
	// bit 1 TMR2IE: Timer2 to PR2 Match Interrupt Enable bit
	//	1 = Enables the Timer2 to PR2 match interrupt
	//	0 = Disables the Timer2 to PR2 match interrupt
	// bit 0 TMR1IE: Timer1 Overflow Interrupt Enable bit
	//	1 = Enables the Timer1 overflow interrupt
	//	0 = Disables the Timer1 overflow interrupt
	PIE1 = 0b00000001; // SSPはポーリングするので割り込みは無効、タイマー１有効

	// ペリフェラルインタラプトリクエスト
	// bit 7 Unimplemented: Read as ‘0’
	// bit 6 ADIF: A/D Converter Interrupt Flag bit
	//	1 = A/D conversion complete (must be cleared in software)
	//	0 = A/D conversion has not completed or has not been started
	// bit 5 RCIF: EUSART Receive Interrupt Flag bit
	//	1 = The EUSART receive buffer is full (cleared by reading RCREG)
	//	0 = The EUSART receive buffer is not full
	// bit 4 TXIF: EUSART Transmit Interrupt Flag bit
	//	1 = The EUSART transmit buffer is empty (cleared by writing to TXREG)
	//	0 = The EUSART transmit buffer is full
	// bit 3 SSPIF: Master Synchronous Serial Port (MSSP) Interrupt Flag bit
	//	1 = The MSSP interrupt condition has occurred, and must be cleared in software before returning from the
	//		Interrupt Service Routine. The conditions that will set this bit are:
	//	 SPI
	//	  A transmission/reception has taken place
	//	 I2 C Slave/Master
	//	  A transmission/reception has taken place
	//	 I2 C Master
	//	  The initiated Start condition was completed by the MSSP module
	//	  The initiated Stop condition was completed by the MSSP module
	//	  The initiated restart condition was completed by the MSSP module
	//	  The initiated Acknowledge condition was completed by the MSSP module
	//	  A Start condition occurred while the MSSP module was idle (Multi-master system)
	//	  A Stop condition occurred while the MSSP module was idle (Multi-master system)
	//	0 = No MSSP interrupt condition has occurred
	// bit 2 CCP1IF: CCP1 Interrupt Flag bit
	//	Capture mode:
	//	 1 = A TMR1 register capture occurred (must be cleared in software)
	//	 0 = No TMR1 register capture occurred
	//	Compare mode:
	//	1 = A TMR1 register compare match occurred (must be cleared in software)
	//	 0 = No TMR1 register compare match occurred
	//	PWM mode:
	//	 Unused in this mode
	// bit 1 TMR2IF: Timer2 to PR2 Interrupt Flag bit
	//	1 = A Timer2 to PR2 match occurred (must be cleared in software)
	//	0 = No Timer2 to PR2 match occurred
	// bit 0 TMR1IF: Timer1 Overflow Interrupt Flag bit
	//	1 = The TMR1 register overflowed (must be cleared in software)
	//	0 = The TMR1 register did not overflow
	PIR1 &= 0b11110110; // TMR1割り込みクリア,SSPIFクリア

	// 割り込み設定
	// bit 7 GIE: Global Interrupt Enable bit
	//	1 = Enables all unmasked interrupts
	//	0 = Disables all interrupts
	// bit 6 PEIE: Peripheral Interrupt Enable bit
	//	1 = Enables all unmasked peripheral interrupts
	//	0 = Disables all peripheral interrupts
	// bit 5 T0IE: Timer0 Overflow Interrupt Enable bit
	//	1 = Enables the Timer0 interrupt
	//	0 = Disables the Timer0 interrupt
	// bit 4 INTE: INT External Interrupt Enable bit
	//	1 = Enables the INT external interrupt
	//	0 = Disables the INT external interrupt
	// bit 3 RBIE: PORTB Change Interrupt Enable bit(1)
	//	1 = Enables the PORTB change interrupt
	//	0 = Disables the PORTB change interrupt
	// bit 2 T0IF: Timer0 Overflow Interrupt Flag bit(2)
	//	1 = TMR0 register has overflowed (must be cleared in software)
	//	0 = TMR0 register did not overflow
	// bit 1 INTF: INT External Interrupt Flag bit
	//	1 = The INT external interrupt occurred (must be cleared in software)
	//	0 = The INT external interrupt did not occur
	// bit 0 RBIF: PORTB Change Interrupt Flag bit
	//	1 = When at least one of the PORTB general purpose I/O pins changed state (must be cleared in
	//		software)
	//	0 = None of the PORTB general purpose I/O pins have changed state
	INTCON = 0b11100000; // グローバル・ペリフェラル割り込み有効化、タイマー0のオーバーフロー割り込みを有効化

}

//--------------
/// フレームワーク
//--------------
/// コールバック処理チェインの実行
//void DoCallbackChain(t_callback_chain* root_)
//{
//	t_callback_chain* callback = root_;
//	while(callback != NIL)
//	{
//		(*(callback->cur))();
//		callback = callback->next;
//	}
//}
//void DoCallbackChain(volatile t_callback root_[])

#define DoCallbackChain(root_) \
{\
	unsigned char i = 0;\
	while(root_[i] != NIL)\
	{\
		if ((*(root_[i]))(0) == FALSE)\
		{\
			unsigned char j;\
			for (j= i; j<3; ++j)\
			{\
				root_[j] = root_[j+1];\
			}\
		}\
		else\
		{\
			break;\
		}\
	}\
}
	
//--------------
/// ニキシー管制御
/// 
/// Nixie_ChangeDigitで表示桁(表示なしもあり)を選択
/// Nixie_ChangeNumberで表示する数字を選択（ただし表示桁が2,5の場合はコロンなので表示に影響はしない）
/// Nixie_ChangeColonでコロン(2,5桁の場合)および、ドット(2,5桁以外の場合)を制御
//--------------

/// 現在桁を変更する
void Nixie_ChangeDigit(e_digit digit_)
{
	// 表示をON
	switch (digit_)
	{
		case DIGIT_OFF:
			PIN_DIGIT_0 = 0;
			PIN_DIGIT_1 = 0;
			PIN_DIGIT_2 = 0;
			PIN_DIGIT_3 = 0;
			PIN_DIGIT_4 = 0;
			PIN_DIGIT_5 = 0;
			PIN_COLON_L = 0;
			PIN_COLON_R = 0;
			break;
		case DIGIT_0:
			PIN_DIGIT_0 = 1;
			PIN_DIGIT_1 = 0;
			PIN_DIGIT_2 = 0;
			PIN_DIGIT_3 = 0;
			PIN_DIGIT_4 = 0;
			PIN_DIGIT_5 = 0;
			PIN_COLON_L = 0;
			PIN_COLON_R = 0;
			break;
		case DIGIT_1:
			PIN_DIGIT_0 = 0;
			PIN_DIGIT_1 = 1;
			PIN_DIGIT_2 = 0;
			PIN_DIGIT_3 = 0;
			PIN_DIGIT_4 = 0;
			PIN_DIGIT_5 = 0;
			PIN_COLON_L = 0;
			PIN_COLON_R = 0;
			break;
		case DIGIT_2:
			PIN_DIGIT_0 = 0;
			PIN_DIGIT_1 = 0;
			PIN_DIGIT_2 = 1;
			PIN_DIGIT_3 = 0;
			PIN_DIGIT_4 = 0;
			PIN_DIGIT_5 = 0;
			PIN_COLON_L = 0;
			PIN_COLON_R = 0;
			break;
		case DIGIT_3:
			PIN_DIGIT_0 = 0;
			PIN_DIGIT_1 = 0;
			PIN_DIGIT_2 = 0;
			PIN_DIGIT_3 = 1;
			PIN_DIGIT_4 = 0;
			PIN_DIGIT_5 = 0;
			PIN_COLON_L = 0;
			PIN_COLON_R = 0;
			break;
		case DIGIT_4:
			PIN_DIGIT_0 = 0;
			PIN_DIGIT_1 = 0;
			PIN_DIGIT_2 = 0;
			PIN_DIGIT_3 = 0;
			PIN_DIGIT_4 = 1;
			PIN_DIGIT_5 = 0;
			PIN_COLON_L = 0;
			PIN_COLON_R = 0;
			break;
		case DIGIT_5:
			PIN_DIGIT_0 = 0;
			PIN_DIGIT_1 = 0;
			PIN_DIGIT_2 = 0;
			PIN_DIGIT_3 = 0;
			PIN_DIGIT_4 = 0;
			PIN_DIGIT_5 = 1;
			PIN_COLON_L = 0;
			PIN_COLON_R = 0;
			break;
		case DIGIT_COLON_L:
			PIN_DIGIT_0 = 0;
			PIN_DIGIT_1 = 0;
			PIN_DIGIT_2 = 0;
			PIN_DIGIT_3 = 0;
			PIN_DIGIT_4 = 0;
			PIN_DIGIT_5 = 0;
			PIN_COLON_L = 1;
			PIN_COLON_R = 0;
			break;
		case DIGIT_COLON_R:
			PIN_DIGIT_0 = 0;
			PIN_DIGIT_1 = 0;
			PIN_DIGIT_2 = 0;
			PIN_DIGIT_3 = 0;
			PIN_DIGIT_4 = 0;
			PIN_DIGIT_5 = 0;
			PIN_COLON_L = 0;
			PIN_COLON_R = 1;
			break;
		default:
			;
	}
}

/// 数字選択用ライン出力を変更
void Nixie_ChangeNumber(unsigned char number_)
{
	if (number_ > sizeof(NUM_2_LINE)/sizeof(NUM_2_LINE[0]))
	{
		return;
	}
	
	{
		unsigned char line_hex;
		line_hex = NUM_2_LINE[number_];
		
		// 数字を変更
		PIN_LINDECODER_0 = ((line_hex >> 0) & 1);
		PIN_LINDECODER_1 = ((line_hex >> 1) & 1);
		PIN_LINDECODER_2 = ((line_hex >> 2) & 1);
		PIN_LINDECODER_3 = ((line_hex >> 3) & 1);
	}
}

/// コロン表示用のトランジスタON/OFF
void Nixie_ChangeColon( unsigned char colon_flag_)
{
	PIN_TRANSISTOR_0 = colon_flag_ & 0x1;
	PIN_TRANSISTOR_1 = (colon_flag_>>1) & 0x1;
}

//--------------
/// RTC処理
///
/// 時計をセットする手順
/// 1) Rtc_UpdatePicTimerByRtc();　// Rtc上の正確な時間を取得
/// 2) // グローバル変数変更
/// 3) Rtc_UpdateRtcByPicTimer(); // Rtcに更新した時間情報を送信
//--------------


// グローバル変数で管理しているPIC上の時間情報をRtcに反映
void Rtc_UpdateRtcByPicTimer()
{
	// stop bitを0
	// RTC書き込み
	// stop bitを1
	// 
	// 調べた範囲だとStopビットを変更がいつ有効化されるかわからなかったので、
	// RTC書き込みはStopConditionを送信してから再度通信をして行うことにした。
 
	// stop
	I2c_Start();
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE); // 書き込みモード
	I2c_Write(0x00); // configアドレス
	I2c_Write(0x20); // STOP=1
	I2c_Write(0x00); // 
	I2c_Stop();
	
	// write
	I2c_Start();
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE); // 書き込みモード
	I2c_Write(0x02);
	I2c_Write(g_second); // 秒の値 0-59
	I2c_Write(g_minute); // 分の値 0-59
	I2c_Write(g_hour);	 // 時の値 0-23
	//I2c_Write(g_date);	 // 日の値 1-31
	//I2c_Write(g_week);	 // 曜の値 日月火水木金土 0123456
	//I2c_Write(0x80|g_month);  // 月の値 (C:MSB)1-12	  Cは1のとき21世紀
	//I2c_Write(g_year);	 // 年の値 00-99
	//I2c_Write(0x80); // AE=1
	//I2c_Write(0x80); // AE=1
	//I2c_Write(0x80); // AE=1
	//I2c_Write(0x80); // AE=1
	I2c_Stop();

	// STOP=0 で時計を開始する
	I2c_Start();
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE); // 書き込みモード
	I2c_Write(0x00); // configアドレス
	I2c_Write(0x00); // STOP=0
	I2c_Stop();
}

// RTCのクロック値を読み出す
void Rtc_UpdatePicTimerByRtc()
{
	I2c_Start();
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE);
	I2c_Write(0x02); // 秒のアドレス
	I2c_RepeatStart();
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_READ);
	g_second = I2c_Read(1); // 秒の値 0-59
	g_minute = I2c_Read(1); // 分の値 0-59
	g_hour	 = I2c_Read(1); // 時の値 0-23
	g_date	 = I2c_Read(1); // 日の値 1-31
	g_week	 = I2c_Read(1); // 曜の値 日月火水木金土 0123456
	g_month	 = I2c_Read(1); // 月の値 (C:MSB)1-12	Cは1のとき21世紀
	g_year	 = I2c_Read(1); // 年の値 00-99
	g_alarm_minute = I2c_Read(1); // アラーム
	g_alarm_hour   = I2c_Read(0); // アラーム
	I2c_Stop();

	// 有効ビットの取り出し
	g_second  &= 0x7f;
	g_minute  &= 0x7f;
	g_hour	  &= 0x3f;
	g_date	  &= 0x3f;
	g_week	  &= 0x07;
	g_month	  &= 0x1f;
	// g_year	  &= 0xff; 年は全ビット有効なのでマスクしない
	// g_century = g_month & 0x80 ? 21 : 20;
	g_alarm_minute &= 0x7f;
	g_alarm_hour &= 0x3f;
}

/// 初期化、起動時に一度だけコールしてRtcのコンフィグを設定する。
/// Rtcの立ち上がりに700ms必要なので内部でwaitする
void Rtc_Init()
{	  
	unsigned char vl_bit;

	// 立ち上がりを1000ms待つ必要がある
	Wait10ms(100);

	// VLビットをチェック
	I2c_Start();
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE);
	I2c_Write(0x02); // 秒のアドレス
	I2c_RepeatStart(); // 読み出しの場合はrepeatする
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_READ);
	vl_bit = (I2c_Read(0) >> 7) & 1; // バックアップ異常検出VL
	I2c_Stop();

	// 異常があれば初期化する
	if (vl_bit)
	{
		// エラー通知の後、初期化
		Beep(1);

		// 初期化
		I2c_Start();
		I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE);	 // 書き込みモード
		I2c_Write(0x00); // control0のアドレス
		I2c_Write(0x0); // test=0, STOP=0
		I2c_Write(0x0); // AIE=TIE=0
		I2c_Stop();
		Rtc_UpdateRtcByPicTimer();
	}
}

// 暫定
void Rtc_UpdateRtcCalendar()
{
	// stop bitを0
	// RTC書き込み
	// stop bitを1
	// 
	// 調べた範囲だとStopビットを変更がいつ有効化されるかわからなかったので、
	// RTC書き込みはStopConditionを送信してから再度通信をして行うことにした。
 
	// stop
	//I2c_Start();
	//I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE); // 書き込みモード
	//I2c_Write(0x00); // configアドレス
	//I2c_Write(0x20); // STOP=1
	//I2c_Write(0x00);
	//I2c_Stop();
	
	// write
	I2c_Start();
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE); // 書き込みモード
	I2c_Write(0x05);
	I2c_Write(g_date);	 // 日の値 1-31
	I2c_Write(g_week);	 // 曜の値 日月火水木金土 0123456
	I2c_Write(0x80|g_month);  // 月の値 (C:MSB)1-12	  Cは1のとき21世紀
	I2c_Write(g_year);	 // 年の値 00-99
	I2c_Stop();

	// STOP=0 で時計を開始する
	//I2c_Start();
	//I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE); // 書き込みモード
	//I2c_Write(0x00); // configアドレス
	//I2c_Write(0x00); // STOP=0
	//I2c_Stop();

}

// 暫定
Rtc_UpdateRtcAlarm()
{
	// stop bitを0
	// RTC書き込み
	// stop bitを1
	// 
	// 調べた範囲だとStopビットを変更がいつ有効化されるかわからなかったので、
	// RTC書き込みはStopConditionを送信してから再度通信をして行うことにした。
 
	// stop
	//I2c_Start();
	//I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE); // 書き込みモード
	//I2c_Write(0x00); // configアドレス
	//I2c_Write(0x20); // STOP=1
	//I2c_Write(0x00);
	//I2c_Stop();
	
	// write
	I2c_Start();
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE); // 書き込みモード
	I2c_Write(0x09);
	I2c_Write(0x80 | g_alarm_minute); // アラームは記憶させるだけ、最上位ビットを０にすればアラーム信号も出ない設定けどどっちにしろ使わないので適当
	I2c_Write(0x80 | g_alarm_hour);   // アラームは記憶させるだけ
	I2c_Write(0x80); // AE=1
	I2c_Write(0x80); // AE=1
	I2c_Stop();

	// STOP=0 で時計を開始する
	//I2c_Start();
	//I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE); // 書き込みモード
	//I2c_Write(0x00); // configアドレス
	//I2c_Write(0x00); // STOP=0
	//I2c_Stop();
}

// 暫定
// RTCのクロック値を読み出す
unsigned char Rtc_GetSecond()
{
	unsigned char second;

	I2c_Start();
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_WRITE);
	I2c_Write(0x02); // 秒のアドレス
	I2c_RepeatStart();
	I2c_Write(I2C_SLAVE_ADDRESS_RTC|I2C_READ);
	second = I2c_Read(0); // 秒の値 0-59
	I2c_Stop();

	// 有効ビットの取り出し
	return second & 0x7f;
}

//--------------
// 便利関数
//--------------

//
void ClearColon()
{
	g_disp_colon[0] = 0;
	g_disp_colon[1] = 0;
	//g_disp_colon[2] = 0;
	g_disp_colon[3] = 0;
	g_disp_colon[4] = 0;
	//g_disp_colon[5] = 0;
	g_disp_colon[6] = 0;
	g_disp_colon[7] = 0;
}


//--------------
// 表示処理
//--------------
e_bool Disp_CallbackOff()
{
	Nixie_ChangeDigit(DIGIT_OFF);
	return FALSE;
}

e_bool Disp_CallbackNormal()
{
	static unsigned char digit= 0;
	static unsigned char state = 0;
	unsigned char brightness_threshold;
	unsigned char brightness_max;

	// 点灯制御
	switch (state)
	{
		case 0:
			// 消灯区間
			Nixie_ChangeDigit(DIGIT_OFF);
		
			// 次の表示へ
			++digit;
			if (digit >= DIGIT_OFF)
				digit = DIGIT_0;
	
			// トランジションの調整
			if (0xff-TRANSITION_RATE > g_disp_transition[digit])
				g_disp_transition[digit] += TRANSITION_RATE;
			else
				g_disp_transition[digit] = 0xff;

			// 次回コールバックまでの時間を調整
			TMR0 = g_cds_adval > 32 ? g_cds_adval - 32 : g_cds_adval;
			// TMR0 = DYNAMIC_OFF_TIMER;

			// 次ステート
			state = 1;

			break;
		case 1:
			// ”今”の数字を点灯
			Nixie_ChangeNumber(g_disp[digit]);
			Nixie_ChangeColon(g_disp_colon[digit]);

			if (g_disp_flag & 1<<digit)
				Nixie_ChangeDigit(digit);
			else
				Nixie_ChangeDigit(DIGIT_OFF);
				

			// 明るさの下限を決定する
			if ((digit == 2)
			||  (digit == 5))
			{
				brightness_threshold = BRIGHTNESS_LOWER_THRESHOLD_NEON;

				// todo コロンの場合はとりあえず点滅するだけ。
				// TMR0 = 0x0;
				state = 0;
				break;
			}
			else
			{
				brightness_threshold = BRIGHTNESS_LOWER_THRESHOLD_NIXIE;
			}
			if (brightness_threshold > g_cds_adval)
			{
				brightness_max = brightness_threshold;
			}
			else
			{
				brightness_max = g_cds_adval;
			}


			// 明るさを決定する
			// 上限の場合はトランジション終了している
			if (brightness_max > g_disp_transition[digit])
			{
				TMR0 = 0xff - g_disp_transition[digit];
				state = 2;
			}
			else
			{
				TMR0 = 0xff - brightness_max;
				g_disp_transition[digit] == 0xff;
				state = 0;
			}

			break;
		case 2:
			// ”以前”の数字を点灯
			Nixie_ChangeNumber(g_disp_prev[digit]);
			Nixie_ChangeColon(g_disp_colon_prev[digit]);

			// 明るさの下限を決定する
			if ((digit == 2)
			||  (digit == 5))
			{
				brightness_threshold = BRIGHTNESS_LOWER_THRESHOLD_NEON;
			}
			else
			{
				brightness_threshold = BRIGHTNESS_LOWER_THRESHOLD_NIXIE;
			}

			// 明るさの上限を決定する
			if (brightness_threshold > g_cds_adval)
			{
				brightness_max = brightness_threshold;
			}
			else
			{
				brightness_max = g_cds_adval;
			}

			// 明るさを決定する
			// 上限の場合はトランジション終了している
			if (brightness_max > g_disp_transition[digit])
			{
				TMR0 = 0xff - (brightness_max - g_disp_transition[digit]);
			}

			state = 0;

			break;
		default:
			;
	}

	return TRUE;
}

// g_disp_flag 似合わせて点滅する
e_bool State_CallbackBlink(unsigned char button_no_)
{
	static unsigned char devide_counter = 0;

	++devide_counter;
	if (devide_counter > 8)
	{
		devide_counter = 0;
	}

	if (devide_counter < 4)
	{
		g_disp_flag |= g_blink_digits;
	}
	else
	{
		g_disp_flag = g_disp_flag & (~g_blink_digits);
	}

	return TRUE;
}

// 一秒間ランダム表示してFALSEを返す
e_bool State_CallbackRandomDisp(unsigned char button_no_)
{
	static unsigned char counter = 0;
	
	// それっぽく
	unsigned char val = (unsigned char)rand() % 10;

	g_disp_flag = 0b11111111;
	g_disp[0] = val + 7 > 9 ? val + 7 - 10 : val + 7;
	g_disp[1] = val + 3 > 9 ? val + 3 - 10 : val + 3;
	g_disp[3] = val + 1 > 9 ? val + 1 - 10 : val + 1;
	g_disp[4] = val + 6 > 9 ? val + 6 - 10 : val + 6;
	g_disp[6] = val + 2 > 9 ? val + 2 - 10 : val + 2;
	g_disp[7] = val + 8 > 9 ? val + 8 - 10 : val + 8;

	g_disp_colon[2] = 0x3;
	g_disp_colon[5] = 0x3;

	++counter;
	if (counter == 30)
	{
		counter = 0;
		return FALSE;
	}

	//TMR1L = TMR1L_1SEC_OFFSET_DEFAULT;
	TMR1H = 0xa0;
	
	return TRUE;
}

// 非表示モード
e_bool State_CallbackDispoff(unsigned char button_no_)
{
	unsigned char i;

	g_disp_flag = 0b00000000;

	for (i=0; i<8;++i)
	{
		g_disp_colon_prev[i] = 0;
		g_disp_colon[i] = 0;
		g_disp_prev[i] = 0;
		g_disp[i] = 0;
		g_disp_transition[i] = 0x0;
	}
	return TRUE;
}

// 次の表示に右移動する
e_bool State_CallbackRightChange(unsigned char button_no_)
{
	const unsigned char NUM_COLON = 6;
	static unsigned char g_time = 0;
	unsigned char i;

	++g_time;
	if ((g_time == 2)
	||  (g_time == 5)
	||  (g_time == 2 + NUM_COLON)
	||  (g_time == 5 + NUM_COLON))
	{
		++g_time;
	}

	if (g_time >= NUM_COLON+8)
	{
		// done
		g_time = 0;
		for (i=0; i<8;++i)
		{
			g_disp_colon_prev[i] = 0;
			g_disp_colon[i] = 0;
			g_disp_prev[i] = 0;
		}
		return FALSE;
	}

	if (g_time < NUM_COLON)
	{
		g_disp[0] = 10;
		g_disp_colon[0] = 3;
	}
	else
	{
		g_disp[0] = g_disp_capture[7 - (g_time - NUM_COLON)];
		g_disp_colon[0] = g_disp_colon_capture[7 - (g_time - NUM_COLON)];
	}
	
	g_disp[1] = g_disp_prev[0];
	g_disp[3] = g_disp_prev[1];
	g_disp[4] = g_disp_prev[3];
	g_disp[6] = g_disp_prev[4];
	g_disp[7] = g_disp_prev[6];
	g_disp_colon[1] = g_disp_colon_prev[0];
	g_disp_colon[3] = g_disp_colon_prev[1];
	g_disp_colon[4] = g_disp_colon_prev[3];
	g_disp_colon[6] = g_disp_colon_prev[4];
	g_disp_colon[7] = g_disp_colon_prev[6];
		
	for (i = 0; i<8; ++i)
	{
		g_disp_prev[i] = g_disp[i];
		g_disp_colon_prev[i] = g_disp_colon[i];
	}
	
	return TRUE;
}

// 次の表示に左移動する
e_bool State_CallbackLeftChange(unsigned char button_no_)
{
	const unsigned char NUM_COLON = 6;
	static unsigned char g_time = 0;
	unsigned char i;

	++g_time;
	if ((g_time == 2)
	||  (g_time == 5)
	||  (g_time == 2 + NUM_COLON)
	||  (g_time == 5 + NUM_COLON))
	{
		++g_time;
	}

	if (g_time >= NUM_COLON+8)
	{
		// done
		g_time = 0;
		for (i=0; i<8;++i)
		{
			g_disp_colon_prev[i] = 0;
			g_disp_colon[i] = 0;
			g_disp_prev[i] = 0;
		}
		return FALSE;
	}

	if (g_time < NUM_COLON)
	{
		g_disp[7] = 10;
		g_disp_colon[7] = 3;
	}
	else
	{
		g_disp[7] = g_disp_capture[g_time - NUM_COLON];
		g_disp_colon[7] = g_disp_colon_capture[g_time - NUM_COLON];
	}
	
	g_disp[6] = g_disp_prev[7];
	g_disp[4] = g_disp_prev[6];
	g_disp[3] = g_disp_prev[4];
	g_disp[1] = g_disp_prev[3];
	g_disp[0] = g_disp_prev[1];
	g_disp_colon[6] = g_disp_colon_prev[7];
	g_disp_colon[4] = g_disp_colon_prev[6];
	g_disp_colon[3] = g_disp_colon_prev[4];
	g_disp_colon[1] = g_disp_colon_prev[3];
	g_disp_colon[0] = g_disp_colon_prev[1];
		
	for (i = 0; i<8; ++i)
	{
		g_disp_prev[i] = g_disp[i];
		g_disp_colon_prev[i] = g_disp_colon[i];
	}
	
	return TRUE;
}


e_bool State_CallbackClock(unsigned char button_no_)
{
	static unsigned char colon_on = 0;
	static unsigned char devide_counter = 0;

	g_disp_flag = 0b11111111;

	// 
	TMR1L = timer1l_1sec_offset;
	TMR1H = timer1h_1sec_offset;

	//
	++devide_counter;
	if (devide_counter >= TMR1_SOFT_PRESCALER)
	{
		devide_counter = 0;
		
		// BCD形式で時間更新
		g_disp_prev[0] = LO(g_second);
		g_disp_transition[0] = 0;
		g_second += 0x01;
		if (LO(g_second) >= 10)
		{
			g_disp_prev[1] = HI(g_second);
			g_disp_transition[1] = 0;
			g_second &= 0xf0;
			g_second += 0x10;
			if (HI(g_second) >= 6)
			{
				g_disp_prev[3] = LO(g_minute);
				g_disp_transition[3] = 0;
				g_second = 0;
				g_minute += 0x01;
				if (LO(g_minute) >= 10)
				{				
					g_disp_prev[4] = HI(g_minute);
					g_disp_transition[4] = 0;
					g_minute &= 0xf0;
					g_minute += 0x10;
					if (HI(g_minute)  >= 6)
					{
						g_disp_prev[6] = LO(g_hour);
						g_disp_transition[6] = 0;
						g_minute = 0;
						g_hour += 0x01;
						if(HI(g_hour) < 2 && LO(g_hour) >= 10)
						{
							g_disp_prev[0] = HI(g_hour);
							g_disp_transition[7] = 0;
							g_hour &= 0xf0;
							g_hour += 0x10;
						}
						else if (HI(g_hour) >= 2 && LO(g_hour) >= 4)
						{
							g_disp_prev[0] = HI(g_hour);
							g_disp_transition[7] = 0;
							g_hour = 0;
						}
					}
				}
				// 1分に一度タイマー速度調整を行う
				g_next_timer1_ajustment = 1;
			}
		}
		
		// 時計調整のデバッグ
		//g_disp[4] = timer1l_1sec_offset % 10;
		//g_disp[6] = (timer1l_1sec_offset / 10) % 10;
		//g_disp[7] = (timer1l_1sec_offset / 100) % 10;
		//g_disp[0] = timer1l_1sec_offset % 10;
		//g_disp[1] = (timer1l_1sec_offset / 10) % 10;
		//g_disp[3] = (timer1l_1sec_offset / 100) % 10;
		//g_disp[0] = g_auto_off_count % 10;
		//g_disp[1] =  (g_auto_off_count/ 10) % 10;
		//g_disp[3] =(g_auto_off_count / 100) % 10;
		
		// 表示データセット
		g_disp[4] = (g_minute>>4) & 0xf;
		g_disp[6] = g_hour & 0xf;
		g_disp[7] = (g_hour>>4) & 0xf;
		g_disp[0] = g_second & 0xf ;
		g_disp[1] = (g_second>>4) & 0xf;
		g_disp[3] = g_minute & 0xf;
	}
	else if (devide_counter == CLOCK_COLON_BLINK_OFFSET)
	{
		// コロンの点滅
		g_disp_colon_prev[2] = colon_on;
		g_disp_colon_prev[5] = colon_on;
		colon_on = colon_on == 0 ? 0b11 : 0;
		g_disp_colon[2] = colon_on;
		g_disp_colon[5] = colon_on;
		g_disp_transition[2] = 0;
		g_disp_transition[5] = 0;
	}

	return TRUE;
}

void SetConfigDigits(unsigned char blink_, unsigned char max_, unsigned char min_, volatile unsigned char* target_)
{
	g_blink_digits = blink_;
	g_target_max = max_;
	g_target_min = min_;
	g_target_bcd_ptr = target_;
}

e_bool State_CallbackSetClock(unsigned char button_no_)
{
	static unsigned char state = 0;
	static unsigned char prev_press = 1;
	unsigned char push = 0;
	g_disp_flag = 0b11111111;

	if (PIN_BUTTON_2 == 0)
	{
		if (prev_press == 0)
		{
			push = 1;
		}
		else
		{
			push = 0;
		}
		prev_press = 1;
	}
	else
	{
		push = 0;
		prev_press = 0;
	}

	
	switch(state)
	{
		case 0:
			SetConfigDigits(0b11000000, 23, 0, &g_hour);
			state = 1;
			break;			
		case 1:
			if (push != 0)
			{
				SetConfigDigits(0b00011000, 59, 0, &g_minute);
				state = 2;
			}
			break;
		case 2:
			if (push != 0)
			{
				SetConfigDigits(0b00000011, 59, 0, &g_second);
				state = 3;
			}
			break;
		case 3:
			if (push != 0)
			{
				state = 0;
				Rtc_UpdateRtcByPicTimer();
				g_timer1_callback[0] = State_CallbackRandomDisp;
				g_timer1_callback[1] = State_CallbackClock;
				g_timer1_callback[2] = NIL;
				g_state_current = &g_state_clock;
			}
			break;
		default :
			return FALSE;
	}

	g_disp[0] = LO(g_second);
	g_disp[1] = HI(g_second);
	g_disp[3] = LO(g_minute);
	g_disp[4] = HI(g_minute);
	g_disp[6] = LO(g_hour);
	g_disp[7] = HI(g_hour);
	g_disp_colon[2] = 0b11;
	g_disp_colon[5] = 0b11;
	ClearColon();

	State_CallbackBlink(button_no_);
	return TRUE;
}

// カレンダーモード
e_bool State_CallbackCalendar(unsigned char button_no_)
{
	g_disp_flag = 0b11111111;
	g_disp[0] = LO(g_date);
	g_disp[1] = HI(g_date);
	g_disp[3] = LO(g_month);
	g_disp[4] = HI(g_month);
	g_disp[6] = LO(g_year);
	g_disp[7] = HI(g_year);
	ClearColon();

	g_disp_colon[2] = 0b11;
	g_disp_colon[5] = 0b11;

	return TRUE;
}


e_bool State_CallbackSetCalendar(unsigned char button_no_)
{
	static unsigned char state = 0;
	static unsigned char prev_press = 1;
	unsigned char push = 0;
	g_disp_flag = 0b11111111;

	if (PIN_BUTTON_2 == 0)
	{
		if (prev_press == 0)
		{
			push = 1;
		}
		else
		{
			push = 0;
		}
		prev_press = 1;
	}
	else
	{
		push = 0;
		prev_press = 0;
	}

	
	switch(state)
	{
		case 0:
			SetConfigDigits(0b11000000, 99, 0, &g_year);
			state = 1;
			break;
		case 1:
			if (push != 0)
			{
				SetConfigDigits(0b00011000, 12, 1, &g_month);
				state = 2;
			}
			break;
		case 2:
			if (push != 0)
			{
				SetConfigDigits(0b00000011, 31, 1, &g_date);
				state = 3;
			}
			break;
		case 3:
			if (push != 0)
			{
				state = 0;
				Rtc_UpdateRtcCalendar();
				g_timer1_callback[0] = State_CallbackRandomDisp;
				g_timer1_callback[1] = State_CallbackCalendar;
				g_timer1_callback[2] = NIL;
				g_state_current = &g_state_calendar;
			}
			break;
		default :
			return FALSE;
	}

	g_disp[0] = LO(g_date);
	g_disp[1] = HI(g_date);
	g_disp[3] = LO(g_month);
	g_disp[4] = HI(g_month);
	g_disp[6] = LO(g_year);
	g_disp[7] = HI(g_year);
	g_disp_colon[2] = 0b11;
	g_disp_colon[5] = 0b11;
	ClearColon();

	State_CallbackBlink(button_no_);

	return TRUE;
}

e_bool State_CallbackAlarm(unsigned char button_no_)
{
	static unsigned char state = 0;
	g_disp_flag = 0b11111111;

	g_disp[0] = 0;
	g_disp[1] = 0;
	g_disp[3] = LO(g_alarm_minute);
	g_disp[4] = HI(g_alarm_minute);
	g_disp[6] = LO(g_alarm_hour);
	g_disp[7] = HI(g_alarm_hour);
	g_disp_colon[2] = 0b11;
	g_disp_colon[5] = 0b11;
	ClearColon();
	
	switch (state)
	{
	case 0:
		g_disp_colon[2] = 1;
		g_disp_colon[5] = 0;
		state = 1;
		break;
	case 1:
		g_disp_colon[2] = 2;
		g_disp_colon[5] = 0;
		state = 2;
		break;
	case 2:
		g_disp_colon[2] = 0;
		g_disp_colon[5] = 2;
		state = 3;
		break;
	case 3:
		g_disp_colon[2] = 0;
		g_disp_colon[5] = 1;
		state = 0;
		break;
	}

	return TRUE;
}



e_bool State_CallbackSetAlarm(unsigned char button_no_)
{
	static unsigned char state = 0;
	static unsigned char prev_press = 1;
	unsigned char push = 1;
	g_disp_flag = 0b11111100;

	if (PIN_BUTTON_2 == 0)
	{
		if (prev_press == 0)
		{
			push = 1;
		}
		else
		{
			push = 0;
		}
		prev_press = 1;
	}
	else
	{
		push = 0;
		prev_press = 0;
	}

	
	switch(state)
	{
		case 0:
			SetConfigDigits(0b11000000, 23, 0, &g_alarm_hour);
			state = 1;
			break;
		case 1:
			if (push != 0)
			{
				SetConfigDigits(0b00011000, 59, 0, &g_alarm_minute);
				state = 2;
			}
			break;
		case 2:
			if (push != 0)
			{
				state = 0;
				Rtc_UpdateRtcAlarm();
				g_timer1_callback[0] = State_CallbackRandomDisp;
				g_timer1_callback[1] = State_CallbackAlarm;
				g_timer1_callback[2] = NIL;
				g_state_current = &g_state_alarm;
			}
			break;
		default :
			return FALSE;
	}

	g_disp[0] = 0;
	g_disp[1] = 0;
	g_disp[3] = LO(g_alarm_minute);
	g_disp[4] = HI(g_alarm_minute);
	g_disp[6] = LO(g_alarm_hour);
	g_disp[7] = HI(g_alarm_hour);
	g_disp_colon[2] = 0b11;
	g_disp_colon[5] = 0b11;
	ClearColon();

	State_CallbackBlink(button_no_);

	return TRUE;
}


// 温度計モード
e_bool State_CallbackTemparature(unsigned char button_no_)
{
	g_disp_flag = 0b11111111;

	g_disp[0] = 10;
	g_disp[1] = (g_temparature_fractional >> 4) & 0xf;
	g_disp[3] = g_temparature_integer & 0xf;
	g_disp[4] = (g_temparature_integer >> 4) & 0xf;;
	g_disp[6] = 10;
	g_disp[7] = 10;
	g_disp_colon[0] = 0b11;
	g_disp_colon[1] = 0;
	g_disp_colon[2] = 0b01;
	g_disp_colon[3] = 0;
	g_disp_colon[4] = 0;
	g_disp_colon[5] = 0;
	g_disp_colon[6] = 0b11;
	g_disp_colon[7] = 0b11;

	// 次のコールバックまでの時間を調整
	//TMR1L = TMR1L_1SEC_OFFSET;
	//TMR1H = TMR1H_1SEC_OFFSET;

	return TRUE;
}

e_bool State_CallbackConfig(unsigned char button_no_)
{
	static unsigned char state = 0;
	static unsigned char count = 0;

	g_disp_flag = 0b11111111;

	ClearColon();
	g_disp_colon[0] = 0b11;
	g_disp_colon[1] = 0b11;
	g_disp_colon[2] = 0b0;
	g_disp_colon[3] = 0b11;
	g_disp_colon[4] = 0b11;
	g_disp_colon[5] = 0b0;
	g_disp_colon[6] = 0b11;
	g_disp_colon[7] = 0b11;

	g_disp[0] = g_cds_1th_place;
	g_disp[1] = g_cds_10th_place;
	//g_disp[2] = 0;
	g_disp[3] = g_cds_100th_place;
	g_disp[4] = g_config_auto_brightness ? 1 : 0;
	//g_disp[5] = 0;
	g_disp[6] = g_config_auto_off_enable ? 1 : 0;
	g_disp[7] = g_config_alarm_enable ? 1 : 0;

	count++;
	if (count >= 3)
	{
		count = 0;
		switch (state)
		{
		case 0:
			g_disp_colon[0] = 0;
			g_disp_colon[1] = 0b11;
			state = 1;
			break;
		case 1:
			g_disp_colon[1] = 0;
			g_disp_colon[3] = 0b11;
			state = 2;
			break;
		case 2:
			g_disp_colon[3] = 0;
			g_disp_colon[4] = 0b11;
			state = 3;
			break;
		case 3:
			g_disp_colon[4] = 0;
			g_disp_colon[6] = 0b11;
			state = 4;
			break;
		case 4:
			g_disp_colon[6] = 0;
			g_disp_colon[7] = 0b11;
			state = 5;
			break;
		case 5:
			g_disp_colon[7] = 0;
			g_disp_colon[0] = 0b11;
			state = 0;
			break;
		}
	}


	// 次のコールバックまでの時間を調整
	//TMR1L = TMR1L_1SEC_OFFSET;
	//TMR1H = TMR1H_1SEC_OFFSET;

	return TRUE;
}

e_bool State_CallbackSetConfig(unsigned char button_no_)
{
	static unsigned char state = 0;
	static unsigned char prev_press = 1;
	unsigned char push = 0;
	g_disp_flag = 0b11111111;

	if (PIN_BUTTON_2 == 0)
	{
		if (prev_press == 0)
		{
			push = 1;
		}
		else
		{
			push = 0;
		}
		prev_press = 1;
	}
	else
	{
		push = 0;
		prev_press = 0;
	}
	
	switch(state)
	{
		case 0:
			SetConfigDigits(0b10000000, 1, 0, &g_config_alarm_enable);
			state = 1;
			break;
		case 1:
			if (push != 0)
			{
				SetConfigDigits(0b01000000, 1, 0, &g_config_auto_off_enable);
				state = 2;
			}	
			break;
		case 2:
			if (push != 0)
			{
				SetConfigDigits(0b00010000, 1, 0, &g_config_auto_brightness);
				state = 3;
			}
			break;
		case 3:
			if (push != 0)
			{
				SetConfigDigits(
					0b00001011,
					BRIGHTNESS_HIGHER_MAX,
					BRIGHTNESS_LOWER_THRESHOLD_NIXIE,
					&g_config_max_brightness);
				state = 4;
			}
			break;
		case 4:
			if (push != 0)
			{
				unsigned char config;
				config = g_config_alarm_enable <<7
					| g_config_auto_off_enable <<6 
					| g_config_auto_brightness << 4;

				Eeprom_Write(EEPROM_ADR_CONFIG, config);
				Eeprom_Write(EEPROM_ADR_MAX_BRIGHTNESS, g_config_max_brightness);
				g_timer1_callback[0] = State_CallbackRandomDisp;
				g_timer1_callback[1] = State_CallbackConfig;
				g_timer1_callback[2] = NIL;
				g_state_current = &g_state_config;
				state = 0;
			}
			break;
		default :
			return FALSE;
	}

	g_disp[0] = g_cds_1th_place;
	g_disp[1] = g_cds_10th_place;
	//g_disp[2] = 0;
	g_disp[3] = g_cds_100th_place;
	g_disp[4] = g_config_auto_brightness ? 1 : 0;
	//g_disp[5] = 0;
	g_disp[6] = g_config_auto_off_enable ? 1 : 0;
	g_disp[7] = g_config_alarm_enable ? 1 : 0;
	ClearColon();

	State_CallbackBlink(button_no_);
	return TRUE;
}

e_bool Mainloop_CallbackNormal(unsigned char button_no_)
{
	static unsigned char devide_count = 0;
	static unsigned char auto_off_devide_count = 0;
	static unsigned char alarm_loop_limit = 0;

	// 分周
	++devide_count;
	if (devide_count < 10)
	{
		return TRUE;
	}
	devide_count = 0;

	// 温度
	{
		unsigned char adval = Pic_GetAd(0);
		unsigned char adval_i = adval/(100/ADVAL_2_TMPARATURE);
		unsigned char adval_f = (adval%4)*25;
		g_temparature_integer	 = (((adval_i / 10) % 10) << 4) | (adval_i % 10);
		g_temparature_fractional = (((adval_f / 10) % 10) << 4);// | (adval_f % 10);
	}
	
	// ADコンバータ放電待ちしてみる
	Wait10ms(1);

	// 暫定　時計の調整
	// 自分のタイマーで進めた時間が早ければタイマー割り込みの間隔を伸ばす
	// 個々にくるときはほぼ必ずg_second==0
	if (g_next_timer1_ajustment != 0)
	{
		g_next_timer1_ajustment = 0;

		char second = Rtc_GetSecond();
		
		if (second > 48) // 0x30
		{
			second -= 96; //0x60
		}
		
		if (second <= (char)g_second)
		{
			timer1h_1sec_offset = TMR1H_1SEC_OFFSET_DEFAULT_DOWN;
			timer1l_1sec_offset = TMR1L_1SEC_OFFSET_DEFAULT_DOWN;
		}
		else
		{
			timer1h_1sec_offset = TMR1H_1SEC_OFFSET_DEFAULT_UP;
			timer1l_1sec_offset = TMR1L_1SEC_OFFSET_DEFAULT_UP;
		}
	}

 
	// アラーム
	// この辺も適当
	if ((g_hour == g_alarm_hour)
	&&  (g_minute == g_alarm_minute)
	&&  (g_second == 0))
	{
		g_alarm_trigger = 1;
		alarm_loop_limit = 0;
	}
	if (g_alarm_trigger != 0)
	{
		++alarm_loop_limit;
		if ((alarm_loop_limit == 0xff)
		||  (g_button0_press | g_button1_press | g_button2_press))
		{
			g_alarm_trigger = 0;
			g_alarm_beep_on = 0;
		}
	}

	// 明るさ
	{
		if (g_config_auto_brightness)
		{
			g_cds_adval = Pic_GetAd(1);
			if (g_cds_adval > g_config_max_brightness)
				g_cds_adval = g_config_max_brightness;
		}
		else
		{
			g_cds_adval = g_config_max_brightness;
		}

		// 表示用の値をメインルーチン上で計算
		// Config用にmax値を表示するモードも用意
		if (g_state_current == &g_state_set_config)
		{
			g_cds_adval = g_config_max_brightness;
		}
		g_cds_100th_place = (g_cds_adval / 100) % 10;
		g_cds_10th_place  = (g_cds_adval / 10) % 10;
		g_cds_1th_place   = g_cds_adval % 10;
				
		// 暗い時間が続いたら表示を消す用
		if (g_config_auto_off_enable)
		{
			if (g_cds_adval < AUTO_OFF_THRESHOLD)
			{
				++auto_off_devide_count;
				if (auto_off_devide_count == 0x14)
				{
					auto_off_devide_count = 0;
					if (g_auto_off_count != 0xff)
					{
						++g_auto_off_count;
					}
				}

				if (g_button0_press | g_button1_press | g_button2_press)
				{
					g_auto_off_count = 0;
				}
			}
			else
			{
				g_auto_off_count = 0;
			}
		}
	}

	return TRUE;
}

//---------------
// ステートハンドラ
//---------------
void SelectTimer1Callback(unsigned char button_no_)
{
	unsigned int i;

	if (button_no_ == 0)
		g_timer1_callback[0] = State_CallbackRightChange;
	else if (button_no_ == 1)
		g_timer1_callback[0] = State_CallbackLeftChange;
	else
		g_timer1_callback[0] = State_CallbackRandomDisp;

	for (i = 0; i<8; ++i)
	{
		g_disp_prev[i] = g_disp[i];
		g_disp_colon_prev[i] = g_disp_colon[i];
	}
}

e_bool State_ChangeClock(unsigned char button_no_)
{
	unsigned char i;
	Rtc_UpdatePicTimerByRtc();
	g_disp_capture[0] = LO(g_second);
	g_disp_capture[1] = HI(g_second);
	//g_disp_capture[2] = 0;
	g_disp_capture[3] = LO(g_minute);
	g_disp_capture[4] = HI(g_minute);
	//g_disp_capture[5] = 0;
	g_disp_capture[6] = LO(g_hour);
	g_disp_capture[7] = HI(g_hour);

	SelectTimer1Callback(button_no_);
	g_timer1_callback[1] = State_CallbackClock;
	g_timer1_callback[2] = NIL;
	g_state_current = &g_state_clock;

	return TRUE;
}

e_bool State_ChangeSetClock(unsigned char button_no_)
{
	g_timer1_callback[0] = State_CallbackSetClock;
	g_timer1_callback[1] = NIL;
	g_state_current = &g_state_set_clock;
	return TRUE;
}

e_bool State_ChangeTemparature(unsigned char button_no_)
{
	g_disp_capture[0] = 10;
	g_disp_capture[1] = (g_temparature_fractional >> 4) & 0xf;
	g_disp_capture[3] = g_temparature_integer & 0xf;
	g_disp_capture[4] = (g_temparature_integer >> 4) & 0xf;;
	g_disp_capture[6] = 10;
	g_disp_capture[7] = 10;
	g_disp_colon_capture[0] = 0b11;
	g_disp_colon_capture[1] = 0;
	g_disp_colon_capture[2] = 0b01;
	g_disp_colon_capture[3] = 0;
	g_disp_colon_capture[4] = 0;
	g_disp_colon_capture[5] = 0;
	g_disp_colon_capture[6] = 0b11;
	g_disp_colon_capture[7] = 0b11;

	SelectTimer1Callback(button_no_);
	g_timer1_callback[1] = State_CallbackTemparature;
	g_timer1_callback[2] = NIL;
	g_state_current = &g_state_temparature;

	return TRUE;
}

e_bool State_ChangeCalendar(unsigned char button_no_)
{
	Rtc_UpdatePicTimerByRtc();
	g_disp_capture[0] = LO(g_date);
	g_disp_capture[1] = HI(g_date);
	//g_disp_capture[2] = 0;
	g_disp_capture[3] = LO(g_month);
	g_disp_capture[4] = HI(g_month);
	//g_disp_capture[5] = 0;
	g_disp_capture[6] = LO(g_year);
	g_disp_capture[7] = HI(g_year);

	SelectTimer1Callback(button_no_);
	g_timer1_callback[1] = State_CallbackCalendar;
	g_timer1_callback[2] = NIL;
	g_state_current = &g_state_calendar;

	return TRUE;
}

e_bool State_ChangeSetCalendar(unsigned char button_no_)
{
	g_timer1_callback[0] = State_CallbackSetCalendar;
	g_timer1_callback[1] = NIL;
	g_state_current = &g_state_set_alarm;
	return TRUE;
}

e_bool State_ChangeAlarm(unsigned char button_no_)
{
	g_disp_capture[0] = 0;
	g_disp_capture[1] = 0;
	//g_disp_capture[2] = 0;
	g_disp_capture[3] = LO(g_alarm_minute);
	g_disp_capture[4] = HI(g_alarm_minute);
	//g_disp_capture[5] = 0;
	g_disp_capture[6] = HI(g_alarm_hour);
	g_disp_capture[7] = HI(g_alarm_hour);


	SelectTimer1Callback(button_no_);
	g_timer1_callback[1] = State_CallbackAlarm;
	g_timer1_callback[2] = NIL;
	g_state_current = &g_state_alarm;

	return TRUE;
}

e_bool State_ChangeSetAlarm(unsigned char button_no_)
{
	g_timer1_callback[0] = State_CallbackSetAlarm;
	g_timer1_callback[1] = NIL;
	g_state_current = &g_state_set_alarm;
	return TRUE;
}


e_bool State_ChangeConfig(unsigned char button_no_)
{
	g_disp_capture[0] = g_cds_1th_place;
	g_disp_capture[1] = g_cds_10th_place;
	//g_disp_capture[2] = 0;
	g_disp_capture[3] = g_cds_100th_place;
	g_disp_capture[4] = g_config_auto_brightness ? 1 : 0;
	//g_disp_capture[5] = 0;
	g_disp_capture[6] = g_config_auto_off_enable ? 1 : 0;
	g_disp_capture[7] = g_config_alarm_enable ? 1 : 0;

	SelectTimer1Callback(button_no_);
	g_timer1_callback[1] = State_CallbackConfig;
	g_timer1_callback[2] = NIL;
	g_state_current = &g_state_config;

	return TRUE;
}

e_bool State_ChangeDispOff(unsigned char button_no_)
{
	unsigned char i;
	SelectTimer1Callback(button_no_);

	for (i=0; i<8; ++i)
	{
		g_disp_capture[i] = 10;
		g_disp_colon_capture[i] = 0b11;
	}
	
	g_timer1_callback[1] = State_CallbackDispoff;
	g_timer1_callback[2] = NIL;
	g_state_current = &g_state_dispoff;

	return TRUE;
}

e_bool State_ChangeSetConfig(unsigned char button_no_)
{
	g_timer1_callback[0] = State_CallbackSetConfig;
	g_timer1_callback[1] = NIL;
	g_state_current = &g_state_set_config;
	return TRUE;
}


e_bool TargetUp(unsigned char arg_)
{
	if (*g_target_bcd_ptr >= g_target_max)
		*g_target_bcd_ptr = g_target_min;
	else
		++(*g_target_bcd_ptr);

	return TRUE;
}

e_bool TargetDown(unsigned char arg_)
{
	if (*g_target_bcd_ptr <= g_target_min)
		*g_target_bcd_ptr = g_target_max;
	else
		--(*g_target_bcd_ptr);

	return TRUE;
}

e_bool TargetBcdUp(unsigned char arg_)
{
	unsigned char dec = 10*HI(*g_target_bcd_ptr) + LO(*g_target_bcd_ptr);
	dec = dec >= g_target_max ? g_target_min : dec + 1;
	*g_target_bcd_ptr = (dec/10<<4) + dec%10;
	return TRUE;
}

e_bool TargetBcdDown(unsigned char arg_)
{
	unsigned char dec = 10*HI(*g_target_bcd_ptr) + LO(*g_target_bcd_ptr);
	dec = dec <= g_target_min ? g_target_max : dec - 1;
	*g_target_bcd_ptr = (dec/10<<4) + dec%10;
	return TRUE;
}

// 初期化
void InitApplication()
{
	unsigned char config;
	
	// Rtc初期化および、時間取得
	Rtc_Init();
	Rtc_UpdatePicTimerByRtc();

	// config
	config = Eeprom_Read(EEPROM_ADR_CONFIG);
	g_config_alarm_enable = config & CONFIG_AUTO_OFF ? 1:0;
	g_config_auto_off_enable = config & CONFIG_ALARM_ON ? 1:0;
	g_config_auto_brightness = config & CONFIG_AUTO_BRIGHTNESS ? 1:0;
	g_config_max_brightness = Eeprom_Read(EEPROM_ADR_MAX_BRIGHTNESS);

	// ステートマシンを初期化
	//最初は時計ステート
	g_state_clock.on_button0			= State_ChangeCalendar;
	g_state_clock.on_button1			= State_ChangeDispOff;
	g_state_clock.on_button2			= State_ChangeSetClock;
	g_state_calendar.on_button0			= State_ChangeTemparature;
	g_state_calendar.on_button1			= State_ChangeClock;
	g_state_calendar.on_button2			= State_ChangeSetCalendar;
	g_state_temparature.on_button0		= State_ChangeAlarm;
	g_state_temparature.on_button1		= State_ChangeCalendar;
	g_state_temparature.on_button2		= NIL;
	g_state_alarm.on_button0			= State_ChangeConfig;
	g_state_alarm.on_button1			= State_ChangeTemparature;
	g_state_alarm.on_button2			= State_ChangeSetAlarm;
	g_state_config.on_button0			= State_ChangeDispOff;
	g_state_config.on_button1			= State_ChangeAlarm;
	g_state_config.on_button2			= State_ChangeSetConfig;
	g_state_dispoff.on_button0			= State_ChangeClock;
	g_state_dispoff.on_button1			= State_ChangeConfig;
	g_state_dispoff.on_button2			= NIL;
	g_state_set_clock.on_button0		= TargetBcdUp;
	g_state_set_clock.on_button1		= TargetBcdDown;
	g_state_set_clock.on_button2		= NIL;
	g_state_set_calendar.on_button0		= TargetBcdUp;
	g_state_set_calendar.on_button1		= TargetBcdDown;
	g_state_set_calendar.on_button2		= NIL;
	g_state_set_alarm.on_button0		= TargetBcdUp;
	g_state_set_alarm.on_button1		= TargetBcdDown;
	g_state_set_alarm.on_button2		= NIL;
	g_state_set_config.on_button0 	    = TargetUp;
	g_state_set_config.on_button1 	    = TargetDown;
	g_state_set_config.on_button2 	    = NIL;

	// コールバックを登録
	g_timer1_callback[0] = State_CallbackClock;
	g_timer1_callback[1] = NIL;
	g_mainloop_callback[0]	 = Mainloop_CallbackNormal;
	g_state_current = &g_state_clock;
}

// 割り込み
void interrupt intr(void)
{
	if (T0IF)
	{
		// 要因クリア
		T0IF = 0;

		// コールバック
		//DoCallbackChain(g_timer0_callback);
		if (g_auto_off_count == 0xff)
			Disp_CallbackOff();
		else
			Disp_CallbackNormal();
	}
	else if (TMR1IF)
	{
		// 一旦タイマーを停止
		//T1CONbits.TMR1ON = 0;
		
		// 要因クリア
		TMR1IF = 0;

		// コールバック
		DoCallbackChain(g_timer1_callback);
		
		// 一旦タイマーを再開
		//T1CONbits.TMR1ON = 1;
	}
	else if (SSPIF)
	{
		// 割り込みで要因クリアはしない
		// SSPIF = 0; 
	}
}


// メイン
void main(void)
{
	static unsigned char alarm_count = 0;

	// 初期化
	InitDevice();
	InitApplication();

	while (1)
	{
		// ボタン取得
		if (!PIN_BUTTON_0)
		{
			if (g_button0_press == 0)
			{
				g_button0_push = 1;
			}
			else
			{
				g_button0_push = 0;
			}

			g_button0_press = 1;
		}
		else
		{
			g_button0_push = 0;
			g_button0_press = 0;
		}

		if (!PIN_BUTTON_1)
		{	
			if (g_button1_press == 0)
			{
				g_button1_push = 1;
			}
			else
			{
				g_button1_push = 0;
			}

			g_button1_press = 1;
		}
		else
		{
			g_button1_press = 0;
			g_button1_push = 0;
		}
					
		if (!PIN_BUTTON_2)
		{	
			if (g_button2_press == 0)
			{
				g_button2_push = 1;
			}
			else
			{
				g_button2_push = 0;
			}

			g_button2_press = 1;
		}
		else
		{
			g_button2_press = 0;
			g_button2_push = 0;
		}

		// ボタンプッシュコールバック
		if ((g_state_current->on_button0 != NIL)
		&&  (g_button0_push))
		{
			(*(g_state_current->on_button0))(0);		
		}

		if ((g_state_current->on_button1 != NIL)
		&&  (g_button1_push))
		{
			(*(g_state_current->on_button1))(1);		
		}

		if ((g_state_current->on_button2 != NIL)
		&&  (g_button2_push))
		{
			(*(g_state_current->on_button2))(2);		
		}

		// 適当スピーカーアラーム優先
		if (g_alarm_trigger != 0)
		{
			++alarm_count;
			if (alarm_count > 10)
			{
				alarm_count = 0;
				g_alarm_beep_on = g_alarm_beep_on == 1 ? 0 : 1; 
			}
		}
		if (g_button0_press | g_button1_press | g_button2_press | g_alarm_beep_on)
		{
			PIN_SPEAKER = 1;
		}
		else
		{
			PIN_SPEAKER = 0;
		}

		// メインコールバック
		DoCallbackChain(g_mainloop_callback);

		Wait10ms(1);
	}
}
