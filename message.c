#define _GNU_SOURCE
#include <strings.h>
#include <stdlib.h>
#include "message.h"

struct commerce_msgbuf create_commerce_msgbuf(long sender, long receiver)
{
	struct commerce_msgbuf res;
	bzero(&res, sizeof(res));

	res.receiver = receiver;
	res.sender = sender;

	return res;
}

struct commerce_msgbuf respond_commerce_msgbuf(const struct commerce_msgbuf *msg_to_respond)
{
	return create_commerce_msgbuf(msg_to_respond->receiver, msg_to_respond->sender);
}

void set_commerce_msgbuf(struct commerce_msgbuf *msg, int type, int amount, int expiry_date, int status)
{
	if(msg == NULL) return;

	msg->cargo_type = type;
	msg->n_cargo_batch = amount;
	msg->n_cargo_batch = amount;
	msg->expiry_date = expiry_date;
	msg->status = status;
}
