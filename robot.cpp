#include <math.h>
#include <GL/freeglut.h>

#include "global.h"


Robot::Robot()
{
  this->ref_obj = LUA_NOREF;
  this->team = TEAM_INVALID;
}

Robot::~Robot()
{
  lua_State *L = lm->get_L();
  // luaL_unref has no effect on LUA_NOREF, so it's ok
  luaL_unref(L, LUA_REGISTRYINDEX, ref_obj);
}

void Robot::matchRegister(unsigned int team)
{
  if( getTeam() != TEAM_INVALID )
    throw(Error("robot is already registered"));

  if( !match )
    throw(Error("no match to register the robot in"));

  this->team = match->registerRobot(this, team);
}


void Robot::matchInit()
{
  if( ref_obj == LUA_NOREF )
    return;
}


RBasic::RBasic()
{
  this->body = NULL;
  this->order = ORDER_NONE;
}

RBasic::RBasic(btCollisionShape *shape, btScalar m)
{
  setup(shape, m);
  this->order = ORDER_NONE;
}

void RBasic::setup(btCollisionShape *shape, btScalar m)
{
  if( this->body != NULL )
    throw(Error("robot already setup"));
  btVector3 inertia;
  shape->calculateLocalInertia(m, inertia);
  this->body = new btRigidBody(
      btRigidBody::btRigidBodyConstructionInfo(m,NULL,shape,inertia)
      );
  SmartPtr_add_ref(shape);
}

RBasic::~RBasic()
{
  //TODO remove from the world
  if( body != NULL )
  {
    SmartPtr_release(body->getCollisionShape());
    delete body;
  }
}

void RBasic::addToWorld(Physics *physics)
{
  physics->getWorld()->addRigidBody(body);
  Robot::addToWorld(physics);
}

void RBasic::draw()
{
  // Use a darker color to contrast with game elements
  glColor4fv(match->getColor(getTeam()) * 0.5);
  glPushMatrix();
  drawTransform(body->getCenterOfMassTransform());
  drawShape(body->getCollisionShape());
  drawDirection();
  glPopMatrix();
}

void RBasic::drawDirection()
{
  btVector3 aabb_min, aabb_max;
  body->getAabb(aabb_min, aabb_max);

  btglTranslate(0, 0, aabb_max.getZ()-getPos().getZ()+cfg->draw_direction_r+cfg->draw_epsilon);
  btglRotate(90.0f, 0.0f, 1.0f, 0.0f);
  glutSolidCone(cfg->draw_direction_r, cfg->draw_direction_h, cfg->draw_div, cfg->draw_div);
}


void RBasic::asserv()
{
  // Go back: order which have priority
  if( order & ORDER_GO_BACK )
  {
    if( distance2(xy, target_back_xy) < threshold_xy )
    {
      set_v(0);
      order &= ~ORDER_GO_BACK;
    }
    else
    {
      set_v(-v_max);
      return;
    }
  }

  // Go in position
  if( order & ORDER_GO_XY )
  {
    if( distance2(xy, target_xy) < threshold_xy )
    {
      set_v(0);
      order &= ~ORDER_GO_XY;
    }
    else
    {
      // Aim target point, then move
      btScalar da = normA( (target_xy-xy).angle() - a );
      if( btFabs( da ) < threshold_a )
      {
        set_av(0);
        set_v(v_max);
      }
      else
      {
        set_v(0);
        set_av( btFsel(da, av_max, -av_max) );
      }
      return;
    }
  }

  // Turn
  if( order & ORDER_GO_A )
  {
    btScalar da = normA( target_a-a );
    if( btFabs( da ) < threshold_a )
    {
      set_av(0);
      order &= ~ORDER_GO_A;
    }
    else
    {
      set_av( btFsel(da, av_max, -av_max) );
      return;
    }
  }
}


void RBasic::order_xy(btVector2 xy, bool rel)
{
  LOG->trace("XY  %c %f,%f", rel?'+':'=', xy.x, xy.y);
  target_xy = xy;
  if( rel )
    target_xy += this->xy;

  order |= ORDER_GO_XY;
}

void RBasic::order_a(btScalar a, bool rel)
{
  LOG->trace("A  %c %f", rel?'+':'=', a);
  target_a = a;
  if( rel )
    target_a += this->a;
  target_a = normA(target_a);

  order |= ORDER_GO_A;
}

void RBasic::order_back(btScalar d)
{
  LOG->trace("BACK  %f", d);
  target_back_xy = xy - d*btVector2(1,0).rotated(a);

  order |= ORDER_GO_BACK;
}


