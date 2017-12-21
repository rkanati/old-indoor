//
// Copyright (C) 2009 Roadkill Software
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include <Rk/Display.hpp>
#include <Rk/RenderContext.hpp>
#include <Rk/OpenGL.hpp>

#include "World.hpp"

static HINSTANCE nt_dll;
typedef unsigned (*TimerResFn) (unsigned, bool, unsigned*);
static TimerResFn NtSetTimerResolution;

static HINSTANCE winmm_dll;
typedef unsigned (*timeGetTimeType) ();
static timeGetTimeType timeGetTime = 0;

/*static double msec_per_count;
static rki64 clock_base;*/

/*void clock_reset ()
{
  QueryPerformanceCounter ((LARGE_INTEGER*) &clock_base);
}*/

inline unsigned clock_now ()
{
  /*rki64 now;
  QueryPerformanceCounter ((LARGE_INTEGER*) &now);
  return double (now - clock_base) * msec_per_count;*/
  return timeGetTime ();
}

static void timing_initialize ()
{
  nt_dll = LoadLibrary ("ntdll.dll");
  assert (nt_dll);
  
  NtSetTimerResolution = (TimerResFn) GetProcAddress (
    nt_dll, "NtSetTimerResolution"
  );
  assert (NtSetTimerResolution);
  
  typedef unsigned (*TimerQueryFn) (unsigned*, unsigned*, unsigned*);
  
  TimerQueryFn NtQueryTimerResolution = (TimerQueryFn) GetProcAddress (
    nt_dll, "NtQueryTimerResolution"
  );
  assert (NtQueryTimerResolution);
  
  winmm_dll = LoadLibrary ("winmm.dll");
  assert (winmm_dll);
  
  timeGetTime = (timeGetTimeType) GetProcAddress (winmm_dll, "timeGetTime");
  assert (timeGetTime);
  
  unsigned best_res;
  unsigned worst_res;
  unsigned dummy;
  NtQueryTimerResolution (&worst_res, &best_res, &dummy);
  
  NtSetTimerResolution (best_res > 10000 ? best_res : 10000, true, &dummy);
  
  /*rki64 clock_freq;
  QueryPerformanceFrequency ((LARGE_INTEGER*) &clock_freq);
  
  msec_per_count = 1000.0 / double (clock_freq);*/
}

static void timing_shutdown ()
{
  unsigned dummy;
  NtSetTimerResolution (0, false, &dummy);
  FreeLibrary (winmm_dll);
  FreeLibrary (nt_dll);
}

//
// handle_message
//
static LRESULT handle_message (Rk::Display* source, unsigned message, WPARAM wp, LPARAM lp)
{
  switch (message)
  {
    case WM_CLOSE:
      PostQuitMessage (0);
    break;
    
    case WM_KEYDOWN:
    break;
    
    case WM_KEYUP:
    break;
    
    default:
      return Rk::display_default_message (source, message, wp, lp);
  }
  
  return 0;
}

//
// frame_begin
//
static void frame_begin (int w, int h)
{
  glViewport (0, 0, w, h);
  
  glClearColor (0.5, 0.75, 0.9, 1.0);
  glClear (GL_COLOR_BUFFER_BIT);
  
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (75.0, float (w) / float (h), 0.1, 100.0);
  
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  gluLookAt (
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    0.0, 0.0, 1.0
  );
  //glTranslatef (-player_x (), -player_y () - player_height () / 2, 0);
}

//
// frame_end
//
static void frame_end (Rk::RenderContext* context)
{
  Rk::rendercontext_flip (context);
}

//
// WinMain
//
int WINAPI WinMain (HINSTANCE instance, HINSTANCE, LPSTR, int)
{
  timing_initialize ();
  
  World* world = world_load ("libindoor/Test.indoor");
  assert (world);
  
  PeekMessage (0, 0, 0, 0, PM_NOREMOVE);
  
  Rk::display_initialize (instance);
  Rk::Display* display = Rk::display_create ("Terrain", handle_message, false, 1024, 768);
  Rk::display_show (display);
  Rk::RenderContext* render_context = rendercontext_create (display);
  
  MSG message;
  const unsigned frame_rate_hz = 75;
  const unsigned frame_delay = 1000 / frame_rate_hz;
  unsigned before = clock_now ();
  
  glEnable (GL_CULL_FACE);
  
  for (;;)
  {
    // Handle messages
    while (PeekMessage (&message, 0, 0, 0, PM_REMOVE))
    {
      if (message.message == WM_QUIT)
        break;
      
      DispatchMessage (&message);
    }
    
    if (message.message == WM_QUIT)
      break;
    
    //
    // Do stuff
    //
    unsigned w, h;
    display_get_size (display, w, h);
    
    frame_begin (w, h);
      world_render (world, Vector3 (), Vector3 (1, 0, 0));
    
    unsigned elapsed = clock_now () - before;
    while (elapsed < frame_delay)
    {
      Sleep (1);
      elapsed = clock_now () - before;
    }
    
    before += frame_delay;
    
    frame_end (render_context);
  }
  
  // Shut down
  Rk::display_hide (display);
  
  Rk::rendercontext_free (render_context);
  Rk::display_free (display);
  Rk::display_shutdown ();
  
  world_free (world);
  
  timing_shutdown ();
  
  return 0;
}
