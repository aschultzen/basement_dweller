import colorama
from colorama import Fore
from colorama import Style
from colorama import Back
import flask
import json
from flask import request, jsonify, render_template
import configparser
import mysql.connector
import datetime
import syslog

app = flask.Flask(__name__)
app.config["DEBUG"] = True
CONFIG_PATH = "config.ini"

add_sample_template = ("INSERT INTO weather"
    "(temp, humid, dweller, timestamp, pressure) "
    " VALUES (%s, %s, %s, %s, %s)")

def t_print(message):
    current_time = datetime.datetime.now().time()
    complete_message = "[" + current_time.isoformat() + "] " + message
    print(complete_message)

 def log_info(message, level = 1):
     syslog.syslog(syslog.LOG_INFO, message)
     if(level > 0):
         t_print(message)

 def log_error(error, level = 1):
     syslog.syslog(syslog.LOG_ERR, error)
     print_msg = error
     if(level > 0):
         t_print(print_msg)

 def init_logging():
     status = syslog.openlog(logoption=syslog.LOG_PID)

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
         log_error("Something went wrong connecting to DB: {}".format(err), 1)
         cnx = 0
     return cnx

 ## Try to make this general
 def write_to_db(cnx, data, template):
     if(cnx != 0):
         try:
             cursor = cnx.cursor()
             cursor.execute(template, data)
             cnx.commit()
             cursor.close()
             cnx.close()
             log_info("Inserted data into db: " + str(cursor.lastrowid),0)
             return cursor.lastrowid
         except mysql.connector.Error as err:
             log_info("Something went wrong: {}".format(err),1)
             return -1

 # Helper function for getting all in table.
 def get_all(tablename):
     cnx = connect_to_db(config)
     cur = cnx.cursor()
     sql_select_query = """select * from """ + tablename
     cursor = cnx.cursor(dictionary=True)
     cursor.execute(sql_select_query)
     all_series = cursor.fetchall()
     cnx.close()
     return jsonify(all_series)

 @app.route("/")
 def index():
     return "Hello, " + flask.request.remote_addr

 @app.errorhandler(404)
 def page_not_found(e):
     return "<h1>404</h1><p>The resource could not be found.</p>", 404

 # Create new series
 @app.route('/api/sample/', methods=['POST'])
 def create_model():
     r = json.loads(request.data)
     cnx = connect_to_db(config)
     data = [r['temp'], r['humid'], r['dweller'], r['timestamp'], r['pressure']]
     return(jsonify(write_to_db(cnx, data, add_sample_template)))

 def general_query(query, params):
     cnx = connect_to_db(config)
     cur = cnx.cursor()
     cursor = cnx.cursor(dictionary=True)
     cursor.execute(query, params)
     result = cursor.fetchall()
     cnx.close()
     return result

 # Main I Guess
 config = configparser.ConfigParser()
 conf_status = config.read(CONFIG_PATH)
 init_logging()
 app.run(host="0.0.0.0")
