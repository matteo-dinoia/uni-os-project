#include <string.h>
#include "message.h"

struct commerce_msgbuf create_commerce_msgbuf(long sender, long receiver)
{
	struct commerce_msgbuf res;
	bzero(res, sizeof(res));

	res.receiver = receiver;
	res.sender = sender;

	return res;
}
