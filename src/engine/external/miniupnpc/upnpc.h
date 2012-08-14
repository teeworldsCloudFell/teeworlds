#ifndef _UPNPC_H_
#define _UPNPC_H_

#ifdef __cplusplus
extern "C"{
#endif

int SetupPortForward(unsigned short ExtPort, int UseIPv6);
int DeletePortForward(unsigned short ExtPort);

#ifdef __cplusplus
}
#endif
#endif
