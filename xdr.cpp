
/*********************************************************************
 * RPC for the Windows NT Operating System
 * 1993 by Martin F. Gergeleit
 * Users may use, copy or modify Sun RPC for the Windows NT Operating
 * System according to the Sun copyright below.
 *
 * RPC for the Windows NT Operating System COMES WITH ABSOLUTELY NO
 * WARRANTY, NOR WILL I BE LIABLE FOR ANY DAMAGES INCURRED FROM THE
 * USE OF. USE ENTIRELY AT YOUR OWN RISK!!!
 *********************************************************************/

/* @(#)xdr.c    2.1 88/07/29 4.0 RPCSRC */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)xdr.c 1.35 87/08/12";
#endif

/*
 * xdr.c, Generic XDR routines implementation.
 *
 * Copyright (C) 1986, Sun Microsystems, Inc.
 *
 * These are the "generic" xdr routines used to serialize and de-serialize
 * most common data items.  See xdr.h for more info on the interface to
 * xdr.
 */

#include <stdio.h>
#include <stdlib.h>             /* malloc,free */
#include <string.h>             /* strlen() */

#ifdef WIN32
#include <Winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "xdr.h"

/*
 * constants specific to the xdr "protocol"
 */
#define XDR_FALSE       ((long) 0)
#define XDR_TRUE        ((long) 1)
#define LASTUNSIGNED    ((u_int) 0-1)

/*
 * for unit alignment
 */
static char xdr_zero[BYTES_PER_XDR_UNIT] = { 0, 0, 0, 0 };

/*
 * Free a data structure using XDR
 * Not a filter, but a convenient utility nonetheless
 */
void
xdr_free(xdrproc_t proc, char *objp)
{
        XDR x;

        x.x_op = XDR_FREE;
        (*proc)(&x, objp);
}

/*
 * XDR nothing
 */
bool_t
xdr_void(/* xdrs, addr */)
        /* XDR *xdrs; */
        /* caddr_t addr; */
{

        return (TRUE);
}

/*
 * XDR integers
 */
bool_t
xdr_int(XDR *xdrs, int *ip)
{

#ifdef lint
        (void) (xdr_short(xdrs, (short *)ip));
        return (xdr_long(xdrs, (uint32_t *)ip));
#else
        if (sizeof (int) == sizeof (long)) {
                return (xdr_long(xdrs, (uint32_t *)ip));
        } else {
                return (xdr_short(xdrs, (short *)ip));
        }
#endif
}

/*
 * XDR long integers
 * same as xdr_u_long - open coded to save a proc call!
 */
bool_t
xdr_long(XDR *xdrs, uint32_t *lp)
{
	printf("long\n");

        if (xdrs->x_op == XDR_ENCODE)
                return (XDR_PUTLONG(xdrs, lp));

        if (xdrs->x_op == XDR_DECODE)
                return (XDR_GETLONG(xdrs, lp));

        if (xdrs->x_op == XDR_FREE)
                return (TRUE);

        return (FALSE);
}

/*
 * XDR unsigned long integers
 * same as xdr_long - open coded to save a proc call!
 */
bool_t
xdr_u_long(XDR *xdrs, uint32_t *ulp)
{

        if (xdrs->x_op == XDR_DECODE)
                return (XDR_GETLONG(xdrs, (uint32_t *)ulp));
        if (xdrs->x_op == XDR_ENCODE)
                return (XDR_PUTLONG(xdrs, (uint32_t *)ulp));
        if (xdrs->x_op == XDR_FREE)
                return (TRUE);
        return (FALSE);
}

/*
 * XDR short integers
 */
bool_t
xdr_short(XDR *xdrs, short *sp)
{
        uint32_t l;

        switch (xdrs->x_op) {

        case XDR_ENCODE:
                l = (uint32_t) *sp;
                return (XDR_PUTLONG(xdrs, &l));

        case XDR_DECODE:
                if (!XDR_GETLONG(xdrs, &l)) {
                        return (FALSE);
                }
                *sp = (short) l;
                return (TRUE);

        case XDR_FREE:
                return (TRUE);
        }
        return (FALSE);
}

/*
 * XDR unsigned short integers
 */
