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

#include "World.hpp"
#include "Portal.hpp"

#include <cassert>
#include <cstdio>

#include <Rk/Types.hpp>
#include <Rk/Plane.hpp>

typedef Rk::Plane <rkf64> Plane;
using Rk::PlaneSide;

namespace In
{
  //inline void debug_trap (bool exp = false) { if (!exp) { *((int*) 0) = 1; } }
  #define debug_trap(exp) { if (!(exp)) { *((int*) 0) = 1; } }
  
  PlaneSide polygon_side (const Plane& plane, const Polygon& poly)
  {
    double first_dist = 0.0;
    
    for (const Vector3*
      v  = poly.vertices;
      v != poly.vertices_end;
      v++)
    {
      double dist = plane.point_distance (*v);
      
      if (dist > -0.00001 && dist < 0.00001)
        dist = 0.0;
      
      if (first_dist == 0.0)
      {
        first_dist = dist;
      }
      else if (dist * first_dist < 0)
      {
        return plane_side_across;
      }
    }
    
    return (first_dist < 0 ? plane_side_back : plane_side_front);
  }
  
  //
  // is_polygon_in
  // Returns true if Poly lies roughly within plane, false otherwise.
  //
  bool is_polygon_in (const Polygon& poly, const Plane& plane)
  {
    for (const Vector3*
      v  = poly.vertices;
      v != poly.vertices_end;
      v++)
    {
      const double d = plane.point_distance (*v);
      if (d > 0.00001 || d < -0.00001)
        return false;
    }
    
    return true;
  }

  //
  // MapPlane
  //
# define mapplane_max_polys   1024
# define mapplane_max_portals 1024
  
  struct MapPlane
  {
    MapPlane *prev, *next;
    
    Plane plane;
    
    Polygon* polys [mapplane_max_polys];
    Polygon** cur_poly;
    
    inline       Polygon**       polys_begin ()       { return polys; }
    inline const Polygon* const* polys_begin () const { return polys; }
    inline       Polygon**       polys_end   ()       { return cur_poly; }
    inline const Polygon* const* polys_end   () const { return cur_poly; }
    
    Portal* portals [mapplane_max_portals];
    Portal** cur_portal;
    
    inline       Portal**       portals_begin ()       { return portals; }
    inline const Portal* const* portals_begin () const { return portals; }
    inline       Portal**       portals_end   ()       { return cur_portal; }
    inline const Portal* const* portals_end   () const { return cur_portal; }
    
    inline unsigned portal_count () const { return cur_portal - portals; }
    
    PlaneSide boundary;
    
    MapPlane () :
      prev (0), next (0),
      cur_poly (polys),
      cur_portal (portals),
      boundary (0)
    {}
    
    ~MapPlane ()
    {
      clear_polys   ();
      clear_portals ();
    }
    
    void add_poly (Polygon* poly)
    {
      assert (this);
      assert (poly);
      assert (cur_poly != polys + mapplane_max_polys);
      
      *cur_poly++ = poly;
    }
    
    void clear_polys ()
    {
      assert (this);
      
      for (Polygon** p = polys_begin (); p != polys_end (); p++)
        if (*p) polygon_free (*p);
      
      cur_poly = polys;
    }
    
    void add_portal (Portal* port)
    {
      assert (this);
      assert (port);
      assert (cur_portal != portals + mapplane_max_portals);
      
      *cur_portal++ = port;
    }
    
    void clear_portals ()
    {
      assert (this);
      
      for (Portal** p = portals_begin (); p != portals_end (); p++)
        if (*p) portal_free (*p);
      
      cur_portal = portals;
    }
    
  };
  
  //
  // mapplane_insert
  //
  static void mapplane_insert (MapPlane** head, MapPlane* mp)
  {
    assert (head);
    assert (mp);
    
    mp -> prev = 0;
    mp -> next = *head;
    if (*head)
      (*head) -> prev = mp;
    *head = mp;
  }
  
