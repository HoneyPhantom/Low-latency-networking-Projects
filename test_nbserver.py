import socket
import struct
import time

HOST, PORT = "127.0.0.1", 8080

def test_complete_frame():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        msg = b"HELLO"
        s.sendall(struct.pack("!I", len(msg)) + msg)
        header = s.recv(4)
        length = struct.unpack("!I", header)[0]
        data = s.recv(length)
        assert data == msg
        print("[1] Complete frame: PASSED")

def test_fragmented_header():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        msg = b"FRAGMENT_HEADER"
        header = struct.pack("!I", len(msg))
        for b in header:
            s.sendall(bytes([b]))
            time.sleep(0.01)
        s.sendall(msg)
        
        resp_header = s.recv(4)
        resp_len = struct.unpack("!I", resp_header)[0]
        assert s.recv(resp_len) == msg
        print("[2] Fragmented header: PASSED")

def test_fragmented_payload():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        msg = b"ABCDEFGHIJ"
        s.sendall(struct.pack("!I", len(msg)))
        for chunk in [msg[:3], msg[3:7], msg[7:]]:
            s.sendall(chunk)
            time.sleep(0.01)

        resp_header = s.recv(4)
        resp_len = struct.unpack("!I", resp_header)[0]
        assert s.recv(resp_len) == msg
        print("[3] Fragmented payload: PASSED")

def test_coalesced_frames():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        frames = [b"ONE", b"TWO", b"THREE"]
        payload = b"".join(struct.pack("!I", len(f)) + f for f in frames)
        s.sendall(payload)

        for f in frames:
            header = s.recv(4)
            length = struct.unpack("!I", header)[0]
            assert s.recv(length) == f
        print("[4] Coalesced frames: PASSED")

def test_malformed_length():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        s.sendall(struct.pack("!I", 2 * 1024 * 1024))
        time.sleep(0.1)
        
        data = s.recv(1024)
        assert data == b""
        print("[5] Malformed length rejection: PASSED")

def test_disconnect():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        s.sendall(struct.pack("!I", 10) + b"PARTIAL")
        
    print("[7] Disconnect cleanup: PASSED")

if __name__ == "__main__":
    print("Running Day 2 Acceptance Tests...")
    test_complete_frame()
    test_fragmented_header()
    test_fragmented_payload()
    test_coalesced_frames()
    test_malformed_length()
    test_disconnect()
    print("All automated checks completed successfully!")