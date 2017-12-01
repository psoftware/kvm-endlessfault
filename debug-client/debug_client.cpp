#include <iostream>
#include <stdio.h>
#include "../debug-server/net_wrapper.h"
#include "../debug-server/messages.h"

using namespace std;

void stampa_indirizzo( struct sockaddr_in *addr )
{
	char str[16];
	inet_ntop(AF_INET, &addr->sin_addr, str, 16);
	printf("ip: %s",str);
	printf("port: %hu", ntohs(addr->sin_port));
}

int main(int argc, char* argv[])
{
	int sd, ret;
	struct sockaddr_in serv_addr;
	my_buffer my_buff;
	my_buff.buf = NULL;
	my_buff.size = 0;

	if( argc < 3 )
	{
		printf("use: ./debug-client <host remoto> <porta>\n");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, argv[1], (void*)&serv_addr.sin_addr);
	if( ret!=1 )
	{
		printf("Errore: server ip not valid.\n");
		return -1;
	}
	/*get server port*/
	sscanf(argv[2],"%hu", &serv_addr.sin_port);
	/*converto in big endian*/
	serv_addr.sin_port = htons(serv_addr.sin_port); 

      /*apro socket tcp per la connessione con il server*/
	sd = socket(AF_INET, SOCK_STREAM, 0);

	if( sd == -1 )
	{
		perror("Errore socket non aperta");
		return -1;
	}

	ret = connect(sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr) );
	if( ret == -1 )
	{
		perror("Errore connect");
		close(sd);		
		exit(1);
	} else 
		printf("Connesso correttamente al server %s\n",argv[1]);
	
	req_dump_mem mess;
	init_req_dump_mem(&mess, 0xBF000, 0xBF000 + 2000);
	cout << "req_dump_mem type:" << mess.t << endl;

	convert_to_network_order(&mess);
	send_data(sd, (char*)&mess, sizeof(mess));

	recv_data(sd, &my_buff);
	printf("-----------Memory dump------------\n");
	for(uint32_t i=0; i<my_buff.size; i++)
		printf("%02x",my_buff.buf[i]);
	printf("----------------------------------\n");
	return 0;
}
