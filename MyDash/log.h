
#ifndef	__ERROR_H
#define	__ERROR_H

/* Error handling (logging) functions */
void	err_dump(const char *, ...);
void	err_msg(const char *, ...);
void	err_quit(const char *, ...);
void	err_ret(const char *, ...);
void	err_sys(const char *, ...);
#endif
