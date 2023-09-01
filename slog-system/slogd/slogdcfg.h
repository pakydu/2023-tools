/*
 * simple slog for the project log modules.
 *
 */
#ifndef _SLOG_CFG_H_
#define _SLOG_CFG_H_

#ifdef __cplusplus

extern"C"
{
#endif

#define KEYVALLEN 256


int init_cfg(char *cfg_name, void *data);

#ifdef __cplusplus
}
#endif

#endif //_SLOG_CFG_H_