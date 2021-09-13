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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <net/if.h>
#include <netinet/ether.h>
#include <sys/time.h>

#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define NTSCF_SUBTYPE                          0x82
/**< Sub type for NTSCF frame*/
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

//#define DUMP_CPU_LOAD_TO_FILE  
/**< Used for checking persistent CPU load*/

const uint8_t bcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*---Globals used by multiple functions---*/

uint8_t exit_loop = 0;
/**< Used to catch a keyboard exception and exit*/
uint8_t detect_mac = 0;
/**< If set, program exits early after detecting the SRC MAC*/
uint8_t verbosity = 0;
/**< Program level. Only 2 options supported right now. 0 and 1*/
char interface[IFNAMSIZ];
/**< Name of ethernet interface on which to receive*/
uint32_t timeout_value = 10;
/**< Timeout for the socket. Value in seconds*/

#define AVTP_ETH_TYPE           (0x22F0)

/*This is used to identify a wakeup frame*/
#define DIAGNOSTIC_FRAME_TYPE    (0xB0)

/*Used to identify a CAN frame that was routed to Ethernet*/
#define CAN_2_ETH_FRAME_TYPE     (0xC0)

/*Used to identify an Eth frame that was routed to another ethernet port*/
#define ETH_2_ETH_FRAME_TYPE     (0xD0)

/*This is used to identify a statistics frame containing information from CAN generator*/
#define STAT_FRAME_TYPE          (0xE0)

#define BUFFER_SIZE		1500

/*Scaling factors for GUI*/
#define LATENCY_SCALING_FACTOR      200    
#define BW_SCALING_FACTOR           5000
#define CPU_LOAD_SCALING_FACTOR     (4 * 100)

/*Variables to track CAN 2 Eth delay*/
uint32_t min_can_2_eth_bridge_delay = 0xffffffff;
uint32_t max_can_2_eth_bridge_delay = 0;
uint32_t avg_can_2_eth_bridge_delay = 0;

/*Variables to track Eth 2 Eth delay*/
uint32_t min_eth_2_eth_bridge_delay = 0xffffffff;
uint32_t max_eth_2_eth_bridge_delay = 0;
uint32_t avg_eth_2_eth_bridge_delay = 0;

/*Variables to track CAN 2 CAN metrics for OOB demo*/
uint32_t max_can_2_can_bridge_delay = 0;
uint32_t avg_can_2_can_bridge_delay = 0;
uint32_t num_can_frames_sent = 0;
uint32_t num_can_frames_rcvd = 0;

/*Variables to track Eth 2 CAN metrics for OOB demo*/
uint32_t max_eth_2_can_bridge_delay = 0;
uint32_t avg_eth_2_can_bridge_delay = 0;
uint32_t num_eth_2_can_frames_rcvd = 0;

float average_swi_load = 0;
float average_isr_load = 0;

uint8_t enable_pipes = 1;     //by default pipes are enabled

/*Named FIFO descriptors*/
int fd_can_2_eth_latency = 0;  //For CAN 2 Eth latency values
int fd_can_2_eth_bw = 0;       //For CAN 2 Eth Bandwidth

int fd_eth_2_eth_latency = 0;  //For Eth 2 Eth latency values
int fd_eth_2_eth_bw = 0;       //For Eth 2 Eth Bandwidth

int fd_gateway_load = 0;        //For CPU load

int zero_cpu_load_watchdog = 0; //Watchdog to check for zero CPU load

float sum_of_cpu_load = 0;
float num_cpu_load_msg = 0;
float average_cpu_load = 0;

float sum_of_isr_load = 0;
float sum_of_swi_load = 0;

/*
  * Description : Structure containing 1722 frame parameters
  */
typedef struct fields_1722_s
{
    /*NTSCF header related params*/
    uint8_t ntscf_version;
    /**< Version*/
    uint8_t stream_valid;
    /**< If stream ID is valid*/
    uint16_t ntscf_data_length;
    /**< Length of NTSCF packet*/
    uint8_t sequence_id;
    /**< Seuqnece ID*/
    uint8_t streamID[8];
    /**< Embedded stream ID*/

    /*Embedded CAN frame values*/
    uint16_t can_msg_len;
    /**< Length of CAN message*/
    uint8_t can_pad;
    /**< Number of pad bytes*/
    uint8_t msg_ts_valid;
    /**< Message timestamp valid bit*/
    uint8_t remote_tx_req;
    /**< Remote transmission request bit*/
    uint8_t ext_frame_format;
    /**< Extended frame format bit*/
    uint8_t bit_rate_switch;
    /**< Bit rate switch bit*/
    uint8_t flex_data_rate;
    /**< Flexible data rate bit*/
    uint8_t error_state_indicator;
    /**< Error state indicator bit*/
    uint8_t can_bus_id;            
    /**< Default CAN Bus ID*/
    uint8_t can_ts[8];
    /**< CAN Timestamp*/
    uint32_t can_ID;
    /**< CAN Timestamp*/
    uint8_t can_msg[64];
    /**<CAN message*/

} fields_1722;

