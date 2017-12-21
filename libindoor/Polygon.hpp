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

#ifndef INDOOR_H_POLYGON
#define INDOOR_H_POLYGON

#include "Vector3.hpp"
#include <cassert>

namespace In
{
  //
  // Polygon
  //
# ifndef polygon_max_vertices
# define polygon_max_vertices 8
# endif
  
  struct Polygon
  {
    Vector3 vertices [polygon_max_vertices];
    Vector3* vertices_end;
    
    inline Polygon () :
      vertices_end (vertices)
    {}
    
    inline Polygon (const Vector3* verts, unsigned count) :
      vertices_end (vertices)
    {
      if (count > polygon_max_vertices)
        return;
      
      while (count--)
        *vertices_end++ = *verts++;
    }
    
    inline void clear ()
    {
      assert (this);
      vertices_end = vertices;
    }
    
    inline bool add_vertex (const Vector3& p)
    {
      assert (this);
      assert (&p);
      
      if (vertices_end == vertices + polygon_max_vertices)
        return false;
      
      *vertices_end++ = p;
      return true;
    }
    
    inline unsigned size () const
    {
      assert (this);
      return vertices_end - vertices;
    }
    
    inline bool empty () const
    {
      assert (this);
      return (vertices_end == vertices);
    }
    
    inline       Vector3* begin ()       { return vertices;     }
    inline       Vector3* end   ()       { return vertices_end; }
    inline const Vector3* begin () const { return vertices;     }
    inline const Vector3* end   () const { return vertices_end; }
    
    inline Vector3 normal () const
    {
      assert (this);
      assert (vertices_end >= vertices + 3);
      
      const Vector3& p0 = vertices [0];
      const Vector3& p1 = vertices [1];
      const Vector3& p2 = vertices [2];
      
      return cross (p2 - p1, p0 - p1).unit ();
    }
    
    inline double distance () const
    {
      assert (this);
      assert (vertices_end >= vertices + 1);
      
      return dot (normal (), vertices [0]);
    }
    
    inline void operator = (const Polygon& other)
    {
      assert (this);
      assert (&other);
      
      clear ();
      for (const Vector3* v = other.vertices; v != other.vertices_end; v++)
        add_vertex (*v);
    }
    
  };
  
  Polygon* polygon_alloc   ();
  void     polygon_free    (Polygon* poly);
  void     polygon_cleanup ();
  
}

#endif
