/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t; -*- */
#define MAX(x,y) (x > y ? x : y)
#define NOW Scheduler::instance().clock()


void GbcAgent::recvgbc(Packet* pkt) {
	hdr_gbc* gbchdr=hdr_gbc::access(pkt); //GBC header
	hdr_cmn* cmnhdr=hdr_cmn::access(pkt); //Global header

	switch( gbchdr->msgtype_ )
	{
    case 1: // Searching
    
    //Node received messages
    recvMsgs_++;
    
    //Resource Found
	if(hasresource(gbchdr->query_elem_)) {		
		if(logtarget) {
			sprintf(logtarget->pt_->buffer(),"e -t %11.9f "
			"-Qid %d -Qel %d -Node %d -Proto %d -Hops %d -Nl AGT GBC Resource Found"
			,NOW,gbchdr->query_id_,gbchdr->query_elem_,addr(),gbchdr->proto_,gbchdr->nHops_);
			logtarget->pt_->dump();
			}
		//Start cancellation as son as possible
		cancelPacket(gbchdr->query_id_,gbchdr->query_elem_,gbchdr->source_,gbchdr->size_,gbchdr->proto_,addr(),gbchdr->initial_delay_,gbchdr->jitter_);
		//Recycle
		Packet::free(pkt);
	}
	else
	{
		//Relay packet - forward without duplications
		if(!prevSearch(gbchdr->query_id_)) {
			
			sentMsgs_++;
			
			//Sempre que faz forward incementa o contador no cabeçalho da mensagem

			//Compute added delay in the searching fase
			double waittime=calcDelayHop(6,gbchdr->nHops_,gbchdr->initial_delay_,gbchdr->jitter_,1);

			if(logtarget) {
				sprintf(logtarget->pt_->buffer(),"t -t %11.9f "
                                "-Hs %d -Hops %d -Nl AGT -Ii %d -It searchforward "
                                "-delay %g",NOW,addr(),gbchdr->nHops_,cmnhdr->uid(),waittime);
                        	logtarget->pt_->dump();
				}
				
			//Query informations stored in the node
			queries_[cqueries_].query_initiator=false;
			queries_[cqueries_].cancel_initiator=false;
			queries_[cqueries_].query_id=gbchdr->query_id_;
			queries_[cqueries_].relay_search=true;
			queries_[cqueries_].relay_cancel=false;
			queries_[cqueries_].relay_answer=false;
			cqueries_++;
			
			//Searching Forward
			insertInTQueue(waittime,pkt);
			}
		else {
			Packet::free(pkt);
			}
	}
	break;

	case 2: // Answer used in BERS - Not implemented
	fprintf(stderr,"Incorrectly defined message type msgtype_: %d\n",gbchdr->msgtype_);
			exit(1);
	break;

	case 3: // Cancellation
	
	//Node received messages
    recvMsgs_++;
    
    //Initiator knows the resource location
	if(addr()==gbchdr->source_) {
		if(logtarget) {
			sprintf(logtarget->pt_->buffer(),"e -t %11.9f "
			"-QryID %d -QryElem %d -NodeRes %d -Hs %d -Hops %d -Proto %d -Nl AGT GBC Resource at Initiator"
			,NOW,gbchdr->query_id_,gbchdr->query_elem_,gbchdr->noderesource_,addr(),gbchdr->nHops_,gbchdr->proto_);
			logtarget->pt_->dump();
			}
	
		// Remove future searching messages
		if(status()==TIMER_PENDING) cancel();
		//if(bcirhdr->resource_!=addr() && !forward_ && status()==TIMER_PENDING) cancel();
		Packet::free(pkt);
	}
	
	else 
	{
		sentMsgs_++;
		
		if(!prevCancel(gbchdr->query_id_) && prevSearch(gbchdr->query_id_)) {
			
			//Compute added delay in the cancellation fase
			double waittime=calcDelayHop(1,gbchdr->nHops_,gbchdr->initial_delay_,gbchdr->jitter_,1);
	
			if(logtarget) {
				sprintf(logtarget->pt_->buffer(),"t -t %11.9f "
					"-Hs %d Hop %d -Nl AGT -Ii %d -It cancelationforward "
					"-delay %g",NOW,addr(),gbchdr->nHops_,cmnhdr->uid(),waittime); //Não sei porque não dá erro
					logtarget->pt_->dump();
			}
			
			//Query informations stored in the node
			queries_[cqueries_].query_initiator=false;
			queries_[cqueries_].cancel_initiator=false;
			queries_[cqueries_].query_id=gbchdr->query_id_;
			queries_[cqueries_].relay_search=false;
			queries_[cqueries_].relay_cancel=true;
			queries_[cqueries_].relay_answer=false;
			cqueries_++;
			
			//Cancellation Forward
			insertInTQueue(waittime,pkt);
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


