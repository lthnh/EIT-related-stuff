import struct
import socket
import warnings
import time
import serial
import serial.tools.list_ports

ports_arduino = [
    port.device
    for port in serial.tools.list_ports.comports()
    if 'Arduino' in port.description
]
if not ports_arduino:
    raise IOError('No Arduino found')
if len(ports_arduino) > 1:
    warnings.warn('Multiple Arduino found - using the first')

serial_obj_baud: int = int(input('Baudrate: '))
serial_obj_timeout = None if (
    input_user := input('Timeout: ')) == '' else float(input_user)
serial_obj = serial.Serial(
    port=ports_arduino[0], baudrate=serial_obj_baud, timeout=serial_obj_timeout)

time_delay: float = 2.5
# Wait for the connection to become stable
time.sleep(time_delay)

number_of_nodes = int(input('Number of EIT nodes: '))
input('Press any key to continue')

cmd_start: bytes = b'S'
# A single measurement takes up to 6 digits behind decimal place.
# Plus '0', '.' and '\n' result in 9 bytes total.
# Result in the following data size
serial_obj_data_size: int = 9
serial_obj_number_of_data: int = number_of_nodes * (number_of_nodes - 3)
serial_obj_expected_number_of_bytes: int = serial_obj_data_size * \
    serial_obj_number_of_data
serial_obj_received_data: bytes = b''
data: list[float] = list()
if serial_obj.is_open:
    if serial_obj.in_waiting > 0:
        serial_obj.reset_input_buffer()
    if serial_obj.out_waiting > 0:
        serial_obj.reset_output_buffer()
    serial_obj.write(cmd_start)
    time.sleep(time_delay)
    if serial_obj.in_waiting > 0:
        serial_obj_received_data = serial_obj.read(
            serial_obj_expected_number_of_bytes)
        serial_obj.flush()
    data_temp: list[str] = serial_obj_received_data.decode(
        'ascii').splitlines(keepends=False)
    data = [float(measurement) for measurement in data_temp]
else:
    raise IOError('Port isn\'t open')

serial_obj.close()

# Print out those values and how many of them
for a_measurement in data:
    print("%.6f" % a_measurement)
print(len(data))

# Create socket for data transfer through TCP/IP protocol
# For now we just need to implement one way transfer

HOST = '127.0.0.1'
PORT = 9090
socket_incoming_comm_limit = 1

socket_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
socket_server.bind((HOST, PORT))
print('Create socket successfully')
socket_server.listen(socket_incoming_comm_limit)
socket_client, address_client = socket_server.accept()
print('Connected by', address_client)

input('Press any key to continue')

for measurement in data:
    socket_client.sendall(f'{measurement:.6f}'.encode())
socket_client.close()
