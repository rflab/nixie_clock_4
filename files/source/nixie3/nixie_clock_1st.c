#include <3694.h>
#include <stdlib.h>

// static assert
#define STATIC_ASSERT(exp) {char is_size[(exp)?1:0]; (void)is_size;}
#define LO(c_) (c_&0x0f)
#define HI(c_) ((c_>>4)&0x0f)


// iicタイムアウト
#define IDLE_LOOP	  (1000)
#define TX_END_LOOP	  (1000)
#define RX_END_LOOP	  (1000)
#define BUSBUSY_LOOP  (1000)

// 表示桁数
#define DIGITS (8)
#define DIGITS_OFF_ALL (DIGITS)


// timer_v
#define MSEC_TO_TIMERV (2)
#define MAX_TRANSITION (40)

// 表示フラグ
// デバイスにあわせる。
unsigned char NUM2REG_TABLE[] = 
{
	0, //数字
	9,
	8,
	7,
	6,
	5,
	4,
	3,
	2,
	1,
};
#define DISP_DOT (sizeof(NUM2REG_TABLE))
#define DISP_OFF (DISP_DOT+1)

// ボタンコールバック
typedef void (*t_callback)(void);
t_callback g_timer_v_callback = NULL;
t_callback g_timer_a_callback = NULL;
static unsigned long g_timer_v_callback_time_left = 0;

// ステートマシン
typedef struct {
	t_callback on_button0;
	t_callback on_button1;
	t_callback on_button2;
}STATE;
STATE g_state_clock			  = {NULL, NULL, NULL};
STATE g_state_calendar		  = {NULL, NULL, NULL};
STATE g_state_dispoff		  = {NULL, NULL, NULL};
STATE g_state_hour_h_set	  = {NULL, NULL, NULL};
STATE g_state_hour_l_set	  = {NULL, NULL, NULL};
STATE g_state_minute_h_set	  = {NULL, NULL, NULL};
STATE g_state_minute_l_set	  = {NULL, NULL, NULL};
STATE g_state_second_h_set	  = {NULL, NULL, NULL};
STATE g_state_second_l_set	  = {NULL, NULL, NULL};
STATE g_state_year_h_set	  = {NULL, NULL, NULL};
STATE g_state_year_l_set	  = {NULL, NULL, NULL};
STATE g_state_month_h_set	  = {NULL, NULL, NULL};
STATE g_state_month_l_set	  = {NULL, NULL, NULL};
STATE g_state_date_h_set	  = {NULL, NULL, NULL};
STATE g_state_date_l_set	  = {NULL, NULL, NULL};
//STATE g_state_alarm_set	  = {NULL, NULL, NULL};
//STATE g_state_stopwatch_set = {NULL, NULL, NULL};
//STATE g_state_timer_set	  = {NULL, NULL, NULL};
STATE* g_state_current;
unsigned char g_setting_digit = DIGITS;
unsigned short g_blink_timer = 0;
static const unsigned short BLINK_INTERVAL = 500*MSEC_TO_TIMERV;


// 表示データ
static unsigned char g_disp[DIGITS] = {0,0,0,0,0,0,0,0};
static unsigned char g_disp_prev[DIGITS] = {0,0,DISP_DOT,0,0,DISP_DOT,0,0};
static unsigned short g_transition[DIGITS] = {0,0,0,0,0,0,0,0};
static unsigned char g_next_random = 0x0;

static unsigned char g_hour	  = 0x00;
static unsigned char g_minute = 0x00;
static unsigned char g_second = 0x00;
static unsigned char g_year	  = 0x00;
static unsigned char g_month  = 0x00;
static unsigned char g_date	  = 0x00;
static unsigned char g_week	  = 0x00;
static unsigned char g_dot    = 0x00; // 0消灯、1点灯
//static unsigned char hour_h	= 0;
//static unsigned char hour_l	= 0;
//static unsigned char minute_h = 0;
//static unsigned char minute_l = 0;
//static unsigned char second_h = 0;
//static unsigned char second_l = 0;
//static unsigned char year_h	= 0;
//static unsigned char year_l	= 0;
//static unsigned char month_h	= 0;
//static unsigned char month_l	= 0;
//static unsigned char date_h	= 0;
//static unsigned char date_l	= 0;



// とりあえずプロトタイプ
void ChangeDiap(unsigned char ch_, unsigned char num_);
void StateChangeRandomDisp();

//-----------------------------------------------
// 基本関数
//-----------------------------------------------

// msec単位でwait
void wait_msec(int msec_)
{
	volatile int i,j;
	for (i=0;i<msec_;i++)
	{
		for (j=0;j<1588;j++); /*1588は約1ms*/
	}
}

