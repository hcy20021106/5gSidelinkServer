# udp_client_repeat_last.py
import socket
import threading
import time

UE_IP = "localhost"
UE_PORT = 6666
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 初始默认消息
last_message = "Hello from Python over UDP"
lock = threading.Lock()
running = True

def sender():
    global last_message, running
    while running:
    	with lock:
    		sock.sendto(last_message.encode(), (UE_IP, UE_PORT))
    
def user_input():
    global last_message, running
    print("=== UDP Client (Auto Send Mode) ===")
    while running:
    	msg = input("please input: ").strip()
    	if msg.strip().lower() == "exit":
    		running = False
    		break
    	if msg.strip():
    		with lock:
    			last_message = msg

# 启动两个线程
threading.Thread(target=sender, daemon=True).start()
user_input()