  //
  // mapplane_remove
  //
  static MapPlane* mapplane_remove (MapPlane** head, MapPlane* mp)
  {
    assert (head);
    assert (mp);
    
    MapPlane* next = mp -> next;
    
    if (mp -> prev)
      mp -> prev -> next = mp -> next;
    else if (*head == mp)
      *head = mp -> next;
    
    if (mp -> next)
      mp -> next -> prev = mp -> prev;
    
    mp -> next = 0;
    mp -> prev = 0;
    
    return next;
  }
  
  //
  // mapplane_free
  //
  static MapPlane* mapplane_free (MapPlane** head, MapPlane* mp)
  {
    MapPlane* next = mapplane_remove (head, mp);
    delete mp;
    return next;
  }
  
  //
  // NodeContents
  //
  typedef char NodeContents;
# define contents_nonleaf -1
# define contents_empty    0
# define contents_solid    1
# define contents_outside  2
  
  //
  // Node
  //
  struct Node
  {
    Plane partition;
    MapPlane* maps;
    Node *front, *back;
    NodeContents contents;
    
    Node () :
      maps (0),
      front (0), back (0)
    {}
    
    inline bool is_leaf () const
    {
      return contents != contents_nonleaf;
    }
    
  };
  
  
  //
  // map_by_plane
  //
  static MapPlane* map_by_plane (Polygon* polys, unsigned count)
  {
    assert (polys);
    assert (count);
    
    MapPlane* maps = 0;
    int map_count = 0;
    
    for (Polygon*
      cur_poly  = polys;
      cur_poly != polys + count;
      cur_poly++)
    {
      MapPlane* cur_map = maps;
      
      for (;cur_map != 0; cur_map = cur_map -> next)
      {
        if (is_polygon_in (*cur_poly, cur_map -> plane))
        {
          cur_map -> add_poly (cur_poly);
          break;                         // ---+
        }                                //    |
      }                                  //    |
                                         //    |
      if (cur_map != 0)                  // <--+
        continue;
      
      Vector3 n = cur_poly -> normal   ();
      double  d = cur_poly -> distance ();
      
      MapPlane* new_map = new MapPlane;
      new_map -> prev = 0;
      new_map -> next = maps;
      new_map -> plane = Plane (n, d);
      new_map -> add_poly (cur_poly);
      maps = new_map;
      map_count++;
    }
    
    return maps;
  }
  
  //
  // mark_boundary_planes
  //
  static unsigned mark_boundary_planes (MapPlane* maps, MapPlane** boundaries, unsigned max_boundaries)
  {
    MapPlane** cur_boundary = boundaries;
    
    for (MapPlane*
      cur_map  = maps;
      cur_map != 0;
      cur_map  = cur_map -> next)
    {
      bool is_boundary = true;
      PlaneSide first_side = 0;
      
      for (MapPlane*
        comp_map  = maps;
        comp_map != 0 && is_boundary;
        comp_map  = comp_map -> next)
      {
        // Don't compare with ourselves
        if (comp_map == cur_map)
          continue;
        
        PlaneSide comp_side = cur_map -> plane.plane_side (comp_map -> plane);
        
        // Parallel planes are easy
        if (comp_side == plane_side_front || comp_side == plane_side_back)
        {
          if (first_side == 0)
            first_side = comp_side;
          else if (comp_side != first_side)
            is_boundary = false;
        }
        else if (comp_side == plane_side_across)
        {
          for (Polygon**
            p_cur_poly  = comp_map -> polys;
            p_cur_poly != comp_map -> cur_poly && is_boundary;
            p_cur_poly++)
          {
            const Polygon* cur_poly = *p_cur_poly;
            
            PlaneSide poly_side = polygon_side (cur_map -> plane, *cur_poly);
            
            if (poly_side == plane_side_front || poly_side == plane_side_back)
            {
              if (first_side == 0)
                first_side = poly_side;
              else if (poly_side != first_side)
                is_boundary = false;
            }
            else if (poly_side == plane_side_across)
            {
              is_boundary = false;
            }
            else // if (poly_side == plane_side_in)
            {
              debug_trap (false);
              // Plane map failure: coplanar polygon in nonparallel plane
            }
            
          }
          // p_cur_poly
        }
        else // if (comp_side == plane_side_in)
        {
          debug_trap (false);
          // Plane map failure: close parallel planes detected
        }
        
      }
      // comp_map
      
      if (is_boundary)
      {
        cur_map -> boundary = first_side;
        *cur_boundary++ = cur_map;
      }
    }
    // cur_map
    
    return cur_boundary - boundaries;
  }
  // mark_boundary_planes
  
