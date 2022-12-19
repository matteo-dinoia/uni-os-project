#define _GNU_SOURCE
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <errno.h>
#include "header/message.h"
#include "header/shared_mem.h"

struct commerce_msgbuf create_commerce_msgbuf(long sender, long receiver)
{
	struct commerce_msgbuf res;
	bzero(&res, sizeof(res));

	res.receiver = receiver + 1;
	res.sender = sender + 1;

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
	msg->expiry_date = expiry_date;
	msg->status = status;
}

void send_commerce_msg(int id, const struct commerce_msgbuf *msg)
{
	dprintf(1, "[SEND on id %d, from %d to %d] type: %d amount: %d expiry-date: %d\n",
		id, msg->sender, msg->receiver, msg->cargo_type, msg->n_cargo_batch, msg->expiry_date);
	msgsnd(id, msg, MSG_SIZE(*msg), 0);
}

void receive_commerce_msg(int id, struct commerce_msgbuf *msg, int type)
{
	dprintf(1, "[RECEIVE on id %d, type %d] initialize\n", id, type + 1);

	msgrcv(id, msg, MSG_SIZE(*msg), 0, 0);

	if(errno != EXIT_SUCCESS)
		dprintf(1, "Error: %d, meaning:%s\n", errno, strerror(errno));
	else
		dprintf(1, "[RECEIVE on id %d, from %d to %d] type: %d amount: %d expiry-date: %d\n",
				id, msg->sender, msg->receiver, msg->cargo_type, msg->n_cargo_batch, msg->expiry_date);
}
