/* $Id: upnpc.c,v 1.97 2012/06/23 23:16:00 nanard Exp $ */
/* Project : miniupnp
 * Author : Thomas Bernard
 * Copyright (c) 2005-2012 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution.
 * NOTICE: THIS FILE HAS BEEN ALTERED */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>
#define snprintf _snprintf
#else
/* for IPPROTO_TCP / IPPROTO_UDP */
#include <netinet/in.h>
#endif
#include <miniwget.h>
#include <miniupnpc.h>
#include <upnpcommands.h>
#include <upnperrors.h>
#include "upnpc.h"

/* Set to 1 to enable printing debug output */
#define DEBUG 0

/* protofix() checks if protocol is "UDP" or "TCP"
 * returns NULL if not */
const char * protofix(const char * proto)
{
	static const char proto_tcp[4] = { 'T', 'C', 'P', 0};
	static const char proto_udp[4] = { 'U', 'D', 'P', 0};
	int i, b;
	for(i=0, b=1; i<4; i++)
		b = b && (   (proto[i] == proto_tcp[i])
		          || (proto[i] == (proto_tcp[i] | 32)) );
	if(b)
		return proto_tcp;
	for(i=0, b=1; i<4; i++)
		b = b && (   (proto[i] == proto_udp[i])
		          || (proto[i] == (proto_udp[i] | 32)) );
	if(b)
		return proto_udp;
	return 0;
}

/* Test function
 * 1 - get connection type
 * 2 - get extenal ip address
 * 3 - Add port mapping
 * 4 - get this port mapping from the IGD */
static void SetRedirectAndTest(struct UPNPUrls * urls,
                               struct IGDdatas * data,
							   const char * iaddr,
							   const char * iport,
							   const char * eport,
                               const char * proto,
                               const char * leaseDuration)
{
	char externalIPAddress[40];
	char intClient[40];
	char intPort[6];
	char duration[16];
#if DEBUG
	int r;

	if(!iaddr || !iport || !eport || !proto)
	{
		fprintf(stderr, "Wrong arguments\n");
		return;
	}
#endif
	proto = protofix(proto);
#if DEBUG
	if(!proto)
	{
		fprintf(stderr, "invalid protocol\n");
		return;
	}
#endif

	UPNP_GetExternalIPAddress(urls->controlURL,
	                          data->first.servicetype,
							  externalIPAddress);
#if DEBUG
	if(externalIPAddress[0])
		printf("ExternalIPAddress = %s\n", externalIPAddress);
	else
		printf("GetExternalIPAddress failed.\n");

	r = UPNP_AddPortMapping(urls->controlURL, data->first.servicetype,
	                        eport, iport, iaddr, 0, proto, 0, leaseDuration);

	if(r!=UPNPCOMMAND_SUCCESS)
		printf("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
		       eport, iport, iaddr, r, strupnperror(r));

	r = UPNP_GetSpecificPortMappingEntry(urls->controlURL,
	                                 data->first.servicetype,
    	                             eport, proto,
									 intClient, intPort, NULL/*desc*/,
	                                 NULL/*enabled*/, duration);

	if(r!=UPNPCOMMAND_SUCCESS)
		printf("GetSpecificPortMappingEntry() failed with code %d (%s)\n",
		       r, strupnperror(r));

	if(intClient[0]) {
		printf("InternalIP:Port = %s:%s\n", intClient, intPort);
		printf("external %s:%s %s is redirected to internal %s:%s (duration=%s)\n",
		       externalIPAddress, eport, proto, intClient, intPort, duration);
	}
#else
	UPNP_AddPortMapping(urls->controlURL, data->first.servicetype,
	                        eport, iport, iaddr, 0, proto, 0, leaseDuration);

	UPNP_GetSpecificPortMappingEntry(urls->controlURL,
	                                 data->first.servicetype,
    	                             eport, proto,
									 intClient, intPort, NULL/*desc*/,
	                                 NULL/*enabled*/, duration);
#endif
}

static void RemoveRedirect(struct UPNPUrls * urls,
               struct IGDdatas * data,
			   const char * eport,
			   const char * proto)
{
#if DEBUG
	int r;

	if(!proto || !eport)
	{
		fprintf(stderr, "invalid arguments\n");
		return;
	}
	proto = protofix(proto);
	if(!proto)
	{
		fprintf(stderr, "protocol invalid\n");
		return;
	}

	r = UPNP_DeletePortMapping(urls->controlURL, data->first.servicetype, eport, proto, 0);

	printf("UPNP_DeletePortMapping() returned : %d\n", r);
#else
	UPNP_DeletePortMapping(urls->controlURL, data->first.servicetype, eport, proto, 0);
#endif
}

