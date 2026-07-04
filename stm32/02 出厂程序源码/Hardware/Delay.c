/*延时函数（粗延时），分别是纳秒、微秒、毫秒*/

#include "Delay.h"

void Delay_Init(void){
    
}

void Delay(u16 t) {
	while(t--);
	return;
}

void Delay_ns(u16 t) {
	while(t--);
	return;
}

void Delay_us(u16 t) {  
   while(t--) {
      Delay(6);    
   }	
}

void Delay_ms(u16 t) { 
   while(t--) {
      Delay_us(1000);    
   }
}
