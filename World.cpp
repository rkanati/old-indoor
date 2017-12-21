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

#include <cstdio>
#include <cassert>
#include <cstring>

#include <Rk/Types.hpp>
#include <Rk/Plane.hpp>

#define WIN32_LEAN_AND_MEAN 1
#define NOCRYPT
#define NOUSER
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>

#include <gl/gl.h>

//
// Node
//
struct Node
{
  rku8 contents;
  Node *front, *back;
  Rk::Plane <rkf32> partition;
  rku32 triangle_count;
  rkf32* triangles;
  
};

//
// World
//
struct World
{
  Node* root;
  
};

//
// world_load_nodes
//
Node* world_load_nodes (FILE* file)
{
  Node* node = new Node;
  
  fread (&node -> contents, 1, 1, file);
  
  if (node -> contents == 255)
  {
    fread (&node -> partition, 4, 4, file);
    
    node -> front = world_load_nodes (file);
    if (!node -> front)
    {
      delete node;
      return 0;
    }
    
    node -> back = world_load_nodes (file);
    if (!node -> back)
    {
      delete node;
      return 0;
    }
    
    node -> triangles = 0;
  }
  else if (node -> contents == 0)
  {
    fread (&node -> triangle_count, 4, 1, file);
    
    node -> triangles = new rkf32 [node -> triangle_count * 9];
    fread (node -> triangles, 4, node -> triangle_count * 9, file);
    
    node -> front = 0;
    node -> back  = 0;
  }
  else if (node -> contents < 3)
  {
    node -> triangles = 0;
    node -> front     = 0;
    node -> back      = 0;
  }
  else
  {
    delete node;
    return 0;
  }
  
  return node;
}

//
// world_load
//
World* world_load (const char* filename)
{
  assert (filename);
  
  FILE* file = fopen (filename, "rb");
  if (!file)
    return 0;
  
  char magic [8];
  fread (magic, 1, 8, file);
  
  if (strncmp (magic, "RKINDOOR", 8))
  {
    fclose (file);
    return 0;
  }
  
  World* world = new World;
  
  world -> root = world_load_nodes (file);
  if (!world -> root)
  {
    delete world;
    world = 0;
  }
  
  fclose (file);
  return world;
}

//
// node_free
//
static void node_free (Node* node)
{
  if (!node)
    return;
  
  node_free (node -> front);
  node_free (node -> back );
  
  if (node -> triangles)
    delete [] node -> triangles;
}

//
// world_free
//
void world_free (World* world)
{
  if (!world)
    return;
  
  node_free (world -> root);
}

//
// node_render
//
static void node_render (Node* node, Vector3 position, Vector3 facing)
{
  if (!node)
    return;
  
  if (node -> triangles)
  {
    glVertexPointer (3, GL_FLOAT, 0, node -> triangles);
    glDrawArrays (GL_TRIANGLES, 0, node -> triangle_count * 3);
  }
  else
  {
    rkf32 dist = node -> partition.point_distance (position);
    
    if (dist > 0.001)
    {
      node_render (node -> front, position, facing);
      node_render (node -> back,  position, facing);
    }
    else
    {
      node_render (node -> back,  position, facing);
      node_render (node -> front, position, facing);
    }
  }
}

//
// world_render
//
void world_render (World* world, Vector3 position, Vector3 facing)
{
  assert (world);
  
  Node* root = world -> root;
  
  glColor3f (1.0, 0.0, 0.0);
  glPolygonMode (GL_FRONT, GL_LINE);
  glEnable (GL_VERTEX_ARRAY);
  node_render (root, position, facing);
  glDisable (GL_VERTEX_ARRAY);
}
