# Gateway Demo Setup
## guide
<https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-j7200/07_03_00_07/exports/docs/gateway-demos/docs/Can-Eth-User_Guide.html>
<https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-jacinto7/08_00_00_12/exports/docs/gateway-demos/docs/Can-Eth-User_Guide.html>

## build the demo 
1. download the startJacinto tools. 
2. run `source env*` 
3. install the PSDKRA. 
4. run `gateway-build`
5. image located in the below 
>  sdk/gateway-demos/binary/can_eth_gateway_app/bin/j721e_evm/can_eth_gateway_app_mcu2_1_release.xer5f
>  sdk/ethfw/out/J721E/R5F/SYSBIOS/release/app_remoteswitchcfg_server.xer5f

## pcan tools install
1. run `make ubuntu-install-pcan-tools`
2. run `make gateway-install-pc-tools`
3. modify the configuration file.(~/startJacinto/tools/peak-linux-driver-8.9.3/test/pctools/app_config.txt)
```
#This is a sample App config. You can use this as a template to create your own configuration.

#The PCAN connection below is for MCAN4. Run lspcan command with the PCAN-USB tool connected, to find out the name of the connection
/dev/pcanusbfd32 - 0xD0
#The PCAN connection below is for MCAN9
/dev/pcanusbfd33 - 0xE0
#This is the primary Ethernet interface. It can be anything but recommendation is to use the built-in Ethernet adapter
enxd037455f8444 - d0:37:45:5f:84:44
#This is the secondary Ethernet interface.
enp1s2 - 00:ad:24:8f:f6:4e
#This is the MAC address for Gateway. You can get this from the CCS or UART logs.
gateway - 70:ff:76:1d:88:32

# Remove all comments starting with # before using this file
```
## Run the demo
### CCS Boot Eemo
1. CCS load server and canapp image to MAINR5F_0 and MAINR5F_1, MAINR5F_0 run first.

#### Run CAN2ETH demo
```
peak-linux-driver-8.9.3/test$ ./pcanfdtst tx ETH_ONLY  --bitrate 1000000 --dbitrate 5000000 --clock 80000000 -n 1 -ie 0xD0 --fd -l 64 --tx-pause-us 1000 /dev/pcanusbfd32
peak-linux-driver-8.9.3/test/pctools$ sudo ./recv_1722.out --eth_interface eno1 --timeout 12 --pipes 0 --verbosity verbose
```
Run the demo that based startJacinto
```
make ...
```
#### Run ETH2ETH demo
```
peak-linux-driver-8.9.3/test/pctools$ sudo ./send_1722.out --eth_interface eno1 --gateway_mac 70:ff:76:1d:9f:4f --dst_mac ec:f4:bb:15:36:a4 --ipg 1000 --route ETH --num_packets 10
peak-linux-driver-8.9.3/test/pctools$ sudo ./recv_1722.out --eth_interface eno1 --timeout 12 --pipes 0 --verbosity verbose
```
Run the demo that based startJacinto
```
make ...
```

2. run below 
sudo ./recv_1722.out --eth_interface eno1 --timeout 12 --pipes 0 --verbosity verbose
sudo ./send_1722.out --eth_interface eno1 --gateway_mac 70:ff:76:1d:9f:4f --dst_mac ec:f4:bb:15:36:a4 --ipg 1000 --route ETH --num_packets 10
./pcanfdtst tx ETH_ONLY  --bitrate 1000000 --dbitrate 5000000 --clock 80000000 -n 1 -ie 0xD0 --fd -l 64 --tx-pause-us 1000 /dev/pcanusbfd32
./peak-linux-driver-8.9.3/test$ python3 pctools/run_demo.py --iterations 1 --run_time 5 --system_test 0 --gui_enabled 0




3. run ETH2ETH


# LOG 
## MCU_log  r5f_0

```
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
```
## MCU_log  r5f_1
```
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
```


