import os
import mysql.connector
import csv 
from datetime import date

ROOT_CSV_PATH = "./"
CSV_IN_FILE_NAME = "export_papi_filtered_small.csv"
FULL_PATH = os.path.join(ROOT_CSV_PATH, CSV_IN_FILE_NAME)


HOST_IP = "localhost"
HOST_port = 3306
DB_NAME = "dbtest"

DB_USER_NAME = "adrien"
DB_PSWD = "adrien"

# Print iterations progress
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print(f'\r{prefix} |{bar}| {percent}% {suffix}', end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()
 
def parse_date (date_in): 
    date_return = ""
    # print ("date_in: ", date_in)
    date_in_split = date_in.split("/")
    if (date_in != ""):
        if (len (date_in_split[-1])> 2): 
            # print ("year is 4 digits: ", date_in_split[-1])
            date_return = date_in_split[-1]+"-"+date_in_split[-2]+"-"+date_in_split[-3]
        else: 
            # print ("year is 2 digits: ", date_in_split[-1])
            year_int  = int(date_in_split[-1])
            if year_int > 50: 
                year_int = year_int+1900
            else: 
                year_int = year_int+2000
            
            date_return = str(year_int)+"-"+date_in_split[-3]+"-"+date_in_split[-2]
    else: 
        # print ("WARNING - empty date, Applying a default date of 20-12-2020")
        date_return = "2020-12-20"
    # print ("date_return: ", date_return)
    return date_return


def parse_subitems (subitems_in):

    # print (">>>>>>>>>>", subitems_in)
    quantity = 1
    subitems_list_tuple = []
    subtiems_split = subitems_in.split(",")
    for elem in subtiems_split: 
        #Del trailing and front spaces
        split_elem = elem.strip()
        #Isolate first number indicating the quantity (if any)
        split_elem = split_elem.split(" ", 1)
        #Read first elem to see if it is a number
        if (split_elem[0].isnumeric()): 
            quantity = int(split_elem[0])
        else: 
            #unsplit the name in order to avoid having truncated names when spaces are used
            split_elem = [elem.strip()]
        #Check if sub item name is not an empty string.
        if (split_elem[-1]!= ""):  
            subitems_list_tuple.append((quantity, split_elem[-1]))
        quantity = 1
    # print ("<<<<<<<<<<<<<",subitems_list_tuple)
    return subitems_list_tuple 
    
mydb = mysql.connector.connect(
  host=HOST_IP,
  user=DB_USER_NAME,
  password=DB_PSWD,
  database = DB_NAME
)


mycursor = mydb.cursor()

# Clean db from all entries
sql = "delete from kit"
mycursor.execute(sql)
mydb.commit()
sql = "ALTER TABLE kit AUTO_INCREMENT = 1"
mycursor.execute(sql)
mydb.commit()

with open(FULL_PATH, newline='') as csvfile:
    reader = csv.DictReader(csvfile, delimiter=';')
    counter = 0
    for row in reader:
        counter +=1
        date_achat = parse_date(row["Date_achat"])
        subitems_list = parse_subitems(row["Info"])
        # print(row)
        
        
        # Ajout des kits dans la DB 
        sql = "INSERT INTO kit (nom, code_kit, date_achat) VALUES (%s, %s, %s)"
        val = (row["Titre"], str(int(row["Codebarre"])), date_achat )
        # print ("Kit inserted n°"+str(counter)+" : " + row["Titre"])
        mycursor.execute(sql, val)
        mydb.commit()
        # print(mycursor.rowcount, "record inserted.")
        
        #insert now subitems
        if (len(subitems_list) > 0):
            for elem in subitems_list: 
                # print ("        Inserting item: ", elem) 
                # elem is a list of tuple like: (quantity, name)
                sql = "INSERT INTO item (name, forkey, quantity, quantity_init) VALUES (%s, %s, %s, %s)"
                val = (elem[1], counter, elem[0], elem[0] )
                mycursor.execute(sql, val)
                mydb.commit()
            
        printProgressBar (counter, 1971, prefix = 'Progression', suffix = 'Fin', decimals = 1, length = 100, fill = '█', printEnd = "\r")
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     