/*Description : Parse a 1722 frame and find out individual parameters
  Input Param : 1722 frame buffer
  Input Param : Size of packet received
  Input Param : Verbosity
  Return Val  : Based on packet type
  */
uint8_t parse_1722_frame(uint8_t *frame, uint16_t packet_size, uint8_t verbosity);

/*Description : Format and print CAN message
  Input Param : Structure containing 1722 frame params
  Return Val  : None
  */
void print_can_msg(fields_1722 *frame);

/*Description : Used to catch keyboard exceptions
  Input Param : None
  Return Val  : None
  */
void exception_handler(void);

/*Description : Used to get command line arguments for program
  Input Param : Command line arguments
  Return Val  : None
  */
void get_cmdline_args(int argc, char *argv[]);

#ifdef DUMP_CPU_LOAD_TO_FILE
    FILE *filePointer;
#endif //DUMP_CPU_LOAD_TO_FILE

/*Main function*/
int main(int argc, char *argv[])
{
    ssize_t number_of_bytes;

    int sockfd, sockopt;
	
    uint32_t num_packets_received = 0, packet_count = 0;

    uint8_t pkt_buf[BUFFER_SIZE];

    uint8_t ret_val;

    /*To catch keyboard termination*/
    struct sigaction sig_int_handler;

    struct timeval tv;

    struct timeval start_time;
    struct timeval stop_time;

    double total_time;

    get_cmdline_args(argc, argv);    
#ifdef DUMP_CPU_LOAD_TO_FILE
    filePointer = fopen("cpu_load.txt", "w");
    if (filePointer == NULL)
    {
        perror("Unable to open file");	
	    return -1;
    }
#endif //DUMP_CPU_LOAD_TO_FILE

    /* Open a raw socket and configure for AVTP_ETH_TYPE */
    if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(AVTP_ETH_TYPE))) == -1) {
            perror("listener: socket");	
	    return -1;
    }

    /* Make the socket reusable */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
	    perror("Could not make the socket reusable");
	    close(sockfd);
	    exit(EXIT_FAILURE);
    }
    /* The device needs to be binded*/
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, interface, IFNAMSIZ-1) == -1)	{
	    perror("Could not bind to socket");
	    close(sockfd);
	    exit(EXIT_FAILURE);
    }

    /*Increase the size to 1MB*/
    int buf_size = 1024 * 1024;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size)) == -1) {
        perror("Could not increase socket size");
	    close(sockfd);
	    exit(EXIT_FAILURE);
    }

    /*Set a timeout*/
    tv.tv_sec = timeout_value;
    tv.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) == -1) {
        perror("Unable to set timeout");
		close(sockfd);
		exit(EXIT_FAILURE);
    }

    sig_int_handler.sa_handler = (__sighandler_t)exception_handler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;

    sigaction(SIGINT, &sig_int_handler, NULL);

    if(enable_pipes && !detect_mac)
    {
        if((fd_can_2_eth_latency = open("/tmp/c2e-latency-pipe", O_WRONLY)) < 0)
        {
            printf("Could not open can 2 eth latency FIFO \n");
        }  

        if((fd_can_2_eth_bw = open("/tmp/c2e-bw-pipe", O_WRONLY)) < 0)
        {
            printf("Could not open can 2 eth BW FIFO \n");
        }

        if((fd_eth_2_eth_latency = open("/tmp/eth-latency-pipe", O_WRONLY)) < 0)
        {
            printf("Could not open eth 2 eth latency FIFO \n");
        }  

        if((fd_eth_2_eth_bw = open("/tmp/eth-bw-pipe", O_WRONLY)) < 0)
        {
            printf("Could not open eth 2 eth BW FIFO \n");
        }

        if((fd_gateway_load = open("/tmp/r51-load-pipe", O_WRONLY)) < 0)
        {
            printf("Could not open CPU load pipe \n");
        }   
    }

    /*Infinite loop*/
    while(1)
    {
        if(exit_loop)
        {
            break;
        }
        if(verbosity)
        {
            printf("Waiting for AVTP-1722 type packet\n");
        }
        number_of_bytes = recvfrom(sockfd, pkt_buf, BUFFER_SIZE, 0, NULL, NULL);

        if(number_of_bytes == -1)
        {
            printf("Receive operation timed out\n");
            break;
        }

        num_packets_received++;
        if(packet_count++ == 1)
        {
            gettimeofday (&start_time, NULL);
        }

        gettimeofday (&stop_time, NULL);

        if(verbosity)
        {
            printf("Received %d packets\n", num_packets_received);
        }

        ret_val = parse_1722_frame(pkt_buf, number_of_bytes, verbosity);

        /*Check watchdog*/
        if (zero_cpu_load_watchdog > 4)
        {
            break;
        }

        /*Process packet and print details*/
        if(0 == ret_val)
        {
            break;
        }
        else if (2 == ret_val)
        {
            /*Don't count diagnostic frames*/
            num_packets_received--;
        }
    }

    total_time = ((double)stop_time.tv_sec * 1000000 + (double)stop_time.tv_usec);
    total_time -= ((double)start_time.tv_sec * 1000000 + (double)start_time.tv_usec);

    printf("received frames : %d\n", num_packets_received);

    printf("Average CAN 2 Eth delay : %d us\n", avg_can_2_eth_bridge_delay);
    printf("Max CAN 2 Eth delay : %d us\n", max_can_2_eth_bridge_delay);

    printf("Average Eth 2 Eth delay : %d us\n", avg_eth_2_eth_bridge_delay);
    printf("Max Eth 2 Eth delay : %d us\n", max_eth_2_eth_bridge_delay);

    /*Subtract by 4 to remove 0 values*/
    num_cpu_load_msg -= 4;

    printf("Average CPU Load : %f\n", (sum_of_cpu_load/num_cpu_load_msg));

    printf("Average SWI Load : %f\n", (sum_of_swi_load/num_cpu_load_msg));
    printf("Average ISR Load : %f\n", (sum_of_isr_load/num_cpu_load_msg));

    printf("Recieved packets for : %f us\n", total_time);

    printf("Num CAN frames sent : %d\n", num_can_frames_sent);
    printf("Num CAN frames received : %d\n", num_can_frames_rcvd);    

    printf("Average CAN 2 CAN delay : %d\n", avg_can_2_can_bridge_delay);
    printf("Max CAN 2 CAN delay : %d\n", max_can_2_can_bridge_delay);

    printf("Average ETH 2 CAN delay : %d\n", avg_eth_2_can_bridge_delay);
    printf("Max ETH 2 CAN delay : %d\n", max_eth_2_can_bridge_delay);


    //close pipes
    if(fd_can_2_eth_latency)
    {
        close(fd_can_2_eth_latency);
    }

    if(fd_can_2_eth_bw)
    {
        close(fd_can_2_eth_bw);
    }

    if(fd_eth_2_eth_latency)
    {
        close(fd_eth_2_eth_latency);
    }

    if(fd_eth_2_eth_bw)
    {
        close(fd_eth_2_eth_bw);
    }

    if(fd_gateway_load)
    {
        close(fd_gateway_load);
    }

    close(sockfd);
