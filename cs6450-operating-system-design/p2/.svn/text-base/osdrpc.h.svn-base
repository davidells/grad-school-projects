
int osdrpc_init(void);
int osdrpc_finalize(void);
int osdrpc_call(int rank, char *lib, char *procname, char *fmt, ...);
/* returns the return code from procname on successful call  */
/* if the rc from procname is < 0, sets errno to 0 */
/* if osdrpc_call wants to return a "bad rc", it returns < 0 and sets
   errno to non-0  */
/* fmt consists of i's and s's, e.g. "isisi" which indicates that
 * there are 5 arguments following: int, string, int, string, int
 */
