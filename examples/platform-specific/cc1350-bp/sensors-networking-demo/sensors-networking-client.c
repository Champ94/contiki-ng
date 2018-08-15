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
    acc_gyr_payload_t *data;//controllare quando viene fatta la free per evitare la cancellazione prima dell'invio
} packet_acc_gyro_t;

typedef struct node_pile{
    acc_gyr_payload_t n_pile[N_VALUES_NODE];
    char type;
    struct node_pile *next;

} node_pile_t;

static node_pile_t *head=null;//head of pile

static mutex_t sem; //mutex to sync access to pile
static uint8_t count_pile_node;

/*---------------------------------------------------------------------------*/
int checkRoute() {
  return NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr);
}

static int send_acc_or_gyro(packet_acc_gyro_t packet) {
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
    /* Set the number of transmissions to use for this packet -
       this can be used to create more reliable transmissions or
       less reliable than the default. Works end-to-end if
       UIP_CONF_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS is set to 1.
    */
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
  packet.data=payload;

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
  static acc_gyr_payload_t *copy_head_data=null;
  static void *ind;
  static void *garbage;
  static char type;
  static boolean yet_lock=false;
  while(true){
    while(!mutex_try_lock(&sem));
    yet_lock=true;
    if(head!=null) {
      copy_head_data = head->n_pile;
      type = head->type;
    }

    if(copy_head_data!=null) {
      mutex_unlock(&sem);
      yet_lock=false;
      if (send_acc_or_gyro(build_acc_or_gyro_packet(copy_head_data, type))) {
        while (!mutex_try_lock(&sem));
        yet_lock=true;
        ind = head->next;
        garbage = head;
        head = ind;
        heapmem_free(garbage);
        copy_head_data=null;
      }
    }
    if(yet_lock)
      mutex_unlock(&sem);
  }



}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensors_networking_client, ev, data) {
  static struct etimer periodic_timer;
  static struct etimer config_timer;
  //static struct etimer sample_timer;
  static unsigned count = 0;
  //static unsigned count_sample = 0;
  static uint8_t i;
  static node_pile_t *tmp;
  int value_i = 0;
  uint32_t value_u = 0;
  packet_sensor_t packet_sensor_impl;
  static node_pile_t *new_node;
  //packet_acc_gyro_t packet_acc_gyro_impl;
  static struct bmi160_sensor_data bmi160_datas;
  //static acc_gyr_payload_t acc_gyr_payload[99
          PROCESS_BEGIN();

  // Initialize UDP connection
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);

  // Initialize timer
  etimer_set(&periodic_timer, 20 * CLOCK_SECOND);

  // Timer needed in order to configure BME280 sensors
  etimer_set(&config_timer, 2 * CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&config_timer));
  start_get_calib();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if (count ==0 /*6*/) {
      // Temp Infra
      value_i = tmp_007_sensor.value(TMP_007_SENSOR_TYPE_OBJECT);
      printf("infra %d\n",value_i);
      packet_sensor_impl = build_general_sensor_packet(value_i, value_u, TEMP_INFRA);
      send_general_sensor(packet_sensor_impl);

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

      while(count_sample < 400) {
           new_node = heapmem_alloc(sizeof(node_pile_t));
           for(i=0;i<N_VALUES_NODE;i++) {
               PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sample_timer));

               bmi160_custom_value(1, &bmi160_datas, NULL);
               new_node->n_pile[i]->axes[1]=bmi160_datas.x;
               new_node->n_pile[i]->axes[2]=bmi160_datas.y;
               new_node->n_pile[i]->axes[3]=bmi160_datas.z;

               etimer_reset(&sample_timer);
               count_sample++;
           }
           new_node->type=ACC;
           new_node->next=null;
           while(!mutex_try_lock(&sem));
           if(head!=null) {
               if(count_pile_node<N_MAX_NODES) {
                   for (tmp = head; tmp->next != null; tmp = tmp->next);
                   tmp->next = new_node;
               }
           }else
               head=new_node;

           count_pile_node++;
           mutex_unlock(&sem);


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

        while(count_sample < 400) {
            new_node = heapmem_alloc(sizeof(node_pile_t));
            for(i=0;i<N_VALUES_NODE;i++) {
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sample_timer));

                bmi160_custom_value(2, &bmi160_datas, NULL);
                new_node->n_pile[i]->axes[1]=bmi160_datas.x;
                new_node->n_pile[i]->axes[2]=bmi160_datas.y;
                new_node->n_pile[i]->axes[3]=bmi160_datas.z;

                etimer_reset(&sample_timer);
                count_sample++;
            }
            new_node->type=GYRO;
            new_node->next=null;
            while(!mutex_try_lock(&sem));//da verificare se funziona come un mutex normale che sospende il processo e aspetta che si liberi la risorsa o no
            if(head!=null) {
                if(count_pile_node<N_MAX_NODES) {
                    for (tmp = head; tmp->next != null; tmp = tmp->next);
                    tmp->next = new_node;
                }
            }else
                head=new_node;

            count_pile_node++;
            mutex_unlock(&sem);


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
