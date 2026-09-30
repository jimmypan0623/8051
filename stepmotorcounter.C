/* 鍵盤+步進馬達+計數器*/
//==宣告區================================================
#include <AT89X51.H>
#include <INTRINS.H>
//#define	KEYP	 P2		// 掃瞄輸出埠(高位元)及鍵盤輸入埠(低位元)
//#define	SCANP	 P3		// 七節顯示器掃瞄信號輸出埠 
//#define	SEGP	 P0		// 七節顯示器顯示信號輸出埠 
#define	digits	 5		// 設定七節顯示器位數 

char flg=0;       //判斷是否有按井號以啟動步進馬達開始對P3_6 腳做計數
char rcr=0;       //判斷按井號順時鐘轉或按米號逆時鐘轉
long dx=0;      //設個變數紀錄最後的七段顯示器數字		
long fx=0;
unsigned char drive;      //步進馬達激磁相位位元數設定變數
void conv(unsigned long x); //各位元數字碼紀錄
void kill0(void);           //數字頭去零 
void keypad(void);          //僅掃描 * 號跟 # 號的鍵盤掃描以讓轉動中的啟動馬達暫停或繼續啟動
char code TAB[]=			// 共陽七節顯示器(g~a)編碼 
{	0xc0, 0xf9, 0xa4, 0xb0, 0x99,	// 數字0-4
 	0x92, 0x82, 0xf8, 0x80, 0x90,	// 數字5-9
	0xa0, 0x83, 0xa7, 0xa1, 0x84,	// 字母a-e(10-14)
 	0x8e, 0xbf, 0xff}; 				// 字母F(15),負號(-),空白(17)
long disp[]={ 17, 17, 17, 17, 0}; 	// 顯示陣列初值為0
char code keyLocation[]={	1, 2, 3, 0xa0, 	// 鍵盤配置陣列 
  							4, 5, 6, 0x83, 
					  		7, 8, 9, 0xa7, 
  							0x84, 0, 0x8e, 0xa1 };
char  oldKcode;				// 宣告變數
void  delay500us(int x);			// 宣告0.5ms延遲函數 
void  beep(char x);					// 宣告嗶聲函數 
void  keyScan(void);				// 宣告掃瞄函數 
void SegScan(unsigned char k);				//掃描各位元七段顯示器	
		
