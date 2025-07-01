import csv
import websocket

CSV_FILE = "live_data.csv"


# Write the header only once
def write_header():
    # Change the column names if needed
    header = [
        "Timestamp",      # adjust if first value is a timestamp, otherwise name as you want
        "A_Temp", "A_AccX", "A_AccY", "A_AccZ",
        "A_GyroX", "A_GyroY", "A_GyroZ", "A_AccAngleX", "A_AccAngleY", "A_AngleX", "A_AngleY", "A_AngleZ",
        "B_Temp", "B_AccX", "B_AccY", "B_AccZ",
        "B_GyroX", "B_GyroY", "B_GyroZ", "B_AccAngleX", "B_AccAngleY", "B_AngleX", "B_AngleY", "B_AngleZ"
    ]
    try:
        with open(CSV_FILE, "x", newline='') as f:
            writer = csv.writer(f)
            writer.writerow(header)
    except FileExistsError:
        pass  # File already exists; don't write header

def on_message(ws, message):
    parts = message.strip().split(",")
    print("Received:", parts)
    with open(CSV_FILE, "a", newline='') as f:
        writer = csv.writer(f)
        writer.writerow(parts)

def on_error(ws, error):
    print("Error:", error)

def on_close(ws, close_status_code, close_msg):
    print("WebSocket closed")

def on_open(ws):
    print("Connected to ESP32 WebSocket!")

if __name__ == "__main__":
    ESP32_IP = "192.168.12.3"  # change this if needed
    WS_URL = f"ws://{ESP32_IP}:81/"

    write_header()
    ws = websocket.WebSocketApp(
        WS_URL,
        on_open=on_open,
        on_message=on_message,
        on_error=on_error,
        on_close=on_close
    )
    ws.run_forever()

