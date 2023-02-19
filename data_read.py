from typing import Any
from datetime import datetime
import csv
import warnings
import time
import serial
import serial.tools.list_ports


def serial_obj_buffer_reset(serial_obj: serial.Serial) -> None:
    if serial_obj.in_waiting > 0:
        serial_obj.reset_input_buffer()
    if serial_obj.out_waiting > 0:
        serial_obj.reset_output_buffer()


def serial_obj_bytes_string_from_int(number: int) -> bytes:
    return bytes(str(number) + '\n', 'ascii')


def serial_obj_data_string_to_float(serial_data: list[str]) -> list[float]:
    return [float(data) for data in serial_data]


def serial_obj_data_into_chunks(serial_data: list[Any], number_of_data_per_frame: int) -> list:
    return [serial_data[i:i+number_of_data_per_frame] for i in range(0, len(serial_data), number_of_data_per_frame)]


ports_arduino = [
    port.device
    for port in serial.tools.list_ports.comports()
    if 'Arduino' in port.description
]
if not ports_arduino:
    raise IOError('No Arduino found')
if len(ports_arduino) > 1:
    warnings.warn('Multiple Arduino found - using the first')

serial_obj_baud: int = 115200
serial_obj_timeout = None
# serial_obj_baud: int = int(input('Baudrate: '))
# serial_obj_timeout = None if (
#    input_user := input('Timeout: ')) == '' else float(input_user)
serial_obj = serial.Serial(
    port=ports_arduino[0], baudrate=serial_obj_baud, timeout=serial_obj_timeout)

time_delay: float = 2.5
# Wait for the connection to become stable
time.sleep(time_delay)

number_of_nodes: int = int(input('Number of EIT nodes: '))
serial_obj_number_of_data_per_loop: int = pow(number_of_nodes, 2)
# A single measurement takes up to 6 digits behind decimal place.
# Plus '0', '.' and '\n' result in 9 bytes total.
# Result in the following data size
serial_obj_data_size: int = 9
serial_obj_received_data: bytes = b''
cmd_start: bytes = b'S'
data = list()
configuration_count: int = 0
if serial_obj.is_open:
    serial_obj_buffer_reset(serial_obj)
    serial_obj.write(serial_obj_bytes_string_from_int(number_of_nodes))
    while True:
        serial_obj_buffer_reset(serial_obj)
        number_of_loops = int(input('Number of loops: '))
        if number_of_loops == 0:
            serial_obj.close()
            break
        configuration_count += 1
        serial_obj_number_of_data: int = serial_obj_number_of_data_per_loop * number_of_loops
        serial_obj_expected_number_of_bytes: int = serial_obj_data_size * \
            serial_obj_number_of_data
        serial_obj.write(serial_obj_bytes_string_from_int(number_of_loops))
        serial_obj.write(cmd_start)
        time.sleep(time_delay)
        if serial_obj.in_waiting > 0:
            serial_obj_received_data = serial_obj.read(
                serial_obj_expected_number_of_bytes)
            serial_obj.flush()
        data_temp: list[str] = serial_obj_received_data.decode(
            'ascii').splitlines(keepends=False)
        data = serial_obj_data_into_chunks(serial_obj_data_string_to_float(
            data_temp), number_of_data_per_frame=serial_obj_number_of_data_per_loop)

        # Print out those values and how many of them
        for i in range(number_of_loops):
            data[i].insert(0, str(configuration_count) + '.' + str(i+1))
            for measurement in data[i][1:]:
                print("%.6f" % measurement)
            print(len(data[i]) - 1)  # Number of measurement each loop
        print(len(data))  # Number of loops
else:
    raise IOError('Port isn\'t open')

# Save data to a .csv file
if input('Do you want to save this session? [y,n] ') == 'y':
    data_author: str = input('Operator\'s name: ')
    data_date_current: str = datetime.now().strftime("%d.%m.%Y")
    file_name: str = data_author + '_' + data_date_current + '.csv'
    file_to_be_written = open(file=file_name, mode='w', newline='')
    writer_obj = csv.writer(file_to_be_written)
    data_cols = [str(i) for i in range(serial_obj_number_of_data_per_loop)]
    data_cols.insert(0, 'nth data')
    writer_obj.writerow(data_cols)
    writer_obj.writerows(data)