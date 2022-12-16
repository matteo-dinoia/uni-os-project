#include <strings.h>
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