//==主程式================================================
main()								// 主程式開始 								
{			  
	while(1)               //無限迴圈
	{		
		 
	  while(!flg)						
	  {					 		  	
		  keyScan();					// 掃瞄鍵盤 	   	
			SegScan(1);         //掃描各位元七段顯示器				
	  }		 
		drive=0xcc;    //步進馬達初始值即設為2相激磁			 
		dx=0;
		while(flg)	
		{ 								
			conv(dx);         //七段顯示器轉換數值+0 再轉換回七段顯示器數值
			if (dx==fx && fx>0){
			     beep(1);	          
			     flg=0;			  
				   dx=0;
			     P1=0xff;				
				   rcr=0;	 
		   } 
			 else
			 {			
		     do 
			   { 			
			   	SegScan(1);  //掃描各位元七段顯示器
			   }		  
			   while (P3_6==0);		      			
		     do 
			   {			
				   SegScan(1);  //掃描各位元七段顯示器
			   }
      while(P3_6==1);					
			dx++;		      	
	  	}			
    }	 	 		
		
	}	 
}									// 主程式結束 
// === 延遲函數,延遲約x*0.5ms ================================
void delay500us(int x)				// 延遲函數開始 
{	int i,j;						// 宣告整數變數i,j 
	for(i=0;i<x;i++)				// 計數x次,延遲約*0.5ms		
		for(j=0;j<60;j++);			// 計數60次,延遲約0.5ms 
}									// 延遲函數結束 
// === 嗶聲函數 ================================
void  beep(char x)					// 嗶聲函數開始 
{	char i,j;						// 宣告字元變數i,j 
	for(i=0;i<x;i++)				// 執行發聲x次 
	{	for(j=0;j<100;j++)			// 重複吸放蜂鳴器100次 
		{	P3_7=0;delay500us(1);	// 蜂鳴器吸下約0.5ms 
			P3_7=1;delay500us(1);	// 蜂鳴器放開約0.5ms 
		}						 
		delay500us(200);			// 靜音約0.1s 
	}							 
}									// 嗶聲函數結束  
// ======= 掃瞄4*4鍵盤及4個七節顯示器函數 ================
void keyScan(void)					// 掃瞄函數開始 	
{	unsigned char col,row,dig;  	// 宣告變數(col:行,row:列,dig:顥示位)
	unsigned char rowkey,kcode;		// 宣告變數(rowkey:列鍵值,kcode:按鍵碼)
	for(col=0;col<4;col++)			// for迴圈,掃瞄第col行 
	{	P2=~(0x10<<col)|0x0F;		// 高4位輸出掃瞄信號 
		rowkey = ~P2 & 0x0F;
		// 讀入KEYP低4位，反相再清除高4位求出列鍵值	
		if(rowkey != 0)						// 若有按鍵 
		{	if(rowkey == 0x01)    row=0;	// 若第0列被按下 
			else if(rowkey == 0x02) row=1;	// 若第1列被按下 
			else if(rowkey == 0x04) row=2;	// 若第2列被按下 
			else if(rowkey == 0x08) row=3;	// 若第3列被按下 
			oldKcode = 4 * col + row; 		// 計算鍵值 
			kcode = keyLocation[oldKcode]; 	// 轉換鍵值 
			if(!flg)
			{
	  	  if(kcode<10) //有效數字才顯示
		    {
			     for(dig = 0;dig < digits-1;dig++)// 顯示陣列之左3字 		
           {	
					 					 
				      disp[dig]=disp[dig+1];		// 將右側數值左移1位  	
           }					 
				   disp[digits-1]=kcode;			// 鍵值寫入個位數 		
			     //beep(1);						// 嗶一聲 			
				
		    }
				 
					fx=(disp[4]>9?0:disp[4])%10+(disp[3]>9?0:disp[3])*10+(disp[2]>9?0:disp[2])*100+(disp[1]>9?0:disp[1])*1000+(disp[0]>9?0:disp[0])*10000; //換算回數字	 
					switch (kcode)
					{
						case 0xa0 ://按 A
						     fx++;						   
					       break;
						case 0x83:   //按 B											 
						     fx--;						   
						     break;
						case 0xa7:            //按 C				  
						     fx=0;				   
						     break;          	
						case 0xa1:        //按 D			   
					    	fx/=10;				     
						    break;			   
       	 }
					if(kcode==0x8e)          //按 # 號執行步進馬達旋轉計數		
            {							
		      	     rcr=0;                  //順時鐘轉				
			 
			            flg=1;	
						}    
				    else if(kcode== 0x84)     //按 * 號    
            {							
				         rcr=1;           //逆時鐘轉
		
				        flg=1;	
						}
			   conv(fx);  
		   } 						
			      
						
	
			 while(rowkey != 0)				// 當按鍵未放開 	
			  {	
			  	rowkey=~P2 & 0x0F;		// 再讀入列鍵值 			  
			  }        
	   
		}									// if敘述(有按鍵時)結束 
		
		delay500us(2);						// 延遲1ms
     
	}										// for迴圈結束(掃瞄col行)	
	
}							// 掃瞄函數scanner()結束 					

void conv(long x)
{		
	if(x>=0 && x < 100000){	  
		disp[4]= x%10;
		disp[3]= (x/10)%10;
		disp[2]= (x/100)%10;
	   disp[1]= (x/1000)%10;
     disp[0]= (x/10000)%10;
   	 kill0();	    
	}	
}

void kill0(void)     //除零頭函數
{	
int u=0	;
 
	while (disp[u]==0 && u<4)
	{
		disp[u]=17;
		u++;
	} 	
}

void SegScan(unsigned char  k)
{
  unsigned char  m,n;
  unsigned char  com;
  com=0xef;
	for(n=0;n<k;n++)
	{
    for(m=0; m<5; m++)
    {
      P0=TAB[disp[m]];		// 輸出顯示信號 	
      P3=com;
      delay500us(2);
	    P3=0xff;			 
	    com=_cror_(com,1);	    
    }			
	}		
		keypad();      //檢查鍵盤是否有被按暫停鍵
	  if (flg)           //如果鍵盤上按井號則在此激磁步進馬達令其轉動
    {   
			if(rcr)
				drive=_cror_(drive,1);   		 	 
			else
				 drive=_crol_(drive,1);	
			P1=drive;
	  }	
	 
}		
void keypad(void)   //只掃描最下面那一列鍵盤
{	
	P2=0x7f;
	if(P2_0==0) 	
		 flg=0;  //按 * 號暫停		
	else if(P2_2==0)
	   flg=1;      //按 # 繼續
}

