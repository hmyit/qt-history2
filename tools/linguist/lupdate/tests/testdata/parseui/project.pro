TEMPLATE = app
LANGUAGE = C++

FORMS += project.ui

TRANSLATIONS        = project_no.ts

exists( $$TRANSLATIONS ) {
    win32 : RES = $$system(del $$TRANSLATIONS)
    unix : RES = $$system(rm $$TRANSLATIONS)
}


