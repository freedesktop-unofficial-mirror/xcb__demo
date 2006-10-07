/*
 *  
 *  Copyright (c) 2002 Steve Slaven, All Rights Reserved.
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA 02111-1307 USA
 *  
*/

/* Generates an X event, like keypress/mouseclick/move/etc
   like a little man in your computer.  :) */

#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xtest.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/*#include "debug.h"*/
#define dmsg(a,b,...)

#define IS_CMD( x, y ) strncmp( x, y, strlen( y ) ) == 0

/* NOTE: demo only supports US keyboards, but could also support German; */
/*  see original configure.in */
/*#include "kbd.h"*/
#define KBDMAP "us"

/* TODO: this is normally defined in configure.in */
#define VERSION "0.97"

#define XK_Shift_L                       0xffe1  /* Left shift */

xcb_key_symbols_t *syms = NULL;

uint8_t thing_to_keycode( xcb_connection_t *c, char *thing ) {
  xcb_keycode_t kc;
  xcb_keysym_t ks;
  
#if 0   /* There is no XCB equivalent to XStringToKeysym */
  ks = XStringToKeysym( thing );
  if( ks == NoSymbol ){
    fprintf( stderr, "Unable to resolve keysym for '%s'\n", thing );
    return( thing_to_keycode( c, "space" ) );
  }
#else
  /* For now, assume thing[0] == Latin-1 keysym */
  ks = (uint8_t)thing[0];
#endif  

  kc = xcb_key_symbols_get_keycode( syms, ks );

  dmsg( 1, "String '%s' maps to keysym '%d'\n", thing, ks );
  dmsg( 1, "String '%s' maps to keycode '%d'\n", thing, kc );

  return( kc );
}

/* xcb_test_fake_input(type,detail,time,window,x,y,device) */

static void
fake_input(xcb_connection_t *c, uint8_t type, uint8_t detail)
{
  xcb_window_t none = { XCB_NONE };

  xcb_test_fake_input( c, type, detail, 0, none, 0, 0, 0 );
}

static void
fake_motion(xcb_connection_t *c, uint8_t relative, uint16_t x, uint16_t y)
{
  xcb_window_t window = { XCB_NONE };

  if (!relative) {
    window = xcb_setup_roots_iterator(xcb_get_setup(c)).data->root;
  }
  xcb_test_fake_input( c, XCB_MOTION_NOTIFY, relative, 0, window, x, y, 0 );
}

void send_key( xcb_connection_t *c, char *thing ) {
  static xcb_keysym_t shift = { XK_Shift_L };
  uint8_t code, wrap_code = 0;

  dmsg( 1, "Sending key '%s'\n", thing );

#if 0
  int probidx;
  /* Catch some common problem characters (thanks Martin Pirker) */
  for( probidx = 0; problems[ probidx ] != NULL; probidx += 3 ) {
    if( strcmp( thing, problems[ probidx ] ) == 0 ) {
      /*wrap_key = problems[ probidx + 1 ]; */
      if (problems[ probidx + 1 ] != NULL) {
        wrap_code = xcb_key_symbols_get_keycode( syms, shift ).id;
      }
      thing = problems[ probidx + 2 ];
      break;
    }
  }
#else
  /* no XStringToKeysym support: do by hand */
/*const char *low = "`1234567890-=[]\\;',./";*/
  const char *cap = "~!@#$%^&*()_+{}|:\"<>?";
  
  if (thing[0] >= 'A' && thing[0] <= 'Z')
    wrap_code = xcb_key_symbols_get_keycode( syms, shift );
  else if (strchr(cap, thing[0]) != NULL)
    wrap_code = xcb_key_symbols_get_keycode( syms, shift );
#endif
  code = thing_to_keycode( c, thing );

  if( wrap_code )
    fake_input( c, XCB_KEY_PRESS, wrap_code );

  fake_input( c, XCB_KEY_PRESS, code );
  fake_input( c, XCB_KEY_RELEASE, code );

  if( wrap_code )
    fake_input( c, XCB_KEY_RELEASE, wrap_code );
}

void mouse_click( xcb_connection_t *c, int button ) {
  dmsg( 1, "Clicking mouse button %d\n", button );
  fake_input( c, XCB_BUTTON_PRESS, button );
  fake_input( c, XCB_BUTTON_RELEASE, button );
}

void mouse_move( xcb_connection_t *c, int x, int y ) {
  dmsg( 1, "Moving mouse to %c,%d\n", x, y );
  fake_motion( c, 0, x, y );
}