#ifdef DUMP_CPU_LOAD_TO_FILE
    fclose(filePointer);
#endif //DUMP_CPU_LOAD_TO_FILE
    return 0;
}

uint8_t parse_1722_frame(uint8_t *frame, uint16_t packet_size, uint8_t verbosity)
{
    uint16_t curr_byte_offset = 14;
    float temp_val = 0;
    uint16_t temp_halfword = 0;
    uint8_t temp_byte = 0;
    uint32_t temp_delay = 0, bridge_delay = 0;

    struct timeval stop_time;
    double current_time_in_sec = 0;
    int    num_packets_sent_in_window = 0;

    static int    last_num_packets_count_can = 0, can_2_eth_counter = 0, eth_2_eth_counter = 0;
    static double last_updated_time_in_sec_can = 0;

    static int    last_num_packets_count_eth = 0;
    static double last_updated_time_in_sec_eth = 0;

    char string_buf[20];

    /*Declare and reset the structure*/
    fields_1722 avtp_fields;
    memset(&avtp_fields, 0x0, sizeof(fields_1722));

    /*Check subtype*/
    temp_byte = *(frame + curr_byte_offset++);
    if(temp_byte != NTSCF_SUBTYPE)
    {
        /*Exit if packet is not NTSCF type*/
        printf("Packet is not NTSCF subtype, received 0x%x, discarding \n", temp_byte);
        return 1;
    }

    /*Check if broadcast packet*/
    if(memcmp(frame, bcast_mac, 6) == 0)
    {
        if (detect_mac) /*Check if we should treat it as special packet*/
        {
            printf("found gateway mac - %x:%x:%x:%x:%x:%x\n", frame[6],frame[7],frame[8],frame[9],frame[10],frame[11]);
            return 0;
        }
        if (*(frame + 41) == DIAGNOSTIC_FRAME_TYPE)
        {
            /*----------------------Read CPU load----------------------*/
            temp_val = *(frame + 42);

            num_cpu_load_msg++;
            sum_of_cpu_load += temp_val;
            
        #ifdef DUMP_CPU_LOAD_TO_FILE
            fprintf(filePointer, "cpu_load : %0.3f\n", temp_val);
        #endif //DUMP_CPU_LOAD_TO_FILE

            if(temp_val <= 3)
            {
                zero_cpu_load_watchdog++;
            }
            else
            {
                /*reset to 0*/
                zero_cpu_load_watchdog = 0;
                average_cpu_load = ((6 * average_cpu_load) + (4 * temp_val)) / 10;
                /*Write CPU load to pipe*/
                if(fd_gateway_load)
                {           
                    float value = (float)average_cpu_load/(float)CPU_LOAD_SCALING_FACTOR;
                    sprintf(string_buf, "%0.3f\n", value);   
                    write(fd_gateway_load, string_buf, strlen(string_buf) + 1);                    
                }
            }

            /*----------------------Read ISR load----------------------*/
            temp_val = *(frame + 43);
            sum_of_isr_load += temp_val;

        #ifdef DUMP_CPU_LOAD_TO_FILE
            fprintf(filePointer, "isr_load : %0.3f\n", temp_val);
        #endif //DUMP_CPU_LOAD_TO_FILE

            /*----------------------Read SWI load----------------------*/
            temp_val = *(frame + 44);
            sum_of_swi_load += temp_val;

        #ifdef DUMP_CPU_LOAD_TO_FILE
            fprintf(filePointer, "swi_load : %0.3f\n", temp_val);
        #endif //DUMP_CPU_LOAD_TO_FILE
        
            return 2;
        }

        if (*(frame + 41) == STAT_FRAME_TYPE)
        {
            /*----------------------Read Packets sent and received by CAN generator app-----------*/
            memcpy(&num_can_frames_rcvd, (frame + 43), 4);
            memcpy(&num_can_frames_sent, (frame + 47), 4);

            /*----------------------Read max and average CAN 2 CAN delay------------------*/
            memcpy(&avg_can_2_can_bridge_delay, (frame + 51), 4);
            memcpy(&max_can_2_can_bridge_delay, (frame + 55), 4);

            /*----------------------Read max and average CAN 2 CAN delay------------------*/
            memcpy(&avg_eth_2_can_bridge_delay, (frame + 59), 4);
            memcpy(&max_eth_2_can_bridge_delay, (frame + 63), 4);

            return 2;
        }        
    }

    
    
    /*Check ACF message type*/
    temp_byte = *(frame + curr_byte_offset + 11);
    
    temp_byte >>= 1;

    if(temp_byte != ACF_MSG_TYPE_CAN)
    {
        printf("Received 0x%x where CAN type should have been, check your formatting code. Discarding packet\n", temp_byte);
        return 1;
    }

    /*We are good. Now parse other fields and print*/
    temp_byte = *(frame + curr_byte_offset++);
    /*get stream valid bit*/
    avtp_fields.stream_valid = temp_byte >> 7;

    /*Get ntscf version*/
    avtp_fields.ntscf_version = (temp_byte >> 4) & 0x7;

    /*Get length*/
    temp_halfword = temp_byte & 0x1;
    temp_halfword <<= 8;

    temp_byte = *(frame + curr_byte_offset++);
    temp_halfword |= temp_byte;
    avtp_fields.ntscf_data_length = temp_halfword;

    /*Get sequence number*/
    avtp_fields.sequence_id = *(frame + curr_byte_offset++);

    /*Get stream ID*/
    memcpy(avtp_fields.streamID, frame + curr_byte_offset, 8);
    curr_byte_offset += 8;

    /*Get CAN message length*/
    temp_byte = *(frame + curr_byte_offset++);
    temp_halfword = temp_byte & 0x1;
    temp_halfword <<= 8;

    temp_byte = *(frame + curr_byte_offset++);
    temp_halfword |= temp_byte;
    avtp_fields.can_msg_len = temp_halfword;

    /*Get pad and other configuration bits*/
    temp_byte = *(frame + curr_byte_offset++);
    avtp_fields.can_pad = (temp_byte >> CAN_SHIFT_VAL) & CAN_PAD_MASK;
    avtp_fields.msg_ts_valid = (temp_byte >> MTV_SHIFT_VAL) & 1;
    avtp_fields.remote_tx_req = (temp_byte >> RTR_SHIFT_VAL) & 1;
    avtp_fields.ext_frame_format = (temp_byte >> EFF_SHIFT_VAL) & 1;
    avtp_fields.bit_rate_switch = (temp_byte >> BRS_SHIFT_VAL) & 1;
    avtp_fields.flex_data_rate = (temp_byte >> FDF_SHIFT_VAL) & 1;
    avtp_fields.error_state_indicator = (temp_byte >> ESI_SHIFT_VAL) & 1;

    /*get CAN bus ID*/
    avtp_fields.can_bus_id = *(frame + curr_byte_offset++);

    /*If timestamp bit is valid then get the timestamp*/
    if(avtp_fields.msg_ts_valid)
    {
        memcpy(avtp_fields.can_ts, frame + curr_byte_offset, 8);
    }
    curr_byte_offset += 8;

    /*Get CAN ID*/
    avtp_fields.can_ID = ntohl(*((uint32_t*)(frame + curr_byte_offset)));
    curr_byte_offset += 4;

    /*Get CAN message*/
    memcpy(avtp_fields.can_msg, frame + curr_byte_offset, 64);

    /*Test automation code*/
    /*Get bridge delay from packet*/
    memcpy(&temp_delay, frame + 42, 4);
    bridge_delay = ntohl(temp_delay); 

    gettimeofday (&stop_time, NULL);

    if(*(frame + 41) == CAN_2_ETH_FRAME_TYPE)  //Indicates CAN to Ethernet routing
    {
        can_2_eth_counter++;

        if(bridge_delay < min_can_2_eth_bridge_delay) {
            min_can_2_eth_bridge_delay = bridge_delay;
        }

        if(bridge_delay > max_can_2_eth_bridge_delay) {
            max_can_2_eth_bridge_delay = bridge_delay;
        }

        avg_can_2_eth_bridge_delay = (8 * avg_can_2_eth_bridge_delay + 2 * bridge_delay) / 10;

        /*Write CAN 2 Eth delay to pipe*/
        if(fd_can_2_eth_latency && ((can_2_eth_counter % 100) == 0))
        {             
            float value = (float)avg_can_2_eth_bridge_delay/(float)LATENCY_SCALING_FACTOR;
            sprintf(string_buf, "%0.3f\n", value); 
            write(fd_can_2_eth_latency, string_buf, strlen(string_buf) + 1);                    
        }

        current_time_in_sec = (double)stop_time.tv_sec;
        if((current_time_in_sec - last_updated_time_in_sec_can) >= 1)
        {
            last_updated_time_in_sec_can = current_time_in_sec;
            /*1 second has elapsed. Get number of packets sent in this window*/
            num_packets_sent_in_window = can_2_eth_counter - last_num_packets_count_can;
            last_num_packets_count_can = can_2_eth_counter;

            /*Write CAN 2 CAN BW to pipe*/
            if(fd_can_2_eth_bw)
            {
                float value = (float)num_packets_sent_in_window/(float)BW_SCALING_FACTOR;
                sprintf(string_buf, "%0.3f\n", value);
                write(fd_can_2_eth_bw, string_buf, strlen(string_buf) + 1);             
            }                           
        }      
    }

    if(*(frame + 41) == ETH_2_ETH_FRAME_TYPE)  //Indicates Ethernet to Ethernet routing
    {
        eth_2_eth_counter++;

        if(bridge_delay < min_eth_2_eth_bridge_delay) {
            min_eth_2_eth_bridge_delay = bridge_delay;
        }

        if(bridge_delay > max_eth_2_eth_bridge_delay) {
            max_eth_2_eth_bridge_delay = bridge_delay;
        }

        avg_eth_2_eth_bridge_delay = (8 * avg_eth_2_eth_bridge_delay + 2 * bridge_delay) / 10;

        /*Write CAN 2 Eth delay to pipe*/
        if(fd_eth_2_eth_latency && ((eth_2_eth_counter % 100) == 0))
        {   
            float value = (float)avg_eth_2_eth_bridge_delay/(float)LATENCY_SCALING_FACTOR;
            sprintf(string_buf, "%0.3f\n", value);            
            write(fd_eth_2_eth_latency, string_buf, strlen(string_buf) + 1);                    
        }

        current_time_in_sec = (double)stop_time.tv_sec;
        if((current_time_in_sec - last_updated_time_in_sec_eth) >= 1)
        {
            last_updated_time_in_sec_eth = current_time_in_sec;
            /*1 second has elapsed. Get number of packets sent in this window*/
            num_packets_sent_in_window = eth_2_eth_counter - last_num_packets_count_eth;
            last_num_packets_count_eth = eth_2_eth_counter;

            /*Write CAN 2 CAN BW to pipe*/
            if(fd_eth_2_eth_bw)
            {
                float value = (float)num_packets_sent_in_window/(float)BW_SCALING_FACTOR;
                sprintf(string_buf, "%0.3f\n", value);
                write(fd_eth_2_eth_bw, string_buf, strlen(string_buf) + 1);             
            }                           
        }   
    }

    if(verbosity)
    {
        /*Print the fields in the received message*/
        print_can_msg(&avtp_fields);
    }

    return 1;
}

