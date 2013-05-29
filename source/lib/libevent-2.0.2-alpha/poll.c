/*	$OpenBSD: poll.c,v 1.2 2002/06/25 15:50:15 mickey Exp $	*/

/*
 * Copyright 2000-2007 Niels Provos <provos@citi.umich.edu>
 * Copyright 2007-2009 Niels Provos and Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifdef HAVE_CONFIG_H
#include "event-config.h"
#endif

#include <sys/types.h>
#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys/_time.h>
#endif
#include <sys/queue.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "event-internal.h"
#include "evsignal-internal.h"
#include "log-internal.h"
#include "evmap-internal.h"

struct pollidx {
	int idxplus1;
};

struct pollop {
	int event_count;		/* Highest number alloc */
	int nfds;                       /* Size of event_* */
	struct pollfd *event_set;
};

static void *poll_init	(struct event_base *);
static int poll_add(struct event_base *, int, short old, short events, void *_idx);
static int poll_del(struct event_base *, int, short old, short events, void *_idx);
static int poll_dispatch	(struct event_base *, struct timeval *);
static void poll_dealloc	(struct event_base *);

const struct eventop pollops = {
	"poll",
	poll_init,
	poll_add,
	poll_del,
	poll_dispatch,
	poll_dealloc,
    0, /* doesn't need_reinit */
	EV_FEATURE_FDS,
	sizeof(struct pollidx),
};

static void *
poll_init(struct event_base *base)
{
	struct pollop *pollop;

	if (!(pollop = mm_calloc(1, sizeof(struct pollop))))
		return (NULL);

	evsig_init(base);

	return (pollop);
}

#ifdef CHECK_INVARIANTS
static void
poll_check_ok(struct pollop *pop)
{
	int i, idx;
	struct event *ev;

	for (i = 0; i < pop->fd_count; ++i) {
		idx = pop->idxplus1_by_fd[i]-1;
		if (idx < 0)
			continue;
		assert(pop->event_set[idx].fd == i);
	}
	for (i = 0; i < pop->nfds; ++i) {
		struct pollfd *pfd = &pop->event_set[i];
		assert(pop->idxplus1_by_fd[pfd->fd] == i+1);
	}
}
#else
#define poll_check_ok(pop)
#endif

static int
poll_dispatch(struct event_base *base, struct timeval *tv)
{
	int res, i, j, msec = -1, nfds;
	struct pollop *pop = base->evbase;

	poll_check_ok(pop);

	if (tv != NULL)
		msec = tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000;

	nfds = pop->nfds;
	res = poll(pop->event_set, nfds, msec);

	if (res == -1) {
		if (errno != EINTR) {
                        event_warn("poll");
			return (-1);
		}

		evsig_process(base);
		return (0);
	} else if (base->sig.evsig_caught) {
		evsig_process(base);
	}

	event_debug(("%s: poll reports %d", __func__, res));

	if (res == 0 || nfds == 0)
		return (0);

	i = random() % nfds;
	for (j = 0; j < nfds; j++) {
		int what;
		if (++i == nfds)
			i = 0;
		what = pop->event_set[i].revents;
		if (!what)
			continue;

		res = 0;

		/* If the file gets closed notify */
		if (what & (POLLHUP|POLLERR))
			what |= POLLIN|POLLOUT;
		if (what & POLLIN)
			res |= EV_READ;
		if (what & POLLOUT)
			res |= EV_WRITE;
		if (res == 0)
			continue;

		evmap_io_active(base, pop->event_set[i].fd, res);
	}

	return (0);
}

static int
poll_add(struct event_base *base, int fd, short old, short events, void *_idx)
{
	struct pollop *pop = base->evbase;
	struct pollfd *pfd = NULL;
	struct pollidx *idx = _idx;
	int i;

	assert((events & EV_SIGNAL) == 0);
	if (!(events & (EV_READ|EV_WRITE)))
		return (0);

	poll_check_ok(pop);
	if (pop->nfds + 1 >= pop->event_count) {
		struct pollfd *tmp_event_set;
		int tmp_event_count;

		if (pop->event_count < 32)
			tmp_event_count = 32;
		else
			tmp_event_count = pop->event_count * 2;

		/* We need more file descriptors */
		tmp_event_set = mm_realloc(pop->event_set,
				 tmp_event_count * sizeof(struct pollfd));
		if (tmp_event_set == NULL) {
			event_warn("realloc");
			return (-1);
		}
		pop->event_set = tmp_event_set;

		pop->event_count = tmp_event_count;
	}

	i = idx->idxplus1 - 1;

	if (i >= 0) {
		pfd = &pop->event_set[i];
	} else {
		i = pop->nfds++;
		pfd = &pop->event_set[i];
		pfd->events = 0;
		pfd->fd = fd;
		idx->idxplus1 = i + 1;
	}

	pfd->revents = 0;
	if (events & EV_WRITE)
		pfd->events |= POLLOUT;
	if (events & EV_READ)
		pfd->events |= POLLIN;
	poll_check_ok(pop);

	return (0);
}

/*
 * Nothing to be done here.
 */

static int
poll_del(struct event_base *base, int fd, short old, short events, void *_idx)
{
	struct pollop *pop = base->evbase;
	struct pollfd *pfd = NULL;
	struct pollidx *idx = _idx;
	int i;

	assert((events & EV_SIGNAL) == 0);
	if (!(events & (EV_READ|EV_WRITE)))
		return (0);

	poll_check_ok(pop);
	i = idx->idxplus1 - 1;
	if (i < 0)
		return (-1);

	/* Do we still want to read or write? */
	pfd = &pop->event_set[i];
	if (events & EV_READ)
		pfd->events &= ~POLLIN;
	if (events & EV_WRITE)
		pfd->events &= ~POLLOUT;
	poll_check_ok(pop);
	if (pfd->events)
		/* Another event cares about that fd. */
		return (0);

	/* Okay, so we aren't interested in that fd anymore. */
	idx->idxplus1 = 0;

	--pop->nfds;
	if (i != pop->nfds) {
		/*
		 * Shift the last pollfd down into the now-unoccupied
		 * position.
		 */
		memcpy(&pop->event_set[i], &pop->event_set[pop->nfds],
		       sizeof(struct pollfd));
		idx = evmap_io_get_fdinfo(&base->io, pop->event_set[i].fd);
		assert(idx);
		assert(idx->idxplus1 == pop->nfds + 1);
		idx->idxplus1 = i + 1;
	}

	poll_check_ok(pop);
	return (0);
}

static void
poll_dealloc(struct event_base *base)
{
	struct pollop *pop = base->evbase;

	evsig_dealloc(base);
	if (pop->event_set)
		mm_free(pop->event_set);

	memset(pop, 0, sizeof(struct pollop));
	mm_free(pop);
}
