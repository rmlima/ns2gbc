/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
 
#ifndef GBCAGENT_H
#define GBCAGENT_H
//#ifndef __bcir_h__
//#define __bcir_h__

#include <agent.h>
#include <trace.h>
#include <cmu-trace.h>

#include "hdr_gbc.h"


// for ns-2
#define GBC_PORT 1234

#define GBC_RCVBUFSIZE 500

#define GBC_BYTES_PER_SOURCE 32

// for gbc
#define GBC_MAXQ 1000


class GbcAgent : public Agent, TimerHandler {
public:
	virtual int command(int argc,const char*const* argv);
	virtual void recv(Packet* pkt,Handler* =0);
	virtual void expire(Event*);
	GbcAgent();

private:
	RNG rng;

	// Messages received: only the first copy is accounted
	unsigned      int recvMsgs_;
	// Messages sent
	unsigned      int sentMsgs_;

	int uid_; //Para remover não estou a usar

	struct QStatus {
		bool  query_initiator;
		bool  cancel_initiator;
		int  query_id;
		bool relay_search;
		bool relay_cancel;
		bool relay_answer;
	};
	
	QStatus queries_[GBC_MAXQ];
	int cqueries_;

	// BCIR - LOG
	bool show;
	// BCIR
	double fix_delay_min;
	double fix_delay_max;
	
	int resource_;
	bool hasresource(int resource);
	double calcDelayHop(int proto, double hop, double delay, double jitter, int M);

	void searchPacket(int query_id, int size, int proto, double delay, double jitter, int M, int resource);
//	void setResource(int resource);
	void cancelPacket(int query_id, int query_elem, int query_source, int size, int proto, int resource, double delay, double jitter);
	void answerPacket(int size, int proto, double delay, double jitter, int initiator, int resource);

	void showheader(char opt, Packet* pkt);

	void recvgbc(Packet* pkt);

	bool prevSearch(int query_id);
	bool prevCancel(int query_id);
	void statusNode();

	// Message waiting queue
	struct TQueue {
		double  expires_;
		int     maxLHops_;
		Packet* origpkt_;
		TQueue* next_;

		TQueue(double expires,Packet* pkt) : 
			expires_(expires),
			maxLHops_(hdr_gbc::access(pkt)->nHops_),
			origpkt_(pkt),
			next_(NULL) {};
		~TQueue() {
			Packet::free(origpkt_);
		}
	};

	TQueue* tQueueHead_;
	void insertInTQueue(double waittime,Packet* pkt);


	// Packet utils

	Packet* createGbcPkt(int size);
	void copyGbcHdr(hdr_gbc* src,hdr_gbc* dst);



	// State output
	Trace* logtarget;
	void printState(Trace* out);
};

#endif