bool_t
xdr_u_short(XDR *xdrs, u_short *usp)
{
        uint32_t l;

        switch (xdrs->x_op) {

        case XDR_ENCODE:
                l = (uint32_t) *usp;
                return (XDR_PUTLONG(xdrs, &l));

        case XDR_DECODE:
                if (!XDR_GETLONG(xdrs, &l)) {
                        return (FALSE);
                }
                *usp = (u_short) l;
                return (TRUE);

        case XDR_FREE:
                return (TRUE);
        }
        return (FALSE);
}

/*
 * XDR unsigned integers
 */
bool_t
xdr_u_int(XDR *xdrs, u_int *up)
{

#ifdef lint
        (void) (xdr_short(xdrs, (short *)up));
        return (xdr_u_long(xdrs, (uint32_t *)up));
#else
        if (sizeof (u_int) == sizeof (u_long)) {
                return (xdr_u_long(xdrs, (uint32_t *)up));
        } else {
                return (xdr_short(xdrs, (short *)up));
        }
#endif
}

/*
 * XDR opaque data
 * Allows the specification of a fixed size sequence of opaque bytes.
 * cp points to the opaque object and cnt gives the byte length.
 */
bool_t
xdr_opaque(XDR *xdrs, caddr_t cp, u_int cnt)
{
        u_int rndup;
        static char* crud[BYTES_PER_XDR_UNIT];

        /*
         * if no data we are done
         */
        if (cnt == 0)
                return (TRUE);

        /*
         * round byte count to full xdr units
         */
        rndup = cnt % BYTES_PER_XDR_UNIT;
        if (rndup > 0)
                rndup = BYTES_PER_XDR_UNIT - rndup;

        if (xdrs->x_op == XDR_DECODE) {
                if (!XDR_GETBYTES(xdrs, cp, cnt)) {
                        return (FALSE);
                }
                if (rndup == 0)
                        return (TRUE);
                return (XDR_GETBYTES(xdrs, crud, rndup));
        }

        if (xdrs->x_op == XDR_ENCODE) {
                if (!XDR_PUTBYTES(xdrs, cp, cnt)) {
                        return (FALSE);
                }
                if (rndup == 0)
                        return (TRUE);
                return (XDR_PUTBYTES(xdrs, xdr_zero, rndup));
        }

        if (xdrs->x_op == XDR_FREE) {
                return (TRUE);
        }

        return (FALSE);
}

/*
 * XDR null terminated ASCII strings
 * xdr_string deals with "C strings" - arrays of bytes that are
 * terminated by a NULL character.  The parameter cpp references a
 * pointer to storage; If the pointer is null, then the necessary
 * storage is allocated.  The last parameter is the max allowed length
 * of the string as specified by a protocol.
 */
bool_t
xdr_string(XDR *xdrs, char **cpp, u_int maxsize)
{
        char *sp = *cpp;  /* sp is the actual string pointer */
        u_int size;
        u_int nodesize;

        /*
         * first deal with the length since xdr strings are counted-strings
         */
        switch (xdrs->x_op) {
        case XDR_FREE:
                if (sp == NULL) {
                        return(TRUE);   /* already free */
                }
                /* fall through... */
        case XDR_ENCODE:
                size = strlen(sp);
                break;
        }
        if (! xdr_u_int(xdrs, &size)) {
                return (FALSE);
        }
        if (size > maxsize) {
                return (FALSE);
        }
        nodesize = size + 1;

        /*
         * now deal with the actual bytes
         */
        switch (xdrs->x_op) {

        case XDR_DECODE:
                if (nodesize == 0) {
                        return (TRUE);
                }
                if (sp == NULL)
                        *cpp = sp = (char *)mem_alloc(nodesize);
                if (sp == NULL) {
                        (void) fprintf(stderr, "xdr_string: out of memory\n");
                        return (FALSE);
                }
                sp[size] = 0;
                /* fall into ... */

        case XDR_ENCODE:
                return (xdr_opaque(xdrs, sp, size));

        case XDR_FREE:
                mem_free(sp, nodesize);
                *cpp = NULL;
                return (TRUE);
        }
        return (FALSE);
}

bool_t
xdr_float(XDR *xdrs, float *fp)
{
        switch (xdrs->x_op) {
        case XDR_ENCODE:
                return (XDR_PUTLONG(xdrs, (uint32_t *)fp));
        case XDR_DECODE:
                return (XDR_GETLONG(xdrs, (uint32_t *)fp));
        case XDR_FREE:
                return (TRUE);
        }
        return (FALSE);
}

