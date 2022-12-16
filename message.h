#ifndef _MESSAGE_H
#define _MESSAGE_H

#define MSG_SIZE(msg) (sizeof(msg) - sizeof(long))
#define STATUS_REQUEST 0
#define STATUS_ACCEPTED 1
#define STATUS_REFUSED 2
#define STATUS_REDUCED 3

/* Prototype */
struct commerce_msgbuf create_commerce_msgbuf(long, long);

struct commerce_msgbuf{
	long receiver;
	long sender;
	int cargo_type;
	int n_cargo_batch; /* Can be positive or negative */
	int status;

};

struct bump_msgbuf{
	long mtype;
};

#endif
