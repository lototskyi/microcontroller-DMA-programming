

#include <stdint.h>
#include "stm32f407xx.h"

void button_init(void);
void uart2_init(void);
void dma1_init(void);
void enable_dma1_stream6(void);
void dma1_interrupt_configuration(void);
static void send_data(void);

void HT_Complete_callback(void);
void FT_Complete_callback(void);
void TE_Complete_callback(void);
void FE_Complete_callback(void);
void DME_Complete_callback(void);


#define BASE_ADDR_OF_GPIOA_PERI		GPIOA

static char data[] = "Hello World\r\n";

int main(void)
{
	button_init();
	uart2_init();
	//send_data();
	dma1_init();
	dma1_interrupt_configuration();
	enable_dma1_stream6();
	
	while(1);
	
	return 0;
}

static void send_data(void)
{
	USART_TypeDef *pUART2;
	pUART2 = USART2;
	
	uint32_t len = sizeof(data);
	
	for (uint32_t i = 0; i < len; i++) {
		//make sure that in the status register TXE is set (TDR is empty)
		while( !(pUART2->SR & USART_SR_TXE) );
		
		pUART2->DR = data[i];
	}
	
}

void button_init(void)
{
	GPIO_TypeDef *pGPIOA;
	pGPIOA = BASE_ADDR_OF_GPIOA_PERI;
	
	RCC_TypeDef *pRCC;
	pRCC = RCC;
	
	EXTI_TypeDef *pEXTI;
	pEXTI = EXTI;
	
	SYSCFG_TypeDef *pSYSCFG;
	pSYSCFG = SYSCFG;
	
	//1. enable the clock for the GPIOA peripheral
	pRCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	
	//2. keep the gpio pin in input mode
	pGPIOA->MODER &= ~GPIO_MODER_MODER0;
	
	//3. enable the interrupt over that gpio pin
	pEXTI->IMR |= EXTI_IMR_MR0;
	
	//4. enable the clock for SYSCFG
	pRCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	
	//5. config SYSCFG EXTICR1 register
	pSYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;
	pSYSCFG->EXTICR[0] |= (SYSCFG_EXTICR1_EXTI0_PA << SYSCFG_EXTICR1_EXTI0_Pos);
	
	//6. configure the edge detection
	pEXTI->RTSR |= EXTI_RTSR_TR0;
	
	//7. enable the IRQ related to that gpio pin in NVIC of the processor
	NVIC_EnableIRQ(EXTI0_IRQn);
	
}

void uart2_init(void)
{
	RCC_TypeDef *pRCC;
	pRCC = RCC;
	
	GPIO_TypeDef *pGPIOA;
	pGPIOA = GPIOA;
	
	USART_TypeDef *pUART2;
	pUART2 = USART2;
	
	//1. enable the peripheral clock for the uart2 peripheral
	pRCC->APB1ENR |= RCC_APB1ENR_USART2EN;
	
	//2. configure the gpio pins for uart_tx and uart_rx functionality
	//PA2 as TX, PA3 as RX
	//clock for GPIOA is alredy enabled by button_init
	
	//alternate function mode
	//PA2
	pGPIOA->MODER &= ~GPIO_MODER_MODER2;
	pGPIOA->MODER |= GPIO_MODER_MODER2_1;
	
	//PA3
	pGPIOA->MODER &= ~GPIO_MODER_MODER3;
	pGPIOA->MODER |= GPIO_MODER_MODER3_1;
	
	//AF7
	//PA2
	pGPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2;
	pGPIOA->AFR[0] |= (0x7 << GPIO_AFRL_AFSEL2_Pos);
	
	//PA3
	pGPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL3;
	pGPIOA->AFR[0] |= (0x7 << GPIO_AFRL_AFSEL3_Pos);
	
	//pull up resistors
	//PA2
	pGPIOA->PUPDR &= ~GPIO_PUPDR_PUPD2;
	pGPIOA->PUPDR |= GPIO_PUPDR_PUPD2_0;
	
	//PA3
	pGPIOA->PUPDR &= ~GPIO_PUPDR_PUPD3;
	pGPIOA->PUPDR |= GPIO_PUPDR_PUPD3_0;
	
	//3. configure the baudrate
	pUART2->BRR = 0x8B;
	
	//4. configure the data width, number of stop bits, etc...
	//default configuration is ok
	
	//5. enable the TX engine of the uart peripheral
	pUART2->CR1 |= USART_CR1_TE;
	
	//6. enable the uart peripheral
	pUART2->CR1 |= USART_CR1_UE;
	
}

