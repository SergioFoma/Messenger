#ifndef H_PAINT
#define H_PAINT

enum Colors {
    PURPLE    = 0,
    YELLOW    = 1,
    GREEN     = 2,
    BLUE      = 3,
    RED       = 4
};

enum Styles {
    BOLD        = 0,
    FADED       = 1,
    ITALICS     = 2,
    UNDERLINED  = 3,
    BLINKING    = 4,
    CROSSEDOUT  = 5,
    NOMODE      = 69,
};

#define makeBold "\033[1;"
#define makeFaded "\033[2;"
#define makeItalics "\033[3;"
#define makeUnderlined "\033[4;"
#define makeBlinking "\033[5;"
#define makeCrossedOut "\033[9;"

#define colorInPurple "35m"
#define colorInYellow "33m"
#define colorInGreen "32m"
#define colorInBlue "34m"
#define colorInRed "31m"
#define colorReset "\033[0m"

void colorPrintf( Styles style, Colors color, const char* line ... );

const char* getColorString( Colors color );

const char* getStyleString( Styles style );


int printfError_(const char* file, const char* function, int line, const char* lineForError ...);

#define printfError( lineForError)                                                  \
    do {                                                                            \
        ( printfError_( __FILE__, __func__ , __LINE__, lineForError) );             \
    } while(false);                                                                 \


#endif
