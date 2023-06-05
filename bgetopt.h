
#pragma once
/*	$NetBSD: getopt.h,v 1.4 2000/07/07 10:43:54 ad Exp $	*/
/*	$FreeBSD$ */

/*-
 * Modified from the original include/getopt.h in the FreeBSD source code to
 * remove reference to BSD system headers, and to remove getopt_long(). All
 * modifications retain the same license conditions as the original.
 *
 * SPDX-License-Identifier: BSD-2-Clause-NetBSD
 *
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Dieter Baron and Thomas Klausner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define BGETOPT_BADCH (int)'?'
#define BGETOPT_BADARG (int)':'

#define _getprogname() ""

/* if error message should be printed */
 
int opterr = 1;

/* index into parent argv vector */
int optind = 1;

/* character checked for validity */
int optopt;

/* reset getopt */
int optreset;

/* argument associated with option */
char* optarg;


static char EMSG[] = "";
/*
* getopt --
*	Parse argc/argv argument vector.
*/
int getopt(int nargc, char* const nargv[], const char* ostr)
{
    static char* place = EMSG; /* option letter processing */
    char* oli; /* option letter list index */

    if(optreset || *place == 0)
    { /* update scanning pointer */
        optreset = 0;
        place = nargv[optind];
        if(optind >= nargc || *place++ != '-')
        {
            /* Argument is absent or is not an option */
            place = EMSG;
            return (-1);
        }
        optopt = *place++;
        if(optopt == '-' && *place == 0)
        {
            /* "--" => end of options */
            ++optind;
            place = EMSG;
            return (-1);
        }
        if(optopt == 0)
        {
            /* Solitary '-', treat as a '-' option
			   if the program (eg su) is looking for it. */
            place = EMSG;
            if(strchr(ostr, '-') == NULL)
                return (-1);
            optopt = '-';
        }
    }
    else
        optopt = *place++;

    /* See if option letter is one the caller wanted... */
    if(optopt == ':' || (oli = (char*)strchr(ostr, optopt)) == NULL)
    {
        if(*place == 0)
            ++optind;
        if(opterr && *ostr != ':')
            (void)fprintf(stderr, "%s: illegal option -- %c\n", _getprogname(), optopt);
        return (BGETOPT_BADCH);
    }

    /* Does this option need an argument? */
    if(oli[1] != ':')
    {
        /* don't need argument */
        optarg = NULL;
        if(*place == 0)
            ++optind;
    }
    else
    {
        /* Option-argument is either the rest of this argument or the
		   entire next argument. */
        if(*place)
            optarg = place;
        else if(oli[2] == ':')
            /*
			 * GNU Extension, for optional arguments if the rest of
			 * the argument is empty, we return NULL
			 */
            optarg = NULL;
        else if(nargc > ++optind)
            optarg = nargv[optind];
        else
        {
            /* option-argument absent */
            place = EMSG;
            if(*ostr == ':')
                return (BGETOPT_BADARG);
            if(opterr)
                (void)fprintf(stderr, "%s: option requires an argument -- %c\n", _getprogname(), optopt);
            return (BGETOPT_BADCH);
        }
        place = EMSG;
        ++optind;
    }
    return (optopt); /* return option letter */
}