void print_can_msg(fields_1722 *frame)
{
    uint8_t count = 0;

    /*First print CAN parameters then print CAN message*/
    printf("CAN ID : \t\t\t0x%x\n", frame->can_ID);
    printf("CAN message length : \t\t%d bytes \n", 64); /*Hardcoded for now*/
    printf("-----------------------------------------------------\n\n");
    printf("Remote Tx Req : \t\t\t%d\n", frame->remote_tx_req);
    printf("Extended Frame Format : \t\t%d\n", frame->ext_frame_format);
    printf("Bit rate switch : \t\t\t%d\n", frame->bit_rate_switch);
    printf("Flex Data Rate : \t\t\t%d\n", frame->flex_data_rate);
    printf("Timestamp Valid : \t\t\t%d\n", frame->msg_ts_valid);
    printf("CAN Bus ID : \t\t\t\t%d\n\n", frame->can_bus_id);

    /*Print message contents*/
    printf("Message : \n");
    for(count = 0; count < 64; count++)
    {
        if((count % 8) == 0)
        {
            printf("\n");
        }
        printf("0x%x ,", frame->can_msg[count]);
    }
    printf("\n\n");

}

void get_cmdline_args(int argc, char *argv[])
{
    uint8_t eth_interface_provided = 0;

    for (int i = 0; i < argc; ++i)
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
            else if(strcmp(argv[i], "--detect_mac") == 0)
            {
                if(strcmp(argv[i + 1], "0") == 0)
                {
                    detect_mac = 0;
                }
                else if(strcmp(argv[i + 1], "1") == 0)
                {
                    detect_mac = 1;
                }
                else
                {
                    printf("Unknown option. detect mac mode is 0\n");
                }
            }
            else if(strcmp(argv[i], "--timeout") == 0)
            {
                timeout_value = atoi(argv[i + 1]);
            }
            else if(strcmp(argv[i], "--pipes") == 0)
            {
                enable_pipes = atoi(argv[i + 1]);
            }
            else if(strcmp(argv[i], "--help") == 0)
            {
                printf("Example usage : sudo recv_1722.out --eth_interface eth0 --verbosity verbose --detect_mac 0 --timeout 10\n\n");
                printf("Here are the options for the program\n");
                printf("--eth_interface\t\t\t\t: Name of the ethernet interface on which to listen. See ifconfig output. This must be provided\n");
                printf("--verbosity (optional)\t\t\t: Control log level. Options are 'verbose' and 'non-verbose' (silent). Default is non-verbose\n");
                printf("--detect_mac (optional)\t\t\t: Special mode to detect MAC address of gateway. Used for test automation. Default is 0\n");
                printf("--timeout (optional)\t\t\t: If no Ethernet frames are received for this much seconds, program will exit. Default is 1000\n");
                printf("--pipes (optional)\t\t\t: Tells the program whether to run without pipes (used for testing performance). Default : enabled\n");
                exit(0);
            }
        }
    }

    if(!eth_interface_provided)
    {
        printf("\n\nYou did not provide an ethernet interface name. Program cannot run. Try running with --help. Exiting\n");
        exit(0);
    } 
        
}

void exception_handler(void)
{
    /*Exit main application*/
    exit_loop = 1;    
}