void mouse_rel_move( xcb_connection_t *c, int x, int y ) {
  dmsg( 1, "Moving mouse relatively by %c,%d\n", x, y );
  fake_motion( c, 1, x, y );
}

void process_command( xcb_connection_t *c, const char *cmd ) {
  /* Process a command */
  int tmpx,tmpy;
  char str[ 128 ];

  bzero( str, 128 );
  if( IS_CMD( cmd, "mouseclick " ) ) {
    sscanf( cmd, "mouseclick %d", &tmpx );
    tmpx = tmpx<1 ? 1 : (tmpx>5 ? 5 : tmpx);
    mouse_click( c, tmpx );
  }else if( IS_CMD( cmd, "key " ) ) {
    strncpy( str, &cmd[ 4 ], 128 );
    send_key( c, str );
  }else if( IS_CMD( cmd, "keydown " ) ) {
    strncpy( str, &cmd[ 8 ], 128 );
    fake_input( c, XCB_KEY_PRESS, thing_to_keycode( c, str ) );
  }else if( IS_CMD( cmd, "keyup " ) ) {
    strncpy( str, &cmd[ 6 ], 128 );
    fake_input( c, XCB_KEY_RELEASE, thing_to_keycode( c, str ) );
  }else if( IS_CMD( cmd, "mousemove " ) ) {
    sscanf( cmd, "mousemove %d %d", &tmpx, &tmpy );
    mouse_move( c, tmpx, tmpy );
  }else if( IS_CMD( cmd, "mousermove " ) ) {
    sscanf( cmd, "mousermove %d %d", &tmpx, &tmpy );
    mouse_rel_move( c, tmpx, tmpy );
  }else if( IS_CMD( cmd, "sleep " ) ) {
    sscanf( cmd, "sleep %d", &tmpx );
    dmsg( 1, "sleep %d\n", tmpx );
    sleep( tmpx );
  }else if( IS_CMD( cmd, "usleep " ) ) {
    sscanf( cmd, "usleep %d", &tmpx );
    dmsg( 1, "usleep %d\n", tmpx );
    usleep( tmpx );
  }else if( IS_CMD( cmd, "mousedown " ) ) {
    sscanf( cmd, "mousedown %d", &tmpx );
    tmpx = tmpx<1 ? 1 : (tmpx>5 ? 5 : tmpx);
    fake_input( c, XCB_BUTTON_PRESS, tmpx );
  }else if( IS_CMD( cmd, "mouseup " ) ) {
    sscanf( cmd, "mouseup %d", &tmpx );
    tmpx = tmpx<1 ? 1 : (tmpx>5 ? 5 : tmpx);
    fake_input( c, XCB_BUTTON_RELEASE, tmpx );
  }else if( IS_CMD( cmd, "str " ) ) {
    cmd += 4;
    while( cmd[ 0 ] != 0 ) {
      str[ 0 ] = cmd[ 0 ];
      send_key( c, str );
      cmd++;
    }
  /* in the absence of XStringToKeysym, allow sending hex syms directly */
  }else if( IS_CMD( cmd, "sym " ) ) {
    xcb_keysym_t sym;
    xcb_keycode_t code;
    sscanf( str, "sym %x", &sym );
    code = xcb_key_symbols_get_keycode( syms, sym );
    fake_input( c, XCB_KEY_PRESS, code );
    fake_input( c, XCB_KEY_RELEASE, code );
  }else if( IS_CMD( cmd, "symdown " ) ) {
    xcb_keysym_t sym;
    sscanf( str, "symdown %x", &sym );
    fake_input( c, XCB_KEY_PRESS, xcb_key_symbols_get_keycode( syms, sym ) );
  }else if( IS_CMD( cmd, "symup " ) ) {
    xcb_keysym_t sym;
    sscanf( str, "symup %x", &sym );
    fake_input( c, XCB_KEY_RELEASE, xcb_key_symbols_get_keycode( syms, sym ) );
  }else{
    fprintf( stderr, "Unknown command '%s'\n", cmd );
  }

  xcb_flush( c );
}

