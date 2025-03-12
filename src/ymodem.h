#pragma once


#define MODEM_SOH			0x01	// Start of Header(for 128 block)
#define MODEM_STX			0x02	// Start of Text(for 1024 block)
#define MODEM_ACK			0x06	// Acknowledge
#define MODEM_NACK			0x15	// Negative Acknowledge
#define MODEM_EOT			0x04	// End of Transmission
#define MODEM_C		 		0x43	// 'C' for sending file
#define MODEM_CAN			0x18	// Cancel



void YM_Init();

void YM_Rcv(char* szDir);
void YM_Snd(char* szFile);
