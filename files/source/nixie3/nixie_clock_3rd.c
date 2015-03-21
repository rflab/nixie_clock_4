
//
// nixie3.c
//
// Created: 2014/07/30 23:03:32
//  Author: Owner
//

//----------------------------
// include std
//----------------------------

// none

//----------------------------
// include avr
//----------------------------
#include <avr/io.h>

// 内蔵クロック8MHzそのままで動かすなら,
// Fuseの分周設定をOFFにする(CKDIV8=0)
#ifndef F_CPU
	#define F_CPU 8000000UL
#endif
#include <util/delay.h>
#include <stdbool.h>

// タイマ割り込み
#include <avr/interrupt.h>

//----------------------------
// macros/defines
//----------------------------
#define cbi(addr,bit) do{addr &= ~(1<<bit)}while(0)
#define sbi(addr,bit) do{addr |=  (1<<bit)}while(0)
#define setbits(reg, mask, offset, dat) \
	do{reg = ((reg & ~mask)|((dat)<<offset));}while(0)
#define chkbit(reg, mask) (reg & (1<<mask))
#define lo(c) ((c)&0x0f)
#define hi(c) (((c)>>4)&0x0f)

#define IIC_READ_BIT  1
#define IIC_WRITE_BIT 0
#define IIC_SCL_CLOCK 100000L

#define RTC8564NB_SLAVE_ADDRESS_8BIT (0x51<<1)
#define LP3331_SLAVE_ADDRESS_8BIT (0x5d<<1)
#define AM2321_SLAVE_ADDRESS_8BIT (0x5c<<1)
#define DS1307_SLAVE_ADDRESS_8BIT (0x68<<1)

//----------------------------
// enum
//----------------------------
typedef enum _e_digit
{
	DIGIT_0        = 0,
	DIGIT_1        = 1,
	DIGIT_COLON_L  = 2,
	DIGIT_2        = 3,
	DIGIT_3        = 4,
	DIGIT_COLON_R  = 5,
	DIGIT_4        = 6,
	DIGIT_5        = 7
}e_digit;

typedef struct BIT_FOELD_PORTB
{
	unsigned NOT_ISE1     : 1;
	unsigned BEEP         : 1;
	unsigned NOT_USE0     : 3;
	unsigned NIXIE_DIGIT  : 3;
}BIT_FOELD_PORTB;
#define PORT_BEEP              PORTB
#define PORT_NIXIE_DIGIT       PORTB
#define OFFSET_BEEP           6
#define OFFSET_NIXIE_DIGIT    0
#define MASK_BEEP             0b01000000
#define MASK_NIXIE_DIGIT      0b00000111
#define BIT_FIELD_BEEP        BIT_FOELD_PORTB
#define BIT_FIELD_NIXIE_DIGIT BIT_FOELD_PORTB

typedef struct BIT_FOELD_PORTC
{
	unsigned NA           : 1;
	unsigned RESET_LOW    : 1;
	unsigned SCL          : 1;
	unsigned SDA          : 1;
	unsigned NIXIE_DRIVER : 4;
}BIT_FOELD_PORTC;
#define PORT_SCL                PORTC
#define PORT_SDA                PORTC
#define PORT_NIXIE_DRIVER       PORTC
#define OFFSET_SCL             5
#define OFFSET_SDA             4
#define OFFSET_NIXIE_DRIVER    0
#define MASK_SCL               0b00100000
#define MASK_SDA               0b00010000
#define MASK_NIXIE_DRIVER      0b00001111
#define BIT_FIELD_SCL          BIT_FOELD_PORTC
#define BIT_FIELD_SDA          BIT_FOELD_PORTC
#define BIT_FIELD_NIXIE_DRIVER BIT_FOELD_PORTC