  //
  // strip_boundary_planes
  //
  static void strip_boundary_planes (MapPlane** boundaries, unsigned boundary_count)
  {
    for (MapPlane**
      b = boundaries;
      b < boundaries + boundary_count;
      b++)
    {
      (*b) -> clear_polys ();
    }
  }
  
  //
  // make_plane_poly
  //
  static void make_plane_poly (const Plane* plane, Polygon* poly)
  {
    assert (plane);
    assert (poly);
    
    double max_len = -1E16;
    char major_axis = 0;
    
    double len = (plane -> normal.x < 0.0 ? -plane -> normal.x : plane -> normal.x);
    if (len > max_len)
    {
      max_len = len;
      major_axis = 'x';
    }
    
    len = (plane -> normal.y < 0.0 ? -plane -> normal.y : plane -> normal.y);        
    if (len > max_len)
    {
      max_len = len;
      major_axis = 'y';
    }
        
    len = (plane -> normal.z < 0.0 ? -plane -> normal.z : plane -> normal.z);        
    if (len > max_len)
    {
      //max_len = len;
      major_axis = 'z';
    }
    
    Vector3 up;
    
    switch (major_axis)
    {
      case 'x':
      case 'y':
        up.z = 1.0;
      break;
      
      case 'z':
        up.x = 1.0;
      break;
      
      default:
        debug_trap (false); // bad plane
    }
    
    double cosine = dot (up, plane -> normal);
    up += plane -> normal * -cosine;
    up.normalize ();
    
    Vector3 right = cross (up, plane -> normal);
    
    up    *= 1000000;
    right *= 1000000;
    
    poly -> clear ();
    
    Vector3 centre = plane -> normal * plane -> distance;
    
    poly -> add_vertex (centre + right + up);
    poly -> add_vertex (centre - right + up);
    poly -> add_vertex (centre - right - up);
    poly -> add_vertex (centre + right - up);
  }
  
  //
  // split_poly
  //
  void split_poly (const Polygon* poly, const Plane* split, Polygon* fore, Polygon* rear)
  {
    fore -> clear ();
    rear -> clear ();
    
    const Vector3* prev;
    double prev_side;
    const Vector3* cur = poly -> begin ();
    double side;
    
    while (cur != poly -> end ())
    {
      side = split -> point_distance (*cur);
      
      if (side < 0.00001 && side > -0.00001)
        side = 0;
      
      if (side * prev_side < 0 && cur != poly -> begin ())
      // Opposite sides
      {
        Vector3 intersection = split -> line_intersection (*cur, *prev);
        rear -> add_vertex (intersection);
        fore -> add_vertex (intersection);
      }
      
      if (side > -0.00001)
        fore -> add_vertex (*cur);
      
      if (side < 0.00001)
        rear -> add_vertex (*cur);
      
      prev = cur;
      cur++;
      prev_side = side;
    }
    
    // Deal with the last edge
    side = split -> point_distance (*poly -> begin ());
    
    if (side * prev_side < 0)
    {
      Vector3 intersection = split -> line_intersection (*poly -> begin (), *prev);
      rear -> add_vertex (intersection);
      fore -> add_vertex (intersection);
    }
  }
  