void RBasic::update()
{
  xy = this->getPos();
  btScalar p, r;
  this->getRot().getEulerYPR(a,p,r);

  v = btVector2(body->getLinearVelocity()).length();
  av = body->getAngularVelocity().getZ();
}


inline void RBasic::set_v(btScalar v)
{
  //XXX we have to force activation, is it a Bullet bug?
  body->activate();
  btVector2 vxy = btVector2(v,0).rotated(a);
  body->setLinearVelocity( btVector3(vxy.x, vxy.y,
        body->getLinearVelocity().z()) );
}

inline void RBasic::set_av(btScalar v)
{
  //XXX we have to force activation, is it a Bullet bug?
  body->activate();
  body->setAngularVelocity( btVector3(0,0,v) );
}



class LuaRobot: public LuaClass<Robot>
{
  static int _ctor(lua_State *L)
  {
    return luaL_error(L, "Robot class is abstract, no constructor");
  }

  static int get_team(lua_State *L)
  {
    unsigned int team = get_ptr(L,1)->getTeam();
    if( team == TEAM_INVALID )
      lua_pushnil(L);
    else
      LuaManager::push(L, team);
    return 1;
  }

  static int match_register(lua_State *L)
  {
    // Default team
    if( lua_isnone(L, 2) )
      get_ptr(L,1)->matchRegister();
    else
      get_ptr(L,1)->matchRegister(LARG_i(2));

    return 0;
  }

public:
  LuaRobot()
  {
    LUA_REGFUNC(_ctor);
    LUA_REGFUNC(get_team);
    LUA_REGFUNC(match_register);
  }

};


class LuaRBasic: public LuaClass<RBasic>
{
  static int _ctor(lua_State *L)
  {
    btCollisionShape *shape;
    shape = *(btCollisionShape **)luaL_checkudata(L, 2, "Shape");
    RBasic *r = new RBasic( shape, LARG_f(3));
    store_ptr(L, r);
    lua_pushvalue(L, 1);
    r->ref_obj = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
  }

  LUA_DEFINE_GETN_SCALED(2, get_xy, get_xy)
  LUA_DEFINE_GET_SCALED(get_v, get_v)
  LUA_DEFINE_GET(get_a , get_a)
  LUA_DEFINE_GET(get_av, get_av)

  LUA_DEFINE_SET1(set_v_max,        set_v_max,        LARG_scaled)
  LUA_DEFINE_SET1(set_av_max,       set_av_max,       LARG_f)
  LUA_DEFINE_SET1(set_threshold_xy, set_threshold_xy, LARG_scaled)
  LUA_DEFINE_SET1(set_threshold_a,  set_threshold_a,  LARG_f)

  static int order_xy(lua_State *L)
  {
    get_ptr(L,1)->order_xy( btVector2(LARG_scaled(2), LARG_scaled(3)), LARG_bn(4) );
    return 0;
  }
  static int order_xya(lua_State *L)
  {
    get_ptr(L,1)->order_xya( btVector2(LARG_scaled(2), LARG_scaled(3)), LARG_f(4), LARG_bn(5) );
    return 0;
  }
  LUA_DEFINE_SET2(order_a,    order_a,    LARG_f, LARG_bn)
  LUA_DEFINE_SET1(order_back, order_back, LARG_scaled)
  LUA_DEFINE_SET0(order_stop, order_stop)

  LUA_DEFINE_SET0(update, update)
  LUA_DEFINE_SET0(asserv, asserv)
  LUA_DEFINE_GET(is_waiting, is_waiting)

public:
  LuaRBasic()
  {
    LUA_REGFUNC(_ctor);

    LUA_REGFUNC(get_xy);
    LUA_REGFUNC(get_v);
    LUA_REGFUNC(get_a);
    LUA_REGFUNC(get_av);

    LUA_REGFUNC(set_v_max);
    LUA_REGFUNC(set_av_max);
    LUA_REGFUNC(set_threshold_xy);
    LUA_REGFUNC(set_threshold_a);

    LUA_REGFUNC(order_xy);
    LUA_REGFUNC(order_xya);
    LUA_REGFUNC(order_a);
    LUA_REGFUNC(order_back);
    LUA_REGFUNC(order_stop);

    LUA_REGFUNC(update);
    LUA_REGFUNC(asserv);
    LUA_REGFUNC(is_waiting);
  }
};


LUA_REGISTER_SUB_CLASS(Robot,Object);
LUA_REGISTER_SUB_CLASS(RBasic,Robot);

