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

#include "Portal.hpp"

namespace In
{
# define portalblock_size 1024
  struct PortalBlock
  {
    Portal portals [portalblock_size];
    Portal* cur_portal;
    PortalBlock* next;
    
    PortalBlock () :
      cur_portal (portals)
    {}
    
    inline bool full () const
    {
      return cur_portal == portals + portalblock_size;
    }
    
  };
  
# define free_portals_size 256
  static Portal* free_portals [free_portals_size];
  static Portal** free_portal_ptr = free_portals;
  
  static PortalBlock* head_portal_block = 0;
  
  Portal* portal_alloc ()
  {
    if (free_portal_ptr > free_portals)
      return *--free_portal_ptr;
    
    if (!head_portal_block || head_portal_block -> full ())
    {
      PortalBlock* new_pb = new PortalBlock;
      new_pb -> next = head_portal_block;
      head_portal_block = new_pb;
    }
    
    return head_portal_block -> cur_portal++;
  }
  
  void portal_free (Portal* portal)
  {
    if (!portal)
      return;
    
    if (portal_valid (portal))
    {
      portal -> a = 0;
      portal -> b = 0;
      return;
    }
    
    if (free_portal_ptr == free_portals + free_portals_size)
      return; // Soft leak. Inefficient, but not deadly.
    
    *free_portal_ptr++ = portal;
  }
  
  void portal_cleanup ()
  {
    while (head_portal_block)
    {
      PortalBlock* next = head_portal_block -> next;
      delete head_portal_block;
      head_portal_block = next;
    }
  }
  
}
