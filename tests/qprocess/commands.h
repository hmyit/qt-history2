const char* baseDir = "../../examples";

static struct {
    const char* label;
    const char* path;
    const char* file;
} command[] = {
    { "Desktop-in-an-application", "multidoc",   "multidoc" },
    { "Info Kiosk - MPEGs",        "kiosk",      "kiosk" },
    { "Help Text Browser",         "helpviewer", "helpviewer" },
    { "Canvas - alpha-blending",   "canvas",     "canvas" },
    { "Text Editor",               "qwerty",     "qwerty unicode.txt" },
    { "Scribble Editor",           "scribble",   "scribble" },
    { "Internationalization",      "i18n",       "i18n all" },
    { "Magnifier",                 "qmag",       "qmag" },
    { 0, 0, 0 }
};

static struct {
    const char* label;
    const char* path;
    const char* file;
} other_command[] = {
    { "aclock",        "aclock",        "aclock" },
    { "addressbook",   "addressbook",   "addressbook" },
    { "buttongroups",  "buttongroups",  "buttongroups" },
    { "checklists",    "checklists",    "checklists" },
    { "cursor",        "cursor",        "cursor" },
    { "customlayout",  "customlayout",  "customlayout" },
    { "dclock",        "dclock",        "dclock" },
    { "dirview",       "dirview",       "dirview" },
    { "drawlines",     "drawlines",     "drawlines" },
    { "hello",         "hello",         "hello" },
    { "layout",        "layout",        "layout" },
    { "life",          "life",          "life" },
    { "lineedits",     "lineedits",     "lineedits" },
    { "listbox",       "listbox",       "listbox" },
    { "listboxcombo",  "listboxcombo",  "listboxcombo" },
    { "menu",          "menu",          "menu" },
    { "movies",        "movies",        "movies" },
    { "popup",         "popup",         "popup" },
    { "progress",      "progress",      "progress" },
    { "progressbar",   "progressbar",   "progressbar" },
    { "qfd",           "qfd",           "qfd" },
    { "rangecontrols", "rangecontrols", "rangecontrols" },
    { "richtext",      "richtext",      "richtext" },
    { "scrollview",    "scrollview",    "scrollview" },
    { "showimg",       "showimg",       "showimg" },
    { "splitter",      "splitter",      "splitter" },
    { "tabdialog",     "tabdialog",     "tabdialog" },
    { "table",         "table",         "table" },
    { "tetrix",        "tetrix",        "tetrix" },
    { "tictac",        "tictac",        "tictac" },
    { "tooltip",       "tooltip",       "tooltip" },
    { "validator",     "validator",     "validator" },
    { "widgets",       "widgets",       "widgets" },
    { "wizard",        "wizard",        "wizard" },
    { 0, 0, 0 }
};
