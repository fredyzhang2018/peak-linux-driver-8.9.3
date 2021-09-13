'''This python script is used to automate the Eth-CAN gateway demo from a Linux PC. 
 Dependencies
 ------------
 1. Peak Linux driver 8.9.3 (older versions may also work) installed
 2. Some socket programs which are provided with the python script
 3. Two Ethernet interfaces on the PC (both connected to the J7ES board)
 4. Two PCAN connections to MCAN4 and MCAN9 on the GESI board
 '''
#For running routines
import subprocess

#For file processing
import os
from os import path

import re
import time
import threading
import signal
import sys
import argparse

import netifaces

pipelist=[
"/tmp/a72-load-pipe",
"/tmp/r51-load-pipe",
"/tmp/r52-load-pipe",
"/tmp/can-bw-pipe",
"/tmp/can-latency-pipe",
"/tmp/e2c-bw-pipe",
"/tmp/e2c-latency-pipe",
"/tmp/eth-bw-pipe",
"/tmp/eth-latency-pipe",
"/tmp/c2e-bw-pipe",
"/tmp/c2e-latency-pipe",
]

CONFIG_FILENAME = './pctools/app_config.txt'
gateway_interface_lut = {}  #Empty Dictionary for Interface MAC addresses
gateway_interfaces = []

num_seconds = 0
exit_program = False

# Number of CAN frames to send for CAN 2 CAN and CAN 2 Eth demo
num_can_frames = 5

initial_setup_complete = False

# Profiling variables
num_can_frames_sent = 0
num_can_frames_received_1 = 0
num_can_frames_received_2 = 0

num_eth_frames_received = 0

avg_can_2_can_delay = 0
max_can_2_can_delay = 0

avg_can_2_eth_delay = 0
max_can_2_eth_delay = 0

num_eth_frames_sent = 0

avg_eth_2_eth_delay = 0
max_eth_2_eth_delay = 0

avg_eth_2_can_delay_1 = 0
avg_eth_2_can_delay_2 = 0
max_eth_2_can_delay_1 = 0
max_eth_2_can_delay_2 = 0

eth_2_can_test_duration_2 = 0

gui_enabled = True

#If CAN ID matches this then packet is sent only to MCAN4 from Eth
ROUTE_TO_MCAN4  = '0xD0'
#If CAN ID matches this then packet is sent only to MCAN9 from Eth
ROUTE_TO_MCAN9  = '0xB0'
#If CAN ID matches this then packet is sent to both MCAN's from Eth
ROUTE_TO_BOTH_CAN = '0xC0'
#If CAN ID matches this then packet is sent to other Eth port*/
ROUTE_TO_ETH = '0xF0'


list_of_processes = []

def initialize_counters():
    # Profiling variables
    num_can_frames_sent = 0
    num_can_frames_received_1 = 0
    num_can_frames_received_2 = 0

    num_eth_frames_received = 0

    avg_can_2_can_delay = 0
    max_can_2_can_delay = 0

    avg_can_2_eth_delay = 0
    max_can_2_eth_delay = 0

    num_eth_frames_sent = 0

    avg_eth_2_eth_delay = 0
    max_eth_2_eth_delay = 0

    avg_eth_2_can_delay_1 = 0
    avg_eth_2_can_delay_2 = 0
    max_eth_2_can_delay_1 = 0
    max_eth_2_can_delay_2 = 0

'''Function to generate PCAN interface config'''
def generate_pcan_config():
    input("Please remove any PCAN devices connected to the PC....hit Enter when ready\n")
    print("Now connect the first PCAN device to MCAN4 interface (J14) on the GESI card...refer to the setup diagram\n")
    input("Hit Enter when ready....\n")

    #Make sure we have one PCAN and find out it's name
    process = subprocess.Popen(['lspcan'], stdout=subprocess.PIPE)
    lines = process.stdout.readlines()  #Read the output of program

    words = []
    for line in lines:
        string_out = line.decode("utf-8")
        words = string_out.split('\t')
        if len(words) == 1:
            print("Coudln't find anything..are you sure you connected something...try running lspcan command yourself and check...Exiting\n")
            sys.exit()
        else:
            print("Found one interface\n")
            temp_string = "/dev/" + words[0].strip()
            gateway_interfaces.append(temp_string)
            gateway_interface_lut[temp_string] = "0xD0"

    print("Now connect the second PCAN device to MCAN9 interface (J6) on the GESI card...refer to the setup diagram\n")
    input("Hit Enter when ready....\n")
            
    process = subprocess.Popen(['lspcan'], stdout=subprocess.PIPE)
    lines = process.stdout.readlines()  #Read the output of program

    words = []
    for line in lines:
        string_out = line.decode("utf-8")
        words = string_out.split('\t')
        if len(words) == 2:
            print("Coudln't find the second interface...Exiting\n")
            sys.exit()
        else:
            print("Found the second interface\n")
            temp_string = "/dev/" + words[1].strip()
            gateway_interfaces.append(temp_string)
            gateway_interface_lut[temp_string] = "0xE0"

