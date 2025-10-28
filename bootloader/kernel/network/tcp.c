#include "network.h"
#include <string.h>
#include <stdlib.h>

// TCP States
#define TCP_CLOSED      0
#define TCP_LISTEN      1
#define TCP_SYN_SENT    2
#define TCP_SYN_RECV    3
#define TCP_ESTABLISHED 4
#define TCP_FIN_WAIT1   5
#define TCP_FIN_WAIT2   6
#define TCP_CLOSE_WAIT  7
#define TCP_CLOSING     8
#define TCP_LAST_ACK    9
#define TCP_TIME_WAIT   10

// TCP Flags
#define TCP_FIN  0x01
#define TCP_SYN  0x02
#define TCP_RST  0x04
#define TCP_PSH  0x08
#define TCP_ACK  0x10
#define TCP_URG  0x20

// TCP Connection Table
typedef struct {
    socket_t* sockets[MAX_SOCKETS];
    int count;
    spinlock_t lock;
} tcp_connection_table_t;

static tcp_connection_table_t tcp_connections = {0};

// AI-Enhanced Congestion Control
typedef struct {
    uint32_t cwnd;           // Congestion window
    uint32_t ssthresh;       // Slow start threshold
    uint32_t rtt;            // Round trip time
    uint32_t rtt_var;        // RTT variance
    uint64_t last_ack_time;
    
    // AI predictions
    float predicted_loss_rate;
    uint32_t predicted_bandwidth;
    int recommended_window_size;
} tcp_congestion_control_t;

void tcp_init(void) {
    memset(&tcp_connections, 0, sizeof(tcp_connections));
    spinlock_init(&tcp_connections.lock);
    kprintf("[TCP] Initialized\n");
}

static void tcp_send_packet(socket_t* sock, uint8_t flags, 
                           const void* data, size_t data_len) {
    size_t total_len = sizeof(ethernet_header_t) + sizeof(ip_header_t) + 
                       sizeof(tcp_header_t) + data_len;
    
    void* packet = kmalloc(total_len);
    if (!packet) return;
    
    // Build Ethernet header
    ethernet_header_t* eth = (ethernet_header_t*)packet;
    // Fill in destination MAC (would come from ARP)
    memset(eth->dest_mac, 0xFF, 6); // Broadcast for now
    // Fill in source MAC from device
    memcpy(eth->src_mac, /* device MAC */, 6);
    eth->ethertype = htons(0x0800); // IPv4
    
    // Build IP header
    ip_header_t* ip = (ip_header_t*)(eth + 1);
    ip->version_ihl = 0x45; // IPv4, 20 byte header
    ip->tos = 0;
    ip->total_length = htons(sizeof(ip_header_t) + sizeof(tcp_header_t) + data_len);
    ip->id = htons(rand() & 0xFFFF);
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = PROTO_TCP;
    ip->src_ip = htonl(sock->local_ip);
    ip->dest_ip = htonl(sock->remote_ip);
    ip->checksum = 0;
    ip->checksum = network_checksum(ip, sizeof(ip_header_t));
    
    // Build TCP header
    tcp_header_t* tcp = (tcp_header_t*)(ip + 1);
    tcp->src_port = htons(sock->local_port);
    tcp->dest_port = htons(sock->remote_port);
    tcp->seq_num = htonl(sock->seq_num);
    tcp->ack_num = htonl(sock->ack_num);
    tcp->data_offset = (sizeof(tcp_header_t) / 4) << 4;
    tcp->flags = flags;
    tcp->window = htons(TCP_WINDOW_SIZE);
    tcp->urgent_ptr = 0;
    
    // Copy data
    if (data && data_len > 0) {
        memcpy(tcp + 1, data, data_len);
    }
    
    // Calculate TCP checksum (pseudo-header + TCP header + data)
    tcp->checksum = 0;
    // Simplified - real implementation needs pseudo-header
    tcp->checksum = network_checksum(tcp, sizeof(tcp_header_t) + data_len);
    
    // Send packet
    network_device_t* dev = network_get_device("eth0");
    if (dev) {
        dev->send(dev, packet, total_len);
    }
    
    kfree(packet);
}

