/* eval.c */
void set_internal_string_var __ARGS((char_u *name, char_u *value));
int eval_to_bool __ARGS((char_u *arg, int *error, char_u **nextcmd, int skip));
char_u *eval_to_string __ARGS((char_u *arg, char_u **nextcmd));
void do_let __ARGS((EXARG *eap));
void set_context_for_expression __ARGS((char_u *arg, CMDIDX cmdidx));
void do_call __ARGS((EXARG *eap));
void do_unlet __ARGS((char_u *arg, int forceit));
char_u *get_user_var_name __ARGS((int idx));
char_u *get_function_name __ARGS((int idx));
char_u *get_expr_name __ARGS((int idx));
void set_vim_var_nr __ARGS((int idx, long val));
void set_vim_var_string __ARGS((int idx, char_u *val));
void var_init __ARGS((struct growarray *gap));
void var_clear __ARGS((struct growarray *gap));
void do_echo __ARGS((EXARG *eap, int echo));
void do_echohl __ARGS((char_u *arg));
void do_execute __ARGS((EXARG *eap, char_u *(*getline)(int, void *, int), void *cookie));
void do_function __ARGS((EXARG *eap, char_u *(*getline)(int, void *, int), void *cookie));
char_u *get_user_func_name __ARGS((int idx));
void do_delfunction __ARGS((char_u *arg));
void *save_funccal __ARGS((void));
void restore_funccal __ARGS((void *fc));
void do_return __ARGS((EXARG *eap));
char_u *get_func_line __ARGS((int c, void *cookie, int indent));
int func_has_ended __ARGS((void *cookie));
int func_has_abort __ARGS((void *cookie));
int read_viminfo_varlist __ARGS((char_u *line, FILE *fp, int writing));
void write_viminfo_varlist __ARGS((FILE *fp));
int store_session_globals __ARGS((FILE *fd));
int modify_fname __ARGS((char_u *src, int *usedlen, char_u **fnamep, char_u **bufp, int *fnamelen));
char_u *do_string_sub __ARGS((char_u *str, char_u *pat, char_u *sub, char_u *flags));