typedef struct BIT_FOELD_PORTD
{
	unsigned LED          : 1; // LED
	unsigned SW_A         : 1; // switch A external interrupt
	unsigned SW_B         : 1; // switch B external interrupt
	unsigned RTC_CLKOE    : 1; // RTC output enable
	unsigned RTC_CLKOUT   : 1; // RTC output port
}BIT_FOELD_PORTD;
#define PORT_LED             PORTD
#define PORT_SWA             PORTD
#define PORT_SWB             PORTD
#define PORT_RTC_CLKOE       PORTD
#define PORT_RTC_CLKOUT      PORTD
#define PIN_LED              PIND
#define PIN_SWA              PIND
#define PIN_SWB              PIND
#define PIN_RTC_CLKOE        PIND
#define PIN_RTC_CLKOUT       PIND
#define OFFSET_LED           4
#define OFFSET_SW_A          3
#define OFFSET_SW_B          2
#define OFFSET_RTC_CLKOE     1
#define OFFSET_RTC_CLKOUT    0
#define MASK_LED             0b00010000
#define MASK_SW_A            0b00001000
#define MASK_SW_B            0b00000100
#define MASK_RTC_CLKOE       0b00000010
#define MASK_RTC_CLKOUT      0b00000001
#define BIT_FIELD_LED        BIT_FOELD_PORTD
#define BIT_FIELD_SWA        BIT_FOELD_PORTD
#define BIT_FIELD_SWB        BIT_FOELD_PORTD
#define BIT_FIELD_RTC_CLKOE  BIT_FOELD_PORTD
#define BIT_FIELD_RTC_CLKOUT BIT_FOELD_PORTD

#define ADC_ILLUMINANCE_SENSOR 6

//----------------------------
// globals
//----------------------------
volatile int number = 0;
volatile bool clkout_flag = false;
volatile bool num_up   = false;
volatile bool num_down = false;

//
volatile static unsigned char g_illuminance  = 0x00; 

// 時間記憶領域
volatile static unsigned char g_hour	     = 0x00;
volatile static unsigned char g_minute	     = 0x00;
volatile static unsigned char g_second	     = 0x00;
volatile static unsigned char g_year	     = 0x00;
volatile static unsigned char g_month	     = 0x00;
volatile static unsigned char g_date	     = 0x00;
volatile static unsigned char g_week         = 0x00;
volatile static unsigned char g_alarm_hour	 = 0x12;
volatile static unsigned char g_alarm_minute = 0x00;


// よく使うのでプロトタイプ
void led(bool on_);
void beep(unsigned char num_);
void beep_hex(unsigned char hex_);

//----------------------------
// util
//----------------------------

//------
// ADC

ISR(ADC_vect)
{
}

// ADコンバータの値を取得する
unsigned char adc_get(unsigned char ch_)
{
	// 上位ビット書き込みしないようにチェック
	if (ch_ > 7)
		return 0;

	// 外部リファレンスAREF、left adjust、channel設定
	ADMUX  = (0<<REFS0) | (0<<REFS1) | (1<<ADLAR) | ch_;
		   
	// ADIF=1で一旦クリア
	// ADIEを立てると割り込み後にADIFがクリアされるのでwaitできない
	// 分周は200kHzまで？
	ADCSRA = (1<<ADEN)  | (0<<ADATE) | (1<<ADIF)  | (0<<ADIE)
	       | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);

	// Free run
	ADCSRB = 0;

	// start ADC
	ADCSRA |= (1<<ADSC);
	
	// wait ADC completion
	while (!(ADCSRA & (1<<ADIF)));
	return ADCH;
};


//------
// IIC
// エラーチェックとかなしでとりあえず動いているだけなので注意
//
// start  : マスター側からSCL_H中にSDA_L (スレーブアドレス+R/~Wを送りackを待つ)
// stop   : マスター側からSCL_H中にSDA_H
// repeat : 基本的にstartと同じ、PICでは個別のレジスタがあったので分けた
// write  : 書き込んでackを待つ
// read   : 読み込んでack/nackを返す
// ack    : 受信側から9bit目にSDA_L, 
// nack   : 受信側から9bit目にSDA_H (マスターからのデータ転送終了要求等に使う)

ISR(TWI_vect)
{
	// none
}

