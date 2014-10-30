/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t; -*- */

#include "gbcagent.h"
#include "gbcproto.cc"
#include <mhash.h>

#define MAX(x,y) (x > y ? x : y)
#define NOW Scheduler::instance().clock()

GbcAgent::GbcAgent() : Agent(PT_GBC),
		recvMsgs_(0),
		sentMsgs_(0),
		cqueries_(0),
		delay_(0) {
	tQueueHead_=NULL;
	logtarget=NULL; 
	show=TRUE;
}

int GbcAgent::command(int argc,const char*const* argv) {

	char dummy;
	//Default parameters
	int size=1000, M=1, query_id, query_elem, res, i;


	//SEARCH command : Used in traffile
	if(!strcmp(argv[1],"search") && argc==6) {
		
		if(sscanf(argv[2],"%d%c",&query_id,&dummy)!=1) {
			fprintf(stderr,"Wrong number of arguments in traffile. "
			"Format: <query_id> <size> <delay> <jitter> <M> <query_elem>\n");
			return (TCL_ERROR);
		}
		if(sscanf(argv[3],"%d%c",&query_elem,&dummy)!=1) {
			fprintf(stderr,"Wrong number of arguments in traffile. "
			"Format: <query_id> <size> <delay> <jitter> <M> <query_elem>\n");
			return (TCL_ERROR);
		}
		if(sscanf(argv[4],"%d%c",&size,&dummy)!=1) {
			fprintf(stderr,"Wrong number of arguments in traffile. "
			"Format: <query_id> <size> <delay> <jitter> <M> <query_elem>\n");
			return (TCL_ERROR);
		}
		if(sscanf(argv[5],"%d%c",&M,&dummy)!=1) {
			fprintf(stderr,"Wrong number of arguments in traffile. "
			"Format: <query_id> <size> <delay> <jitter> <M> <query_elem>\n");
			return (TCL_ERROR);
		}

	searchPacket(query_id,size,6,M,query_elem);
	return (TCL_OK);
	}

	// Set resources in nodes/Agents
	if(!strcmp(argv[1],"resource") && argc==3) {
		if(sscanf(argv[2],"%d%c",&res,&dummy)!=1) {
			fprintf(stderr,"Wrong number of arguments in traffile."
			"Format: gbc_(<node>) resource <resource_id>\n");
			return (TCL_ERROR);
		} else  
		{  //insert head array resources - drop first
		for (i=GBC_MAXR-1; i>0; i--)
			resources_[i]=resources_[i-1];
		resources_[0]=res;
			}
	return (TCL_OK);
	}
	
	// Set delay in nodes/Agents
	if(!strcmp(argv[1],"delay") && argc==3) {
		if(sscanf(argv[2],"%lf%c",&delay_,&dummy)!=1) {
			fprintf(stderr,"Wrong number of arguments in traffile."
			"Format: gbc_(<node>) delay <delay>\n");
			return (TCL_ERROR);
		}
	return (TCL_OK);
	}

	// Set jitter in nodes/Agents
	if(!strcmp(argv[1],"jitter") && argc==3) {
		if(sscanf(argv[2],"%lf%c",&jitter_,&dummy)!=1) {
			fprintf(stderr,"Wrong number of arguments in traffile."
			"Format: gbc_(<node>) resource <resource_id>\n");
			return (TCL_ERROR);
		}
	return (TCL_OK);
	}
		
	
	
	// Results LOG
	if(!strcmp(argv[1],"log-target") && argc==3) {
		logtarget=(Trace*)TclObject::lookup(argv[2]);
		if(!logtarget) {
			fprintf(stderr,"gbcAgent: log target %s not "
					"found\n",argv[2]);
			return (TCL_ERROR);
		}
		return (TCL_OK);
        }
	return Agent::command(argc,argv);
}

