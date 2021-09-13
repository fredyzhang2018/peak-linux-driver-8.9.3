/*
*
* Copyright (c) 2019 Texas Instruments Incorporated
*
* All rights reserved not granted herein.
*
* Limited License.
*
* Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
* license under copyrights and patents it now or hereafter owns or controls to make,
* have made, use, import, offer to sell and sell ("Utilize") this software subject to the
* terms herein.  With respect to the foregoing patent license, such license is granted
* solely to the extent that any such patent is necessary to Utilize the software alone.
* The patent license shall not apply to any combinations which include this software,
* other than combinations with devices manufactured by or for TI ("TI Devices").
* No hardware patent is licensed hereunder.
*
* Redistributions must preserve existing copyright notices and reproduce this license
* (including the above copyright notice and the disclaimer and (if applicable) source
* code license limitations below) in the documentation and/or other materials provided
* with the distribution
*
* Redistribution and use in binary form, without modification, are permitted provided
* that the following conditions are met:
*
* *       No reverse engineering, decompilation, or disassembly of this software is
* permitted with respect to any software provided in binary form.
*
* *       any redistribution and use are licensed by TI for use only with TI Devices.
*
* *       Nothing shall obligate TI to provide you with source code for the software
* licensed and provided to you in object code.
*
* If software source code is provided to you, modification and redistribution of the
* source code are permitted provided that the following conditions are met:
*
* *       any redistribution and use of the source code, including any resulting derivative
* works, are licensed by TI for use only with TI Devices.
*
* *       any redistribution and use of any object code compiled from the source code
* and any resulting derivative works, are licensed by TI for use only with TI Devices.
*
* Neither the name of Texas Instruments Incorporated nor the names of its suppliers
*
* may be used to endorse or promote products derived from this software without
* specific prior written permission.
*
* DISCLAIMER.
*
* THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <netinet/ether.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <net/if.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define NTSCF_SUBTYPE                          0x82
/**< Sub type for NTSCF frame*/
#define NTSCF_SUBTYPE_INSERT_HASH_ENTRY        0x83
/**< Special sub-type used to add a Hash entry*/
#define NTSCF_SUBTYPE_INSERT_MAC_ENTRY         0x84
/**< Special sub-type used to add a Hash entry*/
#define NTSCF_SUBTYPE_TRIGGER_MCU1_0_MSG_SEND          0x85
/**< Special sub-type used to send a message to MCU 1_0*/
#define NTSCF_SUBTYPE_GET_MCU1_0_STATS         0x86
/**< Special sub-type used to get statistics from MCU 1_0*/
#define NTSCF_VERSION                          0U
/**< Version number for NTSCF control header*/
#define SV_BIT                                 0U
/**< Bit which tells if Stream ID is valid or not*/
#define DEFAULT_NTSCF_BIT                      0U
/**< Bit which tells if Stream ID is valid or not*/

#define ACF_MSG_TYPE_FLEXRAY                   1U
/**< ACF message type for Flexray*/
#define ACF_MSG_TYPE_CAN                       1U
/**< ACF message type for CAN*/
#define ACF_MSG_TYPE_CAN_BRIEF                 2U
/**< ACF message type for CAN (abbreviated)*/
#define ACF_MSG_TYPE_LIN                       3U
/**< ACF message type for LIN*/
#define DEFAULT_ACF_MSG_LENGTH_CAN             20U
/**< ACF message length (number of 32 bit words) for CAN*/
#define CAN_PAD_MASK                           3U
/**< Mask for 2 pad bits*/
#define CAN_SHIFT_VAL                          6U
/**< Shift val for CAN pad bits*/
#define MTV_BIT                                0U
/**< Message timestamp valid bit*/
#define MTV_SHIFT_VAL                          5U
/**< Shift val for message timestamp bits*/
#define RTR_BIT                                1U
/**< Remote transmission request bit*/
#define RTR_SHIFT_VAL                          4U
/**< Shift val for RTR bits*/
#define EFF_BIT                                1U
/**< Extended frame format bit*/
#define EFF_SHIFT_VAL                          3U
/**< Shift val for EFF bits*/
#define BRS_BIT                                1U
/**< Bit rate switch bit*/
#define BRS_SHIFT_VAL                          2U
/**< Shift val for BRS bit*/
#define FDF_BIT                                1U
/**< Flexible data rate bit*/
#define FDF_SHIFT_VAL                          1U
/**< Shift val for FDF bit*/
#define ESI_BIT                                0U
/**< Error state indicator bit*/
#define ESI_SHIFT_VAL                          0U
/**< Shift val for ESI bit*/
#define RSV_SHIFT_VAL                          5U
/**< Shift val for 3 reserved bits*/
#define DEFAULT_CAN_BUS_ID                     500U            
/**< Default CAN Bus ID*/
#define CAN_BUS_ID_MASK                        0x00
/**< Shift val for 3 reserved bits*/

