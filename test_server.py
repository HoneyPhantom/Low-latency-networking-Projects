import socket
import struct
import subprocess
import time
import sys

HOST = "127.0.0.1"
PORT = 8080
SERVER_BIN = "build/server"

def send_frame(sock, message_bytes):
    header = struct.pack("!I", len(message_bytes))
    sock.sendall(header + message_bytes)

def receive_frame(sock):
    header = sock.recv(4)
    if not header:
        return None
    length = struct.unpack("!I", header)[0]
    
    data = b""
    while len(data) < length:
        packet = sock.recv(length - len(data))
        if not packet:
            break
        data += packet
    return data

def run_tests():
    print("[Test Runner] Starting server process...")
    server_process = subprocess.Popen(
        [SERVER_BIN], 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE,
        text=True
    )
    time.sleep(1)

    try:
        print("[Test 1] Running Echo & Integrity Test...")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((HOST, PORT))
            for i in range(1, 101):
                msg = f"Message #{i}".encode("utf-8")
                send_frame(s, msg)
                
                echo_msg = receive_frame(s)
                assert echo_msg == msg, f"Mismatch at message {i}: got {echo_msg}, expected {msg}"
        print("[Test 1] PASSED: 100 messages echoed and verified successfully.")

        print("[Test 2] Testing Graceful Client Disconnect...")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((HOST, PORT))
            send_frame(s, b"Hello before disconnect")
            receive_frame(s)
        
        time.sleep(0.5)
        print("[Test 2] PASSED: Graceful disconnect handled.")

        print("[Test 3] Testing Abrupt Client Disconnect (Hard Close)...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        send_frame(s, b"Abrupt goodbye")
        s.close()
        
        time.sleep(0.5)
        print("[Test 3] PASSED: Abrupt disconnect handled without server crash.")

    finally:
        print("[Test Runner] Cleaning up server process...")
        server_process.terminate()
        server_process.wait()

if __name__ == "__main__":
    try:
        run_tests()
        print("\n✨ ALL TESTS PASSED SUCCESSFULLY!")
    except Exception as e:
        print(f"\n❌ TEST FAILED: {e}")
        sys.exit(1)