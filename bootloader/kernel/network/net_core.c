#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>

// Network Configuration
#define MAX_NETWORK_DEVICES 16
#define MTU_SIZE 1500
#define MAX_SOCKETS 1024
#define TCP_WINDOW_SIZE 65535

// Protocol Numbers
#define PROTO_ICMP 1
#define PROTO_TCP  6
#define PROTO_UDP  17

// Ethernet Header
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) ethernet_header_t;

// IP Header
typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ip_header_t;

// TCP Header
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) tcp_header_t;

// UDP Header
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

// Network Device
typedef struct network_device {
    char name[16];
    uint8_t mac_address[6];
    uint32_t ip_address;
    uint32_t netmask;
    uint32_t gateway;
    
    // Statistics
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t errors;
    
    // Operations
    int (*send)(struct network_device* dev, void* packet, size_t size);
    int (*receive)(struct network_device* dev, void* buffer, size_t size);
    
    void* private_data;
} network_device_t;

// Socket Structure
typedef struct {
    int fd;
    int type;           // SOCK_STREAM, SOCK_DGRAM
    int protocol;
    uint32_t local_ip;
    uint16_t local_port;
    uint32_t remote_ip;
    uint16_t remote_port;
    
    void* recv_buffer;
    void* send_buffer;
    size_t recv_buffer_size;
    size_t send_buffer_size;
    
    // TCP specific
    uint32_t seq_num;
    uint32_t ack_num;
    int state;
    
    bool blocking;
    void* private_data;
} socket_t;

// Network Statistics
typedef struct {
    uint64_t total_packets_sent;
    uint64_t total_packets_received;
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    uint64_t dropped_packets;
    uint64_t tcp_connections;
    uint64_t udp_datagrams;
    
    // AI Predictions
    uint32_t predicted_bandwidth;
    uint32_t predicted_latency;
    float congestion_probability;
} network_stats_t;

// Function Prototypes
void network_init(void);
int network_register_device(network_device_t* dev);
network_device_t* network_get_device(const char* name);

// Packet Processing
void network_receive_packet(network_device_t* dev, void* packet, size_t size);
int network_send_packet(network_device_t* dev, void* packet, size_t size);

// Protocol Handlers
void ethernet_handle_packet(void* packet, size_t size);
void ip_handle_packet(void* packet, size_t size);
void tcp_handle_packet(ip_header_t* ip_hdr, void* packet, size_t size);
void udp_handle_packet(ip_header_t* ip_hdr, void* packet, size_t size);

// Socket API
int socket_create(int domain, int type, int protocol);
int socket_bind(int sockfd, uint32_t ip, uint16_t port);
int socket_listen(int sockfd, int backlog);
int socket_accept(int sockfd);
int socket_connect(int sockfd, uint32_t ip, uint16_t port);
int socket_send(int sockfd, const void* buffer, size_t size, int flags);
int socket_recv(int sockfd, void* buffer, size_t size, int flags);
int socket_close(int sockfd);

// Utilities
uint16_t network_checksum(const void* data, size_t length);
void ip_to_string(uint32_t ip, char* buffer);
uint32_t string_to_ip(const char* str);

#endif // NETWORK_H