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
#define NODE_ID 33

#define GYRO 'g'
#define ACC 'a'
#define TEMP 't'
#define PRESS 'p'
#define HUM 'h'
#define TEMP_INFRA 'i'
#define OTP 'o'

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
    uint8_t id_node;
    int data_s;
    uint32_t data_u;
} packet_sensor_t;

// Gyro + Accel
typedef struct packet_acc_gyro {
    char type;
    uint8_t id_node;
    acc_gyr_payload_t data[99];
} packet_acc_gyro_t;

/*---------------------------------------------------------------------------*/
int checkRoute() {
  return NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr);
}

static void send_acc_or_gyro(packet_acc_gyro_t packet) {
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

static void send_general_sensor(packet_sensor_t packet) {
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
  for (int i=0; i<99; i++) {
      packet.data[i] = payload[i];
  }
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
AUTOSTART_PROCESSES(&sensors_networking_client);

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
PROCESS_THREAD(sensors_networking_client, ev, data) {
  static struct etimer periodic_timer;
  static struct etimer config_timer;
  static struct etimer sample_timer;
  static unsigned count = 0;
  static unsigned count_sample = 0;

  int value_i = 0;
  uint32_t value_u = 0;
  packet_sensor_t packet_sensor_impl;
  packet_acc_gyro_t packet_acc_gyro_impl;
  static struct bmi160_sensor_data bmi160_datas[400];
  static acc_gyr_payload_t acc_gyr_payload[99];

  PROCESS_BEGIN();

  // Initialize UDP connection
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);

  // Initialize timer
  etimer_set(&periodic_timer, 300 * CLOCK_SECOND);

  // Timer needed in order to configure BME280 sensors
  etimer_set(&config_timer, 2 * CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&config_timer));
  start_get_calib();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if (count == 6) {
      // Temp Infra
      value_i = tmp_007_sensor.value(TMP_007_SENSOR_TYPE_OBJECT);
      packet_sensor_impl = build_general_sensor_packet(value_i, value_u, TEMP_INFRA);
      send_general_sensor(packet_sensor_impl);

      // Temp BME
      value_i = bme_280_sensor.value(2);
      packet_sensor_impl = build_general_sensor_packet(value_i, value_u, TEMP);
      send_general_sensor(packet_sensor_impl);

      // Press BME
      value_i = bme_280_sensor.value(1);
      packet_sensor_impl = build_general_sensor_packet(value_i, value_u, PRESS);
      send_general_sensor(packet_sensor_impl);

      // Hum BME
      value_i = bme_280_sensor.value(4);
      packet_sensor_impl = build_general_sensor_packet(value_i, value_u, HUM);
      send_general_sensor(packet_sensor_impl);

      count = 0;
    } else if (count%2 == 0) {
      // ACC
      etimer_set(&sample_timer, 1 * (CLOCK_SECOND / 1000));

      while(count_sample < 400) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sample_timer));

        bmi160_custom_value(1, &bmi160_datas[count_sample], NULL);

        count_sample++;
        etimer_reset(&sample_timer);
      }

      for (int i=1; i<5; i++) {
        for (int j=0; j<99; j++) {
          acc_gyr_payload[j] = bmi160_datas[i*j];
        }
        packet_acc_gyro_impl = build_acc_or_gyro_packet(acc_gyr_payload, ACC);
        send_acc_or_gyro(packet_acc_gyro_impl);
      }

      count++;
    } else if (count%2 == 1) {
      // GYRO
      etimer_set(&sample_timer, 1 * (CLOCK_SECOND / 1000));

      while(count_sample < 400) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sample_timer));

        bmi160_custom_value(2, NULL, &bmi160_datas[count_sample]);

        count_sample++;
        etimer_reset(&sample_timer);
      }

      for (int i=1; i<5; i++) {
        for (int j=0; j<99; j++) {
          acc_gyr_payload[j] = bmi160_datas[i*j];
        }
        packet_acc_gyro_impl = build_acc_or_gyro_packet(acc_gyr_payload, GYRO);
        send_acc_or_gyro(packet_acc_gyro_impl);
      }

      count++;
    }

    // Timer reset
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
