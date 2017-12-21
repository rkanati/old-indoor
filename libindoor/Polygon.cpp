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

#include "Polygon.hpp"

namespace In
{
# define polyblock_size 256
  struct PolyBlock
  {
    Polygon polys [polyblock_size];
    Polygon* cur_poly;
    PolyBlock* next;
    
    PolyBlock () :
      cur_poly (polys)
    {}
    
    inline bool full () const
    {
      return cur_poly == polys + polyblock_size;
    }
    
  };
  
# define free_polys_size 256
  static Polygon* free_polys [free_polys_size];
  static Polygon** free_poly_ptr = free_polys;
  
  static PolyBlock* head_poly_block = 0;
  
  Polygon* polygon_alloc ()
  {
    if (free_poly_ptr > free_polys)
      return *--free_poly_ptr;
    
    if (!head_poly_block || head_poly_block -> full ())
    {
      PolyBlock* new_pb = new PolyBlock;
      new_pb -> next = head_poly_block;
      head_poly_block = new_pb;
    }
    
    return head_poly_block -> cur_poly++;
  }
  
  void polygon_free (Polygon* poly)
  {
    if (!poly)
      return;
    
    if (free_poly_ptr == free_polys + free_polys_size)
      return; // Soft leak. Inefficient, but not deadly.
    
    *free_poly_ptr++ = poly;
  }
  
  void polygon_cleanup ()
  {
    while (head_poly_block)
    {
      PolyBlock* next = head_poly_block -> next;
      delete head_poly_block;
      head_poly_block = next;
    }
  }
  
}
