import numpy as np
import cv2
import socket
import time

HOST = "192.168.4.2"
PORT = 4455
MAX_SIZE_TO_RECEIVE = 40000

close_connection = False

buffer = b""
expected_buffer_len = -1
measurements = []

def display_image():
    global close_connection
    offset = 0x01
    w = 240
    h = 100
    l = w * h + 2 * (w // 2 * h)

    img = np.frombuffer(buffer[offset: offset + l], dtype=np.uint8).reshape(-1, w,2 )
    bgr = cv2.cvtColor(img, cv2.COLOR_YUV2BGR_Y422)
    cv2.imshow("frame", bgr)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        cv2.destroyAllWindows()
        close_connection = True

def handle_data(data):
    global expected_buffer_len
    global buffer
    if expected_buffer_len <= 0:
        # this is the first packet of the image
        expected_buffer_len = int.from_bytes(data[:4], "little")
        #expected_buffer_len = 100000
        print(f'expected buffer len: {expected_buffer_len}')
        buffer += data
    else:
        buffer += data
        if len(buffer) >= expected_buffer_len:
            # we have a finished buffer
            # print("writing buffer into file")
            # f = open('./test_data.dat', 'wb')
            # f.write(buffer)
            # f.close()
            display_image()
            buffer = b""
            expected_buffer_len = 0
            return True
    return False

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        print("Server created. Listening for connections...")
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            while True:
          #      start = time.time()
           #     conn.send(bytes([255]))
                while not close_connection:
                    data = conn.recv(MAX_SIZE_TO_RECEIVE)
                    if not data:
                        break
                    handle_data(data)
            #            print(f"took: {(time.time()-start)*1000:.2f} ms")
             #           measurements.append((time.time()-start)*1000)
             #           print(f"{sum(measurements) / len(measurements):.2f} ms avg")
                        #break
                break
                


if __name__ == "__main__":
    main()
