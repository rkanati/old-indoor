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

#if 1

#include "World.hpp"
#include "Polygon.hpp"
#include "Portal.hpp"

#include <cstdio>
#include <cassert>

using namespace In;

//
// load_poly_file
//
static unsigned load_poly_file (const char* filename, Polygon* polys, unsigned size)
{
  FILE* file = fopen (filename, "r");
  if (!file)
    return 0;
  
  unsigned line = 1;
  
  double coord [3];
  int cur_coord = 0;
  
  char coord_buf [64];
  char* cur_buf = coord_buf;
  
  Polygon* cur_poly = polys;
  
  char c;
  bool need_char = true;
  
  for (bool done = false; !done;)
  {
    if (need_char)
    {
      c = fgetc (file);
      if (c == -1)
      {
        c = '\n';
        done = true;
      }
      
      need_char = false;
    }
    
    switch (c)
    {
      // Terminate polygon
      case '\n':
        if (cur_buf > coord_buf)
        {
          printf ("Parse error on line %u - expected \';\'\n", line);
          fclose (file);
          return 0;
        }
        
        if (!cur_poly -> empty ())
        {
          if (cur_poly -> size () < 3)
          {
            printf ("Degenerate polygon on line %u\n", line);
            fclose (file);
            return 0;
          }
          
          cur_poly++;
          
          if (cur_poly == polys + size)
          {
            printf ("Out of polys\n");
            fclose (file);
            return 0;
          }
        }
        
        line++;
      break;
      
      // Skip whitespace or terminate coordinate
      case ' ':
      case '\t':
        if (cur_buf > coord_buf)
        {
          *cur_buf = '\0';
          coord [cur_coord++] = atof (coord_buf);
          cur_buf = coord_buf;
        }
      break;
      
      // Skip comments
      case '/':
        c = fgetc (file);
        
        if (c == '*')
        {
          c = 0;
          
          while (c != '/');
          {
            while (c != '*')
              c = fgetc (file);
            
            c = fgetc (file);
          }
          
          break;
        }
        else if (c != '/')
        {
          printf ("Parse error on line %u - expected \'/\' after \'/\'\n", line);
          fclose (file);
          return 0;
        }
      case '#':
        do { c = fgetc (file); } while (c != '\n');
        continue; // Keep the \n for parsing
      break;
      
      // Read coordinate
      case '-':
        if (cur_buf > coord_buf)
        {
          printf ("Parse error on line %u - expected digit\n", line);
          fclose (file);
          return 0;
        }
      case '0': case '.': 
      case '1': case '2': case '3':
      case '4': case '5': case '6':
      case '7': case '8': case '9':
        if (cur_coord > 2)
        {
          printf ("Parse error on line %u - expected \';\'\n", line);
          fclose (file);
          return 0;
        }
        
        if (cur_buf < coord_buf + 63)
          *cur_buf++ = c;
      break;
      
      // Terminate vector
      case ';':
        if (cur_buf > coord_buf)
        {
          *cur_buf = '\0';
          coord [cur_coord++] = atof (coord_buf);
          cur_buf = coord_buf;
        }
        
        if (cur_coord < 3)
        {
          printf ("Parse error on line %u - expected coordinate\n", line);
          fclose (file);
          return 0;
        }
        
        cur_poly -> add_vertex (Vector3 (coord [0], coord [1], coord [2]));
        
        cur_coord = 0;
      break;
      
      default:
        printf ("Parse error on line %u - unrecognized character \'%c\'\n", line, c);
        fclose (file);
      return 0;
    }
    // switch (c)
    
    need_char = true;
  }
  // for (;;)
  
  fclose (file);
  return cur_poly - polys;
}

//
// print_status
//
static void print_status (const char* status)
{
  printf ("%s\n", status);
}

//
// main
//
int main ()
{
  Polygon polys [1024];
  
  unsigned count = load_poly_file ("Polys.txt", polys, 1024);
  if (!count)
    return 1;
  
  World* world = world_compile (polys, count, print_status);
  
  print_status ("world_save...");
  world_save (world, "Test.indoor");
  
  world_free (world);
  
  polygon_cleanup ();
  portal_cleanup ();
  
  return 0;
}

#endif
