#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/etimer.h"
#include "dev/watchdog.h"
#include "dev/button-hal.h"
#include "button-sensor.h"
#include "batmon-sensor.h"
#include "board-peripherals.h"
#include "rf-core/rf-ble.h"

#include "ti-lib.h"

#include "heapmem.h"
#include "mutex.h"

#include "board-peripherals.h"
#include "bme280.h"
#include "bmi160.h"
#include "bmi160_defs.h"

#include <stdio.h>
#include <stdint.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  0
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

// HARDCODED BS
#define NODE_ID 'f'

#define GYRO 'g'
#define ACC 'a'
#define TEMP 't'
#define PRESS 'p'
#define HUM 'h'
#define TEMP_INFRA 'i'
#define OTP 'o'

#define N_VALUES_NODE 10
#define N_MAX_NODES 4
// Socket
static struct simple_udp_connection udp_conn;
// Destination IP address
static uip_ipaddr_t dest_ipaddr;



// Axes values
typedef struct acc_gyr_payload_s {
    int16_t axes[3]; // x,y,z
} acc_gyr_payload_t;

// General sensors
typedef struct packet_sensor {
    char type;
    /*uint8_t*/ char id_node;
    int data_s;
    uint32_t data_u;
} packet_sensor_t;

// Gyro + Accel
typedef struct packet_acc_gyro {
    char type;
    uint8_t id_node;
    acc_gyr_payload_t data[N_VALUES_NODE];/* *data;controllare quando viene fatta la free per evitare la cancellazione prima dell'invio*/
} packet_acc_gyro_t;

typedef struct node_pile{
    acc_gyr_payload_t n_pile[N_VALUES_NODE];
    char type;
    struct node_pile *next;

} node_pile_t;

static node_pile_t *head=NULL;//head of pile

static mutex_t sem; //mutex to sync access to pile
static uint8_t count_pile_node=0;

/*---------------------------------------------------------------------------*/
int checkRoute() {
  return NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr);
}

int send_acc_or_gyro(packet_acc_gyro_t packet) {
   printf("sending gyro o acc\n");
  // Send to DAG root
  if (checkRoute()) {
     /*Set the number of transmissions to use for this packet -
       this can be used to create more reliable transmissions or
       less reliable than the default. Works end-to-end if
       UIP_CONF_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS is set to 1.*/
    
    uipbuf_set_attr(UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS, 10); // 10 --> number of attempts to transmit
    // Actual sender
    simple_udp_sendto(&udp_conn, &packet, sizeof(packet), &dest_ipaddr);
    return 1;
  } else {
    LOG_INFO("Not reachable yet \n");
    return 0;
  }
}

static void send_general_sensor(packet_sensor_t packet) {
  printf("sending gyro o acc\n");
  // Send to DAG root
  if (checkRoute()) {
     /*Set the number of transmissions to use for this packet -
       this can be used to create more reliable transmissions or
       less reliable than the default. Works end-to-end if
       UIP_CONF_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS is set to 1.*/
    
    uipbuf_set_attr(UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS, 10); // 10 --> number of attempts to transmit
    // Actual sender
    simple_udp_sendto(&udp_conn, &packet, sizeof(packet), &dest_ipaddr);
  } else {
    LOG_INFO("Not reachable yet \n");
  }
}

/*---------------------------------------------------------------------------*/
packet_acc_gyro_t build_acc_or_gyro_packet(acc_gyr_payload_t payload[], char type) {
  packet_acc_gyro_t packet;
  packet.type = type;
  packet.id_node = NODE_ID;
  for(uint8_t i=0; i<N_VALUES_NODE; i++)
    packet.data[i]=payload[i];

  return packet;
}

packet_sensor_t build_general_sensor_packet(int int_payload, uint32_t uint_payload, char type) {
  packet_sensor_t packet;
  packet.type = type;
  packet.id_node = NODE_ID;
  packet.data_s = int_payload;
  packet.data_u = uint_payload;
  return packet;
}

/*---------------------------------------------------------------------------*/
PROCESS(sensors_networking_client, "SNCM"); // Sensors Networking Client Main
PROCESS(sending_acc_gyro,"Sending acc gyro process");
AUTOSTART_PROCESSES(&sensors_networking_client,&sending_acc_gyro);

