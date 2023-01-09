#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <errno.h>
#include "header/message.h"

#define MSG_TYPE(type) ((type) + 1)
#define MSG_DEC_TYPE(type) ((type) - 1)

void create_commerce_msgbuf(struct commerce_msgbuf *msg, long sender, long receiver, int type, int amount, int expiry_date, int status)
{
	if(msg == NULL) return;

	msg->receiver = MSG_TYPE(receiver);
	msg->sender = MSG_TYPE(sender);

	msg->cargo_type = type;
	msg->n_cargo_batch = amount;
	msg->expiry_date = expiry_date;
	msg->status = status;
}

void send_commerce_msg(id_shared_t id, const struct commerce_msgbuf *msg)
{
	int return_value;
	do {
		return_value = msgsnd(id, msg, MSG_SIZE(*msg), 0);
	}while (return_value < 0);
}

bool_t receive_commerce_msg(id_shared_t id, int type, int *sender_id, int *cargo_type, int *amount, int *expiry_date, int *status, bool_t restarting)
{
	struct commerce_msgbuf msg;
	int return_value;

	do {
		return_value = msgrcv(id, &msg, MSG_SIZE(msg), MSG_TYPE(type), 0);
		if (!restarting && return_value < 0)
			return FALSE;
	}while (return_value < 0);

	/* Only if different from null save data */
	if(sender_id != NULL) *sender_id = MSG_DEC_TYPE(msg.sender);
	if(cargo_type != NULL) *cargo_type = msg.cargo_type;
	if(amount != NULL) *amount = msg.n_cargo_batch;
	if(expiry_date != NULL) *expiry_date = msg.expiry_date;
	if(status != NULL) *status = msg.status;

	return TRUE;
}
