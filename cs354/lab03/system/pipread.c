/* pipread.c - pipread */

#include <xinu.h>

/*------------------------------------------------------------------------
 * pipread  --  Read from a pipe, blocking if empty
 *------------------------------------------------------------------------
 */
syscall	pipread(
		pipid32		pip,		/* ID of pipe to read from			*/
		char 		*buf,		/* pointer to buffer to read to		*/
		uint32		len			/* number of bytes to read 			*/
	)
{
	intmask	mask;				/* saved interrupt mask				*/

	mask = disable();

	if (isbadpip(pip)) {
		restore(mask);
		return SYSERR;
	}

	struct pentry *pipptr;		/* ptr to pipe table entry			*/
	
	pipptr = &piptab[pip];
	if (pipptr->pstate != PIPE_CONNECTED ||
		(pipptr->pend1 != getpid() && pipptr->pend2 != getpid())) {
		restore(mask);
		return SYSERR;
	}

	/* Ensure there is enough space for the entire read request */
	if (semcount(pipptr->prdsem) < len) {
		restore(mask);
		return SYSERR;
	}

	char *buffer = buf;			/* local copy of buffer				*/
	int count = 0;				/* character count for buffer		*/
	char c;						/* temporary space for characters	*/

	/* Read characters from the circular buffer */
	while (count < len)
	{
		wait(pipptr->prdsem);

		c = pipptr->pbuf[pipptr->pbufs];
		*buffer++ = c;
		pipptr->pbufc--;
		pipptr->pbufs = (pipptr->pbufs + 1) % PIPE_SIZ;
		count++;

		signal(pipptr->pwrsem);
	}

	restore(mask);
	return count;
}