#define NTSCF_SUBTYPE_OFFSET                   0U
/**< Offset for common control header subtype field*/    

const uint8_t bcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
/** \Broadcast MAC */

uint8_t gateway_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
/** \MAC address for gateway */

#define AVTP_ETH_TYPE           (0x22F0)
/** \Eth type for AVTP (1722) */

#define BUFFER_SIZE		1500
/** \Buffer size for Ethernet frame */

#define ROUTE_TO_MCAN4                        (0xD0)
/**< If CAN ID matches this then packet is sent only to MCAN4 from Eth*/
#define ROUTE_TO_MCAN9                        (0xB0)
/**< If CAN ID matches this then packet is sent only to MCAN9 from Eth*/
#define ROUTE_TO_BOTH_CAN                     (0xC0)
/**< If CAN ID matches this then packet is sent to both MCAN's from Eth*/
#define ROUTE_TO_ETH                          (0xF0)
/**< If CAN ID matches this then packet is sent to other Eth port*/

uint8_t unique_id[2] = {0xfe, 0xfe};
uint8_t sequence_id = 0;


uint8_t can_payload[64] = {0};

/*---Globals used by multiple functions---*/
char interface[IFNAMSIZ];
/**< Name of the interface*/
uint32_t num_packets = 0;
/**< Number of packets*/
uint32_t timeout = 0;
/**< Timeout in seconds*/
uint32_t ipg = 1000;
/**< Gap between two ethernet frames in microseconds. Default is 1 ms*/
struct timeval start_of_program;
/**< Start of the time*/
uint8_t verbosity = 0;
/**< Program level. Only 2 options supported right now. 0 and 1*/
uint8_t update_hash_table = 0;
/**< Tells tool to send command to update hash table*/
uint8_t canID = 0;
/*CAN ID which is embedded in the frame*/
uint16_t canMask = 0;
/*Bitmask for CAN ports*/
uint16_t ethMask = 0;
/*Bitmask for Eth ports*/
uint8_t  send_mac_address = 0;
/**< Tells tool to send mac address of ports to gateway*/
uint8_t  route_for_can_generator = 0;
/**<This tells the gateway to signal the CAN generator app*/
uint8_t  get_stats_from_can_generator = 0;
/**<This tells the gateway to signal the CAN generator app to provide statistics*/

/*Function to prepare 1722 frame*/
uint16_t format_1722_pkt(uint8_t *frame, uint8_t* sa);

/*Description : Used to get command line arguments for program
  Input Param : Command line arguments
  Return Val  : None
  */
void get_cmdline_args(int argc, char *argv[]);

void read_gateway_mac(uint8_t *gateway_mac);

