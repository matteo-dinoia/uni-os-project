#ifndef _MESSAGE_H
#define _MESSAGE_H

#define MSG_SIZE(msg) (sizeof(msg) - sizeof(long))

/* Prototype */
struct commerce_msgbuf create_commerce_msgbuf(long, long);

struct commerce_msgbuf{
	long receiver;
	long sender;

};

struct bump_msgbuf{
	long mtype;
};

#endif
