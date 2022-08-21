import numpy as np
import matplotlib.pyplot as plt
import mysql.connector
import datetime
import os
import sys
import configparser

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

if __name__ == "__main__":
    config = init_config('config.ini')
    dt1 = get_dweller_data(config, (1,))
    dt2 = get_dweller_data(config, (2,))
    
    temp = []
    humid = []
    pressure = []
    timestamp = []

    for item in dt1:
        r = list(item.values())
        temp.append(r[0])
        humid.append(r[1])
        pressure.append(r[2])
        timestamp.append(datetime.datetime.fromtimestamp(r[3]))

    temp2 = []
    humid2 = []
    pressure2 = []
    timestamp2 = []

    for item in dt2:
        r = list(item.values())
        temp2.append(r[0])
        humid2.append(r[1])
        pressure2.append(r[2])
        timestamp2.append(datetime.datetime.fromtimestamp(r[3]))

    fig, axs = plt.subplots(3, 1)
    axs[0].plot(timestamp, temp, timestamp2, temp2)
    axs[0].set_xlabel('time')
    axs[0].set_ylabel('temp')

    axs[1].plot(timestamp, humid, timestamp2, humid2)
    axs[1].set_xlabel('time')
    axs[1].set_ylabel('humid')

    axs[2].plot(timestamp, pressure, timestamp2, pressure2)
    axs[2].set_xlabel('time')
    axs[2].set_ylabel('pressure')

    fig.tight_layout()
    plt.show()