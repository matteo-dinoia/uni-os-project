#ifndef _MESSAGE_H
#define _MESSAGE_H

#define MSG_SIZE(msg) (sizeof(msg) - 8)
#define STATUS_REFUSED -1
#define STATUS_REQUEST 0
#define STATUS_PARTIAL 1
#define STATUS_ACCEPTED 2

#include "utils.h"

struct commerce_msgbuf{
	long receiver;
	long sender;
	int cargo_type;
	int n_cargo_batch; /* Can be positive or negative */
	int expiry_date;
	int status;
};

/* Prototype */
void create_commerce_msgbuf(struct commerce_msgbuf *msg, long sender, long receiver, int type, int amount, int expiry_date, int status);
void send_commerce_msg(id_shared_t id, const struct commerce_msgbuf *msg);
void receive_commerce_msg(id_shared_t id, int type, int *sender_id, int *cargo_type, int *amount, int *expiry_date, int *status);

#endif
