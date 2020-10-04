/**********************************
 * FILE NAME: MP1Node.cpp
zsh:1: command not found: 4
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, 
                &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
        #ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
        #endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
        #ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
        #endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	//int id = *(int*)(&memberNode->addr.addr);
	//int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    	// node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 1;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
    #ifdef DEBUGLOG
    static char s[1024];
    #endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
        #ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
        #endif
        memberNode->inGroup = true;
    }
    else {
	    MessageHdr *msgToSend;
        int msgSize     = sizeof(MessageHdr) + sizeof(node);
        msgToSend       = (MessageHdr *) malloc(sizeof(char) * msgSize);
        long heartbeat  = memberNode->heartbeat;
        int id          = *(int *)(&(memberNode->addr.addr));
        short port      = *(short*)(&(memberNode->addr.addr[4]));
        long timestamp  = memberNode->timeOutCounter;

        node *payload         = (node *) malloc(1 * sizeof(node));
        payload->id           = id;
        payload->port         = port;
        payload->heartbeat    = heartbeat;
        payload->timestamp    = timestamp;

        // create JOINREQ message: format of data is {struct Address myaddr}
        msgToSend->msgType   = JOINREQ;
        msgToSend->nodeCount = 1;
        msgToSend->msgId     = (rand() % MAX_MSG) + 1;
        msgToSend->size      = msgSize;
        msgToSend->src       = memberNode->addr;
        msgToSend->dest      = *joinaddr;
        
        //memcpy((((char *)msgToSend) + sizeof(MessageHdr)), payload, sizeof(sizeof(node)));
        memcpy((msgToSend + 1), payload, sizeof(sizeof(node)));

        #ifdef MsgCompressDbgLog
            printf("<------------------------------------------------->\n");
            printAddress(&memberNode->addr);
            printf("Message ID :- %d \n", msgToSend->msgId);
            printf("Information Added: ID: %d, Port: %hi, HeartBeat: %ld"
                ", Timestamp: %ld\n", payload->id, payload->port,
                         payload->heartbeat, payload->timestamp);
            printf("<------------------------------------------------->\n");
        #endif

        #ifdef MsgDbgLog
        char msgName[] = "MsgSend"; 
        printAddress(&memberNode->addr);
        printMessage(msgToSend, msgSize, msgName);
        #endif

        #ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
        #endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msgToSend, msgSize);

        free(msgToSend);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
	memberNode->bFailed = true;
	memberNode->inited = false;
	memberNode->inGroup = false;
    	// node is down!
	memberNode->nnb = -1;
	memberNode->heartbeat = -1;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;

	return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	//Member *member      = (Member *)env;

    /* Copy Header */
    MessageHdr *msgRecv = (MessageHdr *) malloc(sizeof(MessageHdr));
	memcpy(msgRecv, data, sizeof(MessageHdr));

    /* Copy Payload */
    node *payloadRecv   = (node *) malloc (msgRecv->nodeCount * sizeof(MessageHdr));
    memcpy(payloadRecv, ((char *)data + sizeof(MessageHdr)), (msgRecv->nodeCount * sizeof(MessageHdr)));

	Address src         = memberNode->addr;
    Address dest        = msgRecv->src;
    //int msgRecvSize     = msgRecv->size;
	int id;
	short port;
	long heartbeat;
    long timestamp;
    vector<MemberListEntry>::iterator itr;

    #ifdef DEBUGLOG
    static char s[1024];
    #endif

    #ifdef MsgDbgLog
        char msgName[] = "MsgRecv"; 
        printf ("Size of message Recieved : %d\n", size);
        printAddress(&memberNode->addr);
        printMessage(data, size, msgName);
    #endif 

    /* Validating Message */
    if (!(src == msgRecv->dest)) {
        printAddress(&memberNode->addr);
        printf("Node Address : ");
        printAddress(&src);
        printf("Destination Address in Message: ");
        printAddress(&msgRecv->dest);
        printf("Invalid Message :Dest Address in Message does not match Node"
                "Address\n");
        return false;
    }

    #ifdef DEBUGLOG
        sprintf(s, "Recieved Call Back...");
        log->LOG(&memberNode->addr, s);
    #endif

    #ifdef MemberListDbgLog
        printAddress(&memberNode->addr);
        if (!memberNode->memberList.empty()) {
            printMembershipList();
        }
    #endif 

	switch(msgRecv->msgType) {
		case JOINREQ : 
			{

                #ifdef DEBUGLOG
                    id = *(int *)(&src.addr);
                    port = *(short *)(&src.addr[4]);
				    sprintf(s, "Received JOINREQ Message MsgID: %d from ID :%d and Port: %hi",
                        msgRecv->msgId, id, port);
				    log->LOG(&memberNode->addr, s);
                #endif
				/* 
				 * 1. Add the member which has send JOINREQ to its membership 
				 * 2. Iterate through the list and add info of each member in the message 
				 * 3. Send the message with code JOINREP to the sender of the JOINREQ  
				 */

                id          = payloadRecv->id;
                port        = payloadRecv->port;
                heartbeat   = payloadRecv->heartbeat;
                timestamp   = payloadRecv->timestamp;

                #ifdef MsgDecompressREQDbgLog
                    printf("JOINREQ \n");
                    printAddress(&memberNode->addr);
                    printf("Message ID :- %d \n", msgRecv->msgId);
                    printf("Information Extracted: ID: %d, Port: %hi, HeartBeat: %ld"
                        ", Timestamp: %ld\n", id, port, heartbeat, timestamp);
                #endif
           
                if ((id != *(int *)(&memberNode->addr.addr))) {
                    addMemberToList(id, port, heartbeat, memberNode->timeOutCounter);
                    #ifdef NodeAddREQLog
                        printf("JOINREQ \n");
                        printAddress(&memberNode->addr);
                        printf("Adding Node ID: %d, Port: %hi, HeartBeat: %ld, Timestamp: %ld\n",
                                id, port, heartbeat, memberNode->timeOutCounter);
                    #endif
                } 

                // Number of nodes in Membership List 
                // include ownself also 
                int memberListLength  = memberNode->memberList.size() + 1;
                 //ID -> Int, Port -> Short, HeartBeat -> Long 
                 //Timestamp -> Long 
                int memberInfoSize    = sizeof(node);
                size_t msgToSendSize  = sizeof(MessageHdr) + 
                    (memberListLength * memberInfoSize);
                MessageHdr *msgToSend = (MessageHdr *) malloc 
                    (msgToSendSize * sizeof(char));
                node PayloadToSend[memberListLength]; 

                int index = 0;

                /* Message Header Filling */
                msgToSend->msgType      = JOINREP;
                msgToSend->nodeCount    = memberListLength;
                msgToSend->msgId        = (rand() % MAX_MSG) + 1;
                msgToSend->size         = msgToSendSize;
                msgToSend->src          = src;
                msgToSend->dest         = dest;

                id   = *(int *)(&(memberNode->addr.addr));
                port = *(short *)(&(memberNode->addr.addr[4]));

                // Add own self first in message 
                PayloadToSend[index].id           = id;
                PayloadToSend[index].port         = port;
                PayloadToSend[index].heartbeat    = memberNode->heartbeat;
                PayloadToSend[index].timestamp    = memberNode->timeOutCounter;
                #ifdef MsgCompressREQDbgLog
                    printf("JOINREQ \n");
                    printf("Message ID :- %d \n", msgToSend->msgId);
                    printf("Information Added: ID: %d, Port: %hi, HeartBeat: %ld"
                        ", Timestamp: %ld\n", PayloadToSend[index].id,
                        PayloadToSend[index].port, PayloadToSend[index].heartbeat,
                        PayloadToSend[index].timestamp);
                #endif
                char *ptr = (char *)msgToSend + sizeof(MessageHdr);
                memcpy(ptr, (char *)(&PayloadToSend[index]), sizeof(node));
                
                index++;
                
                for (itr = memberNode->memberList.begin(); 
                        itr != memberNode->memberList.end(); itr++) {
                    PayloadToSend[index].id           = itr->id;
                    PayloadToSend[index].port         = itr->port;
                    PayloadToSend[index].heartbeat    = itr->heartbeat;
                    PayloadToSend[index].timestamp    = itr->timestamp;

                    #ifdef MsgCompressREQDbgLog
                        printf("JOINREQ \n");
                        printAddress(&memberNode->addr);
                        printf("Message ID :- %d \n", msgToSend->msgId);
                        printf("Information Added: ID: %d, Port: %hi, HeartBeat: %ld"
                            ", Timestamp: %ld\n", PayloadToSend[index].id,
                            PayloadToSend[index].port, PayloadToSend[index].heartbeat,
                            PayloadToSend[index].timestamp);

                    #endif
                    ptr = ptr + sizeof(node);
                    memcpy(ptr, (char *)(&PayloadToSend[index]), sizeof(node));

                    index++;
                }


                #ifdef MsgREQDbgLog
                    printf("JOINREQ \n");
                    char msgName[] = "MsgSend"; 
                    printAddress(&memberNode->addr);
                    printMessage(msgToSend, msgToSendSize, msgName);
                #endif

				// send JOINREQ message to introducer member
				emulNet->ENsend(&memberNode->addr, &msgToSend->dest, (char *)msgToSend
                    , msgToSend->size);

                #ifdef DEBUGLOG
                    id   = *(int *)(&(msgToSend->dest.addr));
                    port = *(short *)(&(msgToSend->dest.addr[4]));
				    sprintf(s, "Sent JOINREP Message: msgId: %d and MsgSize : %d "
                        "to ID : %d and Port : %hi \n", 
                        msgToSend->msgId, msgToSend->size,
                        id, port);
				    log->LOG(&memberNode->addr, s);
                #endif

                //free the message 
                free(msgToSend);

                #ifdef MemberListREQDbgLog
                    printf("JOINREQ \n");
                    printf("Message recieved was JOINREQ \n");
                    printAddress(&memberNode->addr);
                    if (!memberNode->memberList.empty()) {
                        printMembershipList();
                    }
                #endif 

				break;
			}
		case JOINREP :
			{
				/* 
				 * 1. set the memberNode->inGroup  to 1 
				 * 2. Get the members in the message and add to the membershiplist.
				 */ 
				memberNode->inGroup = true;

                for (int i = 0; i < msgRecv->nodeCount; i++) {
                    id          = (payloadRecv + i) -> id;
                    port        = (payloadRecv + i) -> port;
                    heartbeat   = (payloadRecv + i) -> heartbeat;
                    timestamp   = (payloadRecv + i) -> timestamp;

                    #ifdef MsgDecompressREPDbgLog
                        printf("JOINREP\n");
                        printAddress(&memberNode->addr);
                        printf("Message ID :- %d \n", msgRecv->msgId);
                        printf("Information Extracted: ID: %d, Port: %hi, HeartBeat"
                            ": %ld, Timestamp: %ld\n", 
                            id, port, heartbeat, timestamp);
                    #endif
                    // Do not add yourself 
                    int self_id     = *(int *) (&(memberNode->addr.addr));
                    if (id == self_id) 
                        continue;
                    
                    addMemberToList(id, port, heartbeat, timestamp);
                    #ifdef NodeAddREPLog
                        printf("JOINREP\n");
                        printAddress(&memberNode->addr);
                        printf("Adding Node ID: %d, Port: %hi, HeartBeat: %ld, Timestamp: %ld\n",
                                id, port, heartbeat, timestamp);
                    #endif
                }

                #ifdef DEBUGLOG
                    id = *(int *)(&(msgRecv->src.addr));
                    port = *(short *)(&(msgRecv->src.addr[4]));
				    sprintf(s, "Received JOINREP Message MsgID %d from id : %d"
                        ", Port: %hi\n", msgRecv->msgId, id, port);
				    log->LOG(&memberNode->addr, s);
                    id = *(int *)(&(memberNode->addr.addr));
                    port = *(short *) (&(memberNode->addr.addr[4]));
				    sprintf(s, "Node with ID : %d Port : %hi Joined the group\n", id, port);
				    log->LOG(&memberNode->addr, s);
                #endif

                #ifdef MemberListREPDbgLog
                    printf("JOINREP \n");
                    printAddress(&memberNode->addr);
                    if (!memberNode->memberList.empty()) {
                        printMembershipList();
                    }   
                #endif 
	            
				break;
			}
        case HEARTBEAT :
            {
            /* 
             * Read the message 
             * Update the heartbeat in the membership list table 
             */

                bool is_Present = false;
                id          = payloadRecv->id;
                port        = payloadRecv->port;
                heartbeat   = payloadRecv->heartbeat;
                timestamp   = payloadRecv->timestamp;


                for (itr = memberNode->memberList.begin(); 
                        itr != memberNode->memberList.end(); itr++) {
                // Already present in the list 
                if (itr->id == id) {
                    // information is latest, we discard the message 
                    is_Present = true;
                      if (itr->heartbeat > heartbeat) {
                        #ifdef MemberListHBDbgLog
                            printf("HeartBeat \n");
                            printf("Message Recieved : HEARTBEAT \n");
                            printf("HeartBeat Information %ld in message is "
                                    "not the latest %ld\n", heartbeat, itr->heartbeat);
                            printAddress(&memberNode->addr);
                            if (!memberNode->memberList.empty()) {
                                printMembershipList();
                            }
                        #endif 
                        return true;
                  } else {
                        // Information is old and we need to update to the latest 
                        itr->heartbeat = heartbeat;
                        itr->timestamp = memberNode->timeOutCounter;
                        #ifdef MemberListHBDbgLog
                            printf("HeartBeat \n");
                            printf("Message Recieved : HEARTBEAT \n");
                            printf("HeartBeat Information %ld in message is "
                                    "the latest %ld, updating\n", heartbeat, itr->heartbeat);
                            printAddress(&memberNode->addr);
                            if (!memberNode->memberList.empty()) {
                                printMembershipList();
                            }
                        #endif 
                        return true;
                    }
                }
            }

            // We should not have reached, ow the node is in the list 
            // we need to add the node 
            if (is_Present == false) {
                addMemberToList(id, port, heartbeat, memberNode->timeOutCounter);
                #ifdef NodeAddHBLog
                    printf("HeartBeat \n");
                    printf("Added new Member due to HeartBeat : %d, %hi, %ld, %ld\n",
                        id, port, heartbeat, memberNode->timeOutCounter);
                    printf("Message Recieved : HEARTBEAT \n");
                    printAddress(&memberNode->addr);
                    if (!memberNode->memberList.empty()) {
                        printMembershipList();
                    }   
                #endif
            }
        }
        default :
            bool is_present = false;
            for (int i = 0; i < msgRecv->nodeCount; i++) {
                id          = (payloadRecv + i) -> id;
                port        = (payloadRecv + i) -> port;
                heartbeat   = (payloadRecv + i) -> heartbeat;
                timestamp   = (payloadRecv + i) -> timestamp;

                // Do not add yourself
                int self_id     = *(int *) (&(memberNode->addr.addr));
                if (id == self_id)
                    continue;

                for (itr = memberNode->memberList.begin();
                        itr != memberNode->memberList.end(); itr++) {
                    // Already present in the list
                    if (itr->id == id) {
                        is_present = true;
                        // information is latest, we discard the message
                        if (itr->heartbeat < heartbeat) {
                            itr->heartbeat = heartbeat;
                        } 
                        if (itr->timestamp < timestamp) {
                            itr->timestamp = timestamp;
                        }
                    }
                    if(is_present == false) {
                        addMemberToList(id, port, heartbeat, timestamp);
                    }
                }
            }
	}
	return true;
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then 
 *              delete the nodes
 * 	            Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/* 
	 * 1. Check Ping Counter 
	 * 2. If pingcounter is 0 then increament the hearbeat and send the
	 *    hearbeat to each of the member in the list. Set the ping counter
	 *    to TFAIL
	 * 3. Iterate through each member and Check the difference of the timestamp
	 *    of each member and the node's timeOutCounter.
	 * 4. if diffrence is greater than TREMOVE, then remove the node from the 
	 *    list of member. 
	 * 5. Increament the timeOutCounter of the node. 
	 */

    
    vector<MemberListEntry>::iterator itr;
    int HBmsgToSendSize = sizeof(MessageHdr) + sizeof(node);
    Address dest, nodeAddress;
    int id;
    short port;
    long heartbeat;
    long timestamp;
    id      = *(int *)(&(memberNode->addr.addr));
    port    = *(short *)(&(memberNode->addr.addr[4]));
    heartbeat = memberNode->heartbeat;
    timestamp = memberNode->timeOutCounter;


    if (memberNode->pingCounter == 0) {
        memberNode->heartbeat++;

        for (itr = memberNode->memberList.begin(); 
                itr != memberNode->memberList.end(); itr++) {

            dest = getAddress(itr->id, itr->port);
            //  Send HeartBeat Message 
            MessageHdr *HBmsgToSend = (MessageHdr *) malloc(sizeof(char) * HBmsgToSendSize);
             // Message Header Filling 
            HBmsgToSend->msgType      = HEARTBEAT;
            HBmsgToSend->nodeCount    = 1;
            HBmsgToSend->msgId        = (rand() % MAX_MSG) + 1;
            HBmsgToSend->size         = HBmsgToSendSize;
            HBmsgToSend->src          = memberNode->addr;
            HBmsgToSend->dest         = dest;

            // Get the Payload 
            node PayloadToSend;
            PayloadToSend.id           = id;
            PayloadToSend.port         = port;
            PayloadToSend.heartbeat    = heartbeat;
            PayloadToSend.timestamp    = timestamp;

            char *ptr = (char *) HBmsgToSend + sizeof(MessageHdr);

            memcpy(ptr, &PayloadToSend, sizeof(node));

            // send HeartBeat message to node
            emulNet->ENsend(&memberNode->addr, &HBmsgToSend->dest, (char *)HBmsgToSend
                     , HBmsgToSend->size);

            #ifdef MsgDbgLog
                printf("Sending Heartbeat from the nodeloops\n");
                char msgName[] = "MsgSend"; 
                printAddress(&memberNode->addr);
                printMessage(HBmsgToSend, HBmsgToSendSize, msgName);
            #endif
            free(HBmsgToSend);

        }
        // reset the ping counter to TFAIL => wait till counter goes to zero 
        memberNode->pingCounter = TFAIL;

    } else {
        // Decrement the pingCounter 
        memberNode->pingCounter --;
    }
    /* Failure Detection & Accuracy */

    for (itr = memberNode->memberList.begin(); itr !=
            memberNode->memberList.end(); itr++) {

        // Make sure that u are not in list 

        if (itr->id != id && itr->port != port) {
                if (memberNode->timeOutCounter - itr->timestamp > TREMOVE) {
                    // Failure Detection :- We have to remove this node from list 
                    memberNode->memberList.erase(itr);

                    #ifdef DEBUGLOG
                        nodeAddress = getAddress(itr->id, itr->port);
                        log->logNodeRemove(&memberNode->addr, &nodeAddress);
                    #endif 

                    #ifdef MemberListDbgLog
                        printf("Failure Detection: Removing the node: Id %d, Port %hi, "
                            "HB %ld, TimeStamp %ld \n", itr->id, itr->port,
                            itr->heartbeat, itr->timestamp);
                        printAddress(&memberNode->addr);
                        if (!memberNode->memberList.empty()) {
                            printMembershipList();
                        }
                    #endif 

                }
        }
    }


    // We increment the timeOutCounter 
    memberNode->timeOutCounter++;

    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

