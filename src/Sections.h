/***********************************************************************

    To build a multi-section application
    1. In makefile, set MULTIPLE_CODE_SECTIONS = TRUE
    2. Edit Sections.def to specify the number of code sections desired
    3. Modify this file to match the code sections
       (Sections.def and this file are set up with 2 extra sections
       by default.)
    4. Include this file in project sources.
    5. Annotate each function with a section define like so:
        void DoWork() EXTRA_SECTION_ONE;
        void DrawForm() EXTRA_SECTION_TWO;
       Notice that the annotations need to be visible at the function
       definition, and everywhere the function is used.  Any function
       without an annotation will go into the default code section.

    To effectively disable the multi-section annotations,
    just define these section macros to expand to nothing.

    Be sure to follow the instructions and warnings given at:
    http://prc-tools.sourceforge.net/doc/prc-tools_3.html#SEC17

***********************************************************************/

#ifndef _SECTIONS_H
#define _SECTIONS_H

extern void *__text__;
extern void *__text__sec1;
extern void *__text__sec2;
extern void *__text__sec3;
extern void *__text__sec4;


#define DEFAULT_SECTION

#define SECTION0
#define SECTION1 __attribute__ ((section("sec1")))
#define SECTION2 __attribute__ ((section("sec2")))
#define SECTION3 __attribute__ ((section("sec3")))


#define MAIN_SECTION            SECTION0
#define VERSION_SECTION		SECTION0
#define COMMANDS_SECTION        SECTION0
#define APPPREF_SECTION         SECTION0
#define SDP_SECTION             SECTION0
#define TOOLS_SECTION           SECTION0
#define HASH_SECTION            SECTION0
#define LOG_SECTION             SECTION0
#define MSGQUEUE_SECTION        SECTION0

#define HIDPROTOCOL_SECTION     SECTION1
#define APPMENU_SECTION         SECTION1
#define CONNECTFORM_SECTION     SECTION1
#define KEYBOARDFORM_SECTION    SECTION1
#define MOUSEFORM_SECTION       SECTION1
#define HIDDESCRIPTOR_SECTION   SECTION1
#define ABOUTFORM_SECTION	SECTION1

#define BLUETOOTH_SECTION       SECTION2

#define HARDKEYS_SECTION        SECTION3
#define AUTOOFF_SECTION		SECTION3
#define UITOOLS_SECTION         SECTION3
#define DATABASE_SECTION        SECTION3
#define DEVLIST_SECTION         SECTION3
#define PROGRESSFORM_SECTION    SECTION3
#define HEAP_SECTION            SECTION3
#define TIMER_SECTION           SECTION3


#endif
