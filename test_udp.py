import socket
import struct

HOST, PORT = "127.0.0.1", 9000

def test_malformed_packets():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    s.sendto(b"SHORT", (HOST, PORT))
    
    bad_magic_pkt = struct.pack("!I Q B Q H", 0xDEADBEEF, 1, 1, 123456789, 4) + b"TEST"
    s.sendto(bad_magic_pkt, (HOST, PORT))
    
    mismatch_pkt = struct.pack("!I Q B Q H", 0x4D444154, 2, 1, 123456789, 100) + b"TEST"
    s.sendto(mismatch_pkt, (HOST, PORT))
    
    s.close()
    print("[Test] Malformed packets dispatched. Check receiver logs.")

if __name__ == "__main__":
    test_malformed_packets()