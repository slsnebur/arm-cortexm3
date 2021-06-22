//
// Program mierzy czas utrzymywania sie stanu wysokiego na linii 0 portu A
// ("szerokosc" impulsu - czas pomiedzy kolejnymi zboczami: rosnacym i opadajacym)
// i wykorzystuje tryb Input Caputre ukladu licznikowego.
//
// Numery rozdzialow odnasza sie do "Reference Manual" procesorow STM43F103 firmy ST.
//
// Uruchomienie: w debugerze-symulatorze:
//
// - otworzyc okna: Peripherals->GPIO-> porty A i B
// - w oknie Watch 1 (View -> Watch Windows) dodac zmienna time_us i wyswielic jej wartosc w sys. dziesietnym
// - sterowanie linia A0 ("generowanie impulsu") w rzedzie "pins" (linia zanaczona - stan wysoki)

// opcjonalnie mozna otworzyc okna:
// - View->Analysis windows->Logic Analyzer (dobrac skale czasu)
// - timera 2 (Peripherals->Timers->Timer 2)

#include "stm32f10x.h"

volatile uint32_t time_us;

int main(){ 

// --- Konfiguracja peryferiow ---

	
// Zmiana zrodla syganlu zegarowego
// z wewnetrznego (HSI) na zewnetrzne (HSE)
	
	RCC->CR |= 1<<16;											// wlacz HSE
	while (!(RCC->CR & (1<<17)));					// czekaj na ustabilizowanie HSE
	RCC->CFGR = 1;												// HSE jako zegar glowny	
	RCC->CR &= ~1;			
		
//--- RCC - Reset Clock Control
	 	
  RCC->APB2ENR |= 1<<4 | 1<<3 | 1<<2;						// wlacz GPIO A, B i C
	RCC->APB1ENR |= 1<<0;										// wlacz timer TIM2

//--- Konfiguracja portow IO

	GPIOA->CRL = 0x22222228;							// linia 0 portu A - wejscie
	GPIOA->CRH = 0x22222222;							// pozostale linie portu A - wyjscia
	GPIOA->ODR = 0;
	GPIOB->CRL = 0x22222222;							// caly port B - wyjscia
	GPIOB->CRH = 0x22222222;	
	GPIOC->CRL = 0x22222222;							// caly port C - wyjscia
	GPIOC->CRH = 0x22222222;

//--- NVIC - Nested Vectored Interrupt Controller

	NVIC->ISER[0] |= 1<<28;					// wlacz przerwanie timera TIM2

//--- Konfiguracja timera TIM2

// --- 1 --- ustaw czestotliwosc zliczania licznika CNT rowna 1 kHz
// zgodnie z ustawieniami projektu timer taktowany jest sygnalem zegarowym
// o czestotliwosci 0.5 MHz
// 16-bitowy preskaler (PSC) dzieli przez liczebe o 1 wieksza niz wpisana!
// 15.1
/* 
15.1
1khz
*/
	
	TIM2->PSC	=	499;										// preskaler TIM2
 	
  TIM2->ARR = 65535;										// pojemnosc licznika - maks. czas pomiaru
	
// --- konfiguracja timera - Capture Mode	(wykonana)
	
// Capture/Compare Mode Register 1

	TIM2->CCMR1 |= 15<<5 | 1<<0; 					// Capture/Compare ch.1 jako wejscie, linia T1

// Slave Mode Control Register

	TIM2->SMCR |= 5<<4 | 4<<0;						// bity 210 - SMS = 100 - Slave Reset Mode
																				// bity 654  - TS = 101 - Trigger Selection - Timer Input 1
// Capture/Compare Enable Register

	TIM2->CCER |=  0<<1 | 1<<0;						// bit 1 = 0 reakcja na zbocze rosnace								
																				// bit 0 Capture/Compare ch.1 enable
// DMA/Interrupt Enable Register

	TIM2->DIER |= 1<<1;										// wlacz przerwanie dla Capture ch. 1

// --- 2 --- wlacz licznik
// Timer2 Control Register 1 (CR1), bit 0 (rozdzial 15.4.1)

 TIM2->CR1 = TIM_CR1_CEN;

// --- petla glowna ---

	for(;;) {
		GPIOC->ODR = TIM2->CNT;								// tylko do wizualizacji
		(GPIOB->ODR) ? (time_us=0) : (time_us=TIM2->CCR1);
	}
} 

// --- procedura obslugi przerwania TIM2 ---

void TIM2_IRQHandler(void) {

// --- 3 --- skasuj Capture/Compare ch.1 interrupt flag (pending request)
// Timer2 Status Register (SR), bit 1 (rozdzial 15.4.5)
// bit ten kasuje sie wpisujac na odpowiednia pozycje zero! (i nie modyfukujac pozostalych bitow)
	
	TIM2->SR = 0 << 1;

// --- 4 --- zmien reakcje na zbocze rosnace<->opadajace
// Timer2 Capture/Compare Enable Register (CCER), bit 1 (rozdzial 15.4.9)
// w kazdym wywolaniu procedury obslugi przerwania zmien ten bit na przeciwny
	
/* Za kazdym razem na przeciwne (XOR)*/
	TIM2->CCER ^= 1 << 1;
		
	GPIOB->ODR ^= 0xFFFF;										// podczas pomiaru czasu zmien stan wszystkich linii portu B na 1	
}
