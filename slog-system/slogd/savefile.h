/*
 * simple slog for the project log modules.
 *
 */

#ifndef _SLOG_SAVEFILE_H_
#define _SLOG_SAVEFILE_H_

#ifdef __cplusplus

extern"C"
{
#endif

#include "libslog.h"
#include "msgqueue.h"

void start_writelog(void * data);
void start_kernellog(void * data);



#ifdef __cplusplus
}
#endif


#endif  /* _SLOG_SAVEFILE_H_ */
