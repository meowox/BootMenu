#ifndef __CRP_BLIT_GADGETS
#define __CRP_BLIT_GADGETS

#include "blit/blit.h"

#define BLIT_GADGETS_X 20
#define BLIT_GADGETS_PIXELS_PER_LINE 20

#define blit_gadgets_init(y) int __blit_gadgets_y = (y)
#define blit_gadgets_setline(y) __blit_gadgets_y = (y)
#define blit_gadgets_getline() __blit_gadgets_y - BLIT_GADGETS_PIXELS_PER_LINE //__blit_gadgets_y has the value of the line where to write next
#define blit_gadgets_newline() __blit_gadgets_y += BLIT_GADGETS_PIXELS_PER_LINE
#define screen_print(s) blit_string(BLIT_GADGETS_X, __blit_gadgets_y, (s)); blit_gadgets_newline()
#define screen_printf(fmt, ...) blit_stringf(BLIT_GADGETS_X, __blit_gadgets_y, (fmt), ##__VA_ARGS__); blit_gadgets_newline()

#define RED RGBT(255,0,0,0)
#define GREEN RGBT(0,255,0,0)
#define BLUE RGBT(0,0,255,0)
#define WHITE RGBT(255,255,255,0)
#define BLACK RGBT(0,0,0,0)

//Additional defines go here
#define MENU_Y 20
#define COUNTDOWN_Y 200                 //Leave 9 lines for menu
#define ERR_MSG_Y COUNTDOWN_Y + 20      //Leave 1 line for countdown
#define INFO_MSG_Y ERR_MSG_Y + 60 //Leave 2 lines for error messages

#endif