#ifndef _MESSAGE_H
#define _MESSAGE_H

#define KEY_MSG_IN_PORT 0x210fff
#define KEY_MSG_OUT_PORT 0x220fff

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
bool_t receive_commerce_msg(id_shared_t id, int type, int *sender_id, int *cargo_type, int *amount, int *expiry_date, int *status, bool_t restarting);


#endif