def generate_eth_config():

    print("The script will now try to determine the Ethernet interfaces and prompt you to select the right one's\n")
    print("Please select the primary interface. It's usually the built in Ethernet card on a laptop or PC\n")
    primary_interface_selected = False
    for interface in netifaces.interfaces():
        print("\nIs ", interface, " your primary interface ?")
        usr_input = input("Press Y/N : ")
        if (usr_input.lower() == "y"):
            primary_interface_selected = True
            addrs = netifaces.ifaddresses(interface)
            array = addrs[netifaces.AF_LINK]
            gateway_interfaces.append(interface)
            gateway_interface_lut[interface] = array[0]['addr']
            break

    if not primary_interface_selected:
        print("You did not select any primary interface. Script will now exit\n")
        sys.exit()

    print("\nPlease select the secondary interface. It's usually a USB dongle on a laptop or a secondary NIC on a PC.\n")
    secondary_interface_selected = False
    for interface in netifaces.interfaces():
        print("\nIs ", interface, " your secondary interface ?")
        usr_input = input("Press Y/N : ")
        if (usr_input.lower() == "y"):
            secondary_interface_selected = True
            addrs = netifaces.ifaddresses(interface)
            array = addrs[netifaces.AF_LINK]
            gateway_interfaces.append(interface)
            gateway_interface_lut[interface] = array[0]['addr']
            break
    
    if not secondary_interface_selected:
        print("You did not select any secondary interface. Script will now exit\n")
        sys.exit()

