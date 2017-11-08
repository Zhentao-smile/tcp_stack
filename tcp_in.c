#include "tcp.h"
#include "tcp_sock.h"
#include "tcp_timer.h"

#include "log.h"
#include "ring_buffer.h"

#include <stdlib.h>

// handling incoming packet for TCP_LISTEN state
//
// 1. malloc a child tcp sock to serve this connection request; 
// 2. send TCP_SYN | TCP_ACK by child tcp sock;
// 3. hash the child tcp sock into established_table (because the 4-tuple 
//    is determined).
void tcp_state_listen(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	// fprintf(stdout, "TODO:[tcp_in.c][tcp_state_listen] implement this function please.\n");
	struct tcp_sock* c_tsk;

	c_tsk = alloc_tcp_sock();
	c_tsk->sk_sip = cb->daddr;
	c_tsk->sk_dip = cb->saddr;
	c_tsk->sk_sport = cb->dport;
	c_tsk->sk_dport = cb->sport;
	// fprintf(stdout, "[YU] DEBUG: sip:"IP_FMT".\n", LE_IP_FMT_STR(tsk->sk_sip));
	// fprintf(stdout, "[YU] DEBUG: dip:"IP_FMT".\n", LE_IP_FMT_STR(tsk->sk_dip));
	// fprintf(stdout, "[YU] DEBUG: sport: %u.\n", tsk->sk_sport);
	// fprintf(stdout, "[YU] DEBUG: dport: %u.\n", tsk->sk_dport);
	// fprintf(stdout, "[YU] DEBUG: sip:"IP_FMT".\n", LE_IP_FMT_STR(c_tsk->sk_sip));
	// fprintf(stdout, "[YU] DEBUG: dip:"IP_FMT".\n", LE_IP_FMT_STR(c_tsk->sk_dip));
	// fprintf(stdout, "[YU] DEBUG: sport: %u.\n", c_tsk->sk_sport);
	// fprintf(stdout, "[YU] DEBUG: dport: %u.\n", c_tsk->sk_dport);
	// fprintf(stdout, "[YU] DEBUG: dport: %u.\n", htons(c_tsk->sk_dport));
	int pkt_size = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE;
	char *new_packet = malloc(pkt_size);
	if (!new_packet) {
		log(ERROR, "malloc tcp control packet failed.");
		return ;
	}

	struct iphdr *ip = packet_to_ip_hdr(new_packet);
	struct tcphdr *tcp = (struct tcphdr *)((char *)ip + IP_BASE_HDR_SIZE);

	u16 tot_len = IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE;
	ip_init_hdr(ip, c_tsk->sk_sip, c_tsk->sk_dip, tot_len, IPPROTO_TCP);
	tcp->sport = htons(c_tsk->sk_sport);
	tcp->dport = htons(c_tsk->sk_dport);
	tcp->ack = htonl(cb->seq);
	tcp->off = TCP_HDR_OFFSET;
	tcp->flags = TCP_SYN | TCP_ACK;
	tcp->checksum = tcp_checksum(ip, tcp);
	ip_send_packet(new_packet, pkt_size);

	tcp_set_state(c_tsk, TCP_SYN_SENT);

	if(tcp_hash(c_tsk)){
		log(ERROR, "insert into established_table failed.");
		return;
	}
}

// handling incoming packet for TCP_CLOSED state, by replying TCP_RST
void tcp_state_closed(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	tcp_send_reset(cb);
}

// handling incoming packet for TCP_SYN_SENT state
//
// If everything goes well (the incoming packet is TCP_SYN|TCP_ACK), reply with 
// TCP_ACK, and enter TCP_ESTABLISHED state, notify tcp_sock_connect; otherwise, 
// reply with TCP_RST.
void tcp_state_syn_sent(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	fprintf(stdout, "TODO:[tcp_in.c][tcp_state_syn_sent] implement this function please.\n");
}

// update the snd_wnd of tcp_sock
//
// if the snd_wnd before updating is zero, notify tcp_sock_send (wait_send)
static inline void tcp_update_window(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u16 old_snd_wnd = tsk->snd_wnd;
	tsk->snd_wnd = cb->rwnd;
	if (old_snd_wnd == 0)
		wake_up(tsk->wait_send);
}

