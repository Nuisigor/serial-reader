import serial.tools.list_ports
import time
import asyncio
import aiosqlite

# Search for Arduino port
ports = serial.tools.list_ports.comports()
arduino_port = None
for port, desc, hwid in sorted(ports):
    if 'Arduino Uno' in desc:
        arduino_port = port
        break

if arduino_port is None:
    print("No Arduino Uno found.")
    exit()

print(f"Using port: {arduino_port}")

async def main():
    ## Cria o Banco de dados
    async with aiosqlite.connect('serial_data.db') as db:
        await db.execute('''CREATE TABLE IF NOT EXISTS records (timestamp DATETIME, room INT, value INT)''')
        await db.commit()

        ser = serial.Serial(arduino_port, 9600, timeout=1)

        ## Lê porta serial
        while True:
            print(ser.readline())
            data = ser.readline().decode('utf-8').strip()
            # Insert into the database if data is in the format '[{plate}]{value}', 
            if data and data.startswith('[') and ']' in data:
                try:
                    plate_id, value = data.split(']')
                    plate_id = plate_id[1:]
                    value = int(value)
                    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
                    print(f"Plate ID: {plate_id}, Value: {value}, Timestamp: {timestamp}")
                    await db.execute("INSERT INTO records VALUES (?, ?, ?)", (timestamp, plate_id, value))
                    await db.commit()
                except ValueError:
                    print(f"Invalid data format: {data}")
            await asyncio.sleep(1)

asyncio.run(main())