def check_CAN_2_CAN_Routing(run_time, gui_enabled, oob_mode):
    #The app will send CAN messages and simultaneously listen for CAN and Ethernet messages
    #First, start listener threads for Ethernet and CAN
    #This will start listening on one ethernet interface

    #We need to program the routing table such that CAN frames received on second interface are routed to first interface
    subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--can_id', gateway_interface_lut[gateway_interfaces[1]], '--update_hash_table', '1', '--can_mask', '0x1', '--eth_mask', '0x0'], stdout=subprocess.PIPE)
    time.sleep(0.2)

    if gui_enabled:
        eth_rcv = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--eth_interface', gateway_interfaces[3], '--timeout', str(run_time+1)], stdout=subprocess.PIPE)
    else:
        eth_rcv = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--pipes', '0', '--eth_interface', gateway_interfaces[3], '--timeout', str(run_time+1)], stdout=subprocess.PIPE)

    time.sleep(1)

    if oob_mode:
        eth_command = subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--can_id', gateway_interface_lut[gateway_interfaces[1]], '--can_generator_route', '1', '--timeout', str(run_time)], stdout=subprocess.PIPE)
        time.sleep(run_time + 2)
        eth_command = subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--can_id', gateway_interface_lut[gateway_interfaces[1]], '--get_can_generator_stats', '1'], stdout=subprocess.PIPE)
    else:        
        #This will start listening for CAN messages
        if gui_enabled:
            can_rcv = subprocess.Popen(['./pcanfdtst', 'rx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '--fd', '-q', '-M', str(run_time+3), gateway_interfaces[0]], stdout=subprocess.PIPE)
        else:
            can_rcv = subprocess.Popen(['./pcanfdtst', 'NO_GUI', 'rx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '--fd', '-q', '-M', str(run_time+3), gateway_interfaces[0]], stdout=subprocess.PIPE)

        time.sleep(1)

        #This will send CAN messages
        can_send = subprocess.Popen(['./pcanfdtst', 'tx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '--brs', '-q', '-M', str(run_time), '-ie', gateway_interface_lut[gateway_interfaces[1]], '--fd', '-l', '64', '--tx-pause-us', '1', gateway_interfaces[1]], stdout=subprocess.PIPE)
        #can_send = subprocess.Popen(['./pcanfdtst', 'tx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '-q', '-M', str(run_time), '-ie', gateway_interface_lut[gateway_interfaces[1]], '--fd', '-l', '64', '--tx-pause-us', '1', gateway_interfaces[1]], stdout=subprocess.PIPE)
        #can_send = subprocess.Popen(['./pcanfdtst', 'tx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '-q', '-n', '1', '-ie', gateway_interface_lut[gateway_interfaces[1]], '--fd', '-l', '64', '--tx-pause-us', '1', gateway_interfaces[1]], stdout=subprocess.PIPE)

        #Wait for CAN receive to end
        lines = can_rcv.stdout.readlines()  #Read the output of program and parse output

        for line in lines:
            string_out = line.decode("utf-8")
            if re.search("received frames", string_out):
                words = string_out.split(":")
                num_can_frames_received_1 = words[1].strip()

            if re.search("Average CAN 2 CAN Delay", string_out):
                words = string_out.split(":")
                avg_can_2_can_delay = words[1].strip()

            if re.search("Max CAN 2 CAN Delay", string_out):
                words = string_out.split(":")
                max_can_2_can_delay = words[1].strip()

        lines = can_send.stdout.readlines()  #Read the output of program and parse output
        for line in lines:
            string_out = line.decode("utf-8")
            if re.search("sent frames", string_out):
                words = string_out.split(":")
                num_can_frames_sent = words[1].strip()

    #Wait for Eth receive to end
    lines = eth_rcv.stdout.readlines()  #Read the output of program
    for line in lines:
        string_out = line.decode("utf-8")
        if re.search("Average CPU Load", string_out):
            words = string_out.split(":")
            avg_cpu_load = words[1].strip()
        if re.search("Average SWI Load", string_out):
            words = string_out.split(":")
            avg_swi_load = words[1].strip()
        if re.search("Average ISR Load", string_out):
            words = string_out.split(":")
            avg_isr_load = words[1].strip()

        if(oob_mode):
            if re.search("Num CAN frames received", string_out):
                words = string_out.split(":")
                num_can_frames_received_1 = words[1].strip()

            if re.search("Num CAN frames sent", string_out):
                words = string_out.split(":")
                num_can_frames_sent = words[1].strip()

            if re.search("Average CAN 2 CAN delay", string_out):
                words = string_out.split(":")
                avg_can_2_can_delay = words[1].strip()

            if re.search("Max CAN 2 CAN delay", string_out):
                words = string_out.split(":")
                max_can_2_can_delay = words[1].strip()             
    

    print("-------------------CAN 2 CAN Test Results----------------------\n")
    print("Number of CAN frames sent \t: ", num_can_frames_sent)
    print("Number of CAN frames received \t: ", num_can_frames_received_1)
    print("Test ran for\t\t\t: ", str(run_time), "seconds \n")

    print("Average CAN 2 CAN delay \t: ", avg_can_2_can_delay)
    print("Max CAN 2 CAN delay \t\t: ", max_can_2_can_delay, "\n")

    print("Average CPU Load \t\t: ", avg_cpu_load, "%")
    print("Average SWI Load \t\t: ", avg_swi_load, "%")
    print("Average ISR Load \t\t: ", avg_isr_load, "%\n")

    can_rx_rate_per_sec = int(num_can_frames_sent)/run_time
    can_tx_rate_per_sec = int(num_can_frames_received_1)/run_time

    print("CAN Rx rate \t\t\t: ", int(can_rx_rate_per_sec), "msgs/sec")
    print("CAN Tx rate \t\t\t: ", int(can_tx_rate_per_sec), "msgs/sec\n")    

    average_of_tx_and_rx_rate = (can_tx_rate_per_sec + can_rx_rate_per_sec) / 2

    cpu_load_for_8k_rate = float(avg_cpu_load) * (8000 / average_of_tx_and_rx_rate)
    print("CPU Load for 8k CAN (Rx + Tx) \t: ", cpu_load_for_8k_rate * 10, "Mhz")


def check_CAN_2_ETH_Routing(run_time, gui_enabled, oob_mode):
    #The app will send CAN messages and simultaneously listen for CAN and Ethernet messages
    #First, start listener threads for Ethernet and CAN

    #We need to program the routing table such that CAN frames received on second interface are routed to ethernet interface
    subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--can_id', gateway_interface_lut[gateway_interfaces[1]], '--update_hash_table', '1', '--can_mask', '0x0', '--eth_mask', '0x1'], stdout=subprocess.PIPE)
    time.sleep(0.2)

    #This will start listening on one ethernet interface
    if gui_enabled:
        eth_rcv = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--eth_interface', gateway_interfaces[3], '--timeout', str(run_time+5)], stdout=subprocess.PIPE)
    else:
        eth_rcv = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--pipes', '0', '--eth_interface', gateway_interfaces[3], '--timeout', str(run_time+5)], stdout=subprocess.PIPE)

    if(oob_mode):
        # This command will trigger CAN messages on generator
        eth_command = subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--can_id', gateway_interface_lut[gateway_interfaces[1]], '--can_generator_route', '1', '--timeout', str(run_time)], stdout=subprocess.PIPE)
        # Wait till transmission is complete
        time.sleep(run_time + 2)
        # Send command to get stats
        eth_command = subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--can_id', gateway_interface_lut[gateway_interfaces[1]], '--get_can_generator_stats', '1'], stdout=subprocess.PIPE)
    else:
        #This will send CAN messages
        can_send = subprocess.Popen(['./pcanfdtst', 'tx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '--brs', '-q', '-M', str(run_time), '-ie', gateway_interface_lut[gateway_interfaces[1]], '--fd', '-l', '64', '--tx-pause-us', '1', gateway_interfaces[1]], stdout=subprocess.PIPE)
        #can_send = subprocess.Popen(['./pcanfdtst', 'tx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '-q', '-M', str(run_time), '-ie', gateway_interface_lut[gateway_interfaces[1]], '--fd', '-l', '64', '--tx-pause-us', '1', gateway_interfaces[1]], stdout=subprocess.PIPE)

        lines = can_send.stdout.readlines()  #Read the output of program and parse output
        for line in lines:
            string_out = line.decode("utf-8")
            if re.search("sent frames", string_out):
                words = string_out.split(":")
                num_can_frames_sent = words[1].strip()
    
    #Wait for Eth receive to end
    lines = eth_rcv.stdout.readlines()  #Read the output of program
    for line in lines:
        string_out = line.decode("utf-8")
        if re.search("received frames", string_out):
            words = string_out.split(":")
            num_eth_frames_received = words[1].strip()

        if re.search("Average CAN 2 Eth delay", string_out):
            words = string_out.split(":")
            avg_can_2_eth_delay = words[1].strip()

        if re.search("Max CAN 2 Eth delay", string_out):
            words = string_out.split(":")
            max_can_2_eth_delay = words[1].strip()

        if re.search("Average CPU Load", string_out):
            words = string_out.split(":")
            avg_cpu_load = words[1].strip()

        if re.search("Recieved packets for", string_out):
            words = string_out.split(":")
            can_2_eth_test_duration = words[1].strip()

        if re.search("Average SWI Load", string_out):
            words = string_out.split(":")
            avg_swi_load = words[1].strip()

        if re.search("Average ISR Load", string_out):
            words = string_out.split(":")
            avg_isr_load = words[1].strip()

        if(oob_mode):
            if re.search("Num CAN frames sent", string_out):
                words = string_out.split(":")
                num_can_frames_sent = words[1].strip()

    print("-------------------CAN 2 ETH Test Results----------------------\n")
    print("Number of CAN frames sent \t: ", num_can_frames_sent)
    print("Number of Eth frames received \t: ", (int(num_eth_frames_received) - 1)/2, "\n")
    print("Test ran for\t: ", str(run_time), "seconds \n")

    print("Average CAN 2 Eth delay \t: ", avg_can_2_eth_delay)
    print("Max CAN 2 Eth delay \t\t: ", max_can_2_eth_delay, "\n")
    print("Average CPU Load \t\t: ", avg_cpu_load, "%")
    print("Average SWI Load \t\t: ", avg_swi_load, "%")
    print("Average ISR Load \t\t: ", avg_isr_load, "%")

    can_rx_rate_per_sec = int(num_can_frames_sent)/run_time
    eth_tx_rate_per_sec = (int(num_eth_frames_received) - 1)/2
    eth_tx_rate_per_sec = eth_tx_rate_per_sec/run_time

    print("CAN Rx rate \t\t\t: ", int(can_rx_rate_per_sec), "msgs/sec")
    print("Eth Tx rate \t\t\t: ", int(eth_tx_rate_per_sec), "msgs/sec\n")

    average_of_tx_and_rx_rate = (can_rx_rate_per_sec + eth_tx_rate_per_sec) / 2

    cpu_load_for_8k_rate = float(avg_cpu_load) * (8000 / average_of_tx_and_rx_rate)
    print("CPU Load for 8k CAN Rx + 8k Eth Tx \t: ", cpu_load_for_8k_rate * 10, "Mhz")

def check_ETH_2_ETH_Routing(run_time, gui_enabled, oob_mode):
    '''The app will send Ethernet messages and simultaneously listen for routed Ethernet messages.'''
    #This will start listening on one ethernet interface

    #We need to program the routing table such that Eth frames received on second interface are routed to ethernet interface
    subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--can_id', ROUTE_TO_ETH, '--update_hash_table', '1', '--can_mask', '0x0', '--eth_mask', '0x2'], stdout=subprocess.PIPE)
    time.sleep(0.2)
    
    if gui_enabled:
        eth_rcv = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--eth_interface', gateway_interfaces[3], '--timeout', str(run_time+1)], stdout=subprocess.PIPE)
    else:
        eth_rcv = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--pipes', '0', '--eth_interface', gateway_interfaces[3], '--timeout', str(run_time+1)], stdout=subprocess.PIPE)

    time.sleep(1)
    #Now send on other Ethernet port
    eth_send = subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[2], '--timeout', str(run_time), '--route', 'ETH', '--ipg', '1'], stdout=subprocess.PIPE)
    #eth_send = subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[2], '--num_packets', '1', '--route', 'ETH', '--ipg', '1'], stdout=subprocess.PIPE)

    lines = eth_rcv.stdout.readlines()  #Read the output of program and parse output
    for line in lines:
        string_out = line.decode("utf-8")
        if re.search("received frames", string_out):
            words = string_out.split(":")
            num_eth_frames_received = words[1].strip()

        if re.search("Average Eth 2 Eth delay", string_out):
            words = string_out.split(":")
            avg_eth_2_eth_delay = words[1].strip()

        if re.search("Max Eth 2 Eth delay", string_out):
            words = string_out.split(":")
            max_eth_2_eth_delay = words[1].strip()

        if re.search("Average CPU Load", string_out):
            words = string_out.split(":")
            avg_cpu_load = words[1].strip()
        
        if re.search("Average SWI Load", string_out):
            words = string_out.split(":")
            avg_swi_load = words[1].strip()

        if re.search("Average ISR Load", string_out):
            words = string_out.split(":")
            avg_isr_load = words[1].strip()

        if re.search("Recieved packets for", string_out):
            words = string_out.split(":")
            eth_2_eth_test_duration = words[1].strip()

    lines = eth_send.stdout.readlines()  #Read the output of program and parse output
    for line in lines:
        string_out = line.decode("utf-8")
        if re.search("sent frames", string_out):
            words = string_out.split(":")
            num_eth_frames_sent = words[1].strip()

    print("-------------------ETH 2 ETH Test Results----------------------\n")
    print("Number of Eth frames sent \t: ", num_eth_frames_sent)
    print("Number of Eth frames received \t: ", int(num_eth_frames_received) - 1, "\n")
    print("Test ran for \t: ", str(run_time), "seconds \n")

    eth_2_eth_delay_avg_number = re.findall(r'\d+', avg_eth_2_eth_delay)[0]
    eth_2_eth_delay_max_number = re.findall(r'\d+', max_eth_2_eth_delay)[0]

    print("Average ETH 2 ETH delay \t: ", int(eth_2_eth_delay_avg_number) , " us")
    print("Max ETH 2 ETH delay \t\t: ", int(eth_2_eth_delay_max_number), " us\n")

    print("Average CPU Load \t\t: ", avg_cpu_load, "%")
    print("Average SWI Load \t\t: ", avg_swi_load, "%")
    print("Average ISR Load \t\t: ", avg_isr_load, "%")

    eth_rx_rate_per_sec = int(num_eth_frames_sent)/run_time
    eth_tx_rate_per_sec = (int(num_eth_frames_received) - 1)/run_time
    
    print("Eth Rx rate \t\t\t: ", int(eth_rx_rate_per_sec), "msgs/sec")
    print("Eth Tx rate \t\t\t: ", int(eth_tx_rate_per_sec), "msgs/sec\n")    

    average_of_tx_and_rx_rate = (eth_tx_rate_per_sec + eth_rx_rate_per_sec) / 2

    cpu_load_for_8k_rate = float(avg_cpu_load) * (8000 / average_of_tx_and_rx_rate)
    print("CPU Load for 8k Eth Rx + Tx \t: ", cpu_load_for_8k_rate * 10, "Mhz")
    
def check_ETH_2_CAN_Routing(run_time, gui_enabled, oob_mode):
    '''The app will send Ethernet messages and simultaneously listen for CAN messages on both ports'''

    #We need to program the routing table such that Eth frames received on second interface are routed to first CAN interface
    subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--can_id', ROUTE_TO_MCAN4, '--update_hash_table', '1', '--can_mask', '0x1', '--eth_mask', '0x0'], stdout=subprocess.PIPE)
    time.sleep(0.2)

    if(not oob_mode):
        # Listen on first CAN port
        if gui_enabled:
            can_rcv_1 = subprocess.Popen(['./pcanfdtst', 'rx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '--fd', '-q', '-M', str(run_time+3), gateway_interfaces[0]], stdout=subprocess.PIPE)
        else:
            can_rcv_1 = subprocess.Popen(['./pcanfdtst', 'NO_GUI', 'rx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '--fd', '-q', '-M', str(run_time+3), gateway_interfaces[0]], stdout=subprocess.PIPE)

        time.sleep(1)

    # Listen for diagnostic messages
    if gui_enabled:
        eth_rcv = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--eth_interface', gateway_interfaces[3], '--timeout', str(run_time + 5)], stdout=subprocess.PIPE)
    else:
        eth_rcv = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--pipes', '0', '--eth_interface', gateway_interfaces[3], '--timeout', str(run_time + 5)], stdout=subprocess.PIPE)

    #Send on secondary ethernet port
    eth_send = subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--timeout', str(run_time+1), '--route', 'MCAN4', '--ipg', '150'], stdout=subprocess.PIPE)

    if(oob_mode):
        time.sleep(run_time + 2)
        eth_command = subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--can_id', gateway_interface_lut[gateway_interfaces[1]], '--get_can_generator_stats', '1'], stdout=subprocess.PIPE)
    else:
        lines = can_rcv_1.stdout.readlines()  #Read the output of program and parse output
        for line in lines:
            string_out = line.decode("utf-8")
            if re.search("received frames", string_out):
                words = string_out.split(":")
                num_can_frames_received_1 = words[1].strip()

            if re.search("Average Eth 2 CAN Delay", string_out):
                words = string_out.split(":")
                avg_eth_2_can_delay_1 = words[1].strip()

            if re.search("Max Eth 2 CAN Delay", string_out):
                words = string_out.split(":")
                max_eth_2_can_delay_1 = words[1].strip()

    lines = eth_send.stdout.readlines()  #Read the output of program and parse output
    for line in lines:
        string_out = line.decode("utf-8")
        if re.search("sent frames", string_out):
            words = string_out.split(":")
            num_eth_frames_sent = words[1].strip()

    lines = eth_rcv.stdout.readlines()  #Read the output of program
    for line in lines:
        string_out = line.decode("utf-8")
        if re.search("Average CPU Load", string_out):
            words = string_out.split(":")
            avg_cpu_load = words[1].strip()

        if re.search("Average SWI Load", string_out):
            words = string_out.split(":")
            avg_swi_load = words[1].strip()

        if re.search("Average ISR Load", string_out):
            words = string_out.split(":")
            avg_isr_load = words[1].strip()

        if(oob_mode):
            if re.search("Num CAN frames received", string_out):
                words = string_out.split(":")
                num_can_frames_received_1 = words[1].strip()

            if re.search("Average ETH 2 CAN delay", string_out):
                words = string_out.split(":")
                avg_eth_2_can_delay_1 = words[1].strip()

            if re.search("Max ETH 2 CAN delay", string_out):
                words = string_out.split(":")
                max_eth_2_can_delay_1 = words[1].strip()

    print("-------------------ETH 2 CAN Test Results----------------------\n")
    print("Number of Eth frames sent \t: ", num_eth_frames_sent)
    print("Number of CAN frames received on 1st interface \t: ", num_can_frames_received_1, "\n")

    print("Test ran for\t: ", str(run_time), "seconds \n")
    eth_2_can_delay_avg_number = re.findall(r'\d+', avg_eth_2_can_delay_1)[0]
    eth_2_can_delay_max_number = re.findall(r'\d+', max_eth_2_can_delay_1)[0]
    
    print("Average ETH 2 CAN delay for 1st interface \t: ", int(eth_2_can_delay_avg_number), " us")
    print("Max ETH 2 CAN delay for 1st interface \t\t: ", int(eth_2_can_delay_max_number), " us \n")

    print("Average CPU Load \t\t: ", avg_cpu_load, "%")
    print("Average SWI Load \t\t: ", avg_swi_load, "%")
    print("Average ISR Load \t\t: ", avg_isr_load, "%")


    eth_rx_rate_per_sec = int(num_eth_frames_sent)/run_time
    can_tx_rate_per_sec = int(num_can_frames_received_1)/run_time
    
    print("Eth Rx rate \t\t\t: ", int(eth_rx_rate_per_sec), "msgs/sec")
    print("CAN Tx rate \t\t\t: ", int(can_tx_rate_per_sec), "msgs/sec\n")

    average_of_tx_and_rx_rate = (eth_rx_rate_per_sec + can_tx_rate_per_sec) / 2

    cpu_load_for_8k_rate = float(avg_cpu_load) * (8000 / average_of_tx_and_rx_rate)
    print("CPU Load for 8k Eth Rx + 8k CAN Tx \t: ", cpu_load_for_8k_rate * 10, "Mhz")

def check_System_tests(run_time, gui_enabled):
    '''Script will now do system tests'''

    # Listen on first CAN port
    if gui_enabled:
        can_rcv_1 = subprocess.Popen(['./pcanfdtst', 'rx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '--fd', '-q', '-M', str(run_time+1), gateway_interfaces[0]], stdout=subprocess.PIPE)
    else:
        can_rcv_1 = subprocess.Popen(['./pcanfdtst', 'rx', 'NO_GUI', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '--fd', '-q', '-M', str(run_time+1), gateway_interfaces[0]], stdout=subprocess.PIPE)
    list_of_processes.append(can_rcv_1)

    # Listen for diagnostic messages on first port
    if gui_enabled:
        eth_rcv_1 = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--eth_interface', gateway_interfaces[2], '--timeout', str(run_time+1)], stdout=subprocess.PIPE)
    else:
        eth_rcv_1 = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--pipes', '0', '--eth_interface', gateway_interfaces[2], '--timeout', str(run_time+1)], stdout=subprocess.PIPE)
    list_of_processes.append(eth_rcv_1)
    time.sleep(1)

    #This will send CAN messages
    can_send_2 = subprocess.Popen(['./pcanfdtst', 'tx', 'ETH_AND_CAN',  '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '-q', '-M', str(run_time), '-ie', gateway_interface_lut[gateway_interfaces[1]], '--fd', '-l', '64', '--tx-pause-us', '1', gateway_interfaces[1]], stdout=subprocess.PIPE)
    list_of_processes.append(can_send_2)

    #Send on secondary ethernet port
    eth_send_1 = subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--gateway_mac', gateway_interface_lut['gateway'], '--timeout', str(run_time), '--route', 'MCAN4', '--ipg', '300'], stdout=subprocess.PIPE)
    list_of_processes.append(eth_send_1)

    #Now send on other Ethernet port
    eth_send = subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--gateway_mac', gateway_interface_lut['gateway'], '--dst_mac', gateway_interface_lut[gateway_interfaces[2]], '--timeout', str(run_time), '--route', 'ETH', '--ipg', '100'], stdout=subprocess.PIPE)
    list_of_processes.append(eth_send)

    lines = can_rcv_1.stdout.readlines()  #Read the output of program and parse output
    for line in lines:
        string_out = line.decode("utf-8")

'''This function zeroes out all pipes when GUI is enabled,
   it is called every time a test is run'''
def zero_all_pipes(gui_enabled):
    if gui_enabled:
        for count in range(2):
            for item in pipelist:
                command = "echo " + "0.0" + " > " + item
                os.system(command)

        print("Check GUI, everything should be 0...")

def detect_linux_crash():
    # Open a file
    while True:
        log_file = open("/tmp/gateway_evm.log", "r")

        lines = log_file.read().splitlines()

        last_lines = ""

        for i in range(1, 6):
            last_lines += lines[-i]

        if "Kernel panic" in last_lines:
            command = "echo 1 > /tmp/warning-pipe"
            os.system(command)
        else:
            command = "echo 0 > /tmp/warning-pipe"
            os.system(command)

        # Close opened file
        log_file.close()

        time.sleep(2)

'''-----------------------------Main Entry Point-----------------------------'''
if __name__ == "__main__":
    #Do we have the configuration file ?
    if not path.exists(CONFIG_FILENAME):
        initial_setup_complete = False
    else:
        initial_setup_complete = True
        config_file = open(CONFIG_FILENAME, 'r')  #Open file
        file_buffer = config_file.readlines()     #Slurp all lines
        config_file.close()                       #close file
        for line in file_buffer:
            line = line.strip()
            if line and not re.match('^#', line):
                words = line.split('-')
                if(words):
                    gateway_interface_name = words[0].strip()
                    gateway_interfaces.append(gateway_interface_name)

                    ID = words[1].strip()
                    gateway_interface_lut[gateway_interface_name] = ID                   
    
    #The scipt will attempt to detect all interfaces and generate a config file. Refer to the setup diagram and
    #be prepared to attach all interfaces as required

    if not initial_setup_complete:
        #Find and populate PCAN config    
        generate_pcan_config()

        generate_eth_config()

        print("--------Two CAN and two Ethernet Interfaces were added as shown below--------")
        print("CAN Interface for MCAN4 \t:", gateway_interfaces[0])
        print("CAN Interface for MCAN9 \t:", gateway_interfaces[1])
        print("Primary Eth Interface \t\t:", gateway_interfaces[2])
        print("Secondary Eth Interface \t\t:", gateway_interfaces[3])

        usr_input = input("If this is ok, then hit Y else hit N (to exit)\n")

        if(usr_input.lower() == "n"):
            print("Exiting script, try again..")
            sys.exit()

        print("If not already, then please run the Gateway app. Script will try to determine the MAC address of gateway device\n")
        input("Hit Enter when ready...")
        if gui_enabled:
            #Now Run the Gateway app...program will wait for Ethernet frame from the EVM to determine the MAC address of Gateway
            process = subprocess.Popen(['sudo', './pctools/recv_1722.out', '--eth_interface', gateway_interfaces[3], '--detect_mac', '1', '--timeout', '5'], stdout=subprocess.PIPE)
        else:
            process = subprocess.Popen(['sudo', '--pipes', '0', './pctools/recv_1722.out', '--eth_interface', gateway_interfaces[3], '--detect_mac', '1', '--timeout', '5'], stdout=subprocess.PIPE)

        #Send 30 CAN frames which will generate some Ethernet frames
        can_send = subprocess.Popen(['./pcanfdtst', 'tx', '--bitrate', '1000000', '--dbitrate', '5000000', '--clock', '80000000', '-q', '-M', '5', '-ie', gateway_interface_lut[gateway_interfaces[1]], '--fd', '-l', '64', '--tx-pause-us', '1', gateway_interfaces[1]], stdout=subprocess.PIPE)
        lines = process.stdout.readlines()  #Read the output of program
        found_gateway_device = False
        for line in lines:
            string_out = line.decode("utf-8")
            if (re.search('found gateway mac',string_out)):
                words = string_out.split('-')
                gateway_interfaces.append('gateway')
                gateway_interface_lut['gateway'] = words[1].strip()
                print("Found Gateway MAC. Good!")
                found_gateway_device = True

        if not found_gateway_device:
            print("Didn't get any packet from gateway, are you sure you are running it ? Maybe run wireshark and check ?")
            sys.exit()

        #Write the complete configuration into config file so we don't do this again and again
        config_file = open(CONFIG_FILENAME, 'w+')
        for i in range(5):
            file_string = gateway_interfaces[i] + " - " + gateway_interface_lut[gateway_interfaces[i]] + "\n"
            config_file.write(file_string) #write the configuration

        config_file.close()
        print("Configuration written to file : ", CONFIG_FILENAME)
    else:
        print("The setup found an existing config file : ", CONFIG_FILENAME, " and it will be used.")
        time.sleep(1)
        print("If you think that's incorrect or if your setup has changed then please delete : ", CONFIG_FILENAME, " and run this script again")
        time.sleep(1)  

    parser = argparse.ArgumentParser(description='Gateway CES Demo Script')
    parser.add_argument("--iterations", default=1, type=int, help="Number of iterations")
    parser.add_argument("--run_time", default=20, type=int, help="Duration of each test")
    parser.add_argument("--system_test", default=1, type=int, help="1 for system_test, 0 for individual tests")
    parser.add_argument("--gui_enabled", default=1, type=int, help="1 for system_test, 0 for individual tests")
    parser.add_argument("--oob_mode", default=0, type=int, help="1 in Out of Box mode and 0 for regular tests")
    args = parser.parse_args()

    if args.gui_enabled:
        thread_detect_linux_crash = threading.Thread(target=detect_linux_crash)
        thread_detect_linux_crash.start()
    else:
        if args.system_test:
            print("\n!!! System Test cannot run with GUI disabled\n")
            sys.exit(0)

    # print (args)
    print ("\nScript will run for", args.iterations, "iteration(s)")
    print ("Each test will run for", args.run_time, "second(s)\n")

    #Program Eth MAC addresses on device
    subprocess.Popen(['sudo', './pctools/send_1722.out', '--eth_interface', gateway_interfaces[3], '--send_mac_address', '1'], stdout=subprocess.PIPE)
    time.sleep(0.25)

    if(args.oob_mode):
        print("------------------------------------------------")
        print("Demo is running in Out Of Box Configuration Mode")
        print("------------------------------------------------")
        print("\n")
        time.sleep(0.5)
    
    for i in range(0, args.iterations):

        if args.system_test == 1:
            print("Iteration #", i, ": Script will now run system tests\n")
            initialize_counters()
            zero_all_pipes(args.gui_enabled)
            time.sleep(1)
            check_System_tests(args.run_time, args.gui_enabled)  

            time.sleep(2)
            
            zero_all_pipes(args.gui_enabled)
            print("Iteration #", i, "test completed\n")
            print("Press Ctrl+C to exit\n")

        else:
            print("Iteration #", i, ": Script will now run individual tests\n")
                                   
            initialize_counters()
            print("Script will now run CAN 2 CAN tests")
            zero_all_pipes(args.gui_enabled)
            time.sleep(1)
            check_CAN_2_CAN_Routing(args.run_time, args.gui_enabled, args.oob_mode) 
             
            initialize_counters()
            print("Script will now run CAN 2 Eth tests")
            zero_all_pipes(args.gui_enabled)
            time.sleep(2)
            check_CAN_2_ETH_Routing(args.run_time, args.gui_enabled, args.oob_mode)           
            
            initialize_counters()
            print("Script will now run Eth 2 Eth tests")
            zero_all_pipes(args.gui_enabled)
            time.sleep(2)
            check_ETH_2_ETH_Routing(args.run_time, args.gui_enabled, args.oob_mode)
            
            initialize_counters()
            print("Script will now run Eth 2 CAN tests")
            zero_all_pipes(args.gui_enabled)
            time.sleep(2)
            check_ETH_2_CAN_Routing(args.run_time, args.gui_enabled, args.oob_mode)
            
            print("Iteration #", i, "completed\n")
            
            # Leave this part as it is
            zero_all_pipes(args.gui_enabled)
            if(args.gui_enabled):
                print("Press Ctrl+C to exit\n")
            