// TWI割り込み完了待ち
bool iic_wait_job()
{
	#if 0
		unsigned char i = 0;
		for(i=0; i<0xff; ++i)
		{
			if ((TWCR & (1<<TWINT)) == 0)
				return true;
			_delay_ms(1);
		}
		return false;
	#else
		while(!(TWCR & (1<<TWINT)));
		return true;
	#endif
}

// IICスタートコンディション
bool iic_start(unsigned char sla_rw_)
{
	// スタートコンディション送信
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);// | (1<<TWIE);
	iic_wait_job();
	
	// スレーブアドレスとRWを送信
	TWDR = sla_rw_;
	TWCR = (1<<TWINT) | (1<<TWEN);// | (1<<TWIE);
	iic_wait_job();

	return true;
}

// IICリピートスタートコンディション
bool iic_repeat(unsigned char sla_rw_)
{
	return iic_start(sla_rw_);
}

// IICストップコンディション
void iic_stop()
{
	// ストップコンディション送信
	TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);// | (1<<TWIE);

	// stopコンディションについては割り込みが発生しない？
	// iic_wait_job();できない
	while(TWCR & (1<<TWSTO));
}

// IIC送信
bool iic_write(unsigned char data_)
{
	TWDR = data_;
	TWCR = (1<<TWINT) | (1<<TWEN);// | (1<<TWIE);
	iic_wait_job();
	
	// ステータスレジスタ確認（ACK 0x28)
	//return (TWSR & 0xF8 ) == 0x28;
	return true;
}

// IIC受信
unsigned char iic_read(bool ack_)
{
	//TWDR = 0;

	if (ack_ != false)
	{
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	}
	else
	{
		TWCR = (1<<TWINT) | (1<<TWEN);// | (1<<TWIE);
	}

	while(!(TWCR & (1<<TWINT)));
	return TWDR;
}

//------
// EEPROM

//----------------------------
// functions
//----------------------------

// LED
void led(bool on_)
{
	if (on_)
		PORT_LED |= MASK_LED;
	else
		PORT_LED &= ~MASK_LED;

	return;
}

//--------
// BEEP
void beep(unsigned char num_)
{
	int i=0;
	for (i=0; i<num_; ++i)
	{
		PORT_BEEP |= MASK_BEEP;
		_delay_ms(50);
		PORT_BEEP &= ~MASK_BEEP;
		_delay_ms(50);
	}
}

// 8bit値で音を変える
// 1 ピッ
// 0 ピー
void beep_hex(unsigned char hex_)
{	
	int i_bit = 0;
	for (; i_bit < 8; ++i_bit)
	{
		if (i_bit==4)
		{
			_delay_ms(200);
		}

		if (hex_ & (0x80>>i_bit))
		{
			PORT_BEEP |= MASK_BEEP;
			_delay_ms(200);
			PORT_BEEP &= ~MASK_BEEP;
			_delay_ms(100);
		}
		else
		{
			PORT_BEEP |= MASK_BEEP;
			_delay_ms(50);
			PORT_BEEP &= ~MASK_BEEP;
			_delay_ms(100);
		}
	}

	// 連続実行の判別のため一旦waitさせる
	_delay_ms(200);

	return;
}

// 明るさセンサ / brightness sensor
static void update_brightness()
{
	return;
}


