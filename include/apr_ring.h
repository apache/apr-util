/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * This code draws heavily from the 4.4BSD <sys/queue.h> macros
 * and Dean Gaudet's "splim/ring.h".
 * <http://www.freebsd.org/cgi/cvsweb.cgi/src/sys/sys/queue.h>
 * <http://www.arctic.org/~dean/splim/>
 *
 * We'd use Dean's code directly if we could guarantee the
 * availability of inline functions.
 */

#ifndef AP_RING_H
#define AP_RING_H

/*
 * for offsetof()
 */
#include <stddef.h>

/*
 * A ring is a kind of doubly-linked list.
 */

/*
 * A struct on a ring contains a field linking it to the other
 * elements in the ring, e.g.
 *
 *      struct my_item_t {
 *          AP_RING_ENTRY(my_item_t) link;
 *          int foo;
 *          char *bar;
 *      };
 *
 * A struct may be put on more than one ring if it has more than one
 * AP_RING_ENTRY field.
 */
#define AP_RING_ENTRY(elem)						\
    struct {								\
	struct elem *next;						\
	struct elem *prev;						\
    }

/*
 * Each ring is managed via its head, which is a struct declared like this:
 *
 *      AP_RING_HEAD(my_ring_t, my_item_t);
 *      struct my_ring_t ring, *ringp;
 *
 * This struct looks just like the element link struct so that we can
 * be sure that the typecasting games will work as expected.
 *
 * The first element in the ring is next after the head, and the last
 * element is just before the head.
 */
#define AP_RING_HEAD(head, elem)					\
    struct head {							\
	struct elem *next;						\
	struct elem *prev;						\
    }

/*
 * The head itself isn't an element, but in order to get rid of all
 * the special cases when dealing with the ends of the ring, we play
 * typecasting games to make it look like one. The sentinel is the
 * magic pointer value that occurs before the first and after the last
 * elements in the ring, computed from the address of the ring's head.
 *
 * Note that for strict C standards compliance you should put the
 * AP_RING_ENTRY first in struct elem_tag unless the head is always
 * part of a larger object with enough earlier fields to accommodate
 * the offsetof() computed below. You can usually ignore this caveat.
 */
#define AP_RING_SENTINAL(hp, elem, link)				\
    (struct elem *)((char *)(hp) - offsetof(struct elem, link))

/*
 * Accessor macros. Use these rather than footling inside the
 * structures directly so that you can more easily change to a
 * different flavour of list from BSD's <sys/queue.h>.
 */
#define AP_RING_FIRST(hp)	(hp)->next
#define AP_RING_LAST(hp)	(hp)->prev
#define AP_RING_NEXT(ep, link)	(ep)->link.next
#define AP_RING_PREV(ep, link)	(ep)->link.prev

/*
 * Empty rings and singleton elements.
 */
#define AP_RING_INIT(hp, elem, link) do {				\
	AP_RING_FIRST((hp)) = AP_RING_SENTINEL((hp), elem, link);	\
	AP_RING_LAST((hp))  = AP_RING_SENTINEL((hp), elem, link);	\
    } while (0)

#define AP_RING_EMPTY(hp, elem, link)					\
    (AP_RING_FIRST((hp)) == AP_RING_SENTINEL((hp), elem, link))

#define AP_RING_ELEM_INIT(ep, link) do {				\
	AP_RING_NEXT((ep), link) = (ep);				\
	AP_RING_PREV((ep), link) = (ep);				\
    } while (0)

/*
 * Adding elements.
 */
#define AP_RING_SPLICE_BEFORE(lep, ep1, epN, link) do {			\
	AP_RING_NEXT((epN), link) = (lep);				\
	AP_RING_PREV((ep1), link) = AP_RING_PREV((lep), link);		\
	AP_RING_NEXT(AP_RING_PREV((lep), link), link) = (ep1);		\
	AP_RING_PREV((lep), link) = (epN);				\
    } while (0)

#define AP_RING_SPLICE_AFTER(lep, ep1, epN, link) do {			\
	AP_RING_PREV((ep1), link) = (lep);				\
	AP_RING_NEXT((epN), link) = AP_RING_NEXT((lep), link);		\
	AP_RING_PREV(AP_RING_NEXT((lep), link), link) = (epN);		\
	AP_RING_NEXT((lep), link) = (ep1);				\
    } while (0)

#define AP_RING_INSERT_BEFORE(lep, nep, link)				\
	AP_RING_SPLICE_BEFORE((lep), (nep), (nep), link)

#define AP_RING_INSERT_AFTER(lep, nep, link)				\
	AP_RING_SPLICE_AFTER((lep), (nep), (nep), link)

/*
 * We could implement these by splicing after and before the sentinel
 * instead of before the first and after the last respectively, but
 * then the caller would have to pass in the element type.
 */
#define AP_RING_SPLICE_HEAD(hp, ep1, epN, link)				\
	AP_RING_SPLICE_BEFORE(AP_RING_FIRST((hp)), (ep1), (epN), link)

#define AP_RING_SPLICE_TAIL(hp, ep1, epN, link)				\
	AP_RING_SPLICE_AFTER(AP_RING_LAST((hp)), (ep1), (epN), link)

#define AP_RING_INSERT_HEAD(lep, nep, link)				\
	AP_RING_SPLICE_HEAD((lep), (nep), (nep), link)

#define AP_RING_INSERT_TAIL(lep, nep, link)				\
	AP_RING_SPLICE_TAIL((lep), (nep), (nep), link)

/*
 * Concatenating ring h2 onto the end of ring h1 leaves h2 empty.
 *
 * Doing this without asking for the element type is ugly.
 */
#define AP_RING_CONCAT(h1, h2, elem, link) do {				\
	if (!AP_RING_EMPTY((h2), elem, link)) {				\
	    AP_RING_SPLICE_BEFORE(AP_RING_SENTINEL((h1), elem, link),	\
				  AP_RING_FIRST((h2)),			\
				  AP_RING_LAST((h2)), link);		\
	    AP_RING_INIT((h2), elem, link);				\
	}								\
    } while (0)

/*
 * Removing elements. Be warned that the unspliced elements are left
 * with dangling pointers at either end!
 */
#define AP_RING_UNSPLICE(ep1, epN, link) do {				\
	AP_RING_NEXT(AP_RING_PREV((ep1), link), link) =			\
		     AP_RING_NEXT((epN), link);				\
	AP_RING_PREV(AP_RING_NEXT((epN), link), link) =			\
		     AP_RING_PREV((ep1), link);				\
    } while (0)

#define AP_RING_REMOVE(ep, link)					\
    AP_RING_UNSPLICE((ep), (ep), link)

/*
 * Iteration.
 */
#define AP_RING_FOREACH(ep, hp, elem, link)				\
    for ((ep)  = AP_RING_FIRST((hp));					\
	 (ep) != AP_RING_SENTINEL((hp), elem, link);			\
	 (ep)  = AP_RING_NEXT((ep), link))

#define AP_RING_FOREACH_REVERSE(ep, hp, elem, link)			\
    for ((ep)  = AP_RING_LAST((hp));					\
	 (ep) != AP_RING_SENTINEL((hp), elem, link);			\
	 (ep)  = AP_RING_PREV((ep), link))

#endif /* !AP_RING_H */
