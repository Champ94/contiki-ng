/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "inttypes.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  0
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define N_VALUES_NODE 10
#define N_MAX_NODES 4

#define GYRO 'g'
#define ACC 'a'
#define TEMP 't'
#define PRESS 'p'
#define HUM 'h'
#define TEMP_INFRA 'i'
#define OTP 'o'
static struct simple_udp_connection udp_conn;
// Axes values
typedef struct acc_gyr_payload_s {
    int16_t axes[3]; // x,y,z
} acc_gyr_payload_t;

// General sensors
typedef struct packet_sensor {
    char type;
    /*uint8_t*/char id_node;
    int data_s;
    uint32_t data_u;
} packet_sensor_t;

// Gyro + Accel
typedef struct packet_acc_gyro {
    char type;
    uint8_t id_node;
    acc_gyr_payload_t data[N_VALUES_NODE]; /* *data;controllare quando viene fatta la free per evitare la cancellazione prima dell'invio*/
    
} packet_acc_gyro_t;

typedef struct pile{
	packet_acc_gyro_t pack;	
	struct packet_acc_gyro *next;
}pile_t;

static uint8_t count_pile_node=0;
static pile_t *head;
static mutex_t sem; //mutex to sync access to pile
uint16_t riv=0;
PROCESS(printer, "Printer values");
PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process, &printer);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  //unsigned count = *(unsigned *)data;
    packet_acc_gyro_t *packet_acc_gyr;
    packet_sensor_t *packet_sens;
    pile_t *new_node;
char *d=(char *)data;
 printf("ty %c \n",d[0]);
 if(d[0]==GYRO || d[0]==ACC){
     packet_acc_gyr=(packet_acc_gyro_t *)data;
      new_node = heapmem_alloc(sizeof(pile_t));
	   if (new_node==NULL){
             printf("e' fallita la malloc %u\n",count_sample);
            heapmem_stats(&heap_stat);
            printf("allocated %d\n",heap_stat.allocated);
            printf("overhead %d\n",heap_stat.overhead);
            printf("available %d\n",heap_stat.available);
            printf("footprint %d\n",heap_stat.footprint);
            printf("chunks %d\n",heap_stat.chunks);
              }
           else{
		new_node->
		
		
		}	


 }else {
     packet_sens= (packet_sensor_t *)data;
     printf("Type: %c, ID_NODO: %c, val: %d \n",packet_sens->type,packet_sens->id_node,packet_sens->data_s);
 }

  printf("Arrivato messaggio \n");
  //LOG_INFO_6ADDR(sender_addr);
  //LOG_INFO_("\n");

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(printer,ev,data){
	static struct etimer t;	
	etimer_set(&t, 30*(CLOCK_SECOND/1000)); 
	static bool yet_lock=false;
        static pile_t *tmp;
        static pile_t *garbage;
	static uint16_t riv=0;	
	while(1){
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&t));
        mutex_try_lock(&sem);
	yet_lock=true;
        if(head!=NULL){
	mutex_unlock(&sem);
	yet_lock=false;
    for(uint8_t i=0; i<N_VALUES_NODE;i++){
     riv++;
     printf("Num: %d Type: %c, ID_NODO: %u, val x: %d, y: %d, z: %d \n",riv,head->type,head->id_node, head->data[i].axes[0], head->data[i].axes[1], head->data[i].axes[2]);}
	mutex_try_lock(&sem);
        yet_lock=true;
        tmp=head->next; 
        garbage=head;
        head=tmp;
        heapmem_free(garbage);
        count_pile_node--;	
	}
	}
	if(yet_lock){
         mutex_unlock(&sem);  
}
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();
  printf("server avviato\n");

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
