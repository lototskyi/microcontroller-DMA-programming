#include "stm32f407xx.h"

void EXTI0_IRQHandler(void);
void DMA1_Stream6_IRQHandler(void);
void clear_exti_pending_bit(void);

extern void HT_Complete_callback(void);
extern void FT_Complete_callback(void);
extern void TE_Complete_callback(void);
extern void FE_Complete_callback(void);
extern void DME_Complete_callback(void);

#define	is_it_HT()	DMA1->HISR & DMA_HISR_HTIF6
#define is_it_FT()  DMA1->HISR & DMA_HISR_TCIF6
#define is_it_TE()  DMA1->HISR & DMA_HISR_TEIF6
#define is_it_FE()  DMA1->HISR & DMA_HISR_FEIF6
#define is_it_DME() DMA1->HISR & DMA_HISR_DMEIF6

void clear_exti_pending_bit(void)
{
	EXTI_TypeDef *pEXTI;
	pEXTI = EXTI;
	
	if (pEXTI->PR & EXTI_PR_PR0) {
		pEXTI->PR |= EXTI_PR_PR0;
	}
}


void EXTI0_IRQHandler(void)
{
	//send UART2_TX DMA request to DMA1 controller
	USART_TypeDef *pUART2;
	pUART2 = USART2;
	
	pUART2->CR3 |= USART_CR3_DMAT;
	
	clear_exti_pending_bit();
}

void DMA1_Stream6_IRQHandler(void)
{
	//Half-transfer
	if (is_it_HT()) {
		DMA1->HIFCR |= DMA_HIFCR_CHTIF6;
		HT_Complete_callback();
	} else if (is_it_FT()) { //Full-transfer
		DMA1->HIFCR |= DMA_HIFCR_CTCIF6;
		FT_Complete_callback();
	} else if (is_it_TE()) { //Transfer error
		DMA1->HIFCR |= DMA_HIFCR_CTEIF6;
		TE_Complete_callback();
	} else if (is_it_FE()) { //FIFO overrun/underrun
		DMA1->HIFCR |= DMA_HIFCR_CFEIF6;
		FE_Complete_callback();
	}  else if (is_it_DME()) { //direct mode error
		DMA1->HIFCR |= DMA_HIFCR_CDMEIF6;
		DME_Complete_callback();
	} else {
		
	}
}
