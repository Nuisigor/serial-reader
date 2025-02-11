import os
from flask import Flask, render_template, request
import psycopg2
from datetime import datetime, timedelta
from dotenv import load_dotenv

load_dotenv()

DB_HOST = os.getenv("DB_HOST")
DB_PORT = os.getenv("DB_PORT")
DB_NAME = os.getenv("DB_NAME")
DB_USER = os.getenv("DB_USER")
DB_PASS = os.getenv("DB_PASS")

app = Flask(__name__)

def query_database(start_time, end_time):
    start_time = start_time.replace("T", " ")
    end_time = end_time.replace("T", " ")

    conn = psycopg2.connect(
        dbname=DB_NAME,
        user=DB_USER,
        password=DB_PASS,
        host=DB_HOST,
        port=DB_PORT
    )
    cursor = conn.cursor()
    
    query = """
    SELECT 
        room,
        SUM(CASE WHEN value = 105 THEN 1 ELSE 0 END) AS entradas,
        SUM(CASE WHEN value = 111 THEN 1 ELSE 0 END) AS saidas
    FROM records
    WHERE timestamp BETWEEN %s AND %s
    GROUP BY room;
    """
    
    cursor.execute(query, (start_time, end_time))
    results = cursor.fetchall()
    conn.close()

    return results

@app.route("/", methods=["GET"])
def index():
    start_time = request.args.get("start_time", (datetime.now() - timedelta(days=1)).strftime("%Y-%m-%d %H:%M:%S"))
    end_time = request.args.get("end_time", datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

    data = query_database(start_time, end_time)

    grid_data = {i: {"entradas": 0, "saidas": 0} for i in range(1, 13)}
    for sala, entradas, saidas in data:
        grid_data[sala] = {"entradas": entradas, "saidas": saidas}

    return render_template("index.html", grid_data=grid_data, start_time=start_time, end_time=end_time)

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=5000)
