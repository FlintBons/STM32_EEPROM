#include "cmsis_boot/stm32f10x.h"
#define EEPROM_Address 0xA0
#define cell_Address 0x01

const uint8_t data_Write[3] = {23, 24, 25};
uint8_t data_Read[3] = {0, 0, 0};
void Write_EEPROM (uint8_t *);
void Read_EEPROM (uint8_t *);

void TIM6_IRQHandler (void)
{
	TIM6->SR &= ~TIM_SR_UIF;
	for (uint8_t cnt = 0; GPIOD->IDR & GPIO_IDR_IDR0; cnt++)
	{
		if (cnt >= 30)
		{
			Write_EEPROM (data_Write);
			break;
		}
	}
	for (uint8_t cnt = 0; GPIOD->IDR & GPIO_IDR_IDR1; cnt++)
	{
		if (cnt >= 30)
		{
			Read_EEPROM (data_Read);
			break;
		}
	}
}

int main(void)
{
	SCB->VTOR = 0x20000000;

	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPDEN | RCC_APB2ENR_AFIOEN;
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN | RCC_APB1ENR_TIM6EN;

	GPIOD->CRL &= ~GPIO_CRL_MODE0 & ~GPIO_CRL_MODE1;
	GPIOD->CRL &= ~GPIO_CRL_CNF0 & ~GPIO_CRL_CNF1;
	GPIOD->CRL |= GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1;

	GPIOB->CRL &= ~GPIO_CRL_MODE6 & ~GPIO_CRL_MODE7;
	GPIOB->CRL &= ~GPIO_CRL_CNF6 & ~GPIO_CRL_CNF7;
	GPIOB->CRL |= GPIO_CRL_MODE6_0 | GPIO_CRL_MODE7_0;
	GPIOB->CRL |= GPIO_CRL_CNF6 | GPIO_CRL_CNF7;

	I2C1->CR1 &= ~I2C_CR1_SMBUS;
	I2C1->CR2 = (uint16_t) 2;
	I2C1->CCR = (uint16_t) 200;
	I2C1->TRISE = 3;
	I2C1->CR1 |= I2C_CR1_PE;

	TIM6->PSC = 71;
	TIM6->ARR = 4999;
	TIM6->DIER |= TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM6_IRQn);
	TIM6->CR1 |= TIM_CR1_CEN;

	while(1);
}

void Write_EEPROM (uint8_t *data_Write)
{
	I2C1->CR1 |= I2C_CR1_START;
	while (!(I2C1->SR1 & I2C_SR1_SB));
	(void) I2C1->SR1;

	I2C1->DR = EEPROM_Address;
	while (!(I2C1->SR1 & I2C_SR1_ADDR));
	(void) I2C1->SR1;
	(void) I2C1->SR2;

	I2C1->DR = cell_Address;
	while (!(I2C1->SR1 & I2C_SR1_BTF));
	(void) I2C1->SR1;

	for (uint8_t i = 0; i < 3; i++)
	{
		I2C1->DR = data_Write[i];
		while(!(I2C1->SR1 & I2C_SR1_BTF));
		(void) I2C1->SR1;
	}

	I2C1->CR1 |= I2C_CR1_STOP;
}

void Read_EEPROM (uint8_t *data_Read)
{
	I2C1->CR1 |= I2C_CR1_ACK;
	I2C1->CR1 |= I2C_CR1_START;
	while(!(I2C1->SR1 & I2C_SR1_SB));
	(void) I2C1->SR1;

	I2C1->DR = EEPROM_Address;
	while(!(I2C1->SR1 & I2C_SR1_ADDR));
	(void) I2C1->SR1;
	(void) I2C1->SR2;

	I2C1->DR = cell_Address;
	while(!(I2C1->SR1 & I2C_SR1_TXE));
	(void) I2C1->SR1;

	I2C1->CR1 |= I2C_CR1_START;
	while(!(I2C1->SR1 & I2C_SR1_SB));
	(void) I2C1->SR1;

	I2C1->DR = EEPROM_Address + 1;
	while(!(I2C1->SR1 & I2C_SR1_ADDR));
	(void) I2C1->SR1;
	(void) I2C1->SR2;

	for (uint8_t i = 0; i < 3; i++)
	{
		if (i == 2) I2C1->CR1 &= ~I2C_CR1_ACK;
		while(!(I2C1->SR1 & I2C_SR1_RXNE));
		data_Read[i] = I2C1->DR;
	}

	I2C1->CR1 |= I2C_CR1_STOP;
}