short iic_init(void)
{
	// 転送レート
	IIC2.ICCR1.BIT.CKS = 0x0f;
	IIC2.ICCR1.BIT.ICE = 1;
	
	return 1;
}

// busy end wait
short iic_busbusy(void)
{
	volatile short	i = 0;

	while (1)
	{
		if (i++ > BUSBUSY_LOOP)
			return 0;
		if (IIC2.ICCR2.BIT.BBSY == 0)
			break;
	}

	return (IIC2.ICCR2.BIT.BBSY == 0);
}

// start condition
short iic_start(void)
{
	volatile short	i = 0;

	// ビジー状態解除待ち
	iic_busbusy();
	
	// マスタ送信モードを指定
	IIC2.ICCR1.BYTE = (IIC2.ICCR1.BYTE & 0xcf) | 0x30; 
	
	// 開始
	//BBSY=1,SCP=0 / スタートコンディション、発行
	IIC2.ICCR2.BYTE = (IIC2.ICCR2.BYTE & 0xbe) | 0x80;
	//do 
	//{
	//	IIC2.ICCR2.BYTE = (IIC2.ICCR2.BYTE & 0xbe) | 0x80;
	//	IIC2.ICDRT = SLA;
	//	
	//}while(IIC2.ICIER.BIT.ACKBR)

	// DRT/DRRが空になるまで待ち
	while (IIC2.ICSR.BIT.TDRE == 0);
	{
		if (i++ > TX_END_LOOP)
		{
			return 0;
		}
	}

	return 1;
}

// stop condition
short iic_stop(void)
{
	volatile short	i = 0;
	_BYTE			result;

	// ダミーリード
	result = IIC2.ICSR.BYTE;
	
	// TENDフラグクリア
	IIC2.ICSR.BIT.TEND = 0;

	// マスタ送信モードを指定
	IIC2.ICCR1.BYTE = (IIC2.ICCR1.BYTE & 0xcf) | 0x30;
	
	// 停止条件生成(BBSY=0, SCP=0/ストップコンディション、発行)
	IIC2.ICCR2.BYTE &= 0x3f;

	// 停止条件生成待ち
	while (IIC2.ICSR.BIT.STOP == 0)
	{
		if (i++ > TX_END_LOOP)
			return 0;
	}
	return 1;
}


short iic_put(_BYTE data)
{
	volatile short	i = 0;

	// マスタ送信モードを指定
	IIC2.ICCR1.BYTE = (IIC2.ICCR1.BYTE & 0xcf) | 0x30; 

	// データ書き込み
	IIC2.ICDRT = data;

	// 送信待ち
	while (IIC2.ICSR.BIT.TEND == 0)
	{
		// タイムアウト
		if (i++ > TX_END_LOOP)
			return 0;
	}
	
	// ACK?
	if (IIC2.ICIER.BIT.ACKBR != 0)
	{
		// 停止条件発行
		iic_stop();
		return 0;
	}
	return 1;
}

char iic_get(int ack_)
{
	volatile short	i = 0;
	_BYTE data = 0;
	
	// マスタ送信モードを指定
	IIC2.ICCR1.BYTE = (IIC2.ICCR1.BYTE & 0xcf) | 0x20; 
	
	// ダミーリードすると受信を開始
	data = IIC2.ICDRR;

	// レシーブデータフル待ち
	while (IIC2.ICSR.BIT.RDRF == 0)
	{
		// タイムアウト
		if (i++ > RX_END_LOOP)
			return 0;
	}

	// 読み込み
	data = IIC2.ICDRR;

	// ACK?
	if (IIC2.ICIER.BIT.ACKBR != 0)
	{
		// 停止条件発行
		iic_stop();
		return data;
	}
	
	return data;
}

// H8初期化
void InitH8(void)
{
	// タイマV設定
	TV.TCRV0.BIT.CCLR = 1;	// コンペアマッチAでTCNVクリア
	TV.TCRV0.BIT.CKS = 0;	// 停止
	TV.TCRV1.BIT.ICKS = 1;	// 上記CKS=3と併用で /128=156.25kHφ z
	TV.TCSRV.BIT.CMFA = 0;	// フラグクリア
	TV.TCNTV = 0;			// タイマカウンタクリア
	TV.TCORA = 80;			// 約500us
	TV.TCRV0.BIT.CMIEA = 1; // コンペアマッチで割り込み
	TV.TCRV0.BIT.CKS = 3;	// タイマVスタート

	// IIC
	iic_init();

	// タイマA設定
	TA.TMA.BIT.CKSI = 8;	// プリスケーラW/時計用選択 φ=1Hz
	IENR1.BIT.IENTA = 1;	// タイマA割り込み要求許可

	// IOポートPCR8 設定
	IO.PCR8 = 0xFF;			// PCR8 = 出力
	
	// IOポートPCR5 設定
	IO.PCR5 = 0xFF;			// PCR5 = 出力
	
	// IOポートPCR1 設定
	IO.PCR1 = 0xe9;			// PIO10 = 出力、PIO11,PIO12,PIO14 = 入力
}

