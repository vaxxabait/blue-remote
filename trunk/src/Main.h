/***********************************************************************
 *
 * Copyright (c) 2006 Karoly Lorentey.  All rights reserved.
 *
 * File: Main.h
 *
 ***********************************************************************/

#undef SECTION
#define SECTION MAIN_SECTION

#ifdef NORMAL_INCLUDE
typedef struct FormDesc {
    /* Pointer to the next form in the formList.  */
    struct FormDesc *next;

    /* The ID of this form.  */
    UInt16 formID;

    /* The index of this form in the form selector popup list, or 65535 if not listed.  */
    UInt16 popupIndex;

    /* The menu item that should select this form, or 0 if none. */
    UInt16 menuCommandID;

    /* Function to initialize this form.  */
    void (*Init) (struct FormDesc *pFormDesc, FormType *pForm);
} FormDesc;

extern Globals *globals;

#elif defined (GLOBALS_INCLUDE)

struct FormDesc *formList;
#define g_formList  g->formList

struct FormDesc *currentForm;
#define g_currentForm   g->currentForm

Int32 eventTimeout;
#define g_eventTimeout  g->eventTimeout

#endif

DEFUN1 (void, RegisterForm, struct FormDesc *form);
DEFUN1 (void, SwitchToForm, UInt16 formID);
DEFUN1 (void, FormDrawn, UInt16 formID);

/*
 * Local Variables:
 * c-file-style: "lk-palm";
 * End:
 */