int main( int argc, char *argv[] ) {
  xcb_connection_t *c = NULL;
  int cnt;  /*, tmp_i; */
  char *buf, *display = NULL;
  int opt;
 
  while( ( opt = getopt( argc, argv, "hx:" ) ) != EOF ) {  /* "hd:x: */
    switch( opt ) {
    case 'h':
      printf( "xte v" VERSION "\n"
	      "Generates fake input using the XTest extension, more reliable than xse\n"
	      "Author: Steve Slaven - http://hoopajoo.net\n"
	      "Ported to XCB: Ian Osgood\n"
	      "Current keyboard map: " KBDMAP "\n"
	      "\n"
	      "usage: %s [-h] [-x display] [arg ..]\n"
	      "\n"
	      "  -h  this help\n"
	      "  -x  send commands to remote X server.  Note that some commands\n"
	      "      may not work correctly unless the display is on the console,\n"
	      "      e.g. the display is currently controlled by the keyboard and\n"
	      "      mouse and not in the background.  This seems to be a limitation\n"
	      "      of the XTest extension.\n"
	      "  arg args instructing the little man on what to do (see below)\n"
	      "      if no args are passec, commands are read from stdin separated\n"
	      "      by newlines, to allow a batch mode\n"
	      "\n"
	      " Commands:\n"
	      "  key k          Press and release key k\n"
	      "  keydown k      Press key k down\n"
	      "  keyup k        Release key k\n"
	      "  str string     Do a bunch of key X events for each char in string\n"
	      "  mouseclick i   Click mouse button i\n"
	      "  mousemove x y  Move mouse to screen position x,y\n"
	      "  mousermove x y Move mouse relative from current location by x,y\n"
	      "  mousedown i    Press mouse button i down\n"
	      "  mouseup i      Release mouse button i\n"
	      "  sleep x        Sleep x seconds\n"
	      "  usleep x       uSleep x microseconds\n"
	      "\n"
	      "Some useful keys (case sensitive)\n"
	      "  Home\n"
	      "  Left\n"
	      "  Up\n"
	      "  Right\n"
	      "  Down\n"
	      "  Page_Up\n"
	      "  Page_Down\n"
	      "  End\n"
	      "  Return\n"
	      "  Backspace\n"
	      "  Tab\n"
	      "  Escape\n"
	      "  Delete\n"
	      "  Shift_L\n"
	      "  Shift_R\n"
	      "  Control_L\n"
	      "  Control_R\n"
	      "  Meta_L\n"
	      "  Meta_R\n"
	      "  Alt_L\n"
	      "  Alt_R\n"
	      "\n"
	      "Sample, drag from 100,100 to 200,200 using mouse1:\n"
	      "  xte 'mousemove 100 100' 'mousedown 1' 'mousemove 200 200' 'mouseup 1'\n"
	      "\n"
	      , argv[ 0 ] );
      exit( 0 );
      break;
#if 0      
    case 'd':
      sscanf( optarg, "%d", &tmp_i );
      dmsg( 2, "Debug set to %d\n", tmp_i );
      debug_level( tmp_i );
      break;
#endif
    case 'x':
      display = optarg;
      break;

    case '?':
      fprintf( stderr, "Unknown option '%c'\n", optopt );
      break;
      
    default:
      fprintf( stderr, "Unhandled option '%c'\n", opt );
      break;
    }
  }

  c = xcb_connect( display, NULL );
  if( c == NULL ) {
    fprintf( stderr, "Unable to open display '%s'\n", display == NULL ? "default" : display );
    exit( 1 );
  }
  
  /* do XTest init and version check (need 2.1) */
  /* xcb_test_init( c );   required? none of the other extension demos do this */
  
  xcb_test_get_version_cookie_t cookie = xcb_test_get_version( c, 2, 1 );

  xcb_generic_error_t *e = NULL;
  xcb_test_get_version_reply_t *xtest_reply = xcb_test_get_version_reply ( c, cookie, &e );
  if (xtest_reply) {
    fprintf( stderr, "XTest version %u.%u\n",
      (unsigned int)xtest_reply->major_version,
      (unsigned int)xtest_reply->minor_version );
    free(xtest_reply);
  }
  if (e) {
    fprintf( stderr, "XTest version error: %d", (int)e->error_code );
    free(e);
  }
  
  
  /* prep for keysym-->keycode conversion */
  syms = xcb_key_symbols_alloc( c );

  if( argc - optind >= 1 ) {
    /* Arg mode */
    for( cnt = optind; cnt < argc; cnt++ ) {
      process_command( c, argv[ cnt ] );
    }
  }else{
    /* STDIN mode */
    buf = (char *)malloc( 128 );
    while( fgets( buf, 128, stdin ) ) {
      buf[ strlen( buf ) - 1 ] = 0; /* Chop \n */
      process_command( c, buf );
    }
  }
  
  xcb_key_symbols_free( syms );

  xcb_disconnect( c );
  exit( 0 );
}
