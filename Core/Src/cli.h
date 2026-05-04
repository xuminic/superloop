
/**
  ******************************************************************************
  * @file           : cli.h
  * @brief          : the head file of the command line interface
  ******************************************************************************
  */
#ifndef	_CLI_H_
#define _CLI_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef	struct	_cli_t	{
	char	*cmd;
	int	(*func)(void *xtcb, int argc, char **argv);
	void	*usage;
} cli_t;


int cli_init(void *taskarg, cli_t *cmds);
int cli_main(void *taskarg, int argc, char **argv);
int cli_mkargs(char *s, char **argv, int argv_len);

#ifdef	CFG_CLI_DUMP
void hexdump(void *taskarg, char *s, int len);
#endif

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _CLI_H_ */

