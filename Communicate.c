/******************************************************************************
  Copyright (c) 2013 Morten Houmøller Nygaard - www.mortz.dk - admin@mortz.dk
 
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include "Communicate.h"

/** 
 * Converts the unsigned 64 bit integer from host byte order to network byte 
 * order.
 */
uint64_t ntohl64(uint64_t value) {
	static const int num = 42;

	/**
	 * If these check is true, the system is using the little endian 
	 * convention. Else the system is using the big endian convention, which
	 * means that we do not have to represent our integers in another way.
	 */
	if (*(char *)&num == 42) {
		const uint32_t high = (uint32_t)(value >> 32);
		const uint32_t low = (uint32_t)(value & 0xFFFFFFFF);

		return (((uint64_t)(htonl(low))) << 32) | htonl(high);
	} else {
		return value;
	}	
}


/**
 * This function is suppose to get the remaining part of the message, if
 * the message from the client is too big to be contained in the buffer.
 */
uint64_t getRemainingMessage(struct node *n, uint64_t msg_length) {
	int buffer_length = 0; 
	uint64_t remaining_length = 0, final_length = 0;
	char buffer[BUFFERSIZE];
	struct message *m = n->message;

	do {
		memset(buffer, '\0', BUFFERSIZE);
	
		/**
		 * Receive new chunk of the message.
		 */	
		if ((buffer_length = recv(n->socket_id, buffer, BUFFERSIZE, 0)) <= 0) {
			printf("Didn't receive anything from remaining part of message. %d"
					"\n\n", buffer_length);
			fflush(stdout);
			return 0;	
		}

		/**
		 * The overall length of the message received. Because the recv call
		 * eventually will merge messages together we have to have a check
		 * whether the overall length we received is greater than the expected
		 * length of the message.
		 */ 
	 	final_length = (msg_length+remaining_length+buffer_length);	

		if (DEBUG_MESSAGES) {
			printf("DEBUG: final_length = %d\n", (int) final_length);
			fflush(stdout);
		}

		/**
		 * If the overall message is longer than the expected length of the
		 * message, we now that this chunk most contain the last part of the
		 * first message, and the first chunk of the new message.
		 */
		if ( final_length > m->len ) {

			uint64_t next_len = final_length-m->len;
			char *temp = (char *) malloc(sizeof(char)*next_len);
			
			if (DEBUG_MESSAGES) {
				printf("We have received a larger message than expected.\n");
				printf("DEBUG: final_length - m->len = %d\n", (int) next_len);
				fflush(stdout);
			}

			if (temp == NULL) {
				printf("6: Couldn't allocate memory.\n\n");
				fflush(stdout);
				return 0;
			}
			memset(temp, '\0', next_len);
			m->next = temp;
			temp = NULL;

		   	memcpy(m->next, buffer + (buffer_length - next_len), next_len);
			m->next_len = next_len;
			
			buffer_length = buffer_length - next_len;
		}

		if (DEBUG_MESSAGES) {
			printf("DEBUG: remaining_length = %d\n", (int) remaining_length);
			fflush(stdout);
		}

		remaining_length += buffer_length;

		if (DEBUG_MESSAGES) {
			printf("DEBUG: remaining_length += buffer_length = %d\n", 
					(int) remaining_length);
			fflush(stdout);
		}

		memcpy(m->msg + (msg_length+(remaining_length-buffer_length)), buffer, 
				buffer_length);

		if (DEBUG_MESSAGES) {
			printf("DEBUG: msg_length + remaining_length = %d\n", 
					(int) (remaining_length + msg_length));
			fflush(stdout);
		}		
	} while( (msg_length + remaining_length) < m->len );

	return remaining_length;
}