  //
  // make_root_portals
  //
  static void make_root_portals (Node* root, Node* outside, MapPlane** boundaries, unsigned boundary_count)
  {
    for (MapPlane**
      p_boundary = boundaries;
      p_boundary < boundaries + boundary_count;
      p_boundary++)
    {
      MapPlane* boundary = *p_boundary;
      
      Polygon portal_poly;
      make_plane_poly (&boundary -> plane, &portal_poly);
      
      for (MapPlane**
        p_clip  = boundaries;
        p_clip != boundaries + boundary_count;
        p_clip++)
      {
        MapPlane* clip = *p_clip;
        
        if (boundary == clip)
          continue; // Don't clip by ourselves
        
        PlaneSide side = boundary -> plane.plane_side (clip -> plane);
        
        if (side == plane_side_front || side == plane_side_back)
          continue; // Don't clip by parallel planes
        
        Polygon front_half, back_half;
        split_poly (&portal_poly, &clip -> plane, &front_half, &back_half);
        
        if (clip -> boundary == plane_side_front && !front_half.empty ())
        {
          portal_poly = front_half;
        }
        else if (!back_half.empty ())
        {
          portal_poly = back_half;
        }
      }
      
      Portal* new_port = portal_alloc ();
      new_port -> poly = portal_poly;
      new_port -> a    = root;
      new_port -> b    = outside;
      
      boundary -> add_portal (new_port);
    }
  }
  
  //
  // select_partition
  //
  static MapPlane* select_partition (MapPlane* maps, Plane* partition)
  {
    MapPlane* best = 0;
    unsigned best_split = ~((unsigned) 0);
    
    for (MapPlane*
      cur_map  = maps;
      cur_map != 0;
      cur_map  = cur_map -> next)
    {
      // Don't use boundary planes
      if (cur_map -> boundary)
        continue;
      
      unsigned split = 0;
      
      for (MapPlane*
        comp_map  = maps;
        comp_map != 0;
        comp_map  = comp_map -> next)
      {
        // Don't compare with ourselves
        if (comp_map == cur_map)
          continue;
        
        PlaneSide comp_side = cur_map -> plane.plane_side (comp_map -> plane);
        
        if (comp_side == plane_side_across)
        {
          for (Polygon**
            cur_poly  = comp_map -> polys_begin ();
            cur_poly != comp_map -> polys_end   ();
            cur_poly++)
          {
            PlaneSide poly_side = polygon_side (cur_map -> plane, **cur_poly);
            
            if (poly_side == plane_side_across)
              split++;
          }
          // CurPoly
        }
      }
      // CompMap
      
      if (split < best_split)
      {
        best_split = split;
        best = cur_map;
      }
    }
    // CurMap
    
    if (best)
      *partition = best -> plane;
    
    return best;
  }
  // select_partition
  
  //
  // add_portal_to_node
  //
  static void add_portals_to_node (Node* node, Node* outside, Portal* p1, Portal* p2)
  {
    for (MapPlane*
      cur_map  = node -> maps;
      cur_map != 0;
      cur_map  = cur_map -> next)
    {
      if (is_polygon_in (p1 -> poly, cur_map -> plane))
      {
        cur_map -> add_portal (p1);
        cur_map -> add_portal (p2);
        return;
      }
    }
    
    debug_trap (node == outside);
    
    MapPlane* new_map = new MapPlane;
    new_map -> plane = Plane (p1 -> poly.normal (), p1 -> poly.distance ());
    mapplane_insert (&node -> maps, new_map);
    
    new_map -> add_portal (p1);
    new_map -> add_portal (p2);
  }
  
