/* window.c */
void do_window __ARGS((int nchar, long Prenum));
int win_split __ARGS((int new_height, int redraw, int req_room));
int win_valid __ARGS((WIN *win));
int win_count __ARGS((void));
int make_windows __ARGS((int count));
void win_move_after __ARGS((WIN *win1, WIN *win2));
void win_equal __ARGS((WIN *next_curwin, int redraw));
void close_windows __ARGS((BUF *buf));
void close_window __ARGS((WIN *win, int free_buf));
void close_others __ARGS((int message, int forceit));
void win_init __ARGS((WIN *wp));
WIN *win_goto_nr __ARGS((int winnr));
void win_enter __ARGS((WIN *wp, int undo_sync));
WIN *buf_jump_open_win __ARGS((BUF *buf));
WIN *win_alloc __ARGS((WIN *after));
void win_free __ARGS((WIN *wp));
int win_alloc_lsize __ARGS((WIN *wp));
void win_free_lsize __ARGS((WIN *wp));
void screen_new_rows __ARGS((void));
void win_setheight __ARGS((int height));
void win_setminheight __ARGS((void));
void win_drag_status_line __ARGS((int offset));
void win_comp_scroll __ARGS((WIN *wp));
void command_height __ARGS((long old_p_ch));
void last_status __ARGS((void));
char_u *file_name_at_cursor __ARGS((int options, long count));
char_u *get_file_name_in_path __ARGS((char_u *line, int col, int options, long count));
char_u *find_file_in_path __ARGS((char_u *ptr, int len, int options, long count));
int min_rows __ARGS((void));
int only_one_window __ARGS((void));
void check_lnums __ARGS((int do_curwin));
