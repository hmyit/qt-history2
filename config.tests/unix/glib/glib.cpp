typedef struct _GMainContext GMainContext;

#include <glib.h>

int main(int, char **)
{
    GMainContext *context;
    GSource *source;
    GPollFD *pollfd;
    context = g_main_context_default();
    source = g_source_new(0, 0);
    g_source_add_poll(source, pollfd);
    return 0;
}