void RtcInit()
{	  
	wait_msec(1700);
	iic_start();
	iic_put(0xa2);	// 書き込みモード
	iic_put(0x00);	// control0のアドレス
	iic_put(0x0);	// test=0	
	iic_put(0x0);	//AIE=TIE=0
	iic_stop(); 
}

// 二進化十進法で日付を設定
void RtcDateSetBcdInDirect(short year_, char month_, char week_, char date_, char hour_, char minute_, char second_)
{
	iic_start();
	iic_put(0xa2);	   // 書き込みモード
	iic_put(0x02);	   // 秒のアドレス
	iic_put(second_);  // 秒の値 0-59
	iic_put(minute_);  // 分の値 0-59
	iic_put(hour_);	   // 時の値 0-23
	iic_put(date_);	   // 日の値 1-31
	iic_put(week_);	   // 曜の値 日月火水木金土 0123456
	iic_put(year_>0x2000 ? 0x80|month_ : month_); // 月の値 (C:MSB)1-12	  Cは1のとき21世紀
	iic_put((char)(year_&0x00ff)); // 年の値 00-99
	iic_stop();
}

// グローバルの値を使ってRTCの時間を設定
void RtcDateSetBcd()
{
	RtcDateSetBcdInDirect(
		0x2000 | ((HI(g_year)<<4)&0xf0) | LO(g_year),
		(HI(g_month)  << 4) | LO(g_month),
		g_week,
		(HI(g_date)   << 4) | LO(g_date),
		(HI(g_hour)   << 4) | LO(g_hour),
		(HI(g_minute) << 4) | LO(g_minute),
		(HI(g_second) << 4) | LO(g_second));
}

// RTCのクロック値を読み出す
void RtcDateGetBcd()
{
	//char sec	 = 0; // 秒の値
	//char min	 = 0; // 分の値
	//char hour	 = 0; // 時の値
	//char day	 = 0; // 日の値
	//char week	 = 0; // 曜の値
	//char month = 0; // 月の値
	//char year	 = 0; // 年の値
	
	// 通信
	#if 1
		iic_start();
		iic_put(0xa2);	 // 書き込みモード
		iic_put(0x02);	 // 秒のアドレス
		wait_msec(100);
		iic_start();
		iic_put(0xa3);	 // 読み込みモード
		g_second = iic_get(1); // 秒の値
		g_minute = iic_get(1); // 分の値
		g_hour	 = iic_get(1); // 時の値
		g_date	 = iic_get(1); // 日の値
		g_week	 = iic_get(1); // 曜の値
		g_month	 = iic_get(1); // 月の値
		g_year	 = iic_get(0); // 年の値
		iic_stop();
		g_second &= 0x7f;
		g_minute &= 0x7f;
		g_hour	 &= 0x3f;
		g_date	 &= 0x3f;
		g_week	 &= 0x07;
		g_month	 &= 0x1f;
		g_year	 &= 0x7f;
	#else
		iic_start();
		iic_put(0xa2);	 // 書き込みモード
		iic_put(0x02);	 // 秒のアドレス
		wait_msec(100);
		iic_start();
		iic_put(0xa3);	 // 読み込みモード
		sec	  = iic_get(1); // 秒の値
		min	  = iic_get(1); // 分の値
		hour  = iic_get(1); // 時の値
		day	  = iic_get(1); // 日の値
		week  = iic_get(1); // 曜の値
		month = iic_get(1); // 月の値
		year  = iic_get(0); // 年の値
		iic_stop();
		// 二進化十進→十進
		sec	  &= 0x7f;
		min	  &= 0x7f;
		hour  &= 0x3f;
		day	  &= 0x3f;
		week  &= 0x07;
		month &= 0x1f;
		year  &= 0x7f;
		// 表示値に変換
		hour_h	 = (hour >> 4)& 0xf;
		hour_l	 = (hour) & 0xf;
		minute_h = (min >> 4) & 0xf;
		minute_l = (min) & 0xf;
		second_h = (sec >> 4) & 0xf;
		second_l = (sec) & 0xf;
		year_h	 = (year >> 4)& 0xf;
		year_l	 = (year) & 0xf;
		month_h	 = (month >> 4) & 0xf;
		month_l	 = (month) & 0xf;
		date_h	 = (day >> 4) & 0xf;
		date_l	 = (day) & 0xf;
	#endif
}