bool_t
xdr_double(XDR *xdrs, double *dp)
{
        uint32_t *lp;

        switch (xdrs->x_op) {
        case XDR_ENCODE:
                lp = (uint32_t *)dp;
                return (XDR_PUTLONG(xdrs, lp+1) && XDR_PUTLONG(xdrs, lp));
        case XDR_DECODE:
                lp = (uint32_t *)dp;
                return (XDR_GETLONG(xdrs, lp+1) && XDR_GETLONG(xdrs, lp));
        case XDR_FREE:
                return (TRUE);
        }
        return (FALSE);
}


/************************* XDR operations with memory buffers **************************/

void xdrmem_destroy(void *);
bool_t xdrmem_getlong(void *xdrs, uint32_t *lp);
bool_t xdrmem_putlong(void *xdrs, uint32_t *lp);
bool_t xdrmem_getbytes(void *xdrs, void *addr, int len);
bool_t xdrmem_putbytes(void *xdrs, void *addr, int len);
u_int  xdrmem_getpos(void *xdrs);
bool_t xdrmem_setpos(void *xdrs, int pos);
long * xdrmem_inline(void *xdrs, int len);

static XDR::xdr_ops xdrmem_ops = {
        &xdrmem_getlong,
        &xdrmem_putlong,
        &xdrmem_getbytes,
        &xdrmem_putbytes,
        &xdrmem_getpos,
        &xdrmem_setpos,
        &xdrmem_inline,
        &xdrmem_destroy
};

/*
 * The procedure xdrmem_create initializes a stream descriptor for a
 * memory buffer.
 */
void
xdrmem_create(XDR *xdrs, caddr_t addr, u_int size, enum xdr_op op)
{

        xdrs->x_op = op;
        xdrs->x_ops = &xdrmem_ops;
        xdrs->x_private = xdrs->x_base = addr;
        xdrs->x_handy = size;
}

void
xdrmem_destroy(void *xdrs)
{
}

bool_t
xdrmem_getlong(void *xdrsv, uint32_t *lp)
{
	XDR *xdrs = (XDR *)xdrsv;
        if ((xdrs->x_handy -= sizeof(uint32_t)) < 0)
                return (FALSE);
        *lp = (long)ntohl((u_long)(*((long *)(xdrs->x_private))));
        xdrs->x_private += sizeof(uint32_t);
        return (TRUE);
}

bool_t
xdrmem_putlong(void *xdrsv, uint32_t *lp)
{
	XDR *xdrs = (XDR *)xdrsv;

	uint32_t value = htonl((uint32_t) (*lp));

	return xdrmem_putbytes(xdrsv, &value, sizeof(uint32_t));
}

bool_t
xdrmem_getbytes(void *xdrsv, void *addr, int len)
{
	XDR *xdrs = (XDR *)xdrsv;
        if ((xdrs->x_handy -= len) < 0)
                return (FALSE);
        memcpy(addr, xdrs->x_private, len);
        xdrs->x_private += len;
        return (TRUE);
}

bool_t
xdrmem_putbytes(void *xdrsv, void *addr, int len)
{
	XDR *xdrs = (XDR *)xdrsv;
        if ((xdrs->x_handy -= len) < 0)
                return (FALSE);
        memcpy(xdrs->x_private, addr, len);
        xdrs->x_private += len;
        return (TRUE);
}

u_int
xdrmem_getpos(void *xdrsv)
{
	XDR *xdrs = (XDR *)xdrsv;
        return (u_int) ((u_long)xdrs->x_private - (u_long)xdrs->x_base);
}

bool_t
xdrmem_setpos(void *xdrsv, int pos)
{
	XDR *xdrs = (XDR *)xdrsv;
        caddr_t newaddr = xdrs->x_base + pos;
        caddr_t lastaddr = xdrs->x_private + xdrs->x_handy;

        if ((long)newaddr > (long)lastaddr)
                return (FALSE);
        xdrs->x_private = newaddr;
        xdrs->x_handy = (int) (lastaddr - newaddr);
        return (TRUE);
}

long *
xdrmem_inline(void *xdrsv, int len)
{
        long *buf = 0;
	XDR *xdrs = (XDR *)xdrsv;
        if (xdrs->x_handy >= len) {
                xdrs->x_handy -= len;
                buf = (long *) xdrs->x_private;
                xdrs->x_private += len;
        }
        return (buf);
}
