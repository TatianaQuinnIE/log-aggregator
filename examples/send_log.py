#!/usr/bin/env python3
import socket
import sys

def send_log(host='localhost', port=8080, level='INFO', message='Test log entry'):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        log_line = f"[{level}] {message}\n"
        s.send(log_line.encode('utf-8'))
        s.close()
        print(f"Sent: {log_line.strip()}")
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python3 send_log.py [level] [message] [host] [port]")
        print("Example: python3 send_log.py ERROR 'Database connection failed'")
        sys.exit(1)
    
    level = sys.argv[1]
    message = sys.argv[2]
    host = sys.argv[3] if len(sys.argv) > 3 else 'localhost'
    port = int(sys.argv[4]) if len(sys.argv) > 4 else 8080
    
    send_log(host, port, level, message)