from flask import Flask, render_template, request
import sqlite3
from datetime import datetime, timedelta

app = Flask(__name__)

def query_database(start_time, end_time):
    conn = sqlite3.connect("serial_data.db")
    cursor = conn.cursor()

    query = """
    SELECT 
        room,
        SUM(CASE WHEN value = 1 THEN 1 ELSE 0 END) AS entradas,
        SUM(CASE WHEN value = 2 THEN 1 ELSE 0 END) AS saidas
    FROM registros
    WHERE strftime('%Y-%m-%d %H:%M:%S', timestamp) BETWEEN ? AND ?
    GROUP BY room;
    """

    cursor.execute(query, (start_time, end_time))
    results = cursor.fetchall()
    conn.close()

    return results

@app.route("/", methods=["GET"])
def index():
    # Filtro de tempo (padrão: últimas 24 horas)
    start_time = request.args.get("start_time", (datetime.now() - timedelta(days=1)).strftime("%Y-%m-%d %H:%M:%S"))
    end_time = request.args.get("end_time", datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
    
    # Consulta ao banco
    data = query_database(start_time, end_time)

    # Estrutura de salas para layout
    grid_data = {i: {"entradas": 0, "saidas": 0} for i in range(1, 13)}
    for sala, entradas, saidas in data:
        grid_data[sala] = {"entradas": entradas, "saidas": saidas}

    return render_template("index.html", grid_data=grid_data, start_time=start_time, end_time=end_time)

if __name__ == "__main__":
    app.run(debug=True)