void MP1Node:: addMemberToList(int id, short port, long heartbeat, long 
        timestamp) {
    Address addr                 = getAddress(id, port);
    MemberListEntry *memberToAdd = new MemberListEntry();
    vector<MemberListEntry>::iterator itr;

    #ifdef MemberAddDbgLog
        printAddress(&memberNode->addr);
        printf("Memebershiplist of Node before adding/updating/donothing to 
            List\n");
        printMembershipList();
        printf("Details of Node To Add : Id : %d, Port : %hi, HeartBeat: %ld"
            "TimeStamp: %ld\n", id, port, heartbeat, timestamp);
    #endif
   
    /* Check if node is already present */
    for (itr = memberNode->memberList.begin(); 
            itr != memberNode->memberList.end(); itr++) {

        // Node is present 
        if (itr->id == id) {
                #ifdef MemberAddDbgLog
                    printAddress(&memberNode->addr);
                    printf("Details of Node Already Present: Id : %d, Port: %hi," 
                        "HeartBeat : %ld, TimeStamp: %ld\n", itr->id, itr->port,
                        itr->heartbeat, itr->timestamp);
                #endif
                return;
        }
    }

   // We reach here and we have verrified that node is not present 
   // Filling Information to Add in the membership List 
    memberToAdd->id        = id;
    memberToAdd->port      = port;
    memberToAdd->heartbeat = heartbeat;
    memberToAdd->timestamp = timestamp;

    /* Appending the node in the list */
    memberNode->memberList.push_back(*memberToAdd);

    #ifdef DEBUGLOG
	    log->logNodeAdd(&memberNode->addr, &addr);
    #endif

    #ifdef MemberAddDbgLog
        printAddress(&memberNode->addr);
        printf("Adding the Node: Id : %d and Port :"
            "%hi, HeartBeat: %ld, TimeStamp: %ld at last" 
            "in the membership List\n", id, port, heartbeat, timestamp);
        printf("Memnbership list after adding the new node \n");
        printMembershipList()
    #endif 
}

Address MP1Node::getAddress(int id, short port) {
    Address addr;
    memset(&addr, 0, sizeof(Address));

    *(int *)(&addr.addr) = id;
    *(short *)(&addr.addr[4]) = port;
    
    return addr;
}


/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                            addr->addr[3], *(short*)&addr->addr[4]) ;    
}