/*---------------------------------------------------------------------------*/
static void udp_rx_callback(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr, uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr, uint16_t receiver_port, const uint8_t *data, uint16_t datalen) {
  unsigned count = *(unsigned *)data;
  /* If tagging of traffic class is enabled tc will print number of
     transmission - otherwise it will be 0 */
  LOG_INFO("Received response %u (tc:%d) from ", count, uipbuf_get_attr(UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS));
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sending_acc_gyro, ev, data) {
  static acc_gyr_payload_t *copy_head_data=NULL;
  static void *ind;
  static void *garbage;
  static char type;
  static bool yet_lock=false;
  static struct etimer t;
  static uint16_t inviati=0;
  PROCESS_BEGIN();
  etimer_set(&t, 53*(CLOCK_SECOND/1000)); //27 
while(true){ 
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&t));

    //printf ("ris mutex %d\n",mutex_try_lock(&sem));
    mutex_try_lock(&sem);
    yet_lock=true;
    //printf("preso lock 1 in sending\n");
    if(head!=NULL) {
      printf("head not null\n");
      copy_head_data = head->n_pile;
      type = head->type;
    }

    if(copy_head_data!=NULL) {
      mutex_unlock(&sem);
      // printf("rilascio mutex in sending pre invio\n");
      yet_lock=false;
      if (send_acc_or_gyro(build_acc_or_gyro_packet(copy_head_data, type))) {
inviati=inviati+10;        
printf("invio riuscito di %c n %d\n",type,inviati);
      //  while (!mutex_try_lock(&sem));
        printf ("ris mutex 2 %d\n",mutex_try_lock(&sem));
        yet_lock=true;
        ind = head->next;
        garbage = head;
        head = ind;
        heapmem_free(garbage);
        count_pile_node--;
        copy_head_data=NULL;
      }
    }
    if(yet_lock){
      mutex_unlock(&sem);
     //printf("rilascio ultimo mutex\n");
     }
    etimer_reset(&t);
  }

  PROCESS_END();

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensors_networking_client, ev, data) {
  static struct etimer periodic_timer;
  static struct etimer config_timer;
  static struct etimer sample_timer;
  static struct etimer config_timer_opt;
  static uint8_t count = 0;
  static int16_t count_sample = 0;
  static uint8_t i;
  static node_pile_t *tmp;
  int value_i = 0;
  uint32_t value_u = 0;
  packet_sensor_t packet_sensor_impl;
  static node_pile_t *new_node;
  //packet_acc_gyro_t packet_acc_gyro_impl;
  static struct bmi160_sensor_data bmi160_datas;
  static heapmem_stats_t heap_stat;
  //static acc_gyr_payload_t acc_gyr_payload[99
          PROCESS_BEGIN();

   SENSORS_ACTIVATE(tmp_007_sensor);//Init tmp controllare la config negli header
   etimer_set(&config_timer_opt, 4*CLOCK_SECOND);
   // Initialize UDP connection
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);

  // Initialize timer
  etimer_set(&periodic_timer, 20 * CLOCK_SECOND);

  // Timer needed in order to configure BME280 sensors
  etimer_set(&config_timer, 2 * CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&config_timer));
  start_get_calib();

   printf("avviato tutto il processo di lettura\n");

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    count_sample=0;
   if (count ==6) {
      // Temp Infra
      tmp_007_sensor.value(TMP_007_SENSOR_TYPE_ALL);
      value_i = tmp_007_sensor.value(TMP_007_SENSOR_TYPE_OBJECT);
      printf("infra %d\n",value_i);
      packet_sensor_impl = build_general_sensor_packet(value_i, value_u, TEMP_INFRA);
      send_general_sensor(packet_sensor_impl);

      //OPT
      SENSORS_ACTIVATE(opt_3001_sensor);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&config_timer_opt));
      value_i = opt_3001_sensor.value(0);
      printf("opt %d\n",value_i);
      packet_sensor_impl = build_general_sensor_packet(value_i, value_u, OTP);
      send_general_sensor(packet_sensor_impl);
      etimer_reset(&config_timer_opt);

      // Temp BME
      value_i = bme_280_sensor.value(2);
      printf("Temp %d\n",value_i);
      packet_sensor_impl = build_general_sensor_packet(value_i, value_u, TEMP);
      send_general_sensor(packet_sensor_impl);

      // Press BME
      value_i = bme_280_sensor.value(1);
      printf("press %d\n",value_i);
      packet_sensor_impl = build_general_sensor_packet(value_i, value_u, PRESS);
      send_general_sensor(packet_sensor_impl);

      // Hum BME
      value_i = bme_280_sensor.value(4);
      printf("hum %d\n",value_i);
      packet_sensor_impl = build_general_sensor_packet(value_i, value_u, HUM);
      send_general_sensor(packet_sensor_impl);

      count = 0;
    } else if (count%2 == 0) {
      // ACC
      etimer_set(&sample_timer, 25 * (CLOCK_SECOND / 10000));
       printf("dentro acc\n");
        printf("sizeof %d\n", sizeof(node_pile_t));
      while(count_sample < 400 && count_pile_node< N_MAX_NODES) {
         printf("count sample: %d count pile %d \n",count_sample,count_pile_node); 
          new_node = heapmem_alloc(sizeof(node_pile_t));
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
           printf("inizio delle rilevazioni. pila: %d\n",count_pile_node);
           for(i=0;i<N_VALUES_NODE;i++) {
               PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sample_timer));

               bmi160_custom_value(1, &bmi160_datas, NULL);
               new_node->n_pile[i].axes[0]=bmi160_datas.x;
               new_node->n_pile[i].axes[1]=bmi160_datas.y;
               new_node->n_pile[i].axes[2]=bmi160_datas.z;

               etimer_reset(&sample_timer);
               count_sample++;
           }
           new_node->type=ACC;
           new_node->next=NULL;
	   bool mt;
           //while(!mutex_try_lock(&sem));
          mt=mutex_try_lock(&sem);
          printf("rsl mut 1 in acc %d\n",mt);
           if(head!=NULL) {
               if(count_pile_node<N_MAX_NODES) {
                   for (tmp = head; tmp->next != NULL; tmp = tmp->next);
                   tmp->next = new_node;
               }
           }else
               head=new_node;
           
           count_pile_node++;
           mutex_unlock(&sem);
           printf("rilasciato il mutex preso per scrivere da acc\n");          
}
       count_sample++;
      }
        count++;
     /* da canc for (int i=1; i<5; i++) {
        for (int j=0; j<99; j++) {
            acc_gyr_payload[j].axes[0] = bmi160_datas[i*j].x;
            acc_gyr_payload[j].axes[1] = bmi160_datas[i*j].y;
            acc_gyr_payload[j].axes[2] = bmi160_datas[i*j].z;
        }
        packet_acc_gyro_impl = build_acc_or_gyro_packet(acc_gyr_payload, ACC);
        send_acc_or_gyro(packet_acc_gyro_impl);
      }*/


    } else if (count%2 == 1) {
      // GYRO
        etimer_set(&sample_timer, 25 * (CLOCK_SECOND / 10000));
        printf("dentro gyro\n");
        printf("count %u\n",count);
        while(count_sample < 400 && count_pile_node< N_MAX_NODES) {
          printf("count sample: %d count pile %d \n",count_sample,count_pile_node);  
          new_node = heapmem_alloc(sizeof(node_pile_t));
           if( new_node==NULL){
              printf("è fallita la malloc %u\n",count_sample);
            heapmem_stats(&heap_stat);
            printf("allocated %d\n",heap_stat.allocated);
            printf("overhead %d\n",heap_stat.overhead);
            printf("available %d\n",heap_stat.available);
            printf("footprint %d\n",heap_stat.footprint);
            printf("chunks %d\n",heap_stat.chunks);
}
           else{
              printf("dentro campionamento. pila: %d\n",count_pile_node);
            for(i=0;i<N_VALUES_NODE;i++) {
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sample_timer));

                bmi160_custom_value(2, &bmi160_datas, NULL);
                new_node->n_pile[i].axes[0]=bmi160_datas.x;
                new_node->n_pile[i].axes[1]=bmi160_datas.y;
                new_node->n_pile[i].axes[2]=bmi160_datas.z;

                etimer_reset(&sample_timer);
                count_sample++;
            }
            new_node->type=GYRO;
            new_node->next=NULL;
            while(!mutex_try_lock(&sem));//da verificare se funziona come un mutex normale che sospende il processo e aspetta che si liberi la risorsa o no
printf("preso il mutex per scrivere \n");            
	if(head!=NULL) {
                if(count_pile_node<N_MAX_NODES) {
                    for (tmp = head; tmp->next != NULL; tmp = tmp->next);
                    tmp->next = new_node;
                }
            }else
                head=new_node;

            count_pile_node++;
            mutex_unlock(&sem);
            printf("rilasciato il mutex preso per scrivere da gyro\n");
          }
         count_sample++;
        }
        count++;


     /* etimer_set(&sample_timer, 1 * (CLOCK_SECOND / 1000));

      while(count_sample < 400) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sample_timer));

        bmi160_custom_value(2, NULL, &bmi160_datas[count_sample]);

        count_sample++;
        etimer_reset(&sample_timer);
      }

      for (int i=1; i<5; i++) {
        for (int j=0; j<99; j++) {
            acc_gyr_payload[j].axes[0] = bmi160_datas[i*j].x;
            acc_gyr_payload[j].axes[1] = bmi160_datas[i*j].y;
            acc_gyr_payload[j].axes[2] = bmi160_datas[i*j].z;
        }
        packet_acc_gyro_impl = build_acc_or_gyro_packet(acc_gyr_payload, GYRO);
        send_acc_or_gyro(packet_acc_gyro_impl);
      }

      count++;*/
    }

    // Timer reset
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