// 表示設定
void ChangeDigit(unsigned char ch_, unsigned char num_)
{
	// shortは2バイト?
	STATIC_ASSERT(sizeof(short) != 2)
	
	if ((ch_ >= DIGITS)
	||	(num_ == DISP_OFF))
	{
		IO.PDR8.BYTE = 0xFF; // 表示桁が16は配線していないので非表示となる
		IO.PDR1.BYTE = 0x01; // ドットなし
	}

	// 表示チャンネル
	if (num_ < sizeof(NUM2REG_TABLE))
	{
		IO.PDR8.BYTE = (ch_<<4) & 0xf0;
	
		//数字
		IO.PDR8.BYTE |= NUM2REG_TABLE[num_] & 0x0f;

		//ドットなし
		IO.PDR1.BYTE = 0x01;
	}
	else if (num_ == DISP_DOT)
	{
		// 触らなければ数字なし
		// num = All Hiは非表示となる
		// 数字部は非表示
		IO.PDR8.BYTE = (ch_<<4) & 0xf0; 
		IO.PDR8.BYTE |= 0x0f;
		
		//ドットあり
		IO.PDR1.BYTE = 0x00;
	}
	else
	{
		//error
	}
}

// 通常表示
// 数値に変化があった場合はクロスフェードする。
void DynamicDispCallbackNormal()
{
	static unsigned char digits= 0;
	static unsigned char flag = 0;
	
	// 点滅カウントアップ
	++g_blink_timer;
	if (g_blink_timer >= BLINK_INTERVAL)
	{
		g_blink_timer = 0;
	}
	
	// 消灯区間
	// 点灯区間
	if( flag == 0)
	{
		ChangeDigit(DIGITS_OFF_ALL, DISP_OFF);
	}
	else
	{
		if (g_transition[digits] == MAX_TRANSITION)
		{
			if (digits == g_setting_digit)
			{
				if (g_blink_timer > BLINK_INTERVAL/2)
				{
					ChangeDigit(digits, g_disp[digits]);
				}
				else
				{
					ChangeDigit(DIGITS_OFF_ALL, DISP_OFF);
				}
			}
			else
			{
				ChangeDigit(digits, g_disp[digits]);
			}
		}
		else
		{
			if (g_transition[digits]%(g_transition[digits]/(MAX_TRANSITION/2)+1) == 0)
			{
				ChangeDigit(digits, g_disp_prev[digits]);
			}
			else
			{
				ChangeDigit(digits, g_disp[digits]);
			}
			
			++g_transition[digits];
		}
	}
	
	++flag;
	if( flag > 3 )
	{
		++digits;
		if( digits >= DIGITS )
		{
			digits = 0;
		}
		flag = 0;
	}
}

// ガチャガチャした表示
void DynamicDispCallbackRandom()
{
	static unsigned char digits= 0;
	static unsigned char flag = 0;
	
	// 消灯区間
	// 点灯区間
	if( flag == 0)
	{
		ChangeDigit(DIGITS_OFF_ALL, DISP_OFF);
	}
	else
	{
		ChangeDigit(digits, rand()%10);
	}
	
	++flag;
	if( flag > 3 )
	{
		++digits;
		if( digits >= DIGITS )
		{
			digits = 0;
		}
		flag = 0;
	}
}

