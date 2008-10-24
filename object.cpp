#include <SDL/SDL.h>
#include <ode/ode.h>

#include "global.h"
#include "object.h"


Object::Object()
{
  LOG->trace("Object: NEW (empty)");
  this->geom = NULL;
  this->visible = true;
}

Object::Object(dGeomID geom)
{
  LOG->trace("Object: NEW (geom)");
  this->geom = geom;
  this->visible = true;
  dSpaceAdd(physics->get_space(), this->geom);
  physics->get_objs().push_back(this);
}

Object::~Object()
{
  if( geom == NULL )
    return;
  dGeomDestroy(geom);
  geom = NULL;
}


void Object::set_pos(dReal x, dReal y)
{
  dReal a[6];
  dGeomGetAABB(geom, a);
  set_pos(x, y, a[5]-a[4] + cfg->drop_epsilon);
}

void Object::draw_geom(dGeomID geom)
{
  glPushMatrix();

  draw_move(geom);

  switch( dGeomGetClass(geom) )
  {
    case dSphereClass:
      glutSolidSphere(dGeomSphereGetRadius(geom), cfg->draw_div, cfg->draw_div);
      break;
    case dBoxClass:
      {
        dVector3 size;
        dGeomBoxGetLengths(geom, size);
        glScalef(size[0], size[1], size[2]);
        glutSolidCube(1.0f);
        break;
      }
    case dCapsuleClass:
      {
        dReal r, len;
        dGeomCapsuleGetParams(geom, &r, &len);
        glTranslatef(0, 0, -len/2);
        glutSolidCylinder(r, len, cfg->draw_div, cfg->draw_div);
        glutSolidSphere(r, cfg->draw_div, cfg->draw_div);
        glTranslatef(0, 0, len);
        glutSolidSphere(r, cfg->draw_div, cfg->draw_div);
        break;
      }
    case dCylinderClass:
      {
        dReal r, len;
        dGeomCylinderGetParams(geom, &r, &len);
        glTranslatef(0, 0, -len/2);
        glutSolidCylinder(r, len, cfg->draw_div, cfg->draw_div);
        break;
      }
    default:
      throw(Error("drawing not supported for this geometry class"));
      break;
  }

  glPopMatrix();
}


void Object::draw_move(dGeomID geom)
{
  const dReal *pos = dGeomGetPosition(geom);
  const dReal *rot = dGeomGetRotation(geom);

  GLfloat m[16] = {
    rot[0], rot[4], rot[8],  0.0f,
    rot[1], rot[5], rot[9],  0.0f,
    rot[2], rot[6], rot[10], 0.0f,
    pos[0], pos[1], pos[2],  1.0f
  };
  glMultMatrixf(m);

  // Offset
  pos = dGeomGetOffsetPosition(geom);
  rot = dGeomGetOffsetRotation(geom);

  GLfloat m2[16] = {
    rot[0], rot[4], rot[8],  0.0f,
    rot[1], rot[5], rot[9],  0.0f,
    rot[2], rot[6], rot[10], 0.0f,
    pos[0], pos[1], pos[2],  1.0f
  };
  glMultMatrixf(m2);
}


ObjectDynamic::ObjectDynamic(): Object()
{
  LOG->trace("ObjectDynamic: NEW (empty)");
  body = NULL;
}

ObjectDynamic::ObjectDynamic(dGeomID geom, dBodyID body):
  Object(geom)
{
  LOG->trace("ObjectDynamic: NEW (geom, body)");
  this->body = body;
  dGeomSetBody(this->geom, this->body);

  add_category(CAT_DYNAMIC);
}

ObjectDynamic::ObjectDynamic(dGeomID geom, dReal m):
  Object(geom)
{
  LOG->trace("ObjectDynamic: NEW (geom, m)");
  this->body = dBodyCreate(physics->get_world());
  dMass mass;
  switch( dGeomGetClass(geom) )
  {
    case dSphereClass:
      {
        dReal r = dGeomSphereGetRadius(geom);
        dMassSetSphereTotal(&mass, m, r);
        break;
      }
    case dBoxClass:
      {
        dVector3 size;
        dGeomBoxGetLengths(geom, size);
        dMassSetBoxTotal(&mass, m, size[0], size[1], size[2]);
        break;
      }
    case dCapsuleClass:
      {
        dReal r, len;
        dGeomCapsuleGetParams(geom, &r, &len);
        dMassSetCapsuleTotal(&mass, m, 3, r, len);
        break;
      }
    case dCylinderClass:
      {
        dReal r, len;
        dGeomCylinderGetParams(geom, &r, &len);
        dMassSetCylinderTotal(&mass, m, 3, r, len);
        break;
      }
    default:
      {
        dReal a[6];
        dGeomGetAABB(geom, a);
        dMassSetBoxTotal(&mass, m, a[1]-a[0], a[3]-a[2], a[5]-a[4]);
        break;
      }
  }
  dBodySetMass(this->body, &mass);
  dGeomSetBody(this->geom, this->body);

  add_category(CAT_DYNAMIC);
}


ObjectDynamic::~ObjectDynamic()
{
  if( body == NULL )
    return;
  dBodyDestroy(body);
  body = NULL;
}



const dReal OGround::size_z;
const dReal OGround::size_start;

