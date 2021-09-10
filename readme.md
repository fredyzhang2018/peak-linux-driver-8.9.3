/home/a0224068local/1Twork/psdkra/psdk_rtos_auto_j7_06_02_00_21/gateway-demos/binary/can_eth_gateway_app/bin/j721e_evm/can_eth_gateway_app_mcu2_1_release.xer5f

/home/a0224068local/1Twork/psdkra/psdk_rtos_auto_j7_06_02_00_21/ethfw/out/J721E/R5F/SYSBIOS/release/app_remoteswitchcfg_server.xer5f

gateway - 70:ff:76:1d:9f:4f

enx00e04c34ae1c 00:e0:4c:34:ae:1c

eno1	ec:f4:bb:15:36:a4

~/Desktop/rui/peak-linux-driver-8.9.3/test/pctools/pctools/app_config.txt

sudo ./recv_1722.out --eth_interface eno1 --timeout 12 --pipes 0 --verbosity verbose

sudo ./send_1722.out --eth_interface eno1 --gateway_mac 70:ff:76:1d:9f:4f --dst_mac ec:f4:bb:15:36:a4 --ipg 1000 --route ETH --num_packets 10


./pcanfdtst tx ETH_ONLY  --bitrate 1000000 --dbitrate 5000000 --clock 80000000 -n 1 -ie 0xD0 --fd -l 64 --tx-pause-us 1000 /dev/pcanusbfd32

1. CCS load server and canapp image to MAINR5F_0 and MAINR5F_1, MAINR5F_0 run first.
2. modify ~/Desktop/rui/peak-linux-driver-8.9.3/test/pctools/app_config.txt
3. run script 
a0224068local@uda0224068:~/Desktop/rui/peak-linux-driver-8.9.3/test$ python3 pctools/run_demo.py --iterations 1 --run_time 5 --system_test 0 --gui_enabled 0

3. run CAN2ETH
a0224068local@uda0224068:~/Desktop/rui/peak-linux-driver-8.9.3/test$ ./pcanfdtst tx ETH_ONLY  --bitrate 1000000 --dbitrate 5000000 --clock 80000000 -n 1 -ie 0xD0 --fd -l 64 --tx-pause-us 1000 /dev/pcanusbfd32
a0224068local@uda0224068:~/Desktop/rui/peak-linux-driver-8.9.3/test/pctools$ sudo ./recv_1722.out --eth_interface eno1 --timeout 12 --pipes 0 --verbosity verbose

3. run ETH2ETH
a0224068local@uda0224068:~/Desktop/rui/peak-linux-driver-8.9.3/test/pctools$ sudo ./send_1722.out --eth_interface eno1 --gateway_mac 70:ff:76:1d:9f:4f --dst_mac ec:f4:bb:15:36:a4 --ipg 1000 --route ETH --num_packets 10
a0224068local@uda0224068:~/Desktop/rui/peak-linux-driver-8.9.3/test/pctools$ sudo ./recv_1722.out --eth_interface eno1 --timeout 12 --pipes 0 --verbosity verbose




main r5f_0 log
=======================================================


           CPSW Ethernet Firmware Demo             


=======================================================


ETHFW Version: 0. 1. 1


ETHFW Build Date (YYYY/MMM/DD):2020/Feb/17


ETHFW Commit SHA:ETHFW PermissionFlag:0x1ffffff, UART Connected:true,UART Id:2IPC_echo_test (core : mcu2_0) .....


CPSW_9G Test on MAIN NAVSS


Remote demo device (core : mcu2_0) .....


CpswPhy_bindDriver: PHY 12: OUI:080028 Model:23 Ver:01 <-> 'dp83867' : OK


CpswPhy_bindDriver: PHY 0: OUI:080028 Model:23 Ver:01 <-> 'dp83867' : OK


CpswPhy_bindDriver: PHY 3: OUI:080028 Model:23 Ver:01 <-> 'dp83867' : OK


CpswPhy_bindDriver: PHY 15: OUI:080028 Model:23 Ver:01 <-> 'dp83867' : OK


PHY 0 is alive


PHY 3 is alive


PHY 12 is alive


PHY 15 is alive


PHY 16 is alive


PHY 17 is alive


PHY 18 is alive


PHY 19 is alive


PHY 23 is alive


Host MAC address: 70:ff:76:1d:9f:4e


[NIMU_NDK] CPSW has been started successfully


Function:CpswProxyServer_attachExtHandlerCb,HostId:4,CpswType:1


Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cee3f4,CoreKey:38acb976, Cmd:20000,InArgsLen:24, OutArgsLen:4 


Function:CpswProxyServer_registerMacHandlerCb,HostId:4,Handle:a2cee3f4,CoreKey:38acb976, MacAddress:70:ff:76:1d:9f:4f, FlowIdx:178, FlowIdxOffset:6


Cpsw_ioctlInternal: CPSW: Registered MAC address.ALE entry:10, Policer Entry:0Cpsw_handleLinkUp: port 3: Link up: 1-Gbps Full-Duplex


Cpsw_handleLinkUp: port 2: Link up: 1-Gbps Full-Duplex


Function:CpswProxyServer_clientNotifyHandlerCb,HostId:4,Handle:38acb976,CoreKey:a2cee3f4,NotifyId:RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_CUSTOM,NotifyLen


main r5f_1 log:
[MAIN_Cortex_R5_0_1] 
Gateway App:Variant - Pre Compile being used !!!
CAN_APP: Successfully Enabled CAN Transceiver Main Domain Inst 4,9,11!!!
 
Gateway App: CDD IPC MCAL Version Info
Gateway App:---------------------
Gateway App: Vendor ID           : 44
Gateway App: Module ID           : 255
Gateway App: Starting CAN Rx
Gateway App: Starting CAN Rx Interface
Gateway App: SW Major Version    : 1
Gateway App: SW Minor Version    : 0
Gateway App: SW Patch Version    : 0
 
Gateway App:MAC Port 1 Address: 70:ff:76:1d:9f:4f
Gateway App: Starting Eth Rx Interface
Gateway App: Starting Gateway Router