// クロックモード
void ModeCallbackClock()
{
	if (g_next_random == 1)
	{
		g_next_random = 0;
		
		// なんとなく10分に一度ランダム表示する
		StateChangeRandomDisp();
		g_timer_v_callback_time_left = MSEC_TO_TIMERV * 6000L;
	}

	// 秒 下位
	g_disp_prev[2] = (g_dot&0x01)==0 ? DISP_OFF : DISP_DOT;
	g_disp_prev[5] = (g_dot&0x01)==0 ? DISP_OFF : DISP_DOT;
	++g_dot;
	g_transition[2] = 0; //dot
	g_transition[5] = 0; //dot
	
	g_disp_prev[0] = LO(g_second);
	g_transition[0] = 0;
	g_second += 0x01;

	if( LO(g_second) >= 10)
	{
		// 秒 上位
		g_disp_prev[1] = HI(g_second);
		g_transition[1] = 0;
		g_second &= 0xf0;
		g_second += 0x10;

		if (HI(g_second) >= 6)
		{
			// 分 下位
			g_disp_prev[3] = LO(g_minute);
			g_transition[3] = 0;
			g_second = 0x00;
			g_minute += 0x01;

			if (LO(g_minute) >= 10)
			{
				// なんとなく10分に一度ランダム表示する
				g_next_random = 1;
				
				// 分 上位
				g_disp_prev[4] = HI(g_minute);
				g_transition[4] = 0;
				g_minute &= 0xf0;
				g_minute += 0x10;

				if (HI(g_minute)  >= 6)
				{
					// 時間 下位
					g_disp_prev[6] = LO(g_hour);
					g_transition[6] = 0;
					g_minute = 0x00;
					g_hour += 0x01;

					if(HI(g_hour) < 2 && LO(g_hour) >= 10)
					{
						// 時間上位
						g_disp_prev[6] = HI(g_hour);
						g_disp_prev[6] = LO(g_hour);
						g_transition[7] = 0;
						g_hour &= 0xf0;
						g_hour += 0x10;
					}
					else if (HI(g_hour) >= 2 && LO(g_hour) >= 4)
					{
						g_disp_prev[6] = 2;
						g_disp_prev[6] = 4;
						g_transition[7] = 0;
						#if 1
							// 現在時刻より前で23時59分とかの場合、ここを繰り返すようになるのでwaitでちょっと余裕を持たせる。
							wait_msec(1000);
							RtcDateGetBcd();
						#else
							//// 時間 上位 + 日付 下位
							//g_hour = 0x00;
							//g_date += 1;
							//g_disp_prev[7] = 0;
							//g_transition[7] = 0;
							// 面倒なのでやめ
							//if (LO(g_date) >= 10)
							//{
							//	// 日付 上位
							//	g_date &= 0xf0;
							//	  g_date += 0x10;
							//	if (month_l == 2 || month_l == 4 || month_l == 6 || month_l == 9 || month == 11 )
							//	{
							//		// 西向く侍小の月
							//
							//	}
							//	else (date_h >= 3 && date_l >= 1)
							//	{
							//		date_h = 0
							//		date_l = 1
							//		++month_l;
							//		if (month >= 9)
							//		{
							//		}
							//		else
							//		{
							//		}
							//	}
							//	if (LO(g_date) >= 10)
							//	{
							//		g_date += 0x10;;
							//	}
							//	
							//}
						#endif
					}
				}
			}
		}
	}
	
	// 表示データセット
	g_disp[0] = LO(g_second);
	g_disp[1] = HI(g_second);
	g_disp[2] = (g_dot&0x01)==0 ? DISP_OFF : DISP_DOT;
	g_disp[3] = LO(g_minute);
	g_disp[4] = HI(g_minute);
	g_disp[5] = (g_dot&0x01)==0 ? DISP_OFF : DISP_DOT;
	g_disp[6] = LO(g_hour);
	g_disp[7] = HI(g_hour);
}

// カレンダーモード
void ModeCallbackCalendar()
{
	g_disp[0] = LO(g_date);
	g_disp[1] = HI(g_date);
	g_disp[2] = LO(g_month);
	g_disp[3] = HI(g_month);
	g_disp[4] = LO(g_year);
	g_disp[5] = HI(g_year);
	g_disp[6] = 0;
	g_disp[7] = 2;
}

// 非表示モード
void ModeCallbackDispoff()
{
	g_disp[0] = DISP_OFF;
	g_disp[1] = DISP_OFF;
	g_disp[2] = DISP_OFF;
	g_disp[3] = DISP_OFF;
	g_disp[4] = DISP_OFF;
	g_disp[5] = DISP_OFF;
	g_disp[6] = DISP_OFF;
	g_disp[7] = DISP_OFF;
}

//--------------------------------------------------
// 割り込み
//--------------------------------------------------

// タイマV割り込み
void int_timerv(void)
{
	// 停止
	TV.TCRV0.BIT.CKS = 0;

	// フラグクリア
	TV.TCSRV.BIT.CMFA = 0;
	
	// 表示変化終了
	if (g_timer_v_callback_time_left == 0)
	{
		g_timer_v_callback = DynamicDispCallbackNormal;
	}
	else
	{
		--g_timer_v_callback_time_left;
	}
	
	// コールバック
	if (g_timer_v_callback != NULL)
	{
		(*g_timer_v_callback)();
	}

	// タイマVスタート
	TV.TCRV0.BIT.CKS = 3; 
}