int parseMessage(char *buffer, uint64_t buffer_length, struct node *n) {
	struct message *m = n->message;
	int length, has_mask, skip, j;
	uint64_t message_length = m->len, i, remaining_length = 0, buf_len;
	char *temp = NULL;

	/**
	 * Extracting information from frame
	 */
	has_mask = buffer[1] & 0x80 ? 1 : 0;
	length = buffer[1] & 0x7f;

	if (!has_mask) {
		printf("Message didn't have masked data, received: 0x%x\n\n", 
				buffer[1]);
		fflush(stdout);
		return -1;
	}

	if (length <= 125) {
		m->len += length;	
		skip = 6;
		memcpy(&m->mask, buffer + 2, sizeof(m->mask));
	} else if (length == 126) {
		uint16_t sz16;
		memcpy(&sz16, buffer + 2, sizeof(uint16_t));

		m->len += ntohs(sz16);

		skip = 8;
		memcpy(&m->mask, buffer + 4, sizeof(m->mask));
	} else if (length == 127) {
		uint64_t sz64;
		memcpy(&sz64, buffer + 2, sizeof(uint64_t));

		m->len += ntohl64(sz64);

		skip = 14;
		memcpy(&m->mask, buffer + 10, sizeof(m->mask));
	} else {
		printf("Obscure length received from client: %d\n\n", length);
		fflush(stdout);
		return -1;	
	}

	if (DEBUG_MESSAGES) {
		printf("DEBUG: m->len = %d\n", (int) m->len);
		fflush(stdout);
	}
	
	/**
	 * Allocating memory to hold the message sent from the client.
	 */ 
	temp = (char *) malloc(sizeof(char) * (m->len+1));
	if (temp == NULL) {
		printf("1: Couldn't allocate memory.\n\n");
		fflush(stdout);
		return -1;
	}
	memset(temp, '\0', (m->len + 1));
	m->msg = temp;
	temp = NULL;

	buf_len = (buffer_length-skip);

	if (DEBUG_MESSAGES) {
		printf("DEBUG: buf_len = %d\n", (int) buf_len);
		fflush(stdout);
	}

	/**
	 * The message read from recv is larger than the message we are supposed
	 * to receive. This means that we have received the first part of the next
	 * message aswell.
	 */
	if (buf_len > m->len) {
		uint64_t next_len = buf_len - m->len;
		
		if (DEBUG_MESSAGES) {
			printf("We have received a larger message than expected.\n");
			printf("DEBUG: buf_len - m->len = %d\n", (int) next_len);
			fflush(stdout);
		}

		temp = malloc(next_len);
		if (temp == NULL) {
			printf("2: Couldn't allocate memory.\n\n");
			fflush(stdout);
			return -1;
		}
		memset(temp, '\0', next_len);
		m->next = temp;
		temp = NULL;

		memcpy(m->next, buffer + (m->len+skip), next_len);
		m->next_len = next_len;

		if (DEBUG_MESSAGES) {
			printf("DEBUG: buffer[0] = %x\n", buffer[0]);
			printf("DEBUG: buffer[1] = %x\n", buffer[1]);
			printf("DEBUG: buffer[2] = %x\n", buffer[2]);
			fflush(stdout);
		}

		buf_len = m->len;	
	}

	memcpy(m->msg+message_length, buffer+skip, buf_len);

	message_length += buf_len;

	if (message_length < m->len) {

		if (DEBUG_MESSAGES) {
			printf("The buffer is too small to received whole message.\n");
			printf("DEBUG: message_length = %d\n", (int) message_length);
			fflush(stdout);
		}

		if ((remaining_length = getRemainingMessage(n, message_length)) == 0) {
			return -1;
		}
	}

	message_length += remaining_length;

	if (DEBUG_MESSAGES) {
		printf("DEBUG: message_length += remaining_length = %d\n", 
				(int) message_length);
		fflush(stdout);
	}

	if (message_length != m->len) {
		printf("Message does not fit. Expected: %d but got %d\n\n", 
				(int) m->len, (int) message_length);
		fflush(stdout);
		return -1;
	}

	for (i = 0, j = 0; i < message_length; i++, j++){
		m->msg[j] = m->msg[i] ^ m->mask[j % 4];
	}

	return 0;
}