//-----------------
// 気圧センサ / atmospheric pressure sensor
// update_atmospheric_pressureから呼び出す１バイト読み出し
// 連続よみもできるようだが、面倒なのでネットで見つけた方法のままにする
unsigned char LPS331_read(unsigned char address_)
{
	unsigned char data;
	
	iic_start(LP3331_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
	iic_write(address_);
	iic_repeat(LP3331_SLAVE_ADDRESS_8BIT|IIC_READ_BIT);
	data = iic_read(false);
	iic_stop();
	
	return data;
}

bool LPS331_write(unsigned char address_, unsigned char data_)
{
	iic_start(LP3331_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
	iic_write(address_);
	iic_write(data_);
	iic_stop();
	
	return true;
}

// 初期化
unsigned char LPS331_init()
{
	unsigned char data;

	data = LPS331_read(0x20);
	data &= 0x7F;                // PDビットクリア
	LPS331_write(0x20, data);

	LPS331_write(0x10, 0x7A);		// アベレージング回数設定：気圧512回、温度128回
	//  LPS331_write(0x10, 0x00);	// アベレージング回数設定：気圧1回、温度1回
	LPS331_write(0x20, 0x04);		// ワンショット、BDU有効、
	LPS331_write(0x20, 0x84);		// PDビットON（パワーON)

	_delay_ms(10);					// 動作開始まで待つ
}

// 更新
static void LPS331_load()
{
	long tmp;
	float pressure;
	float temparature; 
	
	tmp = LPS331_read(0x2A);
	tmp = LPS331_read(0x29) | (tmp << 8);
	tmp = LPS331_read(0x28) | (tmp << 8);
	pressure = (float) tmp / 4096.0f;
	
	tmp = LPS331_read(0x2C);
	tmp = LPS331_read(0x2B) | (tmp << 8);
	temparature = ((float)tmp / 480.0f) + 42.5f;
}


//--------------------
// 湿度センサ / temparature and humidity sensor
static void AM2321_load()
{
	// 湿度は（３バイト目×256)+4バイト目 / 10で計算出来ます。
	// 温度は（5バイト目×256)+6バイト目 / 10で計算出来ますが
	static unsigned char data[8] = {0};

	// wakeup
	iic_start(AM2321_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
	_delay_ms(2);  // 800us~3000us
	iic_stop();

	// set read bytes
	iic_start(AM2321_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
	iic_write(0x03); // register read
	iic_write(0x00); // register address
	iic_write(0x04); // register size
	iic_stop();
	
	// センサのjob待ち
	_delay_ms(3); // 1500us

	iic_start(AM2321_SLAVE_ADDRESS_8BIT|IIC_READ_BIT);
	_delay_us(100);
	data[0] = iic_read(true);  // 機能コード [03]　機能コードの返し
	data[1] = iic_read(true);  // 返送するバイト数　（この例では湿度と温度の4バイト）
	data[2] = iic_read(true);  // アドレス[00]の内容　（湿度の上位バイト）
	data[3] = iic_read(true);  // アドレス[01]の内容　（湿度の下位バイト）
	data[4] = iic_read(true);  // アドレス[02]の内容　（温度の上位バイト）
	data[5] = iic_read(true);  // アドレス[03]の内容　（温度の下位バイト）
	data[6] = iic_read(true);  // 16ビットのCRC（巡回冗長符号）　（下位バイトが先）
	data[7] = iic_read(false); // 16ビットのCRC（巡回冗長符号）　（上位バイト）
	iic_stop();
	
	return;
}

//----------
// 人感センサ / motion sensor
static void Napion_update()
{
	return;
}


//---------------
// RTC / real time clock
static void RTC8564NB_load()
{
	iic_start(RTC8564NB_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
	iic_write(0x02); // 秒のアドレス
	iic_repeat(RTC8564NB_SLAVE_ADDRESS_8BIT|IIC_READ_BIT);
	g_second       = iic_read(true); // 秒の値 0-59
	g_minute       = iic_read(true); // 分の値 0-59
	g_hour	       = iic_read(true); // 時の値 0-23
	g_date	       = iic_read(true); // 日の値 1-31
	g_week	       = iic_read(true); // 曜の値 日月火水木金土 0123456
	g_month	       = iic_read(true); // 月の値 (C:MSB)1-12	Cは1のとき21世紀
	g_year         = iic_read(true); // 年の値 00-99
	g_alarm_minute = iic_read(true); // アラーム
	g_alarm_hour   = iic_read(false); // アラーム
	iic_stop();

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

static void RTC8564NB_save()
{
	return;
}

static void RTC8564NB_init()
{
	unsigned char vl_bit;
	
	// Rtcの立ち上がりに700ms必要なので内部でwaitする
	_delay_ms(250);
	_delay_ms(250);
	_delay_ms(200);
	
	// VLビットをチェック
	// 読み出しの場合はrepeatする
	// 秒のアドレスの先頭ビットがバックアップ異常検出VL
	iic_start(RTC8564NB_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
	iic_write(0x02);
	iic_repeat(RTC8564NB_SLAVE_ADDRESS_8BIT|IIC_READ_BIT);
	vl_bit = iic_read(false) & 0x80; 
	iic_stop();

	// VLビット異常があれば初期化する
	if (vl_bit)
	{
		// 初期化
		iic_start(RTC8564NB_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
		iic_write(0x00); // control0のアドレス
		iic_write(0x0);  // control0 -> test=0, STOP=0
		iic_write(0x0);  // control1 -> AIE=TIE=0
		iic_write(0x0);  // seconds
		iic_write(0x0);  // minutes
		iic_write(0x0);  // hours
		iic_write(0x0);  // days
		iic_write(0x0);  // weekdays
		iic_write(0x0);  // month/century
		iic_write(0x0);  // years
		iic_write(0x0);  // alarm_minutes
		iic_write(0x0);  // alarm_hours
		iic_write(0x0);  // alarm_days
		iic_write(0x0);  // alarm_weekdays
		iic_write(0x83); // clkout -> enable,1Hz
		iic_write(0x0);  // timer control
		iic_write(0x0);  // timer
		iic_stop();
		beep(1);
	}
	else
	{
		iic_start(RTC8564NB_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
		iic_write(0x00); // control0のアドレス
		iic_write(0x0);  // control0 -> test=0, STOP=1
		iic_write(0x0);  // control1 -> AIE=TIE=0
		iic_stop();
		
		iic_start(RTC8564NB_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
		iic_write(0x0d); // clkoutのアドレス
		iic_write(0x83); // clkout -> enable,1Hz
		iic_stop();
	}
		
	return;
}

//---------------
// RTC / real time clock
static void DS1307_read()
{
	#if 0
		iic_start(DS1307_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
		iic_write(0x00); // word address
		iic_stop();
	
		iic_start(DS1307_SLAVE_ADDRESS_8BIT|IIC_READ_BIT);
		g_second = iic_read(true);  // 0x00 seconds
		g_minute = iic_read(true);  // 0x01 minutes
		g_hour	 = iic_read(true);  // 0x02 hours
		g_week	 = iic_read(true);  // 0x03 week
		g_date	 = iic_read(true);  // 0x04 date
		g_month	 = iic_read(true);  // 0x05 month
		g_year	 = iic_read(false);  // 0x06 year
		iic_stop();
	#else
		// repeat start による連続リード
		iic_start(DS1307_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
		iic_write(0x00); // word address
		iic_repeat(DS1307_SLAVE_ADDRESS_8BIT|IIC_READ_BIT);
		g_second = iic_read(true);  // 0x00 seconds
		g_minute = iic_read(true);  // 0x01 minutes
		g_hour	 = iic_read(true);  // 0x02 hours
		g_week	 = iic_read(true);  // 0x03 week
		g_date	 = iic_read(true);  // 0x04 date
		g_month	 = iic_read(true);  // 0x05 month
		g_year	 = iic_read(false);  // 0x06 year
		iic_stop();
	#endif

	// 有効ビットの取り出し
	g_second  &= 0x7f; // 最上位ビットは休止可能情報 CH
	g_minute  &= 0x7f;
	g_hour	  &= 0x3f;
	g_week	  &= 0x07;
	g_date	  &= 0x3f;
	g_month	  &= 0x1f;
	g_year	  &= 0xff; // 0~99
}

static void DS1307_write()
{
	return;
}


static void DS1307_init()
{
	// VLビットをチェック
	// 読み出しの場合はrepeatする
	// 秒のアドレスの先頭ビットがバックアップ異常検出VL
	iic_start(DS1307_SLAVE_ADDRESS_8BIT|IIC_WRITE_BIT);
	#if 0
		iic_write(0x00);  // word address
		//static const unsigned char CH  = 7;
		iic_write(0x00); // 0x00 seconds
		iic_write(0x00); // 0x01 minutes
		iic_write(0x00); // 0x02 hours, bit6=1:24 hour mode
		iic_write(0x00); // 0x03 week
		iic_write(0x00); // 0x04 date
		iic_write(0x00); // 0x05 month
		iic_write(0x00); // 0x06 year
	#else
		iic_write(0x07);  // word address
	#endif
	static const unsigned char OUT  = 7;
	static const unsigned char SQWE = 4;
	static const unsigned char RS1  = 1;
	static const unsigned char RS0  = 0;
	iic_write((1<<OUT) | (1<<SQWE) | (0<<RS1) | (0<<RS0));  // 0x07 control 1kHz 
	iic_stop();
}

// ニキシー管 / Nixie tube
// 現在表示桁の変更
// 繰り返しコールしてダイナミック点灯を行う
static void ChangeDigit(e_digit digit_)
{
	setbits(PORT_NIXIE_DIGIT, MASK_NIXIE_DIGIT, OFFSET_NIXIE_DIGIT, (char)digit_);
	return;
}

// ニキシー管 / Nixie tube
// 現在表示桁を指定した値に変更
// 10~15は点灯せず
static void ChangeNumber(unsigned char num_)
{
	static const unsigned char num2bit[] = {3, 4, 6, 9, 8, 1, 5, 0, 2, 7, 10};
	setbits(PORT_NIXIE_DRIVER, MASK_NIXIE_DRIVER, OFFSET_NIXIE_DRIVER, num2bit[num_]);
	return;
}


//----------------------------
// interrupts
//----------------------------
// タイマ割り込み
ISR(TIMER0_COMPA_vect)
{
	static unsigned char digit = 0;
	static unsigned char blank = 0;

	// blank期間なら全LEDをOFF
	blank = blank == 0 ? 1 : 0;
	if (blank)
	{
		ChangeNumber(10); // 10は点灯せず

		return;
	}
	
	// 表示LEDを切り替え
	digit++;
	digit &= 0x7;
	switch (digit)
	{
		case 0:
			ChangeDigit(DIGIT_0);
			ChangeNumber(number);
			break;
		case 1:
			ChangeDigit(DIGIT_1);
			ChangeNumber(number);
			break;
		case 2:
			ChangeDigit(DIGIT_COLON_L);
			ChangeNumber(number);
			break;
		case 3:
			ChangeDigit(DIGIT_2);
			ChangeNumber(number);
			break;
		case 4:
			ChangeDigit(DIGIT_3);
			ChangeNumber(number);
			break;
		case 5:
			ChangeDigit(DIGIT_COLON_R);
			ChangeNumber(number);
			break;
		case 6:
			ChangeDigit(DIGIT_4);
			ChangeNumber(number);
			break;
		case 7:
			ChangeDigit(DIGIT_5);
			ChangeNumber(number);
			break;
		default:
			break;
	}
	return;
}

// External interrupt 0
ISR(INT0_vect)
{
	if (!num_up)
	{
		num_down = true;
	}
}

// External interrupt 1
ISR(INT1_vect)
{
	if (!num_down)
	{
		num_up = true;
	}
}

// PIN change interrupt
ISR(PCINT2_vect)
{
	// PCINTでロータリーエンコーダ検出するとき
	#if 0
	if (((PIND & 0b10000000) == 0)
	||  ((PIND & 0b01000000) == 1))
	{
		if (!num_up)
		{
			num_down = true;
		}
	}

	if (((PIND & 0b01000000) == 0)
	||  ((PIND & 0b10000000) == 1))
	{
		if (!num_down)
		{
			num_up = true;
		}
	}
	#endif
	
	if (PIN_RTC_CLKOUT & MASK_RTC_CLKOUT)
	{
		clkout_flag = true;
	}
}


//----------------------------
// initialize
//----------------------------
static void setup()
{
	MCUCR &= 0b11101111; // PUD = 0, intrrupt on

	// PIN (I/O, interrupt)
	{
		// port config
		DDRB   = 0b11111111; // 0~5が有効 0134:line direct(012:line decoder), 6:beep
		DDRC   = 0b10001111; // 0~7が有効 0123:nixie_driver, 45:IIC, 6:reset
		DDRD   = 0b00010010; // 0~7が有効 0:rtc_out, 1:rtc_oe, 23:sw_AB, 5:remote, 6:焦電センサ(プルダウン内臓), 7:sw_push
		PORTB  = 0b00000000;
		PORTC  = 0b00110000; // 45:IICプルアップ, 6:resetプルアップ
		PORTD  = 0b10101111; // 6はセンサが内臓プルダウンなのでHiZ

		// pin change interrupt
		PCMSK0 = 0b00000000;
		PCMSK1 = 0b00000000;
		PCMSK2 = 0b00000001; // 7:PCINT7
		PCICR  = 0b00000100; // PCINT2 enable
		
		// interrupt
		EICRA  = 0b00001010; // INT0, INT1立ち下がり検出
		EIMSK  = 0b00000011; // INT0, INT1有効化
	}

	// Timer
	// 8桁ニキシーを30Hz表示したい、ブランク時間を半分とする
	// マスタクロック8Mhz / 8 / 30 / 2 = 166666 (16.3 * 1024)回に一度割り込みでOK
	// 約2msで割り込みさせる
	{
		TIMSK0 = 0b0000010; // output compare A

		// CTC, 1024分周prescaler入力
		TCCR0A = 0b00000010;
		TCCR0B = 0b00000101;
		
		// 16でリセット
		OCR0A  = 8; // 暫定で16→8
		
		// Compare Match A Interrupt Enable
		TIMSK0 = 0b00000010;
	}
	
	// IIC
	{
		// 温度湿度センサ・AM2321と通信するI2Cバスのデータ転送速度は、最大100KHzです。
		// F_SCL = (F_CPU / ((16 + 2(TWBR)) * prescaler)
		// TWBR = (F_CPU / (F_SCL * prescaler) -16) / 2
		// 20kHz通信、4分周とすると
		setbits(TWSR, 0x0, 0, 2); // 0, 1, 2, 3 -> 1, 4, 16, 64
		TWBR = ((F_CPU/20000UL) - 16) / 2;
		TWCR = 1<<TWEN;
	}
	
	// ADC
	{
		// デジタルピンdisable
		DIDR0 = 0b00000000;
	}

	// 割り込み有効化アセンブラ命令コール
	sei();
	
	///// 外部デバイス初期化
	_delay_ms(200); // DS1307の電源充電を待つ
	//RTC8564NB_init();
	//RTC8564NB_load();
	LPS331_init();
	LPS331_load();
	AM2321_load();
	DS1307_init();
	
	// 初期化完了、起動時一度だけ
	beep(1);
}

//----------------------------
// routine
//----------------------------

// loop
static void loop()
{	
	if ((PIND & 0b10000000) == 0)
	{
		number = 0;
		//RTC8564NB_load();
		//beep(g_second);

		DS1307_read();
		beep_hex(g_second);
		
		//g_illuminance = adc_get(ADC_ILLUMINANCE_SENSOR);
		//beep_hex(g_illuminance);
		led(true);
	}	
	
	if (num_up)
	{
		++number;
		if (number > 9)
		{
			number = 0;
		}
		_delay_ms(200);
	}
	else if (num_down)
	{
		--number;
		if (number < 0)
		{
			number = 9;
		}
		_delay_ms(200);
	}
	num_up = false;
	num_down = false;
	
	////人感センサ
	//if (chkbit(PIND, PIND6))
	//{
	//	led(true);
	//}
	//else
	//{
	//	led(false);
	//}


	if (clkout_flag)
	{
		//beep(1);
		clkout_flag = 0;
	}

	_delay_ms(20);

}

// main
int main(void)
{
	setup();
	
	while(1)
	{
		loop();
	}
	
	return 0;
}