# This script takes two binaries, viz. CAN-Eth gateway binary and Remote Switch server binary and combines
# them into one bootable image for OSPI booting. This uses PDK tools that come with SDK installer
rm ./app_switch2.appimage
cp ../binary/can_eth_gateway_app_tirtos/bin/j7200/can_eth_gateway_app_tirtos_mcu2_1_release.xer5f ./
cp ../../ethfw/out/J7200/R5Ft/SYSBIOS/release/app_remoteswitchcfg_server_ccs.xer5f ./
cp ../binary/can_traffic_generator_app_tirtos/bin/j7200/can_traffic_generator_app_tirtos_mcu1_0_release.xer5f ./
../../ti-cgt-arm_20.2.0.LTS/bin/armstrip app_remoteswitchcfg_server_ccs.xer5f
../../ti-cgt-arm_20.2.0.LTS/bin/armstrip can_eth_gateway_app_tirtos_mcu2_1_release.xer5f
../../ti-cgt-arm_20.2.0.LTS/bin/armstrip can_traffic_generator_app_tirtos_mcu1_0_release.xer5f
mono ../../pdk/packages/ti/boot/sbl/tools/out2rprc/bin/out2rprc.exe can_eth_gateway_app_mcu2_1_tirtos_release.xer5f can_eth_gateway.rprc
mono ../../pdk/packages/ti/boot/sbl/tools/out2rprc/bin/out2rprc.exe app_remoteswitchcfg_server_ccs.xer5f app_switch_server.rprc
mono ../../pdk/packages/ti/boot/sbl/tools/out2rprc/bin/out2rprc.exe can_traffic_generator_app_tirtos_mcu1_0_release.xer5f can_generator.rprc
../../pdk/packages/ti/boot/sbl/tools/multicoreImageGen/bin/MulticoreImageGen LE 55 app_switch2.appimage 4 can_generator.rprc 6 app_switch_server.rprc 7 can_eth_gateway.rprc
rm *.rprc
rm ./can_eth_gateway_app_tirtos_mcu2_1_release.xer5f
rm ./app_remoteswitchcfg_server_ccs.xer5f
rm ./can_traffic_generator_app_tirtos_mcu1_0_release.xer5f