int SetupPortForward(unsigned short ExtPort, int UseIPv6)
{
	struct UPNPDev * devlist = 0;
	char lanaddr[64];	/* my ip address on the LAN */
	int i;
	int error = 0;
	int retcode = 0;

	char extPort[6];
	sprintf(extPort, "%hu", ExtPort);

	if((devlist = upnpDiscover(2000, 0, 0, 0/*sameport*/, UseIPv6, &error)))
	{
		struct UPNPUrls urls;
		struct IGDdatas data;
#if DEBUG
		struct UPNPDev * device;
		if(devlist)
		{
			printf("List of UPNP devices found on the network :\n");
			for(device = devlist; device; device = device->pNext)
			{
				printf(" desc: %s\n st: %s\n\n",
					   device->descURL, device->st);
			}
		}
		else
		{
			printf("upnpDiscover() error code=%d\n", error);
		}
#endif
		i = 1;
		if(UPNP_GetIGDFromUrl(0, &urls, &data, lanaddr, sizeof(lanaddr)) || (i = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr))))
		{
#if DEBUG
			switch(i) {
			case 1:
				printf("Found valid IGD : %s\n", urls.controlURL);
				break;
			case 2:
				printf("Found a (not connected?) IGD : %s\n", urls.controlURL);
				printf("Trying to continue anyway\n");
				break;
			case 3:
				printf("UPnP device found. Is it an IGD ? : %s\n", urls.controlURL);
				printf("Trying to continue anyway\n");
				break;
			default:
				printf("Found device (igd ?) : %s\n", urls.controlURL);
				printf("Trying to continue anyway\n");
			}
			printf("Local LAN ip address : %s\n", lanaddr);
#endif
			SetRedirectAndTest(&urls, &data, lanaddr, extPort, extPort, "udp", "0");
			FreeUPNPUrls(&urls);
		}
		else
		{
#if DEBUG
			fprintf(stderr, "No valid UPNP Internet Gateway Device found.\n");
#endif
			retcode = 1;
		}
		freeUPNPDevlist(devlist); devlist = 0;
	}
	else
	{
#if DEBUG
		fprintf(stderr, "No IGD UPnP Device found on the network !\n");
#endif
		retcode = 1;
	}
	return retcode;
}

int DeletePortForward(unsigned short ExtPort)
{
	struct UPNPDev * devlist = 0;
	char lanaddr[64];	/* my ip address on the LAN */
	int i;
	int ipv6 = 0;
	int error = 0;
	int retcode = 0;

	char extPort[6];
	sprintf(extPort, "%hu", ExtPort);

	if((devlist = upnpDiscover(2000, 0, 0, 0/*sameport*/, ipv6, &error)))
	{
		struct UPNPUrls urls;
		struct IGDdatas data;
#if DEBUG
		struct UPNPDev * device;
		if(devlist)
		{
			printf("List of UPNP devices found on the network :\n");
			for(device = devlist; device; device = device->pNext)
			{
				printf(" desc: %s\n st: %s\n\n",
					   device->descURL, device->st);
			}
		}
		else
		{
			printf("upnpDiscover() error code=%d\n", error);
		}
#endif
		i = 1;
		if(UPNP_GetIGDFromUrl(0, &urls, &data, lanaddr, sizeof(lanaddr)) || (i = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr))))
		{
#if DEBUG
			switch(i) {
			case 1:
				printf("Found valid IGD : %s\n", urls.controlURL);
				break;
			case 2:
				printf("Found a (not connected?) IGD : %s\n", urls.controlURL);
				printf("Trying to continue anyway\n");
				break;
			case 3:
				printf("UPnP device found. Is it an IGD ? : %s\n", urls.controlURL);
				printf("Trying to continue anyway\n");
				break;
			default:
				printf("Found device (igd ?) : %s\n", urls.controlURL);
				printf("Trying to continue anyway\n");
			}
			printf("Local LAN ip address : %s\n", lanaddr);
#endif
			RemoveRedirect(&urls, &data, extPort, "udp");
			FreeUPNPUrls(&urls);
		}
		else
		{
#if DEBUG
			fprintf(stderr, "No valid UPNP Internet Gateway Device found.\n");
#endif
			retcode = 1;
		}
		freeUPNPDevlist(devlist); devlist = 0;
	}
	else
	{
#if DEBUG
		fprintf(stderr, "No IGD UPnP Device found on the network !\n");
#endif
		retcode = 1;
	}
	return retcode;
}
