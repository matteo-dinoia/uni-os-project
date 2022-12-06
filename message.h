#ifndef _MESSAGE_H
#define _MESSAGE_H

#define MSG_SIZE(msg) (sizeof(msg) - sizeof(long))

struct commerce_msgbuf{
	long receiver;
	long sender;

};

struct bump_msgbuf{
	long mtype;
};

#endif
