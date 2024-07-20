/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
//#include <zephyr/bluetooth/services/hrs.h>
//#include <zephyr/bluetooth/services/ias.h>

#include <stdio.h>

#include "bat.h"

static bool m_rgb_notify_on;
static bool m_bat_notify_on;


// Note, quick and dirty for testing
void get_leds(void**leds, int *size);
int16_t get_speed(void);
void set_speed(int16_t);


// 65dbc53e-a859-4422-947c-f016c0e0af10
#define BT_UUID_CUSTOM_SERVICE_VAL BT_UUID_128_ENCODE(0x65dbc53e, 0x0000, 0x4422, 0x947c, 0xf016c0e0af10)


static const struct bt_uuid_128 hat_srv_uuid = BT_UUID_INIT_128(
		BT_UUID_CUSTOM_SERVICE_VAL
	);

static const struct bt_uuid_128 hat_chr_rgb_uuid = BT_UUID_INIT_128(
		BT_UUID_128_ENCODE(0x65dbc53e, 0x0001, 0x4422, 0x947c, 0xf016c0e0af10)
	);


static const struct bt_uuid_128 hat_chr_spd_uuid = BT_UUID_INIT_128(
		BT_UUID_128_ENCODE(0x65dbc53e, 0x0002, 0x4422, 0x947c, 0xf016c0e0af10)
	);

static const struct bt_uuid_128 hat_dir_spd_uuid = BT_UUID_INIT_128(
		BT_UUID_128_ENCODE(0x65dbc53e, 0x0003, 0x4422, 0x947c, 0xf016c0e0af10)
	);

static const struct bt_uuid_128 hat_chr_bat_uuid = BT_UUID_INIT_128(
		BT_UUID_128_ENCODE(0x65dbc53e, 0x0100, 0x4422, 0x947c, 0xf016c0e0af10)
	);
 // 0x2B18
#define hat_MAX_LEN 20


static ssize_t read_rgb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset){
	puts("read_rgb");
	void *value;// = attr->user_data;
	int size;
	get_leds(&value,&size);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, size);
}

static ssize_t write_rgb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags) {
		puts("write_rgb");

	void *value;
	int size;
	get_leds(&value,&size);
	if ((offset+len)>size )
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	memcpy(value + offset, buf, len);
	return len;
}

//--
static ssize_t read_spd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset){
	uint16_t speed = get_speed();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &speed, sizeof(speed));
}

static ssize_t write_spd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags) {
	puts("write_spd");

	uint16_t speed = *(uint16_t*)(buf);
	set_speed(speed);
	return len;
}



static ssize_t read_dir(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset){

	extern bool get_direction();
	bool dir =  get_direction();;
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &dir, sizeof(dir));
}


static ssize_t write_dir(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags) {

	extern void set_direction(bool direction); 
	set_direction ( *(bool*)(buf) );
	
	return len;
}



static ssize_t read_bat(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset){
	uint16_t voltage = adc_measure();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &voltage, sizeof(voltage));
}
//--

static void rgb_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	printf("rgb_ccc_cfg_changed, value %04X\n", value);
	m_rgb_notify_on = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static void bat_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	printf("bat_ccc_cfg_changed, value %04X\n", value);
	m_bat_notify_on = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

const struct bt_gatt_cpf m_spd_cpf = {
	0x06 , // uint16
	-3 ,   // *10⁻³
	0x2703 , // second 
	0x0001 , // Bluetooth SIG assigned numbers 
	0x0001, // ?? 
}; 


const struct bt_gatt_cpf m_bat_cpf = {
	0x06 , // uint16
	-3 ,   // *10⁻³
	0x2728 , // volt 
	0x0001 , // Bluetooth SIG assigned numbers 
	0x0001, // ?? 
}; 

const struct bt_gatt_cpf m_dir_cpf = {
	0x01 , // bool
	0 ,   // *10⁻⁰
	0x2700 , // unitless 
	0x0001 , // Bluetooth SIG assigned numbers 
	0x0001, // ?? 
}; 


/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(hat_svc,

	BT_GATT_PRIMARY_SERVICE(&hat_srv_uuid.uuid),
		BT_GATT_CHARACTERISTIC(&hat_chr_rgb_uuid.uuid,
					   BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
					   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					   read_rgb, write_rgb, NULL),
			BT_GATT_CCC(rgb_ccc_cfg_changed,
					BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
			BT_GATT_CUD("RGB", BT_GATT_PERM_READ),

	BT_GATT_CHARACTERISTIC(&hat_chr_spd_uuid.uuid,
					   BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE ,
					   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					   read_spd, write_spd, NULL),
			BT_GATT_CUD("Speed", BT_GATT_PERM_READ),
			BT_GATT_CPF(&m_spd_cpf),

	BT_GATT_CHARACTERISTIC(&hat_chr_spd_uuid.uuid,
					   BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE ,
					   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					   read_dir, write_dir, NULL),
			BT_GATT_CUD("Direction", BT_GATT_PERM_READ),
			BT_GATT_CPF(&m_dir_cpf),

	BT_GATT_CHARACTERISTIC(&hat_chr_bat_uuid.uuid,
					   BT_GATT_CHRC_READ |  BT_GATT_CHRC_NOTIFY,
					   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					   read_bat, NULL , NULL),
			BT_GATT_CCC(bat_ccc_cfg_changed,
					BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
			BT_GATT_CUD("Voltage", BT_GATT_PERM_READ),
			BT_GATT_CPF(&m_bat_cpf),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL ),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = mtu_updated
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
	printk("Disconnected (reason 0x%02x)\n", reason);


}


BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};



static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialised\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}
						//BT_LE_ADV_OPT_CONNECTABLE ??
//	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void bas_notify(void) {
	int16_t voltage = adc_measure();
	voltage -= 3300;

	// Note: we need to determine the discharge curve, this is a placeholder.
	int  battery_level = (voltage * 100 ) / (4200 - 3300) ;
	if (battery_level < 0) battery_level = 0;
	if (battery_level > 100) battery_level = 100;
	bt_bas_set_battery_level(battery_level);
}

void bat_notify(void) {	
	int16_t voltage = adc_measure();
//	int ret = bt_gatt_notify(NULL, &hat_svc.attrs[7], &voltage, sizeof(voltage));

	int ret = bt_gatt_notify_uuid(NULL, &hat_chr_bat_uuid.uuid,  hat_svc.attrs,  &voltage, sizeof(voltage));

	printf("bat not returned %X\n");
}



int ble_init(void)
{
	struct bt_gatt_attr *hat_ind_attr;
	char str[BT_UUID_STR_LEN];
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_ready();
	bt_gatt_cb_register(&gatt_callbacks);
	return 0;
}




void ble_battery_process(void) {
	/* Battery level simulation */
	bas_notify();
	bat_notify();
}