  //
  // recursive_partition
  // I CAN SEE FOREVER
  //
  static void recursive_partition (Node* node, NodeContents potential_contents, Node* outside, void (*status) (const char*))
  {
    // Select partition
    MapPlane* partition_map = select_partition (node -> maps, &node -> partition);
    
    // If we can't find a partition, then we must be a leaf, so we're done.
    if (!partition_map)
    {
      status ("Leaf");
      
      node -> contents = potential_contents;
      
      for (MapPlane*
        m  = node -> maps;
        m != 0;
        m  = m -> next)
      {
        for (Portal**
          p  = m -> portals_begin ();
          p != m -> portals_end   ();
          p++)
        {
          if (!portal_valid (*p))
          {
            portal_free (*p);
            *p = 0;
            continue;
          }
          
          Node* other;
          
          if      ((*p) -> a == node) other = (*p) -> b;
          else if ((*p) -> b == node) other = (*p) -> a;
          else                        debug_trap (false); // Mislinked
          
          if (other -> is_leaf () && other -> contents != node -> contents)
          {
            portal_free (*p);
            *p = 0;
          }
        }
        
        if (node -> contents != contents_empty)
          m -> clear_polys ();
      }
      
      return;
    }
    
    // Not a leaf
    node -> contents = contents_nonleaf;
    node -> front = new Node;
    node -> back  = new Node;
    
    // Create the portal for this partition
    Portal* partition_portal = portal_alloc ();
    partition_portal -> a = node -> front;
    partition_portal -> b = node -> back;
    
    make_plane_poly (&partition_map -> plane, &partition_portal -> poly);
    
    // Sort geometry into children
    for (MapPlane*
      cur_map  = node -> maps;
      cur_map != 0;)
    {
      PlaneSide side;
      
      MapPlane* p_front = 0;
      MapPlane* p_back  = 0;
      
#     define CHECKFRONT                                      \
        if (!p_front) {                                      \
          p_front = new MapPlane;                            \
          p_front -> plane = cur_map -> plane;               \
          p_front -> boundary = cur_map -> boundary;         \
          mapplane_insert (&node -> front -> maps, p_front); \
        };
      
#     define CHECKBACK                                      \
        if (!p_back) {                                      \
          p_back = new MapPlane;                            \
          p_back -> plane = cur_map -> plane;               \
          p_back -> boundary = cur_map -> boundary;         \
          mapplane_insert (&node -> back -> maps, p_back);  \
        };
      
      if (cur_map == partition_map)
        side = plane_side_in;
      else
        side = node -> partition.plane_side (cur_map -> plane);
      
      // Parallel planes are easy
      if (side == plane_side_front)
      {
        status ("Front");
        
        // Move the map
        p_front = cur_map;
        cur_map = mapplane_remove (&node -> maps, cur_map);
        mapplane_insert (&node -> front -> maps, p_front);
        
        // Relink portals
        for (Portal**
          cur_portal  = p_front -> portals_begin ();
          cur_portal != p_front -> portals_end   ();
          cur_portal++)
        {
          if (!portal_valid (*cur_portal))
          {
            portal_free (*cur_portal);
            *cur_portal = 0;
            continue;
          }
          
          if      ((*cur_portal) -> a == node) (*cur_portal) -> a = node -> front;
          else if ((*cur_portal) -> b == node) (*cur_portal) -> b = node -> front;
          else    debug_trap (false); //  ("(X) Mislinked portal");
        }
        
        // Don't mis-iterate - old *cur_map is in *front now, new *cur_map is the next one already
        continue;
      }
      else if (side == plane_side_back)
      {
        status ("Back");
        
        // Move the map
        p_back = cur_map;
        cur_map = mapplane_remove (&node -> maps, cur_map);
        mapplane_insert (&node -> back -> maps, p_back);
        
        // Relink portals
        for (Portal**
          cur_portal  = p_back -> portals_begin ();
          cur_portal != p_back -> portals_end   ();
          cur_portal++)
        {
          if (!portal_valid (*cur_portal))
          {
            portal_free (*cur_portal);
            *cur_portal = 0;
            continue;
          }
          
          if      ((*cur_portal) -> a == node) (*cur_portal) -> a = node -> back;
          else if ((*cur_portal) -> b == node) (*cur_portal) -> b = node -> back;
          else    debug_trap (false); //  ("(X) Mislinked portal");
        }
        
        // Don't mis-iterate - old *cur_map is in *back now, new *cur_map is the next one already
        continue;
      }
      else if (side == plane_side_in)
      {
        status ("In");
        
        CHECKFRONT
        p_front -> boundary = plane_side_front;
        p_front -> add_portal (partition_portal);
        
        CHECKBACK
        p_back -> boundary = plane_side_back;
        p_back -> add_portal (partition_portal);
        
        for (Polygon**
          cur_poly  = cur_map -> polys_begin ();
          cur_poly != cur_map -> polys_end   ();
          cur_poly++)
        {
          const double dist = dot (
            (*cur_poly) -> normal (),
            node -> partition.normal
          );
          
          if (dist > 0)
            p_front -> add_poly (*cur_poly);
          else
            p_back -> add_poly (*cur_poly);
          
          *cur_poly = 0;
        }
      }
      else if (side == plane_side_across)
      {
        status ("Across");
        
        // *cur_map is not parallel to Partition
        
        // Clip our portal polygon if this map is a boundary
        if (cur_map -> boundary)
        {
          Polygon portal_front, portal_back;
          split_poly (&partition_portal -> poly, &cur_map -> plane, &portal_front, &portal_back);
          
          if (cur_map -> boundary == plane_side_front && !portal_front.empty ())
          {
            partition_portal -> poly = portal_front;
          }
          else if (cur_map -> boundary == plane_side_back && !portal_back.empty ())
          {
            partition_portal -> poly = portal_back;
          }
          else
          {
            status ("Portal polygon empty after clipping by boundary plane");
            debug_trap (false);
          }
        }
        
        // We need to sort every
        //  polygon in *cur_map into the correct child, splitting when necessary
        for (Polygon**
          cur_poly  = cur_map -> polys_begin ();
          cur_poly != cur_map -> polys_end   ();
          cur_poly++)
        {
          const PlaneSide poly_side = polygon_side (node -> partition, **cur_poly);
          
          if (poly_side == plane_side_front)
          {
            // Add this plane into the front if we haven't already
            CHECKFRONT
            p_front -> add_poly (*cur_poly);
            *cur_poly = 0; // **cur_poly is in *front, don't free it next map
          }
          else if (poly_side == plane_side_back)
          {
            // Add this plane into the back if we haven't already
            CHECKBACK
            p_back -> add_poly (*cur_poly);
            *cur_poly = 0; // **cur_poly is in *back, don't free it next map
          }
          else if (poly_side == plane_side_across)
          {
            Polygon* front_half = polygon_alloc ();
            Polygon* back_half  = polygon_alloc ();
            
            split_poly (*cur_poly, &node -> partition, front_half, back_half);
            
            // Leave these new polygons for later. Saves duplicating the front
            //  and back cases here. Extra empty checks, just in case.
            
            if (!front_half -> empty ())
            {
              CHECKFRONT
              p_front -> add_poly (front_half);
            }
            
            if (!back_half -> empty ())
            {
              CHECKBACK
              p_back -> add_poly (back_half);
            }
          }
          // if (PolySide == ...)
        }
        // for (Polys)
        
        //
        // Perform the same process for portals
        //
        for (Portal**
          cur_portal  = cur_map -> portals_begin ();
          cur_portal != cur_map -> portals_end ();
          cur_portal++) // shoulder been erase
        {
          if (!portal_valid (*cur_portal))
          {
            portal_free (*cur_portal);
            *cur_portal = 0;
            continue;
          }
          
          PlaneSide portal_side = polygon_side (node -> partition, (*cur_portal) -> poly);
          
          if (portal_side == plane_side_front)
          {
            
            if      ((*cur_portal) -> a == node) (*cur_portal) -> a = node -> front;
            else if ((*cur_portal) -> b == node) (*cur_portal) -> b = node -> front;
            else                                 debug_trap (false); // mislinked
            
            CHECKFRONT
            p_front -> add_portal (*cur_portal);
            *cur_portal = 0; // **cur_portal is in *front, don't free it next map
          }
          else if (portal_side == plane_side_back)
          {
            if      ((*cur_portal) -> a == node) (*cur_portal) -> a = node -> back;
            else if ((*cur_portal) -> b == node) (*cur_portal) -> b = node -> back;
            else                                 debug_trap (false); // mislinked
            
            CHECKBACK
            p_back -> add_portal (*cur_portal);
            *cur_portal = 0; // **cur_portal is in *back, don't free it next map
          }
          else if (portal_side == plane_side_across)
          {
            Node* other_node = (
              (*cur_portal) -> a == node ?
              (*cur_portal) -> b :
              (*cur_portal) -> a
            );
            
            Portal* front_portal = portal_alloc ();
            front_portal -> a = node -> front;
            front_portal -> b = other_node;
            
            Portal* back_portal = portal_alloc ();
            back_portal -> a = node -> back;
            back_portal -> b = other_node;
            
            split_poly (
              &((*cur_portal) -> poly),
              &node -> partition,
              &(front_portal -> poly),
              &(back_portal  -> poly)
            );
            
            debug_trap (!front_portal -> poly.empty ());
            debug_trap (!back_portal  -> poly.empty ());
            
            add_portals_to_node (other_node, outside, front_portal, back_portal);
            
            CHECKFRONT
            p_front -> add_portal (front_portal);
            
            CHECKBACK
            p_back -> add_portal (back_portal);
          }
          // if (PortalSide == ...)
        }
        // for (Portals)
      }
      else // if (Side == PlaneType::In)
      {
        debug_trap (false); //  ("(X) Coplanar plane detected");
      }
      // if (Side == ...)
      
      cur_map = mapplane_free (&node -> maps, cur_map);
    }
    // for (Maps)
    
    status ("recursive_partition (front)...");
    recursive_partition (node -> front, contents_empty, outside, status);
    
    status ("recursive_partition (back)...");
    recursive_partition (node -> back,  contents_solid, outside, status);
  }
  // recursive_partition
  
