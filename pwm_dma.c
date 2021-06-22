#include "stm32f10x.h"

volatile uint32_t CouNTer, CCR1_register;

//--- 1 ---
//
// Uzupelnic tablice duty_tab (kilkanascie elementow)
// wspolczynnikami wypelnienia
// z zakresu 0 - ARR (maks. 2^16 - 1 = 65535)
// w praktyce (symulacja...) ARR = maks. kilka tysiecy.

 uint16_t duty_tab[8]={0, 127, 63, 255, 511, 900, 800, 1023};

int main ()  { 

// --- Konfiguracja peryferiow ---
	
// Zmiana zrodla syganlu zegarowego
// z wewnetrznego (HSI) na zewnetrzne (HSE)
	
	RCC->CR |= 1<<16;											// wlacz HSE
	while (!(RCC->CR & (1<<17)));					// czekaj na ustabilizowanie HSE
	RCC->CFGR = 1;												// HSE jako zegar glowny	
	RCC->CR &= ~1;		

//--- RCC - Reset Clock Control
 	
  RCC->APB2ENR |= (1<<2) | (1<<0);			//wlacz GPIO A i B	
	RCC->APB1ENR |= (1<<1) | (1<<0);			//wlacz TIM2 i 3 	
	RCC->AHBENR |= 1;											//wlacz DMA1

//--- Konfiguracja linii 6 portu A
	
	GPIOA->CRL  &= ~(15<<24);							//wyzeruj pola MODE6 i CNF6
	GPIOA->CRL  |= (1<<24);								//MODE - output
	GPIOA->CRL  |= (2<<26);								//CNF - alternate out, push-pull

//--- Konfiguracja timera TIM2
//
//--- 2a ---
//
// Ustawic jego czestotliwosc 
// generowania zgloszen transferow DMA
// na kilka (np. 0.5 - 5) Hz

	// 2hz
	TIM2->PSC			= 999;
  TIM2->ARR     = 499;
  TIM2->DIER   |= 1<<8;       					//DMA/IRQ Enable Register - en IRQ on update 
  TIM2->CR1    |= 1;	// wlacz timer 

//--- Konfiguracja timera TIM3
//
//--- 2b ---
//
// Generator fali prostokatnej:
// ustawic jego czestotliwosc ok. 10 - 20 x wieksza
// od czestotliwosci zgloszen trnasferow DMA (TIM2)
// majac na uwadze, ze pojemnosc licznika (ARR) musi odpowiadac 
// maks. wartosci z tablicy duty_tab

// przykladowo:
// wspolczynniki wypelnienia 0 - 999
// ARR = 999 (1000 stanow)
// czetotliwosc przebiegu PWM 10 Hz
// zgloszenia DMA co 1 s

// 20hz
	TIM3->PSC 		= 48;
	TIM3->ARR 		= 1023;

// Capture/Compare Mode Register 1

	TIM3->CCMR1 |= (6<<4)|(1<<3);					//PWM mode 1, CC preload, Ch.1 - output
	
// Capture/Compare Enable Register
		
	TIM3->CCER |= (0<<1)|(1<<0);					//polaryzacja: aktywny stan wysoki, sygnal PWM na wyjscie portu
	
	TIM3->CR1 |= 1<<7 | 1;								//wlacz licznik i buforowanie ARR
	
//--- Konfiguracja DMA
// Configuration Register - Bity: 

//4 - kierunek przeplywu danych
//5 - Circular mode
//7-6 - inkrementacja adresow
//11-10, 9-8 typy przesylanych danych

	DMA1_Channel2->CCR = 1<<10 | 1<<8| 1<<7 |1<<5 | 1<<4;

//--- 3 ---
//
// liczba przesylanych elementow tablicy duty_tab
	
	DMA1_Channel2->CNDTR = 8;
	
// adresy: zrodlowy i docelowy
	
	DMA1_Channel2->CPAR =(uint32_t)&(TIM3->CCR1);	
	DMA1_Channel2->CMAR =(uint32_t)duty_tab;
	
//wlacz DMA
	
	DMA1_Channel2->CCR |= 1;
	
//--- 4 ---
//	
//W setupie Logic Analyzera
//poprawnie zdefiniowac min i max wartosci zmiennych: 
//CouNTer i CCR1_register (kopie rejestrow timera TIM3)

	for(;;)	{		
		CouNTer = TIM3->CNT;				// tylko w celu wizualizacji!
		CCR1_register=TIM3->CCR1;
	}
} 
