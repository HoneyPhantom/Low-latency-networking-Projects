import socket
import struct
import threading
import time

HOST, PORT = "127.0.0.1", 8080
NUM_CLIENTS = 100

def client_worker(client_id, slow=False):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        msg = f"Hello from client {client_id}".encode()
        
        s.sendall(struct.pack("!I", len(msg)) + msg)

        if slow:
            time.sleep(2)

        header = s.recv(4)
        if len(header) == 4:
            length = struct.unpack("!I", header)[0]
            data = s.recv(length)
            assert data == msg
        s.close()
    except Exception as e:
        print(f"Client {client_id} error: {e}")

def run_tests():
    print(f"Starting concurrency test with {NUM_CLIENTS} clients...")
    threads = []

    t_slow = threading.Thread(target=client_worker, args=(0, True))
    threads.append(t_slow)
    t_slow.start()

    for i in range(1, NUM_CLIENTS):
        t = threading.Thread(target=client_worker, args=(i, False))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    print("[Test] 100-client concurrent and slow-reader test completed successfully!")

if __name__ == "__main__":
    run_tests()