OGround::OGround(const Color4 color, const Color4 color_t1, const Color4 color_t2):
  Object(dCreateBox(0, Rules::table_size_x, Rules::table_size_y, size_z))
{
  dGeomSetPosition(this->geom, 0, 0, -size_z/2);

  this->color[0] = color[0];
  this->color[1] = color[1];
  this->color[2] = color[2];
  this->color[3] = 0.0;

  this->color_t1[0] = color_t1[0];
  this->color_t1[1] = color_t1[1];
  this->color_t1[2] = color_t1[2];
  this->color_t1[3] = 0.0;

  this->color_t2[0] = color_t2[0];
  this->color_t2[1] = color_t2[1];
  this->color_t2[2] = color_t2[2];
  this->color_t2[3] = 0.0;

  set_category(CAT_GROUND);
  set_collide(CAT_DYNAMIC);
}

void OGround::draw()
{
  glPushMatrix();

  draw_move();

  // Ground
  
  glPushMatrix();

  glColor3fv(color);
  dReal size[3];
  dGeomBoxGetLengths(geom, size);
  glScalef(size[0], size[1], size[2]);
  glutSolidCube(1.0f);

  glPopMatrix();


  // Starting areas
  
  glPushMatrix();

  glColor3fv(color_t1);
  glTranslatef(-(size[0]-size_start)/2, (size[1]-size_start)/2, size[2]/2);
  glScalef(size_start, size_start, 2*cfg->draw_epsilon);
  glutSolidCube(1.0f);

  glPopMatrix();

  glPushMatrix();

  glColor3fv(color_t2);
  glTranslatef((size[0]-size_start)/2, (size[1]-size_start)/2, size[2]/2);
  glScalef(size_start, size_start, 2*cfg->draw_epsilon);
  glutSolidCube(1.0f);

  glPopMatrix();

  glPopMatrix();
}



class LuaObject: public LuaClass<Object>
{
  //TODO category/collide

  static int _ctor(lua_State *L)
  {
    Object **ud = get_userdata(L);
    dGeomID geom = (dGeomID)LARG_lud(2);
    *ud = new Object(geom);
    return 0;
  }

  static int set_pos(lua_State *L)
  {
    LOG->trace("Object: set_pos");
    if( lua_isnone(L, 4) )
      get_ptr(L)->set_pos(LARG_f(2), LARG_f(3));
    else
      get_ptr(L)->set_pos(LARG_f(2), LARG_f(3), LARG_f(4));
    return 0;
  }

  static int set_rot(lua_State *L)
  {
    dQuaternion q;
    for( int i=0; i<4; i++ )
      q[i] = LARG_f(i+2);
    get_ptr(L)->set_rot(q);
    return 0;
  }

  LUA_DEFINE_SET1(set_visible, LARG_b)

public:
  LuaObject()
  {
    LUA_REGFUNC(_ctor);
    LUA_REGFUNC(set_pos);
    LUA_REGFUNC(set_rot);
    LUA_REGFUNC(set_visible);
  }
};


class LuaObjectDynamic: public LuaClass<ObjectDynamic>
{
  static int _ctor(lua_State *L)
  {
    ObjectDynamic **ud = get_userdata(L);
    dGeomID geom = (dGeomID)LARG_lud(2);
    *ud = new ObjectDynamic(geom, LARG_f(3));
    return 0;
  }

public:
  LuaObjectDynamic()
  {
    LUA_REGFUNC(_ctor);
  }
};


class LuaObjectColor: public LuaClass<ObjectColor>
{
  static int _ctor(lua_State *L)
  {
    ObjectColor **ud = get_userdata(L);
    dGeomID geom = (dGeomID)LARG_lud(2);
    *ud = new ObjectColor(geom);
    return 0;
  }

  static int set_color(lua_State *L)
  {
    Color4 color;
    LuaManager::checkcolor(L, 2, color);
    get_ptr(L)->set_color( color );
    return 0;
  }

public:
  LuaObjectColor()
  {
    LUA_REGFUNC(_ctor);
    LUA_REGFUNC(set_color);
  }
};

class LuaObjectDynamicColor: public LuaClass<ObjectDynamicColor>
{
  static int _ctor(lua_State *L)
  {
    ObjectDynamicColor **ud = get_userdata(L);
    dGeomID geom = (dGeomID)LARG_lud(2);
    *ud = new ObjectDynamicColor(geom, LARG_f(3));
    return 0;
  }

  static int set_color(lua_State *L)
  {
    Color4 color;
    LuaManager::checkcolor(L, 2, color);
    get_ptr(L)->set_color( color );
    return 0;
  }

public:
  LuaObjectDynamicColor()
  {
    LUA_REGFUNC(_ctor);
    LUA_REGFUNC(set_color);
  }
};


class LuaOGround: public LuaClass<OGround>
{
  static int _ctor(lua_State *L)
  {
    OGround **ud = get_userdata(L);

    Color4 color, color_t1, color_t2;
    LuaManager::checkcolor(L, 2, color);
    LuaManager::checkcolor(L, 3, color_t1);
    LuaManager::checkcolor(L, 4, color_t2);
    *ud = new OGround(color, color_t1, color_t2);
    return 0;
  }

public:
  LuaOGround()
  {
    LUA_REGFUNC(_ctor);
  }
};


LUA_REGISTER_BASE_CLASS(Object);
LUA_REGISTER_SUB_CLASS(ObjectDynamic,Object);
LUA_REGISTER_SUB_CLASS(ObjectColor,Object);
LUA_REGISTER_SUB_CLASS(ObjectDynamicColor,ObjectDynamic);
LUA_REGISTER_SUB_CLASS(OGround,Object);