void GbcAgent::searchPacket(int query_id, int size, int proto,
				 int M, int query_elem) {
					
	Packet* pkt=createGbcPkt(size);
	hdr_gbc* gbchdr=hdr_gbc::access(pkt);

    //Carregar o cabeçalho das mensagens
    gbchdr->query_id_=query_id; // Identify the query
    gbchdr->size_=size;         // "Real" size of the packet (from a simulation point of view)
    gbchdr->proto_=proto;
	gbchdr->M_=M; 				//Parametro não usado mas sugerido pelo Carlos
	gbchdr->query_elem_=query_elem;
	
	gbchdr->source_=addr();     // Originator of the packet (initiator)
	gbchdr->timesent_=NOW;      // time at the sender of the packet
	gbchdr->nHops_=1;           // First Hop
	gbchdr->msgtype_=1;			// Searching Message
	
	printf("searchPacket: Start - Qid:%d Node:%d Qele:%d "
				"Proto:%d Size:%d Initial_delay:%lf M:%d"
				" \n",query_id,gbchdr->source_,query_elem,proto,size,delay_,M);
			
	if (getpos(gbchdr->query_id_)==-1) {
		queries_[cqueries_].query_initiator=true;
		queries_[cqueries_].relay_search=true;
		queries_[cqueries_].cancel_initiator=false;
		queries_[cqueries_].query_id=query_id;
		queries_[cqueries_].relay_cancel=false;
		queries_[cqueries_].relay_answer=false;
		queries_[cqueries_].resource_found=false;
		cqueries_++;
		send(pkt,0);
		
		statusNode();
		//Node data memory
		sentMsgs_++;
		//Write to LOG file
		if(logtarget) {
			sprintf(logtarget->pt_->buffer(),"e -t %11.9f "
					"-Qid %d -Qel %d -Initiator %d -Proto %d -Nl AGT GBC Search START"
					,NOW,query_id,query_elem,gbchdr->source_,proto);
			logtarget->pt_->dump();
			}
			
	    //Send header to  terminal
		if (show) {showheader('i',pkt);};
	}
	else {
			printf("QueryID Error: Start an Previous Query\n");
			exit(1);
		}
}

void GbcAgent::cancelPacket(int query_id, int query_elem, int query_source, int size, int proto, int noderesource, int nHops, double time) {
	double waittime;
	Packet* pkt=createGbcPkt(size);
	hdr_gbc* gbchdr=hdr_gbc::access(pkt);

	gbchdr->msgtype_=3;			// CANCELLATION PACKET
	gbchdr->query_id_=query_id;
	gbchdr->query_elem_=query_elem;	
	gbchdr->source_=query_source;       // Originator of the query
    gbchdr->size_=size;          		// "Real" size of the packet (from a simulation point of view)
    gbchdr->proto_=proto;
    gbchdr->nHops_=nHops;
    gbchdr->noderesource_=noderesource;
	//Node data memory    
	sentMsgs_++;
	gbchdr->timesent_=time;
	gbchdr->timesentCancel_=NOW;       // time at the sender of the packet
	
	//Compute added delay for the fist cancellation transmission
	waittime=calcDelayHop(1,gbchdr->nHops_,1);
	
	
	insertInTQueue(waittime,pkt);   //send(pkt,0);

	//Send header to terminal
	if (show) {showheader('k',pkt);};
}


void GbcAgent::recv(Packet* pkt,Handler*) {
        hdr_gbc* gbchdr=hdr_gbc::access(pkt);

//Select forward protocol - Now is only available GBC
	switch( gbchdr->proto_ ) {
		//case 1: return recvflood(pkt);	// FLOOD
		//case 2: return recvbers(pkt);	// BERS
		//case 3: return recvbers(pkt);	// BERS*
		//case 4: return recvbcir(pkt);	// BCIR
		//case 5: return recvbcir(pkt);   // BCIR*
		case 6: // GBC
			recvgbc(pkt); 
			break;
		default :
			fprintf(stderr,"Incorrectly defined searching protocol proto_: %d\n",gbchdr->proto_);
			exit(1);
	}
}