// タイマA割り込み
void int_timera(void)
{
	// 表示
	if (g_timer_a_callback != NULL)
	{
		(*g_timer_a_callback)();
	}

	// タイマA割り込み要求フラグクリア
	IRR1.BIT.IRRTA = 0;
}

//--------------------------------------------------
// アプリ
//--------------------------------------------------

///
/// ステートごとの処理
///

void StateChangeClock()
{
	int i = 0;
	for (i=0; i<DIGITS; ++i)
	{
		g_transition[i]=MAX_TRANSITION;
	}
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackClock;
	g_timer_v_callback = DynamicDispCallbackRandom;
	g_timer_v_callback_time_left = MSEC_TO_TIMERV * 300L;
	g_state_current = &g_state_clock;
	g_setting_digit = 8;
	g_disp[0] = LO(g_second);
	g_disp[1] = HI(g_second);
	g_disp[2] = DISP_DOT;
	g_disp[3] = LO(g_minute);
	g_disp[4] = HI(g_minute);
	g_disp[5] = DISP_DOT;
	g_disp[6] = LO(g_hour);
	g_disp[7] = HI(g_hour);
}
void StateChangeCalendar()
{
	int i = 0;
	for (i=0; i<DIGITS; ++i)
	{
		g_transition[i]=MAX_TRANSITION;
	}
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackCalendar;
	g_timer_v_callback = DynamicDispCallbackRandom;
	g_timer_v_callback_time_left = MSEC_TO_TIMERV * 300L;
	g_state_current = &g_state_calendar;
	g_setting_digit = 8;
	g_disp[0] = LO(g_date);
	g_disp[1] = HI(g_date);
	g_disp[2] = LO(g_month);
	g_disp[3] = HI(g_month);
	g_disp[4] = LO(g_year);
	g_disp[5] = HI(g_year);
	g_disp[6] = 0;
	g_disp[7] = 2;
}
void StateChangeDispoff()
{
	int i = 0;
	for (i=0; i<DIGITS; ++i)
	{
		g_transition[i]=MAX_TRANSITION;
	}
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackDispoff;
	g_timer_v_callback = DynamicDispCallbackRandom;
	g_timer_v_callback_time_left = MSEC_TO_TIMERV * 300L;
	g_state_current = &g_state_dispoff;
	g_setting_digit = 8;
	g_disp[0] = DISP_OFF;
	g_disp[1] = DISP_OFF;
	g_disp[2] = DISP_OFF;
	g_disp[3] = DISP_OFF;
	g_disp[4] = DISP_OFF;
	g_disp[5] = DISP_OFF;
	g_disp[6] = DISP_OFF;
	g_disp[7] = DISP_OFF;
}
void StateChangeRandomDisp()
{
	g_timer_v_callback = DynamicDispCallbackRandom;
	g_timer_v_callback_time_left = MSEC_TO_TIMERV * 300L;
}
void StateChangeHourHSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackClock;
	g_timer_v_callback = DynamicDispCallbackNormal;
	//g_timer_v_callback_time_left = MSEC_TO_TIMERV * 300L;
	g_state_current = &g_state_hour_h_set;
	g_setting_digit = 7;
	g_blink_timer = 0;
}
void StateChangeHourLSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackClock;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_hour_l_set;
	g_setting_digit = 6;
	g_blink_timer = 0;
}
void StateChangeMinuteHSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackClock;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_minute_h_set;
	g_setting_digit = 4;
	g_blink_timer = 0;
}
void StateChangeMinuteLSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackClock;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_minute_l_set;
	g_setting_digit = 3;
	g_blink_timer = 0;
}
void StateChangeSecondHSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackClock;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_second_h_set;
	g_setting_digit = 1;
	g_blink_timer = 0;
}
void StateChangeSecondLSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackClock;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_second_l_set;
	g_setting_digit = 0;
	g_blink_timer = 0;
}
void StateChangeYearHSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackCalendar;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_year_h_set;
	g_setting_digit = 5;
	g_blink_timer = 0;
}
void StateChangeYearLSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackCalendar;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_year_l_set;
	g_setting_digit = 4;
	g_blink_timer = 0;
}
void StateChangeMonthHSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackCalendar;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_month_h_set;
	g_setting_digit = 3;
	g_blink_timer = 0;
}
void StateChangeMonthLSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackCalendar;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_month_l_set;
	g_setting_digit = 2;
	g_blink_timer = 0;
}
void StateChangeDateHSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackCalendar;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_date_h_set;
	g_setting_digit = 1;
	g_blink_timer = 0;
}
void StateChangeDateLSet()
{
	RtcDateGetBcd();
	g_timer_a_callback = ModeCallbackCalendar;
	g_timer_v_callback = DynamicDispCallbackNormal;
	g_state_current = &g_state_date_l_set;
	g_setting_digit = 0;
	g_blink_timer = 0;
}


