import serial
import time

def read_from_com7(log_file="com7_log.txt", baudrate=9600):
    try:
        ser = serial.Serial('COM7', baudrate, timeout=1)
        print(f"Connected to {ser.port} at {baudrate} baud.")
    except serial.SerialException as e:
        print(f"Error opening COM7: {e}")
        return

    with open(log_file, "a", encoding="utf-8") as f:
        while True:
            try:
                line = ser.readline().decode(errors="ignore").strip()
                if line:
                    print(line)
                    timestamp = time.strftime("[%Y-%m-%d %H:%M:%S] ")
                    f.write(timestamp + line + "\n")
                    f.flush()  # ensures log updates instantly
            except KeyboardInterrupt:
                print("Stopped by user.")
                break
            except Exception as e:
                print(f"Error reading: {e}")
                break

    ser.close()
    print("Serial connection closed.")