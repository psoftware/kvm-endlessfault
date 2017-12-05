#include <iostream>
#include <stdio.h>
#include "../debug-server/net_wrapper.h"
#include "../debug-server/messages.h"

using namespace std;
#include <linux/kvm.h>

void stampa_indirizzo( struct sockaddr_in *addr )
{
	char str[16];
	inet_ntop(AF_INET, &addr->sin_addr, str, 16);
	printf("ip: %s",str);
	printf("port: %hu", ntohs(addr->sin_port));
}

void trace_user_program(kvm_regs &regs, kvm_sregs &sregs) {
	cout << "Target program dump: " << endl;
	cout << "\tRIP: " << (void *)regs.rip << endl;
	cout << "\tRSP: " << (void *)regs.rsp << endl;
	cout << "\tCR4: " << (void *)sregs.cr4 << endl;
	cout << "\tCR3: " << (void *)sregs.cr3 << endl;
	cout << "\tCR2: " << (void *)sregs.cr2 << endl;
	cout << "\tCR0: " << (void *)sregs.cr0 << endl;
	cout << "\tEFER: " << (void *)sregs.efer << endl;

	cout << "\tSREGS base: " << (unsigned int)sregs.ds.base << endl;
	cout << "\tSREGS limit: " << (unsigned int)sregs.ds.limit << endl;
	cout << "\tSREGS selector: " << (unsigned int)sregs.ds.selector << endl;
	cout << "\tSREGS present: " << (unsigned int)sregs.ds.present << endl;
	cout << "\tSREGS type: " << (unsigned int)sregs.ds.type << endl;
	cout << "\tSREGS dpl: " << (unsigned int)sregs.ds.dpl << endl;
	cout << "\tSREGS db: " << (unsigned int)sregs.ds.db << endl;
	cout << "\tSREGS s: " << (unsigned int)sregs.ds.s << endl;
	cout << "\tSREGS l: " << (unsigned int)sregs.ds.l << endl;
	cout << "\tSREGS g: " << (unsigned int)sregs.ds.g << endl;
	cout << "\tSREGS type: " << (unsigned int)sregs.ds.type << endl;
	cout << "\tSREGS selector: " << (unsigned int)sregs.ds.selector << endl;

	cout << "\tIDT base: " << (void *)sregs.idt.base << endl;
	cout << "\tIDT limit: " << (unsigned int)sregs.idt.limit << endl;
	cout << "\tGDT base: " << (void *)sregs.gdt.base << endl;
	cout << "\tGDT limit: " << (unsigned int)sregs.gdt.limit << endl;
}

int main(int argc, char* argv[])
{
	int sd, ret;
	struct sockaddr_in serv_addr;
	simple_msg msg_req;
	info_msg msg_ans;
	my_buffer my_buff;
	my_buff.buf = NULL;
	my_buff.size = 0;
	kvm_regs regs;
	kvm_sregs sregs;

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


	init_simple_msg(&msg_req, REQ_INFO);
	convert_to_network_order(&msg_req);
	send_data(sd, (char*)&msg_req, sizeof(msg_req));
	int r = recv_data(sd, &my_buff);
	if( r != -1 ){
		convert_to_host_order(my_buff.buf);
		memcpy(&msg_ans, my_buff.buf,sizeof(info_msg));

		printf("---------------Info---------------\n");
		printf( "Mem_size: %lu \n",msg_ans.mem_size);

		recv_data(sd, &my_buff);
		memcpy(&regs, my_buff.buf, sizeof(regs));
		recv_data(sd, &my_buff);
		memcpy(&sregs, my_buff.buf, sizeof(sregs));
		trace_user_program(regs,sregs);
	}
	return 0;
}
