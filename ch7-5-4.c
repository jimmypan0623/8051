/* ch7-5-5.c - 秒錶實驗 - 七段顯示器 (000.00 ~ 999.99秒) */
//== 標頭檔與腳位定義 ================================
#include    <reg51.h>   // 包含 8x51 暫存器定義 
#define SEGP    P0      // 七段顯示器資料埠 Port 0 (a~h)
#define SCANP   P2      // 七段顯示器掃描埠 Port 2 (P2.0~P2.4，控制哪一位元顯示)
sbit    PB0=P3^2;       // 按鈕 PB0 開啟 P3^2 (INT0 啟動/暫停)
sbit    PB1=P3^3;       // 按鈕 PB1 開啟 P3^3 (INT1 歸零)

/* 設定 T0 計時器：計時 0.01 秒 (10ms) 中斷一次 */  
#define  count_M1   10000               // T0 (MODE 1) 計時值，10ms
#define  TH_M1  (65536-count_M1)/256    // T0 (mode 1) 高 8 位元
#define  TL_M1  (65536-count_M1)%256    // T0 (mode 1) 低 8 位元

/* 設定 T1 計時器：掃描時間 0.25ms 中斷一次 */
#define  count_M2   250                 // T1 (mode 2) 計時值，0.25ms
#define  TH_M2  (256-count_M2)          // T1 (mode 2) 重載暫存器 
#define  TL_M2  (256-count_M2)          // T1 (mode 2) 計數器 
char count_T1=0;                    // 計錄 T1 中斷次數  

/* 共陽極七段顯示器編碼表 (0-9) */
char code TAB[10]={ 0xc0, 0xf9, 0xa4, 0xb0, 0x99,   // 數字 0-4
                    0x92, 0x83, 0xf8, 0x80, 0x98 }; // 數字 5-9

/* 顯示 5 位數的資料緩衝區 (預設 000.00) */
char disp[5]={ 0, 0, 0, 0, 0 };                

/* 全域變數宣告 */   
unsigned int time_count=0;          // 計時器計數值 (注意：若要達到 999.99 秒需要 99999，
                                    // 建議改為 unsigned long，因為 int 最大只到 65535)
char scan=0;                        // 掃描顯示位元 (0~4) 

//== 主程式 ================================
main()                      
{   EA=EX0=EX1=ET0=ET1=1;   // 開啟全域中斷與外部中斷0/1及Timer0/1中斷 
    PT1=1;                  // 設定 T1 為高優先權中斷 
    TCON=0x05;              // 設定 INT0 與 INT1 為邊緣觸發 
    TMOD=0x21;              // T1 為 mode 2 (8位元自動重載), T0 為 mode 1 (16位元)
    TH0=TH_M1; TL0=TL_M1;   // 載入 T0 初值
    TR0=0;                  // 暫時停止 T0 計時器
    TH1=TH_M2; TL1=TL_M2;   // 載入 T1 初值 
    TR1=1;                  // 啟動 T1 (持續進行七段顯示器掃描)
    P3=0xFF;                // 設定 P3 埠為輸入模式 
    while(1);               // 無窮迴圈，等待中斷觸發 
}                            

//== T0 中斷服務程式 - 計時器累加 ===================
void T0_10ms(void) interrupt 1 // T0 中斷服務程式 (每 10ms 觸發一次)
{   TH0=TH_M1; TL0=TL_M1;   // 重新載入 T0 計數值 
    
    // 累加計數值 (若使用 unsigned int，最大至 65535 後溢位)
    if (++time_count == 65536) 
    {   
        time_count = 0;     // 超過上限歸零
    }                       
    
    // 分割 5 位數至顯示陣列
    disp[0] = time_count % 10;           // 個位數 (000.0X 秒)
    disp[1] = (time_count / 10) % 10;    // 十位數 (000.X0 秒)
    disp[2] = (time_count / 100) % 10;   // 百位數 (00X.00 秒) -> 包含小數點
    disp[3] = (time_count / 1000) % 10;  // 千位數 (0X0.00 秒)
    disp[4] = (time_count / 10000) % 10; // 萬位數 (X00.00 秒)
}


//=== T1 中斷服務程式 - 動態掃描 5 位數顯示器 ===========================
void  T1_2ms(void)  interrupt  3    // T1 中斷服務程式
{   if (++count_T1==8)      // 計數 8 次，0.25ms * 8 = 2ms 進行一次切換
    {   count_T1=0;         
        SEGP=0xFF;          // 消影 (關閉顯示，避免殘影)
        SCANP=~(1<<scan);   // 選擇當前顯示位元 (P2.0~P2.4 低電位致能)
        
        // 當 scan == 2 時 (代表第 3 位數，即 00X.00)，點亮小數點 (dp)
        if (scan == 2) {
            SEGP = TAB[disp[scan]] & 0x7F; // 清除最高位元(bit 7)點亮小數點
        } else {
            SEGP = TAB[disp[scan]];        // 一般數字顯示
        }
        
        if (++scan==5) scan=0; // 輪流掃描 5 個位數，滿 5 歸零 	
    }                       
}                           

//== INT0 中斷服務程式 - 啟動 / 暫停 ==================
void int0_sw(void) interrupt 0 
{   TR0=!TR0;               // 切換 Timer 0 的啟動/停止狀態 
    while(!PB0);            // 放開按鈕檢測 (防彈跳/等待放開)
}                           

//== INT1 中斷服務程式 - 歸零重置 =======================
void int1_RST(void) interrupt 2     
{   while(!PB1);            // 放開按鈕檢測
    TR0=0;                  // 停止計時
    time_count=0;           // 計數值歸零 
    disp[0]=disp[1]=disp[2]=disp[3]=disp[4]=0; // 顯示陣列全部清零
}