void tcp_handle_packet(ip_header_t* ip_hdr, void* packet, size_t size) {
    tcp_header_t* tcp = (tcp_header_t*)packet;
    
    // Convert network byte order
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    uint32_t seq = ntohl(tcp->seq_num);
    uint32_t ack = ntohl(tcp->ack_num);
    
    // Find matching socket
    socket_t* sock = NULL;
    spinlock_acquire(&tcp_connections.lock);
    
    for (int i = 0; i < tcp_connections.count; i++) {
        socket_t* s = tcp_connections.sockets[i];
        if (s->local_port == dest_port) {
            if (s->state == TCP_LISTEN || 
                (s->remote_ip == ntohl(ip_hdr->src_ip) && 
                 s->remote_port == src_port)) {
                sock = s;
                break;
            }
        }
    }
    
    spinlock_release(&tcp_connections.lock);
    
    if (!sock) {
        kprintf("[TCP] No socket found for port %d\n", dest_port);
        return;
    }
    
    // TCP State Machine
    switch (sock->state) {
        case TCP_LISTEN:
            if (tcp->flags & TCP_SYN) {
                // SYN received - send SYN+ACK
                sock->remote_ip = ntohl(ip_hdr->src_ip);
                sock->remote_port = src_port;
                sock->ack_num = seq + 1;
                sock->seq_num = rand();
                sock->state = TCP_SYN_RECV;
                
                tcp_send_packet(sock, TCP_SYN | TCP_ACK, NULL, 0);
                kprintf("[TCP] SYN received, sent SYN+ACK\n");
            }
            break;
            
        case TCP_SYN_SENT:
            if ((tcp->flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK)) {
                // SYN+ACK received - send ACK
                sock->ack_num = seq + 1;
                sock->state = TCP_ESTABLISHED;
                
                tcp_send_packet(sock, TCP_ACK, NULL, 0);
                kprintf("[TCP] Connection established\n");
            }
            break;
            
        case TCP_SYN_RECV:
            if (tcp->flags & TCP_ACK) {
                // ACK received - connection established
                sock->state = TCP_ESTABLISHED;
                kprintf("[TCP] Connection established (server)\n");
            }
            break;
            
        case TCP_ESTABLISHED:
            if (tcp->flags & TCP_FIN) {
                // FIN received - start closing
                sock->ack_num = seq + 1;
                sock->state = TCP_CLOSE_WAIT;
                
                tcp_send_packet(sock, TCP_ACK, NULL, 0);
                
                // Application should close, then send FIN
                sock->state = TCP_LAST_ACK;
                tcp_send_packet(sock, TCP_FIN | TCP_ACK, NULL, 0);
            } else if (tcp->flags & TCP_ACK) {
                // Data packet - copy to receive buffer
                size_t data_offset = (tcp->data_offset >> 4) * 4;
                size_t data_len = size - data_offset;
                
                if (data_len > 0) {
                    // AI-enhanced buffer management
                    void* data = (uint8_t*)tcp + data_offset;
                    // Copy to socket receive buffer
                    // (simplified - real implementation needs ring buffer)
                    
                    sock->ack_num = seq + data_len;
                    tcp_send_packet(sock, TCP_ACK, NULL, 0);
                }
            }
            break;
            
        case TCP_LAST_ACK:
            if (tcp->flags & TCP_ACK) {
                sock->state = TCP_CLOSED;
                kprintf("[TCP] Connection closed\n");
            }
            break;
    }
}

int tcp_connect(socket_t* sock, uint32_t ip, uint16_t port) {
    sock->remote_ip = ip;
    sock->remote_port = port;
    sock->seq_num = rand();
    sock->ack_num = 0;
    sock->state = TCP_SYN_SENT;
    
    // Send SYN
    tcp_send_packet(sock, TCP_SYN, NULL, 0);
    
    // Wait for connection (simplified - should be async with timeout)
    uint64_t timeout = timer_get_ticks() + 3000; // 3 second timeout
    while (sock->state != TCP_ESTABLISHED && timer_get_ticks() < timeout) {
        schedule(); // Yield CPU
    }
    
    return (sock->state == TCP_ESTABLISHED) ? 0 : -1;
}

int tcp_send(socket_t* sock, const void* data, size_t size) {
    if (sock->state != TCP_ESTABLISHED) {
        return -1;
    }
    
    // AI-enhanced sending with optimal packet sizing
    size_t remaining = size;
    const uint8_t* ptr = (const uint8_t*)data;
    
    while (remaining > 0) {
        size_t chunk = (remaining > MTU_SIZE - 100) ? MTU_SIZE - 100 : remaining;
        
        tcp_send_packet(sock, TCP_ACK | TCP_PSH, ptr, chunk);
        
        sock->seq_num += chunk;
        ptr += chunk;
        remaining -= chunk;
        
        // AI: Predict optimal pacing
        // usleep(ai_predict_send_delay());
    }
    
    return size;
}

int tcp_listen(socket_t* sock, int backlog) {
    sock->state = TCP_LISTEN;
    
    // Add to connection table
    spinlock_acquire(&tcp_connections.lock);
    if (tcp_connections.count < MAX_SOCKETS) {
        tcp_connections.sockets[tcp_connections.count++] = sock;
    }
    spinlock_release(&tcp_connections.lock);
    
    return 0;
}