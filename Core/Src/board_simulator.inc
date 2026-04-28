/**
  ******************************************************************************
  * @file           : board.c
  * @brief          : the board class interface
  ******************************************************************************
  STM32CubeMX Configuration

  USART1: Mode Asynchronous.
  DMA Settings:
    Add USART1_TX (Stream 7, Channel 4). Mode: Normal, Increment Address: Memory.
    Add USART1_RX (Stream 5, Channel 4). Mode: Circular (best for continuous data), Increment Address: Memory.
  NVIC Settings: Enable USART1 global interrupt.
  ******************************************************************************
  */
#include <stdio.h>


void bai_uart_reset(void *huart)
{
}

int bai_uart_dma_receive(void *huart, char *rx_ring, int rx_len)
{
}

int bai_uart_dma_index(void *huart, int rx_len)
{
}

int bai_uart_dma_send(void *huart, char *tx_buf, int tx_len)
{
}

int bai_uart_poll_try(void *huart)
{
}

int bai_uart_poll_receive(void *huart, char *rx_buf, int rx_len)
{
}

int bai_uart_poll_send(void *huart, char *tx_buf, int tx_len)
{
}

void bai_led_on(void *gpio, int pin)
{
	write(STDOUT_FILENO, "#\b", 2);
}

void bai_led_off(void *gpio, int pin)
{
	write(STDOUT_FILENO, " \b", 2);
}

void bai_spin_lock(void)
{
}

void bai_spin_unlock(void)
{
}



