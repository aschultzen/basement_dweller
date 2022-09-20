import numpy as np
import matplotlib.pyplot as plt
import mysql.connector
import datetime
import os
import sys
import configparser

FIRST_EPOCH = 1609459200
LAST_EPOCH = 1893456000
FIVE_MINUTES = 300
LAST_VALID_TIMESTAMP = 1609459200

def connect_to_db(config):
	config = {
	  'user': config['database']['User'],
	  'password': config['database']['Passwd'],
	  'host': config['database']['Host'],
	  'database': config['database']['Db'],
	  'raise_on_warnings': True,
	  'connect_timeout': 1
	}
	try:
		cnx = mysql.connector.connect(**config)
	except mysql.connector.Error as err:
		print("Something went wrong connecting to DB: {}".format(err), 0)
		cnx = 0
	return cnx

# General constants
CONFIG_PATH = "config.ini"

def smooth(y, box_pts):
    box = np.ones(box_pts)/box_pts
    y_smooth = np.convolve(y, box, mode='same')
    return y_smooth

def init_config(config_path):
    config = configparser.ConfigParser()
    try:
        conf_status = config.read(config_path)
    except configparser.MissingSectionHeaderError as MSHE:
        print("Configuration file is missing a header." + config_path)
        return 0

    if len(conf_status) == 0:
        print("Failed to load " + config_path)
        return 0

    return config

def get_dweller_data(config, dweller):
    cnx = connect_to_db(config)
    sql_select_query = """select temp, humid, pressure, timestamp from weather where dweller = %s"""
    if(cnx != 0):
        try:
            cursor = cnx.cursor(dictionary=True)
            cursor.execute(sql_select_query, dweller)
            record = cursor.fetchall()
            cnx.close()
            return record
        except mysql.connector.Error as err:
            print("Something went wrong: {}".format(err),0)
    return -1

def produce_date(timestamp):
    mod_timestamp = timestamp
    global LAST_VALID_TIMESTAMP
    if(timestamp > LAST_EPOCH or timestamp < FIRST_EPOCH):
        mod_timestamp = LAST_VALID_TIMESTAMP + FIVE_MINUTES;
        print("Invalid timestamp %d, using %d instead" % (timestamp,mod_timestamp))
    
    LAST_VALID_TIMESTAMP = mod_timestamp # Not really a valid timestamp...
    return datetime.datetime.fromtimestamp(mod_timestamp)

if __name__ == "__main__":
    config = init_config('config.ini')
    dt1 = get_dweller_data(config, (1,))
    dt2 = get_dweller_data(config, (2,))
    dt3 = get_dweller_data(config, (3,))
    
    temp = []
    humid = []
    pressure = []
    timestamp = []

    for item in dt1:
        r = list(item.values())
        temp.append(r[0])
        humid.append(r[1])
        pressure.append(r[2])
        timestamp.append(produce_date(r[3]))

    temp2 = []
    humid2 = []
    pressure2 = []
    timestamp2 = []

    for item in dt2:
        r = list(item.values())
        temp2.append(r[0])
        humid2.append(r[1])
        pressure2.append(r[2])
        timestamp2.append(produce_date(r[3]))

    temp3 = []
    humid3 = []
    pressure3 = []
    timestamp3 = []

    for idx, item in enumerate(dt3):
        r = list(item.values())
        temp3.append(r[0])
        humid3.append(r[1])
        pressure3.append(r[2])
        timestamp3.append(produce_date(r[3]))

    fig, axs = plt.subplots(3, 1)
    axs[0].plot(timestamp, temp, label="Dweller 1")
    axs[0].plot(timestamp2, temp2,label="Dweller 2")
    axs[0].plot(timestamp3, temp3, label="Dweller 3")
    axs[0].set_xlabel('time')
    axs[0].set_ylabel('temp')

    axs[1].plot(timestamp, humid, timestamp2, humid2, timestamp3, humid3)
    axs[1].set_xlabel('time')
    axs[1].set_ylabel('humid')

    axs[2].plot(timestamp, pressure, timestamp2, pressure2, timestamp3, pressure3)
    axs[2].set_xlabel('time')
    axs[2].set_ylabel('pressure')

    fig.tight_layout()
    #plt.legend(ncol=3, loc="upper right")
    plt.show()