double GbcAgent::calcDelayHop(int proto, double hopcount, int M) {
	double fix_delay_min=0.001;
	//double fix_delay_max;


	double tmp = rng.uniform(fix_delay_min,jitter_);

	//printf("CalcDelayHop: initial_delay = %lf uniform = %f waittime = %f\n",initial_delay,tmp,2*(hopcount)*initial_delay+tmp);
	switch(proto) {
		case 1  : return delay_+tmp; break;//FLOOD Primeira mensagem de cancelamento
		case 2  : return (2*(hopcount)*delay_)+tmp; break;//BERS and BCIR
		case 3  : return (hopcount)*delay_+tmp; break;//BERS* and BCIR*
		case 4  : return (((hopcount/2)+(3/2))*delay_)+tmp; break;//BCIR2
		case 5  : return (((hopcount/4)+(7/4))*delay_)+tmp; break;//BCIR fast
		case 6  : if (hopcount==1)  return delay_+tmp;
				else return (hopcount-1)*delay_+tmp; break;//GBC
//		case 7  : return initial_delay+rng.uniform((jitter*.5),jitter); break;//BCIR - ACK and Cancellation DELAYS
//		case 7  : return 2*((hopcount+1)*initial_delay)+tmp; break;		  //BCIR 2*Dellay no primeiro HOP
		case 7  : return rng.uniform(fix_delay_min,jitter_/10); break; // Speed Cancellation 
		case 8  : return rng.uniform(jitter_,2*jitter_); break; // Prof. Hugo rml  Só poupa as mensagens de ACK.
		case 9  : return 10*(delay_+tmp); break;//Delay Tolerant
		default :
 			fprintf(stderr,"Error calcDelayGbc: %d\n",proto);
                	exit(1);
		}
}


//Not efficient
bool GbcAgent::hasresource(int find) {
int i=0;
bool found=false;

for (i=0; i<GBC_MAXR; i++)
	if (resources_[i]==find)
	    found=true;

return found;	
}


void GbcAgent::showheader(char opt, Packet* pkt) {
        hdr_gbc* gbchdr=hdr_gbc::access(pkt);

if(logtarget) {
	sprintf(logtarget->pt_->buffer(),"%c -t %11.9f "
            "-Qid %d -Qel %d -Node %d -MsgType %d -Proto %d"
            ,opt,NOW,gbchdr->query_id_,gbchdr->query_elem_,addr(),gbchdr->msgtype_,gbchdr->proto_);
    logtarget->pt_->dump();
    }
}

void GbcAgent::insertInTQueue(double time,Packet* pkt) {
	TQueue* newmsg=new TQueue(time,pkt);
	if(!tQueueHead_)
                tQueueHead_=newmsg;
        else {
	 		if(tQueueHead_->expires_>newmsg->expires_) {
	                        newmsg->next_=tQueueHead_;
	                        tQueueHead_=newmsg;
	                }
			else {
				TQueue* iter=tQueueHead_;
				while(iter->next_ && 
					  iter->next_->expires_ < newmsg->expires_)
						iter=iter->next_;
				newmsg->next_=iter->next_;
				iter->next_=newmsg;
			}
		}
	if(tQueueHead_) resched(tQueueHead_->expires_);
	//resched(0);

}


