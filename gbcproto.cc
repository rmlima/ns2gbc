/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t; -*- */
#define MAX(x,y) (x > y ? x : y)
#define NOW Scheduler::instance().clock()


void GbcAgent::recvgbc(Packet* pkt) {
	hdr_gbc* gbchdr=hdr_gbc::access(pkt); //GBC header
	hdr_cmn* cmnhdr=hdr_cmn::access(pkt); //Global header
	
	
    recvMsgs_++; //Node received messages
    
	switch( gbchdr->msgtype_ )
	{
    case 1: // Searching
    if (getpos(gbchdr->query_id_)==-1) {
		//Message with new query
	    //Resource Found
		if(hasresource(gbchdr->query_elem_) && !prevCancel(gbchdr->query_id_)) {		
			
			//Start cancellation as son as possible
			queries_[cqueries_].query_initiator=false;
			queries_[cqueries_].relay_search=true;
			queries_[cqueries_].cancel_initiator=true;
			queries_[cqueries_].query_id=gbchdr->query_id_;
			queries_[cqueries_].relay_cancel=true;
			queries_[cqueries_].pre_relay_cancel=true;
			queries_[cqueries_].relay_answer=false;
			cqueries_++;
			cancelPacket(gbchdr->query_id_,gbchdr->query_elem_,gbchdr->source_,gbchdr->size_,gbchdr->proto_,addr(),gbchdr->nHops_,gbchdr->timesent_);
			
			if(logtarget) {
				sprintf(logtarget->pt_->buffer(),"e -t %11.9f "
				"-Qid %d -Qel %d -Node %d -Proto %d -Hops %d -Nl AGT GBC Resource Found"
				,NOW,gbchdr->query_id_,gbchdr->query_elem_,addr(),gbchdr->proto_,gbchdr->nHops_);
				logtarget->pt_->dump();
				}
			//Recycle
			Packet::free(pkt);
		}
		else
		{
			//Relay packet - forward without duplications
			//if(!queries_[getpos(gbchdr->query_id_)].relay_search || ) {
				//if(getpos(gbchdr->query_id_)==-1) {
				//if(true) {
				sentMsgs_++;
				queries_[cqueries_].query_initiator=false;
				queries_[cqueries_].relay_search=false;
				queries_[cqueries_].pre_relay_search=true;
				queries_[cqueries_].cancel_initiator=false;
				queries_[cqueries_].query_id=gbchdr->query_id_;
				queries_[cqueries_].relay_cancel=false;
				queries_[cqueries_].pre_relay_cancel=false;
				queries_[cqueries_].relay_answer=false;
				cqueries_++;
				
				//Sempre que faz forward incementa o contador no cabeçalho da mensagem
	
				//Compute added delay in the searching fase
				double waittime=calcDelayHop(9,gbchdr->nHops_,1);
	
				if(logtarget) {
					sprintf(logtarget->pt_->buffer(),"t -t %11.9f "
	                                "-Hs %d -Hops %d -Nl AGT -Ii %d -It searchforward "
	                                "-delay %g",NOW,addr(),gbchdr->nHops_,cmnhdr->uid(),waittime);
	                        	logtarget->pt_->dump();
					}	
				//Searching Forward
				insertInTQueue(waittime,pkt);
				/*}
			else {
				Packet::free(pkt);
				}*/
		}
	} else {
				Packet::free(pkt);
				}
	break;

	case 2: // Answer used in BERS - Not implemented
	fprintf(stderr,"Incorrectly defined message type msgtype_: %d\n",gbchdr->msgtype_);
			exit(1);
	break;

	case 3: // Cancellation
    
    //Receve msg cancelamento esta escalonado para searchforward e ainda não transmitiu, então limpa a fila de eventos futuros.
    if (queries_[getpos(gbchdr->query_id_)].pre_relay_search && !queries_[getpos(gbchdr->query_id_)].relay_search)
    {
		if(tQueueHead_) tQueueHead_=NULL; //Como libertar a memória ?
		Packet::free(pkt);
	}
    else {
	    //Initiator knows the resource location - Only first discovery is logged
		if(addr()==gbchdr->source_ && !queries_[getpos(gbchdr->query_id_)].resource_found) {
			
			if(logtarget ) {
				sprintf(logtarget->pt_->buffer(),"e -t %11.9f "
				"-Qid %d -Qelem %d -NodeRes %d -Hs %d -Hops %d -Proto %d -sent %11.9f -cancel %11.9f -Nl AGT GBC Resource at Initiator"
				,NOW,gbchdr->query_id_,gbchdr->query_elem_,gbchdr->noderesource_,addr(),gbchdr->nHops_,gbchdr->proto_,gbchdr->timesent_,gbchdr->timesentCancel_);
				logtarget->pt_->dump();
				}
			queries_[getpos(gbchdr->query_id_)].resource_found=true;
		}
		if(!prevCancel(gbchdr->query_id_) && prevSearch(gbchdr->query_id_)) {
				
				//Compute added delay in the cancellation fase
				double waittime=calcDelayHop(1,gbchdr->nHops_,1);
	
				if(logtarget) {
					sprintf(logtarget->pt_->buffer(),"t -t %11.9f "
						"-Hs %d Hop %d -Nl AGT -Ii %d -It cancelationforward "
						"-delay %g",NOW,addr(),gbchdr->nHops_,cmnhdr->uid(),waittime);
						logtarget->pt_->dump();
				}
	
				//Query informations stored in the node
				queries_[getpos(gbchdr->query_id_)].pre_relay_cancel=true;
	
				//Cancellation Forward
				insertInTQueue(waittime,pkt);
				
				sentMsgs_++;
			}
			else {
				Packet::free(pkt);
			 }
			}
		break;

	default :
		fprintf(stderr,"Incorrectly defined message type msgtype_: %d\n",gbchdr->msgtype_);
		exit(1);
	}
}

int GbcAgent::getpos(int query_id) {
int i=0;
bool found=false;	
	
for (i=0; i<=cqueries_; i++)
	if (queries_[i].query_id == query_id) {
	    found=true;
	    break;}
if (found) return i; else return -1;
}


bool GbcAgent::prevSearch(int query_id) {
int i=0;
bool search=false;

for (i=0; i<=cqueries_; i++)
	if ((queries_[i].query_id == query_id) && queries_[i].relay_search)
	    search=true;
return search;
}

bool GbcAgent::prevCancel(int query_id) {
int i=0;
bool cancel=false;

for (i=0; i<=cqueries_; i++)
	if ((queries_[i].query_id == query_id) && queries_[i].pre_relay_cancel)
	    cancel=true;
return cancel;
}