int encodeMessage(struct message *m) {
	char *encoded = NULL;
	uint64_t length = m->len;

	if (m->len <= 125) {
		length += 2;
		encoded = (char *) malloc(sizeof(char) * length);
		if (encoded == NULL) {
			printf("3: Couldn't allocate memory.\n\n");
			fflush(stdout);
			return -1;
		}
		encoded[0] = '\x81';
		encoded[1] = m->len;
		memcpy(encoded + 2, m->msg, m->len);
	} else if (m->len <= 65535) {
		uint16_t sz16;
		length += 4;
		encoded = (char *) malloc(sizeof(char) * length);
		if (encoded == NULL) {
			printf("4: Couldn't allocate memory.\n\n");
			fflush(stdout);
			return -1;
		}
		encoded[0] = '\x81';
		encoded[1] = 126;
		sz16 = htons(m->len);
		memcpy(encoded + 2, &sz16, sizeof(uint16_t));
		memcpy(encoded + 4, m->msg, m->len);
	} else {
		uint64_t sz64;
		length += 10;
		encoded = (char *) malloc(sizeof(char) * length);
		if (encoded == NULL) {
			printf("5: Couldn't allocate memory.\n\n");
			fflush(stdout);
			return -1;
		}
		encoded[0] = '\x81';
		encoded[1] = 127;
		sz64 = ntohl64(m->len);
		memcpy(encoded + 2, &sz64, sizeof(uint64_t));
		memcpy(encoded + 10, m->msg, m->len);
	}

	m->enc = encoded;
	m->enc_len = length;
	encoded = NULL;
	return 0;
}

int communicate(struct node *n, char *next, uint64_t next_len) {
	int buffer_length = 0;
	uint64_t buf_len;
	char buffer[BUFFERSIZE];
	n->message = message_new();
	
	/*
	 * Receiving and decoding the message.
	 */
	do {
		memset(buffer, '\0', BUFFERSIZE);
			
		memcpy(buffer, next, next_len);

		if (next_len <= 6 || ((next[1] & 0x7f) == 126 && next_len <= 8) ||
				((next[1] & 0x7f) == 127 && next_len <= 14)) {

			if (DEBUG_MESSAGES) {
				printf("\nDEBUG: Reading from recv.\n");
				fflush(stdout);
			}

			if ((buffer_length = recv(n->socket_id, (buffer+next_len), 
							(BUFFERSIZE-next_len), 0)) <= 0) {
				printf("Didn't receive any message from client.\n\n");
				fflush(stdout);
				return -1;	
			}
		}

		buf_len = (uint64_t)(buffer_length + next_len);

		if (DEBUG_MESSAGES) {
			printf("DEBUG: buf_len = %d\n", (int) buf_len);
			fflush(stdout);
		}

		if (n->message->opcode[0] == '\0') {
			memcpy(n->message->opcode, buffer, sizeof(n->message->opcode));

			if (DEBUG_MESSAGES) {
				printf("DEBUG: opcode = %x\n", n->message->opcode[0]);
				fflush(stdout);
			}
		}

		if (parseMessage(buffer, buf_len, n) < 0) {
			return -1;
		}

		next_len = 0;
	} while( !(buffer[0] & 0x80) );	

	/**
	 * Checking which type of frame the client has sent.
	 */
	if (n->message->opcode[0] == '\x88' || n->message->opcode[0] == '\x08') {
		/* CLOSE: client wants to close connection, so we do. */
		printf("Client:\n"
			  "\tSocket: %d\n"
		  	  "\tAddress: %s\n"
			  "reports that he is shutting down.\n\n", n->socket_id, 
			  (char *) n->client_ip);
		fflush(stdout);
		
		send(n->socket_id, (char *) '\x88', 1, 0);
		return -1;
	} else if (n->message->opcode[0] == '\x8A' || n->message->opcode[0] == '\x0A') {
		/* PONG: Client is still alive */
		printf("Pong arrived\n\n");
		fflush(stdout);	
	} else if (n->message->opcode[0] == '\x89' || n->message->opcode[0] == '\x09') {
		/* PING: I am still alive */
		/* SEND PONG BACK */
		/* send(n->socket_id, (char *) '\x8A', 1, 0); */
		printf("Ping arrived\n\n");
		fflush(stdout);
	} else if (n->message->opcode[0] == '\x02' || n->message->opcode[0] == '\x82') {
		/* Binary data */
		printf("Binary data arrived\n\n");
		fflush(stdout);
	} else if (n->message->opcode[0] == '\x01' || n->message->opcode[0] == '\x81') {
		/* Text data */

		if (DEBUG_MESSAGES) {
			printf("DEBUG: %d - %s\n", (int) n->message->len, n->message->msg);
			fflush(stdout);
		}

		if (encodeMessage(n->message) > 0) {
			return -1;
		}
	} else {
		printf("Something very strange happened, received opcode: 0x%x\n\n", 
				n->message->opcode[0]);
		fflush(stdout);
		return -1;
	}


	return 0;
}