void dma1_init(void)
{
	RCC_TypeDef *pRCC;
	pRCC = RCC;
	
	DMA_TypeDef *pDMA;
	pDMA = DMA1;
	
	DMA_Stream_TypeDef *pSTREAM6;
	pSTREAM6 = DMA1_Stream6;
	
	USART_TypeDef *pUART2;
	pUART2 = USART2;
	
	//1. enable the peripheral clock for the dma1
	pRCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
	
	//2. identify the stream which is sutable for your peripheral
	//channel 4, stream 6
	
	
	//3. identify the channel number on which uart2 peripheral sends dma request
	//channel 4
	pSTREAM6->CR &= ~DMA_SxCR_CHSEL;
	pSTREAM6->CR |= (0x4 << DMA_SxCR_CHSEL_Pos);
	
	//4. program the source address
	pSTREAM6->M0AR = (uint32_t) data;
	
	//5. program the destination address 9peripheral)
	pSTREAM6->PAR = (uint32_t)&pUART2->DR;
	
	//6. program number of data items to send
	uint32_t len = sizeof(data);
	pSTREAM6->NDTR = len;
	
	//7. the direction of data transfer (m2p, p2m, m2m)
	pSTREAM6->CR &= ~DMA_SxCR_DIR;
	pSTREAM6->CR |= DMA_SxCR_DIR_0;
	
	//8. program the source and destination data width (byte by byte)
	pSTREAM6->CR &= ~DMA_SxCR_MSIZE;
	pSTREAM6->CR &= ~DMA_SxCR_PSIZE;
	//8a. enable memory autoincrement
	pSTREAM6->CR |= DMA_SxCR_MINC;
	
	//9. direct mode or fifo mode
	pSTREAM6->FCR |= DMA_SxFCR_DMDIS;
	
	//10. select the fifo threshold
	pSTREAM6->FCR |= DMA_SxFCR_FTH;
	
	//11. enable the circular mode if required
	
	//12. single transfer or burst transfer
	
	//13. stream priority

}

void enable_dma1_stream6(void)
{
	DMA_Stream_TypeDef *pSTREAM6;
	pSTREAM6 = DMA1_Stream6;
	pSTREAM6->CR |= DMA_SxCR_EN;
}

void dma1_interrupt_configuration(void)
{
	DMA_Stream_TypeDef *pSTREAM6;
	pSTREAM6 = DMA1_Stream6;
	
	//1. half-transfer IE
	pSTREAM6->CR |= DMA_SxCR_HTIE;
	
	//2. transfer complete IE
	pSTREAM6->CR |= DMA_SxCR_TCIE;
	
	//3. transfer error IE
	pSTREAM6->CR |= DMA_SxCR_TEIE;
	
	//4. FIFO overrun/underrun IE
	pSTREAM6->FCR |= DMA_SxFCR_FEIE;
	
	//5. direct mode error
	pSTREAM6->CR |= DMA_SxCR_DMEIE;
	
	//6. enable the IRQ for DMA1 stream6 global interruptin NVIC
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);
	
}

void HT_Complete_callback(void)
{
	
}

void FT_Complete_callback(void)
{
	//send UART2_TX DMA request to DMA1 controller
	USART_TypeDef *pUART2;
	pUART2 = USART2;
	
	DMA_Stream_TypeDef *pSTREAM6;
	pSTREAM6 = DMA1_Stream6;
	
	uint32_t len = sizeof(data);
	pSTREAM6->NDTR = len;
	
	pUART2->CR3 &= ~USART_CR3_DMAT;
	
	enable_dma1_stream6();
}

void TE_Complete_callback(void)
{
	while(1);
}

void FE_Complete_callback(void)
{
	while(1);
}

void DME_Complete_callback(void)
{
	while(1);
}