  //
  // verify_portals
  //
  static void verify_portals (Node* node)
  {
    if (node -> is_leaf ())
    {
      unsigned portal_count = 0;
      unsigned removed_count = 0;
      
      for (MapPlane*
        m  = node -> maps;
        m != 0;
        m  = m -> next)
      {
        portal_count += m -> portal_count ();
        
        for (Portal**
          p  = m -> portals_begin ();
          p != m -> portals_end ();
          p++)
        {
          if (!portal_valid (*p))
          {
            portal_free (*p);
            *p = 0;
            removed_count++;
            continue;
          }
          
          Node* n;
          
          if      ((*p) -> a == node) n = (*p) -> b;
          else if ((*p) -> b == node) n = (*p) -> a;
          else                        debug_trap (false); // mislinked
          
          debug_trap (n -> is_leaf ()); // portal to non-leaf
        }
        // for (Portals ...
      }
      // for (Maps ...
      
      //cout << "Leaf with " << PortalCount << " portals now has " << PortalCount - RemovedCount << "\n";
    }
    else
    {
      verify_portals (node -> front);
      verify_portals (node -> back);
    }
  }
  
  //
  // OutsideLeaves
  //
# define outsideleaves_max_size 1024
  struct OutsideLeaves
  {
    Node* leaves [outsideleaves_max_size];
    Node** cur_leaf;
  };
  