int main(int argc, char *argv[])
{
    int sockfd;
    struct ifreq if_idx;
    struct ifreq if_mac;
    int tx_len = 0;
    char sendbuf[BUFFER_SIZE];
    struct ether_header *eh = (struct ether_header *) sendbuf;
    struct sockaddr_ll socket_address;

    uint16_t pkt_size = 0;
    /*Fix delay to 1000 us*/
    uint32_t pkt_counter = 0;

    /*Variable for detecting timeout*/
    uint8_t timeout_occured = 0;

    /*Current time*/
    struct timeval current_time;

    /*Temp variable*/
    float time_diff = 0;

    /*Num packets sent*/
    uint32_t num_packets_sent = 0;

    /*Get the start time*/
    gettimeofday (&start_of_program, NULL);

    /*Get the arguments*/
    get_cmdline_args(argc, argv);
    
    /*Get gateway mac from config file*/
    read_gateway_mac(gateway_mac);

    /* Open RAW socket to send on */
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("socket");
    }

    /*Increase the size to 1MB*/
    int buf_size = 2024 * 2024;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size)) == -1) {
        perror("Could not increase socket size");
	    close(sockfd);
	    exit(EXIT_FAILURE);
    }

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, interface, IFNAMSIZ-1);

	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("Unable to get interface");

	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, interface, IFNAMSIZ-1);

	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("Unable to get MAC address of interface");

	/* Build header */
	memset(sendbuf, 0, BUFFER_SIZE);
	/* Ethernet header */
    memcpy(eh->ether_shost, ((uint8_t *)&if_mac.ifr_hwaddr.sa_data), 6);
   	memcpy(eh->ether_dhost, gateway_mac, 6);

	/* Ethertype field */
	eh->ether_type = htons(AVTP_ETH_TYPE);
	tx_len += sizeof(struct ether_header);

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
    memcpy(socket_address.sll_addr, gateway_mac, 6);

    if(num_packets)
    {
        for(pkt_counter = 0; pkt_counter < num_packets; pkt_counter++)
        {
            usleep(ipg);
          
            /* Prepare Packet data which is randomized every time */
            pkt_size = format_1722_pkt((uint8_t*)sendbuf, eh->ether_shost);

            /* Send packet */
            if (sendto(sockfd, sendbuf, pkt_size, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
            {
                printf("Send failed for packet\n");
            }
            else
            {
                num_packets_sent++;
                if(verbosity)
                    printf("Sent packet number %d\n", pkt_counter);
            }

            if(timeout)
            {
                /*Check for timeout*/
                gettimeofday (&current_time, NULL);
                time_diff = current_time.tv_sec - start_of_program.tv_sec;
                
                if(time_diff > timeout)
                {
                    break;
                }
            }
                                      
        }        
    }
    else
    {
        while(!timeout_occured)
        {

            usleep(ipg);
       
            /* Prepare Packet data which is randomized every time */
            pkt_size = format_1722_pkt((uint8_t*)sendbuf, eh->ether_shost);

            /* Send packet */
            if (sendto(sockfd, sendbuf, pkt_size, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
            {
                printf("Send failed for packet\n");
            }
            else
            {
                num_packets_sent++;
                if(verbosity)
                    printf("Sent packet number %d\n", pkt_counter);
            }

            pkt_counter++; 

            /*Should we exit*/
            gettimeofday (&current_time, NULL);
            time_diff = current_time.tv_sec - start_of_program.tv_sec;
            if(time_diff > timeout)
            {
                timeout_occured = 1;
            }
        }
        
    }

    close(sockfd);
    printf("sent frames : %d\n", num_packets_sent);
  	
	return 0;
}

uint16_t format_1722_pkt(uint8_t* frame, uint8_t* sa)
{
    uint8_t temp_byte = 0;
    /*Variable to keep track of current byte. Starts after packet type*/
    uint16_t curr_byte_offset = 14;
    /*Size of the ACF data. Only 11 bits are used*/
    uint16_t ntscf_data_length = DEFAULT_ACF_MSG_LENGTH_CAN * 4, temp_halfWord = 0;
    uint8_t can_pad_val = 0;
    uint8_t can_payload_counter = 0;

    FILE *app_config_handle;
    char *line = NULL;
    size_t length = 0;
    uint8_t num_lines = 0, count = 0, mac_id_val = 0;
    char* token_1;
    char* token_2;
    uint8_t byte_offset = 0;

    *(frame + curr_byte_offset) = NTSCF_SUBTYPE;

    /*Based on special requests we will overwrite the packet*/

    /*Prepare 1722 message for a 64B CAN frame*/
    if(update_hash_table)
    {
        /*We tell gateway to update the hash table*/
    	*(frame + curr_byte_offset) = NTSCF_SUBTYPE_INSERT_HASH_ENTRY;
    }

    if(send_mac_address)
    {
        /*We tell gateway to update the MAC table*/
    	*(frame + curr_byte_offset) = NTSCF_SUBTYPE_INSERT_MAC_ENTRY;
    }

    if(route_for_can_generator != 0)
    {
        /*We tell gateway to update the MAC table*/
    	*(frame + curr_byte_offset) = NTSCF_SUBTYPE_TRIGGER_MCU1_0_MSG_SEND;
    }

    if(get_stats_from_can_generator != 0)
    {
        /*We tell gateway to update the MAC table*/
    	*(frame + curr_byte_offset) = NTSCF_SUBTYPE_GET_MCU1_0_STATS;
    }

    /*Increment offset counter*/
    curr_byte_offset++;

    temp_byte = 0;
    temp_byte |= ((uint8_t)SV_BIT << 7) | (((uint8_t)NTSCF_VERSION & 7) << 4) | (uint8_t)((ntscf_data_length & 0x100) >> 8);

    /*Second byte is a composite value*/
    *(frame + curr_byte_offset++) = temp_byte;

    /*Write third byte which is lower 8 bits of payload length*/
    temp_byte = ntscf_data_length & 0xff;    
    *(frame + curr_byte_offset++) = temp_byte;

    /*fourth byte is sequence num*/
    *(frame + curr_byte_offset++) = sequence_id++;

    /*Next 8 bytes are stream ID*/
    memcpy(frame + curr_byte_offset, sa, 6);
    memcpy(frame + curr_byte_offset + 6, unique_id, 2);
    curr_byte_offset += 8;
    
    /*Now prepare the ACF message payload*/
    temp_byte = 0;
    temp_byte |= (((uint8_t)ACF_MSG_TYPE_CAN & 0x7f) << 1);
    temp_byte |= (uint8_t)(((uint16_t)DEFAULT_ACF_MSG_LENGTH_CAN & 0x100) >> 8);
    *(frame + curr_byte_offset++) = temp_byte;

    /* Next byte is all ACF message length*/
    temp_byte = DEFAULT_ACF_MSG_LENGTH_CAN & 0xff;
    *(frame + curr_byte_offset++) = temp_byte;

    /*
        Next byte is information about the CAN message type. Presently the settings are hardcoded
        padding  : 0
        Message timestamp valid : False
        Remote transmission request : False
        Extended frame format : True
        Bit rate switching : True
        Flexible data format : True
        Error status : False
    */
    temp_byte = ((can_pad_val & CAN_PAD_MASK) << CAN_SHIFT_VAL) | (MTV_BIT << MTV_SHIFT_VAL) \
    | (RTR_BIT << RTR_SHIFT_VAL) | (EFF_BIT << EFF_SHIFT_VAL) | (BRS_BIT << BRS_SHIFT_VAL) \
    | (FDF_BIT << FDF_SHIFT_VAL) | (ESI_BIT << ESI_SHIFT_VAL);
    *(frame + curr_byte_offset++) = temp_byte;

    /*Next byte is the default CAN BUS ID and reserved bits*/
    temp_byte = 0;
    temp_byte |= ((uint8_t)DEFAULT_CAN_BUS_ID & CAN_BUS_ID_MASK);
    *(frame + curr_byte_offset++) = temp_byte;

    /*Next 8 bytes of message timestamp is zero*/
    memset(frame + curr_byte_offset, 0x0, 8);
    curr_byte_offset += 8;

    /*Next byte is 3 bits of reserved and 5 MSB of CAN ID*/
    /*Fixme*/
    temp_byte = canID;
    *(frame + curr_byte_offset++) = temp_byte;

    temp_byte = 0x0;
    *(frame + curr_byte_offset++) = temp_byte;

    temp_byte = 0x0;
    *(frame + curr_byte_offset++) = temp_byte;

    temp_byte = 0x0;
    *(frame + curr_byte_offset++) = temp_byte;

    /*Generate random payload*/
    for(can_payload_counter = 0; can_payload_counter < 64; can_payload_counter++)
    {
        can_payload[can_payload_counter] = rand() % 255;
    }

    /*Copy CAN payload*/
    memcpy(frame + curr_byte_offset, can_payload, 64);
    //The first byte contains special indicators so use 0 instead of random value
    *(frame + curr_byte_offset) = 0x00;

    if(update_hash_table)
    {
        temp_halfWord = ntohs(canMask);
        memcpy(frame + curr_byte_offset, &temp_halfWord, 2);

        temp_halfWord = ntohs(ethMask);
        memcpy(frame + curr_byte_offset + 2, &temp_halfWord, 2);
    }

    if(send_mac_address)
    {
        app_config_handle = fopen("./pctools/app_config.txt", "r");
        if(app_config_handle == NULL)
        {
            perror("Unable to open app_config.txt\n");
            exit(0);
        }
        while(getline(&line, &length, app_config_handle) != -1)
        {
            if((num_lines < 2) || (num_lines > 3)) {
                num_lines++;
                continue;
            }
            else
            {
                token_1 = strtok(line, "-"); 
                while (token_1 != NULL) 
                {
                    if(count++ == 1)
                    {
                        token_2 = strtok(token_1, ":");
                        while (token_2 != NULL) 
                        {
                            mac_id_val = strtol(token_2, NULL, 16);
                            *(frame + curr_byte_offset + byte_offset++) = mac_id_val;
                            token_2 = strtok(NULL, ":");
                        }
                    }                
                    token_1 = strtok(NULL, "-"); 
                }
                num_lines++;
                count = 0;
            }
             
        }
        fclose(app_config_handle);
    }

    if(route_for_can_generator != 0)
    {
        /*Embed route and number of seconds to signal MCU1_0 app*/
        *(frame + curr_byte_offset) = route_for_can_generator;
        *(frame + curr_byte_offset + 1) = timeout;
    }

    /*increment offset*/
    curr_byte_offset += 64;

    return curr_byte_offset;
}

void get_cmdline_args(int argc, char *argv[])
{
    uint8_t eth_interface_provided = 0, timeout_provided = 0, num_packets_provided = 0, \
            eth_mask_provided = 0, can_mask_provided = 0, \
            update_hash_table_provided = 0;
    int i = 0;

    for (i = 0; i < argc; ++i)
    {
        /*parse argv, until we get the --option which indicates an option*/
        if(strncmp(argv[i], "--", 2) == 0)
        {
            if(strcmp(argv[i], "--eth_interface") == 0)
            {
                strcpy(interface, argv[i + 1]);
                printf("Name of Eth Interface : %s\n", interface);
                eth_interface_provided = 1;
            }
            else if(strcmp(argv[i], "--verbosity") == 0)
            {
                if(strcmp(argv[i + 1], "verbose") == 0)
                {
                    verbosity = 1;
                }
                else if(strcmp(argv[i + 1], "non-verbose") == 0)
                {
                    verbosity = 0;
                }
                else
                {
                    printf("Unknown option. Verbosity is non-verbose\n");
                }
            }
            else if(strcmp(argv[i], "--route") == 0)
            {                
                if(strcmp(argv[i + 1], "MCAN4") == 0)
                {
                    canID = ROUTE_TO_MCAN4;
                }
                else if(strcmp(argv[i + 1], "MCAN9") == 0)
                {
                    canID = ROUTE_TO_MCAN9;
                }
                else if(strcmp(argv[i + 1], "BOTH") == 0)
                {
                    canID = ROUTE_TO_BOTH_CAN;
                }
                else if(strcmp(argv[i + 1], "ETH") == 0)
                {
                    canID = ROUTE_TO_ETH;
                }
                else
                {
                    printf("Unsupported mode\n");
                    printf("\nOptions for mode are 'MCAN4', 'MCAN9', 'BOTH' and 'ETH' \n");
                    printf("Try with --help to understand\n");
                    exit(0);
                }
            }
            else if(strcmp(argv[i], "--num_packets") == 0)
            {
                num_packets = atoi(argv[i + 1]);
                num_packets_provided = 1;
            }
            else if(strcmp(argv[i], "--can_id") == 0)
            {
                canID = strtol(argv[i + 1], NULL, 16);
            }
            else if(strcmp(argv[i], "--eth_mask") == 0)
            {
                ethMask = strtol(argv[i + 1], NULL, 16);
                eth_mask_provided = 1;
            }
            else if(strcmp(argv[i], "--can_mask") == 0)
            {
                canMask = strtol(argv[i + 1], NULL, 16);
                can_mask_provided = 1;
            }
            else if(strcmp(argv[i], "--update_hash_table") == 0)
            {
                update_hash_table = atoi(argv[i + 1]);
                update_hash_table_provided = 1;
                num_packets = 1;
                num_packets_provided = 1;
            }
            else if(strcmp(argv[i], "--send_mac_address") == 0)
            {
                send_mac_address = atoi(argv[i + 1]);
		canID = ROUTE_TO_MCAN4;
                num_packets = 1;
                num_packets_provided = 1;
            }
            else if(strcmp(argv[i], "--can_generator_route") == 0)
            {
                route_for_can_generator = atoi(argv[i + 1]);
                num_packets = 1;
                num_packets_provided = 1;
            }
            else if(strcmp(argv[i], "--get_can_generator_stats") == 0)
            {
                get_stats_from_can_generator = atoi(argv[i + 1]);
                num_packets = 1;
                num_packets_provided = 1;
            }
            else if(strcmp(argv[i], "--timeout") == 0)
            {
                timeout = atoi(argv[i + 1]);
                timeout_provided = 1;
            }
            else if(strcmp(argv[i], "--ipg") == 0)
            {
                ipg = atoi(argv[i + 1]);
            }
            else if(strcmp(argv[i], "--help") == 0)
            {
                printf("Example usage : sudo send_1722.out --eth_interface eth0 --timeout x --num_packets y\n\n");
                printf("Here are the options for the program\n");
                printf("--eth_interface\t\t\t\t: Name of the ethernet interface on which to send. See ifconfig output. This must be provided\n");
                printf("--update_hash_table (optional)\t\t: Used to update hash table entry in Gateway. Requires --can_id, --eth_mask, --can_mask options\n");
                printf("--can_id\t\t\t : CAN ID of the embedded CAN frame. This must be provided if setting hash table entry.\n");
                printf("--eth_mask\t\t\t : Bit mask for ethernet ports on Gateway. This must be provided if setting hash table entry.\n");
                printf("--can_mask\t\t\t : Bit mask for ethernet ports on Gateway. This must be provided if setting hash table entry.\n");
                printf("--send_mac_address (optional)\t\t: Used to send Eth MAC address to gateway device. Requires app_config.txt to be present\n");
                printf("--can_generator_route (optional)\t\t: Used to signal CAN generator App to send CAN frames. Valid values are 1 : CAN 2 CAN, 2: CAN 2 Eth, 3: Eth 2 CAN\n");
                printf("--get_can_generator_stats (optional)\t\t: Used to signal CAN generator App to send routing stats. Valid values are 0/1\n");
                printf("--verbosity (optional)\t\t\t: Control log level. Options are 'verbose' and 'non-verbose' (silent). Default is non-verbose\n");
                printf("--timeout \t\t\t\t: Time in seconds, after which Transmission will end.\n");
                printf("--num_packets \t\t\t\t: Number of packets to send, either this or --timeout must be provided.\n");
                printf("--ipg (optional) \t\t\t: Gap between two ethernet frames in microseconds. Default value is 1000\n");
                exit(0);
            }
        }
    }

    if(!eth_interface_provided)
    {
        printf("\n\nYou did not provide an ethernet interface name. Program cannot run. Try running with --help. Exiting\n");
        exit(0);
    }

    /*At least one must be provided*/
    if ((timeout_provided + num_packets_provided) == 0)
    {
        printf("Neither timeout nor number of packets provided. At least one of the two is a must.\n");
        printf("Try with --help for more options\n");
        exit(0);
    }

    if(canID == 0)
    {
        printf("Did not provide CAN ID, it's a must\n");
        printf("Try with --help for more options\n");
        exit(0);
    }

    if(update_hash_table_provided)
    {
        if(!can_mask_provided || !eth_mask_provided)
        {
            printf("You provided option to update hash table but did not provide the masks\n");
            exit(0);
        }
    }   
        
}

void read_gateway_mac(uint8_t *gateway_mac)
{
    FILE *app_config_handle;
    char *line = NULL;
    size_t length = 0;
    uint8_t num_lines = 0, count = 0, mac_id_val = 0;
    char* token_1;
    char* token_2;
    uint8_t byte_offset = 0;

    app_config_handle = fopen("./pctools/app_config.txt", "r");
    if(app_config_handle == NULL)
    {
        perror("Unable to open app_config.txt\n");
        exit(0);
    }
    while(getline(&line, &length, app_config_handle) != -1)
    {
        if(num_lines != 4) {
            num_lines++;
            continue;
        }
        else
        {
            token_1 = strtok(line, "-"); 
            while (token_1 != NULL) 
            {
                if(count++ == 1)
                {
                    token_2 = strtok(token_1, ":");
                    while (token_2 != NULL) 
                    {
                        mac_id_val = strtol(token_2, NULL, 16);
                        *(gateway_mac + byte_offset++) = mac_id_val;
                        token_2 = strtok(NULL, ":");
                    }
                }                
                token_1 = strtok(NULL, "-"); 
            }
            num_lines++;
            count = 0;
        }
            
    }
    fclose(app_config_handle);
}