void GbcAgent::expire(Event*) {
		while(tQueueHead_) {
			hdr_gbc* hdrold=hdr_gbc::access(tQueueHead_->origpkt_);
			Packet* rebroad=createGbcPkt(hdrold->size_);
			hdr_gbc* hdrnew=hdr_gbc::access(rebroad);
			copyGbcHdr(hdrold,hdrnew);
						
			if (show) {showheader('z',rebroad);};
			
			if (hdrnew->msgtype_==1)
			{
				queries_[getpos(hdrnew->query_id_)].relay_search=true;
				if(logtarget) {
				sprintf(logtarget->pt_->buffer(),"f -t %11.9f "
					//"-Qid %d -Hs %d -Nl AGT -Ii %d -It searchagent "
					"-Hs %d -Nl AGT -Ii %d -It searching expire "
					,NOW,addr(),
					//hdr_cmn::access(tQueueHead_->origpkt_)->query_id_,
					hdr_cmn::access(tQueueHead_->origpkt_)->uid());
				logtarget->pt_->dump();
				hdrnew->nHops_++; 	//Only count searching Hops
				}
			}
			if (hdrnew->msgtype_==3)
			{
				queries_[getpos(hdrnew->query_id_)].relay_cancel=true;
				if(logtarget) {
				sprintf(logtarget->pt_->buffer(),"f -t %11.9f "
					//"-Qid %d -Hs %d -Nl AGT -Ii %d -It searchagent "
					"-Hs %d -Nl AGT -Ii %d -It cancellation expire "
					,NOW,addr(),
					//hdr_cmn::access(tQueueHead_->origpkt_)->query_id_,
					hdr_cmn::access(tQueueHead_->origpkt_)->uid());
				logtarget->pt_->dump();
				}
			}
			
			send(rebroad,0);
			
			//statusNode();
			
			TQueue* tmp=tQueueHead_;
			tQueueHead_=tQueueHead_->next_;
			delete tmp;

			if(tQueueHead_) resched(tQueueHead_->expires_);
		}
}

Packet* GbcAgent::createGbcPkt(int size) {
	Packet* pkt=allocpkt();

	hdr_ip::access(pkt)->daddr()=IP_BROADCAST;
	hdr_ip::access(pkt)->dport()=GBC_PORT;
	hdr_cmn::access(pkt)->size()=size;

	return pkt;
}


void GbcAgent::copyGbcHdr(hdr_gbc* src,hdr_gbc* dst) {

	dst->source_=src->source_;
	dst->query_id_=src->query_id_;
	dst->size_=src->size_;
	dst->timesent_=src->timesent_;
	dst->timesentCancel_=src->timesentCancel_;
	dst->nHops_=src->nHops_;
	//gbc:
	dst->proto_=src->proto_;
	dst->query_elem_=src->query_elem_;
	dst->noderesource_=src->noderesource_;
	dst->msgtype_=src->msgtype_;
	dst->M_=src->M_;
}

void GbcAgent::statusNode() {
int i=0;
printf("###########\n");
printf("NO: %d\n",addr());
printf("Cqueries: %d\n",cqueries_);
for (i=0; i<cqueries_; i++) {
	if (queries_[i].query_initiator) printf("Iniciador\n"); else
			printf("Nao Iniciador\n");
	if (queries_[i].cancel_initiator) printf("Iniciador do cancelamento \n"); else
			printf("Nao Iniciador de cancelamento\n");	
				
	printf("query_id=%d\n",queries_[i].query_id);
	if (queries_[i].relay_search) printf("Relay Search\n");
	if (queries_[i].relay_cancel) printf("Relay Cancel\n");				
	}	
}


/*
void GbcAgent::printState(Trace* out) {
	sprintf(out->pt_->buffer(),"-Hs %d -t %g -uid %d -sentMsg %d "
		"-recvb %d",addr(),NOW,
		uid_, sentMsgs_, recvMsgs_);
	out->pt_->dump();
}*/

/*********************************************************************
 *
 * TCL binding stuff 
 *
 *********************************************************************/

int hdr_gbc::offset_;

static class GbcHeaderClass : public PacketHeaderClass {
public:
        GbcHeaderClass() : PacketHeaderClass("PacketHeader/GBC",
					      sizeof(hdr_gbc)) {
                bind_offset(&hdr_gbc::offset_);
        };
} class_GbcHeader;

static class GbcAgentClass : public TclClass {
public:
	GbcAgentClass() : TclClass("Agent/GBC") {};
	TclObject* create(int,const char*const*) {
		return (new GbcAgent);
	}
} class_GbcAgent;