  //
  // fill_outside_flow
  //
  static void fill_outside_flow (Node* node, OutsideLeaves* ols)
  {
    for (MapPlane*
      m  = node -> maps;
      m != 0;
      m  = m -> next)
    {
      m -> clear_polys ();
      
      for (Portal**
        p  = m -> portals_begin ();
        p != m -> portals_end ();
        p++)
      {
        if (!portal_valid (*p))
        {
          portal_free (*p);
          *p = 0;
          continue;
        }
        
        Node* other = (
          (*p) -> a == node
        ? (*p) -> b
        : (*p) -> a
        );
        
        for (Node**
          ol  = ols -> leaves;
          ol != ols -> cur_leaf;
          ol++)
        {
          if (*ol == other)
          {
            other = 0;
            break;
          }
        }
        
        if (other == 0)
          continue;
        
        other -> contents = node -> contents;
        *ols -> cur_leaf++ = other;
        
        fill_outside_flow (other, ols);
        
        portal_free (*p);
        *p = 0;
      }
    }
  }
  
  //
  // fill_outside
  //
  static void fill_outside (Node* outside)
  {
    OutsideLeaves ols;
    ols.cur_leaf = ols.leaves;
    *ols.cur_leaf++ = outside;
    
    fill_outside_flow (outside, &ols);
  }
  
  //
  // world_locate
  //
  static Node* world_locate (Node* node, Vector3 point)
  {
    assert (node);
    
    while (node -> front && node -> back)
    {
      double side = node -> partition.point_distance (point);
      if (side < -0.00001)
        node = node -> back;
      else
        node = node -> front;
    }
    
    return node;
  }
  
