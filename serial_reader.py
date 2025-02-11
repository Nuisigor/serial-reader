import os
import serial.tools.list_ports
import time
import asyncio
import asyncpg
from dotenv import load_dotenv

load_dotenv()

DB_HOST = os.getenv("DB_HOST")
DB_PORT = os.getenv("DB_PORT")
DB_NAME = os.getenv("DB_NAME")
DB_USER = os.getenv("DB_USER")
DB_PASS = os.getenv("DB_PASS")

ports = serial.tools.list_ports.comports()
arduino_port = None

for port, desc, hwid in sorted(ports):
    if 'COM3' in desc:
        arduino_port = port
        break

if arduino_port is None:
    print("No Arduino Uno found.")
    exit()

print(f"Using port: {arduino_port}")

async def main():
    conn = await asyncpg.connect(
        user=DB_USER,
        password=DB_PASS,
        database=DB_NAME,
        host=DB_HOST,
        port=DB_PORT
    )

    # Cria a tabela se não existir
    await conn.execute('''
        CREATE TABLE IF NOT EXISTS records (
            timestamp TIMESTAMP DEFAULT now(),
            room INT,
            value INT
        )
    ''')

    ser = serial.Serial(arduino_port, 500000, timeout=1)

    while True:
        data = ser.readline().decode('utf-8').strip()
        print(data)

        if data and data.startswith('[') and ']' in data:
            try:
                plate_id, value = data.split(']')
                plate_id = plate_id[1:]
                value = int(value)
                timestamp = time.strftime('%Y-%m-%d %H:%M:%S')

                print(f"Plate ID: {plate_id}, Value: {value}, Timestamp: {timestamp}")

                await conn.execute("INSERT INTO records (timestamp, room, value) VALUES ($1, $2, $3)",
                                   timestamp, plate_id, value)

                await asyncio.sleep(1)

            except ValueError:
                print(f"Invalid data format: {data}")

asyncio.run(main())
