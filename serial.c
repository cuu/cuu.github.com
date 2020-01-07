/*
	CUU

	A simple exe for test serial  port
	gnubsd@me.com
	
	a.exe baudrate COME1 xx xx xx xx xx < hex data

*/
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


HANDLE m_hCommPort=NULL;

unsigned char ret_buf[ 512 ];

void init_dcb( DCB*dcb, int brate, unsigned char bs)
{
	dcb->DCBlength = sizeof(DCB);
	dcb->BaudRate = brate;
	dcb->ByteSize = bs;
	dcb->StopBits = ONESTOPBIT;
	dcb->Parity = NOPARITY;
}

void init_comtimeout( COMMTIMEOUTS *cmts)
{
	cmts->ReadIntervalTimeout = 3;
// Specify value that is multiplied  by the requested number of bytes to be read. 
	cmts->ReadTotalTimeoutMultiplier = 3;
// Specify value is added to the product of the  ReadTotalTimeoutMultiplier member
	cmts->ReadTotalTimeoutConstant = 2;
// Specify value that is multiplied by the requested number of bytes to be sent. 
	cmts->WriteTotalTimeoutMultiplier = 3;
// Specify value is added to the product of the WriteTotalTimeoutMultiplier member
	cmts->WriteTotalTimeoutConstant = 2;

}

int send_data(void*port, unsigned char*str, int size)
{

	int len;

	if(WriteFile( port,   // handle to file to write to
    	str,              // pointer to data to write to file
    	size,              // number of bytes to write
    	(PDWORD)&len,NULL) == 0)      // pointer to number of bytes written
	{
		printf("Writing of serial communication has problem. %d\n", GetLastError() );
		return 0;
	}

	return len;
}

int read_data( void*port, unsigned char*buf)
{
	int len;
	int size;
	size = 100;
	printf("size %d\n", size);
	
	if(ReadFile( port,  // handle of file to read
    	buf,               // handle of file to read
    	size,              // number of bytes to read
    	(PDWORD)&len,                 // pointer to number of bytes read
    	NULL) == 0)      // pointer to structure for data
	{
		printf("Reading of serial communication has problem.\n");
		return 0;
	}
	if(len > 0)
	{
		printf("len in read_data() %d\n", len);
	//	buf[len] = '\0'; // Assign end flag of message.
		return len;  
	} 
	return 0;
}

int init_serial_port()
{


}

void exit_serial()
{

	if(CloseHandle( m_hCommPort ) == 0)    // Call this function to close port.
	{
		printf("Port Closeing isn't successed.\n");
		return;
	}

}

int main(int argc,char**argv )
{

	int brate,bs;
	
	int i;
	int read_ret_len;
	int send_len;
	int send_ret_len;
	
	int error_no;

	unsigned char comport[32];	
	unsigned char arg_buf[512];


	DCB dcb = {0};
	DCB config_ = {0};
	COMMTIMEOUTS comTimeOut;
	
	read_ret_len = 0;
	send_len = 0;
	send_ret_len = 0;

	if (argc < 4 ) {
		printf("a.exe baudrate COM[1-n]  fe 00 21 01 20 \n");
		exit(0);
	}

			
	brate = strtol(argv[1], NULL,10);
	if(brate == 0) 
	{
			printf("brate input error!\n");
			exit(0);
	}
	
	bs = 8;
	init_dcb( &dcb, brate, bs);
	init_comtimeout(&comTimeOut);
	
	sprintf(comport, "\\\\.\\%s",argv[2] );

	printf("%s\n", comport);

	m_hCommPort = CreateFile(  comport,
            GENERIC_READ|GENERIC_WRITE,//access ( read and write)
            0,    //(share) 0:cannot share the COM port                        
            NULL,    //security  (None)                
            OPEN_EXISTING,// creation : open_existing
//            FILE_FLAG_OVERLAPPED, 
			0,
            NULL// no templates file for COM port...
            );

	if( m_hCommPort == INVALID_HANDLE_VALUE)
	{
		error_no = GetLastError();
		
		printf("open serial port failed %d ",error_no);
		switch(error_no)
		{
			case 5:
				printf("ERROR_ACCESS_DENIED\n");
			break;
			default:break;
		}
		return -1;
	}

	SetupComm(m_hCommPort, (DWORD)2048, (DWORD)2048);
	PurgeComm(m_hCommPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;


	if (GetCommState( m_hCommPort, &config_) == 0)
	{	
		printf("Get configuration port has problem.\n");
		return 0;
	}

	if(SetCommState( m_hCommPort, &dcb ) == 0)
	{
		printf("Set configuration port has problem.\n");
		return 0;
	}

	SetCommTimeouts( m_hCommPort,&comTimeOut);

	memset(arg_buf,0, 512);
	for(i=3;i< argc; i++)
	{
		if(strlen(argv[i]) > 2) 
		{
			exit_serial();
			printf("You must input like xx ,only two chars\n");
			exit(0);
		}
		printf("%x\n", strtol(argv[i], NULL,16) );
		//strcat(arg_buf, strtol(argv[i],NULL,16) );
		arg_buf[i-3] = strtol(argv[i], NULL, 16);
		send_len++;
	}
		printf("i-3=%d\n", i-3);
		//arg_buf[i-3] = '\0';
		printf("send_len = %d\n",send_len);

	send_ret_len = send_data( m_hCommPort, arg_buf, send_len );
	
	printf("send_ret_len %d\n",send_ret_len);
	//usleep(300);
//	Sleep(500);

	memset(ret_buf,0,sizeof(ret_buf));
	
	read_ret_len = read_data( m_hCommPort, ret_buf );

	printf("after read_data strlen ret_buf %d\n", strlen(ret_buf));

	for(i=0; i < read_ret_len; i++)
	{
		printf("%x ", ret_buf[i]);
	}
	printf("\n");

	exit_serial();

	return 0;
}
