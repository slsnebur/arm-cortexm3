#include "stm32f10x.h"

// Switch debouncing:
// filtracja drgan zestykow (przycisku, wylacznika krancowego itp.) 
//
// "Symulowany" przycisk podlaczony jest do linii 0 portu A:
// nacisiniety: styki zwarte - stan wysoki
// domyslnie - zwolniony: styki rozwarte - stan niski
//
// W procedurze obslugi przerwania timera probkowany jest stan linii A0:
// - 16 kolejnych, jednakowych probek powinno dawac jednoznaczna informacje
// o stanie przycisku (wartosc zmiennej sw_state = 1 - przycisk nacisniety). 
// - w przeciwnym przypadku (szybsze zmiany satnu na linii A0, > 16 probek
// o jednakowej wartosci) stan przycisku pozostje niezmieniony.
//
// Na podstawie wartosci zmiennej sw_state (wspoldzielonej miedzy ISR i "main"),
// w petli glownej wysterowywana jest linia 15 portu C.

volatile uint32_t sw_state = 0, buff;

void TIM2_IRQHandler(void);

int main(void){

// Zmiana zrodla syganlu zegarowego
// z wewnetrznego (HSI) na zewnetrzne (HSE)
	
	RCC->CR |= 1<<16;											// wlacz HSE
	while (!(RCC->CR & (1<<17)));					// czekaj na ustabilizowanie HSE
	RCC->CFGR = 1;												// HSE jako zegar glowny	
	RCC->CR &= ~1;												// wylacz HSI

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
	

//--- NVIC Nested Vectored Interrupt Controller
	
	NVIC->ISER[0] |= 1<<28;					// wlacz przerwanie timera TIM2

//--- Konfiguracja timera TIM2
//
// --- 1 ---
//
// Ustawic "odpowiednia" czestotliwosc probkowania stanu przycisku
// (czestotliwosc generowania przerwan przez TIM2).
// Czestotliwosc sygn. taktujacego timer = 1 MHz
// Preskaler i licznik 16 bitowe

	TIM2->PSC = 9999;
	TIM2->ARR = 4;
	TIM2->DIER |= 1;
	TIM2->CR1 |= 1;

// --- petla glowna ---

	for(;;) {
	
// --- 2 ---
//
// Dopisac, wykorzystujac Bit Set/Reset Register (BSRR):
//		
// jezeli swtch_state = 1 to linia 15 portu C = 1, w przeciwnym wypadku = 0
//
// STM32F103 Reference manual, rozdzial 9, opis rejestru BSRR str. 173.		
		if(sw_state) GPIOC->BSRR = 1<<15;
		else GPIOC->BSRR = 1<<31;				
	}
}

// --- procedura obslugi przerwania TIM2 ---

void TIM2_IRQHandler(void) {
	
	TIM2->SR &= ~1;

// --- 3 ---
//
// Zadanie opcjonalne - zmodyfikowac "pojemnosc" bufora
// oraz liczbe testowanych probek.

	buff = (( buff << 1 ) | ( GPIOA->IDR & 1 )) & 0xFFFF;
	
	GPIOB->ODR = buff;	//tylko dla "wizualizacji" w symulatorze
		
	if (buff == 0xFFFF) sw_state = 1;
	if (buff == 0x0000) sw_state = 0;
	
}