// update the snd_wnd safely: cb->ack should be between snd_una and snd_nxt
static inline void tcp_update_window_safe(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	if (tsk->snd_una <= cb->ack && cb->ack <= tsk->snd_nxt)
		tcp_update_window(tsk, cb);
}

// handling incoming ack packet for tcp sock in TCP_SYN_RECV state
//
// 1. remove itself from parent's listen queue;
// 2. add itself to parent's accept queue;
// 3. wake up parent (wait_accept) since there is established connection in the
//    queue.
void tcp_state_syn_recv(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	fprintf(stdout, "TODO:[tcp_in.c][tcp_state_syn_recv] implement this function please.\n");
}

#ifndef max
#	define max(x,y) ((x)>(y) ? (x) : (y))
#endif

// check whether the sequence number of the incoming packet is in the receiving
// window
static inline int is_tcp_seq_valid(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u32 rcv_end = tsk->rcv_nxt + max(tsk->rcv_wnd, 1);
	if (cb->seq < rcv_end && tsk->rcv_nxt <= cb->seq_end) {
		return 1;
	}
	else {
		log(ERROR, "received packet with invalid seq, drop it.");
		return 0;
	}
}

// put the payload of the incoming packet into rcv_buf, and notify the
// tcp_sock_read (wait_recv)
int tcp_recv_data(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	fprintf(stdout, "TODO:[tcp_in.c][tcp_recv_data] implement this function please.\n");

	return 0;
}

// Process an incoming packet as follows:
// 	 1. if the state is TCP_CLOSED, hand the packet over to tcp_state_closed;
// 	 2. if the state is TCP_LISTEN, hand it over to tcp_state_listen;
// 	 3. if the state is TCP_SYN_SENT, hand it to tcp_state_syn_sent;
// 	 4. check whether the sequence number of the packet is valid, if not, drop
// 	    it;
// 	 5. if the TCP_RST bit of the packet is set, close this connection, and
// 	    release the resources of this tcp sock;
// 	 6. if the TCP_SYN bit is set, reply with TCP_RST and close this connection,
// 	    as valid TCP_SYN has been processed in step 2 & 3;
// 	 7. check if the TCP_ACK bit is set, since every packet (except the first 
//      SYN) should set this bit;
//   8. process the ack of the packet: if it ACKs the outgoing SYN packet, 
//      establish the connection; if it ACKs new data, update the window;
//      if it ACKs the outgoing FIN packet, switch to corresponding state;
//   9. process the payload of the packet: call tcp_recv_data to receive data;
//  10. if the TCP_FIN bit is set, update the TCP_STATE accordingly;
//  11. at last, do not forget to reply with TCP_ACK if the connection is alive.
void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	// fprintf(stdout, "TODO:[tcp_in.c][tcp_process] implement this function please.\n");
	switch(tsk->state)
	{
		case TCP_CLOSED:
			fprintf(stdout, "[YU] DEBUG: TCP state is TCP_CLOSED.\n");
			tcp_state_closed(tsk, cb, packet);
			return;
			break;
		case TCP_LISTEN:
			fprintf(stdout, "[YU] DEBUG: TCP state is TCP_LISTEN.\n");
			tcp_state_listen(tsk, cb, packet);
			return;
			break;
		case TCP_SYN_SENT:
			fprintf(stdout, "[YU] DEBUG: TCP state is TCP_SYN_SENT.\n");
			tcp_state_syn_sent(tsk, cb, packet);
			return;
			break;
		default:
			fprintf(stdout, "[YU] DEBUG: TCP state is default.\n");
			break;
	}

	if(!is_tcp_seq_valid(tsk, cb))
	{
		// drop
	}
	
	if(cb->flags | TCP_RST)
	{
		//close this connection, and release the resources of this tcp sock
		return;
	}

	if(cb->flags | TCP_SYN)
	{
		//reply with TCP_RST and close this connection
		return;
	}

	if(!(cb->flags | TCP_ACK))
	{
		//drop
		return;
	}

	//process the ack of the packet


	if(cb->flags | TCP_FIN)
	{
		//update the TCP_STATE accordingly
		return;
	}

	//reply with TCP_ACK if the connection is alive
}	