#define SET_HI(c_, val_) (c_=(c_&0x0f)|(val_<<4))
#define SET_LO(c_, val_) (c_=(c_&0xf0)|val_)

void HourHDown()
{
	RtcDateGetBcd();
	HI(g_hour) == 0 ? SET_HI(g_hour, 2) : (g_hour-=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void HourHUp()
{
	RtcDateGetBcd();
	HI(g_hour) >= 2 ? SET_HI(g_hour, 0) : (g_hour+=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void HourLDown()
{
	RtcDateGetBcd();
	LO(g_hour) == 0 ? SET_LO(g_hour, 9) : (g_hour-=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void HourLUp()
{
	RtcDateGetBcd();
	LO(g_hour) >= 9 ? SET_LO(g_hour, 0) : (g_hour+=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void MinuteHDown()
{
	RtcDateGetBcd();
	HI(g_minute) == 0 ? SET_HI(g_minute, 5) : (g_minute-=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void MinuteHUp()
{
	RtcDateGetBcd();
	HI(g_minute) >= 5 ? SET_HI(g_minute, 0) : (g_minute+=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void MinuteLDown()
{
	RtcDateGetBcd();
	LO(g_minute) == 0 ? SET_LO(g_minute, 9) : (g_minute-=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void MinuteLUp()
{
	RtcDateGetBcd();
	LO(g_minute) >= 9 ? SET_LO(g_minute, 0) : (g_minute+=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void SecondHDown()
{
	RtcDateGetBcd();
	HI(g_second) == 0 ? SET_HI(g_second, 5) : (g_second-=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void SecondHUp()
{
	RtcDateGetBcd();
	HI(g_second) >= 5 ? SET_HI(g_second, 0) : (g_second+=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void SecondLDown()
{
	RtcDateGetBcd();
	LO(g_second) == 0 ? SET_LO(g_second, 9) : (g_second-=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void SecondLUp()
{
	RtcDateGetBcd();
	LO(g_second) >= 9 ? SET_LO(g_second, 0) : (g_second+=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void YearHDown()
{
	RtcDateGetBcd();
	HI(g_year) == 0 ? SET_HI(g_year, 9) : (g_year-=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void YearHUp()
{
	RtcDateGetBcd();
	HI(g_year) >= 9 ? SET_HI(g_year, 0) : (g_year+=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void YearLDown()
{
	RtcDateGetBcd();
	LO(g_year) == 0 ? SET_LO(g_year, 9) : (g_year-=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void YearLUp()
{
	RtcDateGetBcd();
	LO(g_year) >= 9 ? SET_LO(g_year, 0) : (g_year+=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void MonthHDown()
{
	RtcDateGetBcd();
	HI(g_month) == 0 ? SET_HI(g_month, 1) : (g_month-=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void MonthHUp()
{
	RtcDateGetBcd();
	HI(g_month) >= 1 ? SET_HI(g_month, 0) : (g_month+=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void MonthLDown()
{
	RtcDateGetBcd();
	LO(g_month) == 0 ? SET_LO(g_month, 9) : (g_month-=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void MonthLUp()
{
	RtcDateGetBcd();
	LO(g_month) >= 9 ? SET_LO(g_month, 0) : (g_month+=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void DateHDown()
{
	RtcDateGetBcd();
	HI(g_date) == 0 ? SET_HI(g_date, 3) : (g_date-=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void DateHUp()
{
	RtcDateGetBcd();
	HI(g_date) >= 3 ? SET_HI(g_date, 0) : (g_date+=0x10);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void DateLDown()
{
	RtcDateGetBcd();
	LO(g_date) == 0 ? SET_LO(g_date, 9) : (g_date-=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}
void DateLUp()
{
	RtcDateGetBcd();
	LO(g_date) >= 9 ? SET_LO(g_date, 0) : (g_date+=0x01);
	RtcDateSetBcd();
	g_blink_timer = BLINK_INTERVAL/2;
}




// メイン
void main(void)
{
	// 表示コールバックを設定
	g_timer_v_callback = DynamicDispCallbackRandom;
	g_timer_a_callback = ModeCallbackClock;
	g_timer_v_callback_time_left = MSEC_TO_TIMERV * 2000L;

	// マイコンを初期化
	InitH8();
	ChangeDigit(DIGITS_OFF_ALL, DISP_OFF);

	RtcInit();
	//RtcDateSetBcd();
	RtcDateGetBcd();

	// ステートマシンを初期化
	//最初は時計ステート
	g_state_clock.on_button0		= StateChangeDispoff;
	g_state_clock.on_button1		= StateChangeCalendar;
	g_state_clock.on_button2		= StateChangeHourHSet;
	g_state_hour_h_set.on_button0	= HourHDown;
	g_state_hour_h_set.on_button1	= HourHUp;
	g_state_hour_h_set.on_button2	= StateChangeHourLSet;
	g_state_hour_l_set.on_button0	= HourLDown;
	g_state_hour_l_set.on_button1	= HourLUp;
	g_state_hour_l_set.on_button2	= StateChangeMinuteHSet;
	g_state_minute_h_set.on_button0 = MinuteHDown;
	g_state_minute_h_set.on_button1 = MinuteHUp;
	g_state_minute_h_set.on_button2 = StateChangeMinuteLSet;
	g_state_minute_l_set.on_button0 = MinuteLDown;
	g_state_minute_l_set.on_button1 = MinuteLUp;
	g_state_minute_l_set.on_button2 = StateChangeSecondHSet;
	g_state_second_h_set.on_button0 = SecondHDown;
	g_state_second_h_set.on_button1 = SecondHUp;
	g_state_second_h_set.on_button2 = StateChangeSecondLSet;
	g_state_second_l_set.on_button0 = SecondLDown;
	g_state_second_l_set.on_button1 = SecondLUp;
	g_state_second_l_set.on_button2 = StateChangeClock;
	g_state_calendar.on_button0		= StateChangeClock;
	g_state_calendar.on_button1		= StateChangeDispoff;
	g_state_calendar.on_button2		= StateChangeYearHSet;
	g_state_year_h_set.on_button0	= YearHDown;
	g_state_year_h_set.on_button1	= YearHUp;
	g_state_year_h_set.on_button2	= StateChangeYearLSet;
	g_state_year_l_set.on_button0	= YearLDown;
	g_state_year_l_set.on_button1	= YearLUp;
	g_state_year_l_set.on_button2	= StateChangeMonthHSet;
	g_state_month_h_set.on_button0	= MonthHDown;
	g_state_month_h_set.on_button1	= MonthHUp;
	g_state_month_h_set.on_button2	= StateChangeMonthLSet;
	g_state_month_l_set.on_button0	= MonthLDown;
	g_state_month_l_set.on_button1	= MonthLUp;
	g_state_month_l_set.on_button2	= StateChangeDateHSet;
	g_state_date_h_set.on_button0	= DateHDown;
	g_state_date_h_set.on_button1	= DateHUp;
	g_state_date_h_set.on_button2	= StateChangeDateLSet;
	g_state_date_l_set.on_button0	= DateLDown;
	g_state_date_l_set.on_button1	= DateLUp;
	g_state_date_l_set.on_button2	= StateChangeCalendar;
	g_state_dispoff.on_button0		= StateChangeCalendar;
	g_state_dispoff.on_button1		= StateChangeClock;
	g_state_dispoff.on_button2		= StateChangeRandomDisp;
	StateChangeClock();
	
	EI; //?
	
	//main loop
	{
		// ステート
		static int prev_button0_pressed = 0;
		static int prev_button1_pressed = 0;
		static int prev_button2_pressed = 0;
		while (1)
		{
			// ボタン１
			if (IO.PDR1.BIT.B1 == 0)
			{	
				if ((g_state_current->on_button0 != NULL)
				&&	(prev_button0_pressed == 0))
				{
					(*(g_state_current->on_button0))();
				}
				
				prev_button0_pressed = 1;
			}
			else
			{
				prev_button0_pressed = 0;
			}
			
			// ボタン２
			if (IO.PDR1.BIT.B2 == 0)
			{	
				if ((g_state_current->on_button1 != NULL)
				&&	(prev_button1_pressed == 0))
				{
					(*(g_state_current->on_button1))();
				}
				
				prev_button1_pressed = 1;
			}
			else
			{
				prev_button1_pressed = 0;
			}
					
			// ボタン３
			if (IO.PDR1.BIT.B4 == 0)
			{	
				if ((g_state_current->on_button2 != NULL)
				&&	(prev_button2_pressed == 0))
				{
					(*(g_state_current->on_button2))();
				}
				
				prev_button2_pressed = 1;
			}
			else
			{
				prev_button2_pressed = 0;
			}

			//チャタリング防止
			wait_msec(20);
		}
	}
}