  //
  // check_entities
  //
  static void check_entities (Node* root, void (*status) (const char*))
  {
    Node* origin_leaf = world_locate (root, Vector3 ());
    
    if (origin_leaf -> contents == contents_outside)
      status ("LEAK LEAK LEAK");
    else if (origin_leaf -> contents == contents_solid)
      status ("- Warning: entity embedded in solid");
  }
  
  //
  // World
  //
  struct World
  {
    Node root;
    Node outside;
    
  };
  
  //
  // dummy_status
  //
  static void dummy_status (const char*)
  {
    
  }
  
  //
  // world_compile
  //
  World* world_compile (Polygon* polys, unsigned count, void (*status) (const char*))
  {
    if (!status)
      status = dummy_status;
    
    assert (polys);
    assert (count);
    
    World* world = new World;
    world -> outside.contents = contents_outside;
    
    status ("map_by_plane...");
    world -> root.maps = map_by_plane (polys, count);
    
    status ("mark_boundary_planes...");
    MapPlane* boundaries [1024];
    unsigned boundary_count = mark_boundary_planes (world -> root.maps, boundaries, 1024);
    
    status ("strip_boundary_planes...");
    strip_boundary_planes (boundaries, boundary_count);
    
    status ("make_root_portals...");
    make_root_portals (&world -> root, &world -> outside, boundaries, boundary_count);
    
    status ("recursive_partition...");
    recursive_partition (&world -> root, contents_empty, &world -> outside, status);
    
    status ("verify_portals...");
    verify_portals (&world -> root);
    
    status ("fill_outside...");
    fill_outside (&world -> outside);
    
    status ("check_entities...");
    check_entities (&world -> root, status);
    
    status ("Done");
    return world;
  }
  
  //
  // world_free
  //
  void world_free (World* world)
  {
    if (!world)
      return;
    
    MapPlane* map = world -> root.maps;
    while (map)
    {
      MapPlane* next = map -> next;
      delete map;
      map = next;
    }
    
    delete world;
  }
  
  //
  // world_save_recursive
  //
  static bool world_save_recursive (Node* node, FILE* file)
  {
    assert (node);
    assert (file);
    
    fwrite (&node -> contents, 1, 1, file);
    
    if (!node -> front && !node -> back) // Leaf
    {
      if (node -> contents != contents_empty)
        return true;
      
      rku32 tri_count = 0;
      fseek (file, 4, SEEK_CUR); // We'll write this later
      
      for (MapPlane*
        m  = node -> maps;
        m != 0;
        m  = m -> next)
      {
        for (Polygon**
          p  = m -> polys_begin ();
          p != m -> polys_end   ();
          p++)
        {
          if (!*p)
            continue;
          
          unsigned size = (*p) -> size ();
          if (!size)
            continue;
          
          Rk::Vector3f verts [3];
          verts [0] = (*p) -> vertices [0];
          
          for (int i = 0; i < size - 2; i++)
          {
            verts [1] = (*p) -> vertices [i + 1];
            verts [2] = (*p) -> vertices [i + 2];
            fwrite (verts, 36, 1, file);
            tri_count++;
          }
        }
      }
      
      int skip = tri_count * 36;
      fseek (file, -skip - 4, SEEK_CUR);
      fwrite (&tri_count, 4, 1, file);
      fseek (file, skip, SEEK_CUR);
    }
    else if (node -> front && node -> back) // Non-Leaf
    {
      rkf32 plane [4] = {
        node -> partition.normal.x,
        node -> partition.normal.y,
        node -> partition.normal.z,
        node -> partition.distance
      };
      
      fwrite (plane, 4, 4, file);
      
      world_save_recursive (node -> front, file);
      world_save_recursive (node -> back,  file);
    }
    else
    {
      assert (false);
    }
    
    return true;
  }
  
  //
  // world_save
  //
  bool world_save (World* world, const char* filename)
  {
    assert (world);
    assert (filename);
    
    FILE* file = fopen (filename, "wb");
    if (!file)
      return false;
    
    fputs ("RKINDOOR", file);
    
    bool ok = world_save_recursive (&world -> root, file);
    
    fclose (file);
    
    return ok;
  }
  
}
// namespace In
