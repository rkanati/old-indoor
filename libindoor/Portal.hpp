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

#ifndef INDOOR_H_PORTAL
#define INDOOR_H_PORTAL

#include "Polygon.hpp"
#include <cassert>

namespace In
{
  //
  // Portal
  //
  struct Node;
  
  struct Portal
  {
    Polygon poly;
    Node *a, *b;
    
    inline Portal () : a (0), b (0) { }
    
  };
  
  Portal* portal_alloc   ();
  void    portal_free    (Portal* portal);
  void    portal_cleanup ();
  
  inline bool portal_valid (const Portal* portal)
  {
    if (!portal)
      return false;
    else if (portal -> a && portal -> b)
      return true;
    else if (!portal -> a && !portal -> b)
      return false;
    else
      assert (false);
  }
  
}

#endif
