#include "stm32f10x.h"

// Program demonstracyjny, sluzacy tylko do przetestowania poprawnosci
// konfiguracji srodowiska uVision oraz opanowania obslugi symulatora/debuggera.

// Z czestotliwoscia 2 Hz wywolywana jest procedura obslugi przerwania timera
// zmieniajaca stan linii wyjsciowych portu B - sprawdzic, czy symulator
// zachowuje poprawna szybkosc.
// W petli glownej inkrementowana jest zmienna. Inkrementacje mozna zatrzymac
// wymuszajac stan wysoki na dowolnej linii wejsciowej portu A,
// (zaznacajac odpowiedni bit "Pins" w oknie General Purpose IO A debuggera).

void TIM2_IRQHandler(void);

static uint32_t main_counter;

int main() { 

// Zmiana zrodla syganlu zegarowego
// z wewnetrznego (HSI) na zewnetrzne (HSE)
	
	RCC->CR |= 1<<16;											// wlacz HSE
	while (!(RCC->CR & (1<<17)));					// czekaj na ustabilizowanie HSE
	RCC->CFGR = 1;												// HSE jako zegar glowny	
	RCC->CR &= ~1;												// wylacz HSI
		
//--- RCC - Reset Clock Control
	
  RCC->APB2ENR |= (1<<3)|(1<<2);				// wlacz sygnal zegarowy portow GPIO A i B
	RCC->APB1ENR |= (1<<0);								// wlacz sygnal zegarowy timer TIM2 
  
//--- Konfiguracja portow IO
	
	GPIOA->CRL = 0x44444444;							// mlodsze 8 linii portu A - wejscia
	GPIOA->CRH = 0x22222222;							// starsze 8 linii portu A - wyjscia
	GPIOB->CRL = 0x22222222;							// caly port B - wyjscia
	GPIOB->CRH = 0x22222222;
	
//--- NVIC Nested Vectored Interrupt Controller
	
	NVIC->ISER[0] |= (1<<28);							//wlacz przerwanie od timera TIM2 - nr 28

//--- Konfiguracja timera TIM2

	TIM2->PSC		= 1999;										// preskaler=2000: 1 MHz / 2000 = 0.5 kHz
  TIM2->ARR		= 499;										// pojemnosc licznika=500 (od 0 do 499), przepelnienie co 1 s (1 Hz)
  TIM2->DIER	|= 1;											// wlacz generowanie przerwan w chwili pzepelnienia licznika 
  TIM2->CR1		|= 1;											// wlacz timer

// --- petla glowna ---

	for(;;){			
		main_counter++;
		while (GPIOA->IDR & 255);						// "przyblokuj" wykonywanie petli glownej programu
	}																			// jesli ktorakolwiek z mlodszych linii potu A
																				// jest w stanie wysokim
} 

// --- procedura obslugi przerwania TIM2 ---

void TIM2_IRQHandler(void) {
	
	TIM2->SR &= ~1;												// skasuj flage UIF - oczekiwania na obsluge przerwania
	GPIOB->ODR ^= 0xFFFF;									// zmien stan wszystkich linii portu B na przeciwny
}