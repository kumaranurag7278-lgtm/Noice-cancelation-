import http.server
import socketserver
import threading
import json
import time
import os
import sys
import webbrowser
import urllib.parse

try:
    import serial
except ImportError:
    print("pyserial is required. Install via: pip install pyserial")
    sys.exit(1)

PORT = 3000
SERIAL_PORT = "COM9"
BAUD_RATE = 115200

# Shared state
latest_telemetry = {
    "anc": 1,
    "noise_rms": 0,
    "noise_peak": 0,
    "anti_rms": 0,
    "red_db": 0.0,
    "gain": 1.15,
    "delay": 3,
    "mic": 1,
    "amp": 1,
    "spk": 1
}

serial_lock = threading.Lock()
ser = None

def serial_worker():
    global latest_telemetry, ser
    while True:
        try:
            if ser is None or not ser.is_open:
                print(f"[SERIAL] Connecting to {SERIAL_PORT} @ {BAUD_RATE}...")
                ser = serial.Serial()
                ser.port = SERIAL_PORT
                ser.baudrate = BAUD_RATE
                ser.dtr = False
                ser.rts = False
                ser.timeout = 1
                ser.open()
                print(f"[SERIAL] Connected to {SERIAL_PORT} successfully!")

            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line and line.startswith("{") and line.endswith("}"):
                try:
                    data = json.loads(line)
                    if "noise_rms" in data:
                        with serial_lock:
                            latest_telemetry.update(data)
                except json.JSONDecodeError:
                    pass
        except Exception as e:
            time.sleep(1)

class DashboardHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=os.path.join(os.path.dirname(__file__), "web"), **kwargs)

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/api/telemetry":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            with serial_lock:
                payload = json.dumps(latest_telemetry)
            self.wfile.write(payload.encode('utf-8'))
        elif parsed.path == "/api/command":
            query = urllib.parse.parse_qs(parsed.query)
            cmd = query.get("cmd", [""])[0]
            if cmd and ser and ser.is_open:
                try:
                    with serial_lock:
                        ser.write((cmd + "\n").encode('utf-8'))
                    print(f"[SERIAL] Sent command: {cmd}")
                except Exception as ex:
                    print(f"[SERIAL ERROR] Failed to send cmd: {ex}")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
        else:
            super().do_GET()

def start_server():
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), DashboardHandler) as httpd:
        print(f"\n=======================================================")
        print(f"  AI-ANC Dashboard running at: http://localhost:{PORT}")
        print(f"=======================================================\n")
        webbrowser.open(f"http://localhost:{PORT}")
        httpd.serve_forever()

if __name__ == "__main__":
    t = threading.Thread(target=serial_worker, daemon=True)
    t.start()
    start_server()