void MP1Node::printMembershipList() {
    vector<MemberListEntry>::iterator itr;
    int index = 0;

    printf("<---------------------------------------------------------------->\n");
    printf("Membership List for Member : ");
    printAddress(&memberNode->addr);
    for (itr = memberNode->memberList.begin(); itr != memberNode->memberList.end();
            itr++) {
        printf("Index : %d, ID : %d, Port: %hi, HeartBeat: %ld, TimeStamp: %ld\n", 
                index++, itr->id, itr->port, itr->heartbeat, itr->timestamp);
    }
    printf("<---------------------------------------------------------------->\n");
}

void MP1Node::printMessage(void *data, int size, char *msg_info) {
    /* Copy Header */
    MessageHdr *msg      = (MessageHdr *) malloc(sizeof(MessageHdr));
    memcpy(msg, data, sizeof(MessageHdr));
    /* Copy Payload */
    node payload[msg->nodeCount];
    char *ptr = (char *)data + sizeof(MessageHdr);
    for (int i = 0; i < msg->nodeCount; i++) {
        memcpy((char *)(&payload[i]), ptr, sizeof(node));
        ptr = ptr + sizeof(node);
    }
    //memcpy(payload, (msg + 1), sizeof(msg->nodeCount * sizeof(node)));
    
    int id;
    short port;
    long timestamp;
    long heartbeat;

    printf("<------------------------Message Info------------------------->\n");
    printf("Message Op : %s", msg_info); 

    if (msg->msgType == JOINREP) {
        printf("  Type : JOINREP\n");
    } else if (msg->msgType == JOINREQ) {
        printf("  Type : JOINREQ\n");
    } else if (msg->msgType == HEARTBEAT) {
        printf("  Type : HEARTBEAT\n");
    } else {
        printf("  Type : Unknown\n");
    }
    printf("Src : ");
    printAddress(&msg->src);
    printf("Dest : ");
    printAddress(&msg->dest);
    printf("ID : %d", msg->msgId);
    printf("   Size :  %d Bytes", msg->size);
    printf("   Node Count : %d \n", msg->nodeCount);

    printf("Nodes Present in the Message: \n");

    for(int i = 0; i < msg->nodeCount; i++) {
        id        = payload[i].id;
        port      = payload[i].port;
        heartbeat = payload[i].heartbeat;
        timestamp = payload[i].timestamp;

        printf("Index : %d, ID: %d, Port: %hi, HeartBeat: %ld, TimeStamp: %ld\n",
                i, id, port, heartbeat, timestamp);
    }
    printf("<------------------------------------------------------------->\n");
}
