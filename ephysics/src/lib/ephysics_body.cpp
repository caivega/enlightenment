#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Evas.h>

#include "ephysics_private.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define RAD_TO_DEG 57.29582 /* 2 * pi radians == 360 degree */

typedef struct _EPhysics_Body_Callback EPhysics_Body_Callback;

struct _EPhysics_Body_Callback {
     EINA_INLIST;
     void (*func) (void *data, EPhysics_Body *body, void *event_info);
     void *data;
     EPhysics_Callback_Type type;
};

struct _EPhysics_Body {
     btCollisionShape *collision_shape;
     btRigidBody *rigid_body;
     Evas_Object *evas_obj;
     EPhysics_World *world;
     Eina_Inlist *callbacks;
     Eina_Bool active:1;
};

void
ephysics_body_active_set(EPhysics_Body *body, Eina_Bool active)
{
   EPhysics_Body_Callback *cb;

   if (body->active == !!active) return;
   body->active = !!active;
   if (active) return;

   EINA_INLIST_FOREACH(body->callbacks, cb)
      if (cb->type == EPHYSICS_CALLBACK_BODY_STOPPED)
        cb->func(cb->data, body, (void *) body->evas_obj);
}

static EPhysics_Body *
_ephysics_body_add(EPhysics_World *world, btCollisionShape *collision_shape)
{
   EPhysics_Body *body;
   btScalar mass = 1;

   body = (EPhysics_Body *) calloc(1, sizeof(EPhysics_Body));
   if (!body)
     {
        ERR("Couldn't create a new body instance.");
        return NULL;
     }

   btDefaultMotionState *motion_state =
      new btDefaultMotionState();
   if (!motion_state)
     {
        ERR("Couldn't create a motion state.");
        free(body);
        return NULL;
     }

   btVector3 inertia(0, 0, 0);
   collision_shape->calculateLocalInertia(mass, inertia);

   btRigidBody::btRigidBodyConstructionInfo rigid_body_ci(
      mass, motion_state, collision_shape, inertia);
   btRigidBody *rigid_body = new btRigidBody(rigid_body_ci);
   if (!rigid_body)
     {
        ERR("Couldn't create a rigid body.");
        delete motion_state;
        free(body);
        return NULL;
     }

   body->collision_shape = collision_shape;
   body->rigid_body = rigid_body;
   body->world = world;
   body->rigid_body->setUserPointer(body);
   body->rigid_body->setLinearFactor(btVector3(1, 1, 0));
   body->rigid_body->setAngularFactor(btVector3(0, 0, 1));

   return body;
}

static void
_ephysics_body_evas_obj_del_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EPhysics_Body *body = (EPhysics_Body *) data;
   body->evas_obj = NULL;
   DBG("Evas object deleted. Updating body: %p", body);
}

static void
_ephysics_body_del(EPhysics_Body *body)
{
   EPhysics_Body_Callback *cb;

   if (body->evas_obj)
     evas_object_event_callback_del(body->evas_obj, EVAS_CALLBACK_DEL,
                                    _ephysics_body_evas_obj_del_cb);

   while (body->callbacks)
     {
        cb = EINA_INLIST_CONTAINER_GET(body->callbacks,
                                         EPhysics_Body_Callback);
        body->callbacks = eina_inlist_remove(body->callbacks, body->callbacks);
        free(cb);
     }

   delete body->rigid_body->getMotionState();
   delete body->collision_shape;
   delete body->rigid_body;

   free(body);
}

static void
_ephysics_body_geometry_set(EPhysics_Body *body, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   double rate, mx, my, sx, sy;
   btTransform trans;
   int wy, height;

   rate = ephysics_world_rate_get(body->world);
   ephysics_world_render_geometry_get(body->world, NULL, &wy, NULL, &height);
   height += wy;

   mx = (x + w / 2) / rate;
   my = (height - (y + h / 2)) / rate;
   sx = w / rate;
   sy = h / rate;

   body->rigid_body->getMotionState()->getWorldTransform(trans);
   trans.setOrigin(btVector3(mx, my, 0));
   body->rigid_body->proceedToTransform(trans);

   body->collision_shape->setLocalScaling(btVector3(sx, sy, 1));

   if(!body->rigid_body->isStaticObject())
      ephysics_body_mass_set(body, ephysics_body_mass_get(body));

   body->rigid_body->getMotionState()->setWorldTransform(trans);

   DBG("Body position changed to %lf, %lf.", mx, my);
   DBG("Body scale changed to %lf, %lf.", sx, sy);
}

static void
_ephysics_body_evas_object_default_update(EPhysics_Body *body)
{
   btTransform trans;
   int wy, height, x, y, w, h;
   double rate, rot;
   Evas_Map *map;

   if (!body->evas_obj)
     return;

   body->rigid_body->getMotionState()->getWorldTransform(trans);
   ephysics_world_render_geometry_get(body->world, NULL, &wy, NULL, &height);
   height += wy;

   evas_object_geometry_get(body->evas_obj, NULL, NULL, &w, &h);
   rate = ephysics_world_rate_get(body->world);
   x = (int) (trans.getOrigin().getX() * rate) - w / 2;
   y = height - (int) (trans.getOrigin().getY() * rate) - h / 2;

   evas_object_move(body->evas_obj, x, y);

   rot = - trans.getRotation().getAngle() * RAD_TO_DEG *
      trans.getRotation().getAxis().getZ();

   map = evas_map_new(4);
   evas_map_util_points_populate_from_object(map, body->evas_obj);
   evas_map_util_rotate(map, rot, x + (w / 2), y + (h / 2));
   evas_object_map_set(body->evas_obj, map);
   evas_object_map_enable_set(body->evas_obj, EINA_TRUE);
   evas_map_free(map);
}

void
ephysics_body_evas_object_update_select(EPhysics_Body *body)
{
   Eina_Bool callback_called = EINA_FALSE;
   EPhysics_Body_Callback *cb;

   if (!body)
     return;

   EINA_INLIST_FOREACH(body->callbacks, cb)
     {
        if (cb->type == EPHYSICS_CALLBACK_BODY_UPDATE) {
             cb->func(cb->data, body, (void *) body->evas_obj);
             callback_called = EINA_TRUE;
        }
     }

   if (!callback_called)
     _ephysics_body_evas_object_default_update(body);
}

void
ephysics_body_contact_processed(EPhysics_Body *body, EPhysics_Body *contact_body)
{
   EPhysics_Body_Callback *cb;

   if ((!body) || (!contact_body))
     return;

   EINA_INLIST_FOREACH(body->callbacks, cb)
     {
        if (cb->type == EPHYSICS_CALLBACK_BODY_COLLISION)
          cb->func(cb->data, body, (void *) contact_body);
     }
}

btRigidBody *
ephysics_body_rigid_body_get(EPhysics_Body *body)
{
   return body->rigid_body;
}

EAPI EPhysics_Body *
ephysics_body_circle_add(EPhysics_World *world)
{
   EPhysics_Body *body;

   btCollisionShape *collision_shape = new btCylinderShapeZ(
      btVector3(0.5, 0.5, 0.5));
   if (!collision_shape)
     {
        ERR("Couldn't create a cylinder shape on z.");
        return NULL;
     }

   body = _ephysics_body_add(world, collision_shape);
   if (!body)
     {
        ERR("Couldn't create a circle body.");
        delete collision_shape;
        return NULL;
     }

   if (!ephysics_world_body_add(body->world, body, body->rigid_body))
     {
        ERR("Couldn't add body to world's bodies list");
        _ephysics_body_del(body);
        return NULL;
     }

   INF("Circle body added.");
   return body;
}

EAPI EPhysics_Body *
ephysics_body_box_add(EPhysics_World *world)
{
   EPhysics_Body *body;

   btCollisionShape *collision_shape = new btBoxShape(
      btVector3(0.5, 0.5, 0.5));
   if (!collision_shape)
     {
        ERR("Couldn't create a 2d box shape.");
        return NULL;
     }

   body = _ephysics_body_add(world, collision_shape);
   if (!body)
     {
        ERR("Couldn't create a box body.");
        delete collision_shape;
        return NULL;
     }

   if (!ephysics_world_body_add(body->world, body, body->rigid_body))
     {
        ERR("Couldn't add body to world's bodies list");
        _ephysics_body_del(body);
        return NULL;
     }

   INF("Box body added.");
   return body;
}

void
ephysics_body_world_boundaries_resize(EPhysics_World *world)
{
   Evas_Coord x, y, width, height;
   EPhysics_Body *bottom, *top, *left, *right;

   ephysics_world_render_geometry_get(world, &x, &y, &width, &height);

   bottom = ephysics_world_boundary_get(world, EPHYSICS_WORLD_BOUNDARY_BOTTOM);
   if (bottom)
     ephysics_body_geometry_set(bottom, x, y + height, width, 10);

   right = ephysics_world_boundary_get(world, EPHYSICS_WORLD_BOUNDARY_RIGHT);
   if (right)
     ephysics_body_geometry_set(right, x + width, 0, 10, y + height);

   left = ephysics_world_boundary_get(world, EPHYSICS_WORLD_BOUNDARY_LEFT);
   if (left)
     ephysics_body_geometry_set(left,  x - 10, 0, 10, y + height);

   top = ephysics_world_boundary_get(world, EPHYSICS_WORLD_BOUNDARY_TOP);
   if (top)
     ephysics_body_geometry_set(top, 0, y - 10, x + width, 10);
}

static EPhysics_Body *
_ephysics_body_boundary_add(EPhysics_World *world, EPhysics_World_Boundary boundary, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   EPhysics_Body *body;

   if (!world)
     {
        ERR("Can't add boundary, world is null.");
        return NULL;
     }

   body = ephysics_world_boundary_get(world, boundary);
   if (body)
     return body;

   body = ephysics_body_box_add(world);
   if (!body)
     return NULL;

   ephysics_body_mass_set(body, 0);
   ephysics_world_boundary_set(world, boundary, body);
   ephysics_body_geometry_set(body, x, y, w, h);

   return body;
}

EAPI EPhysics_Body *
ephysics_body_top_boundary_add(EPhysics_World *world)
{
   Evas_Coord x, y, w;
   ephysics_world_render_geometry_get(world, &x, &y, &w, NULL);
   return _ephysics_body_boundary_add(world, EPHYSICS_WORLD_BOUNDARY_TOP,
                                      0, y - 10, x + w, 10);
}

EAPI EPhysics_Body *
ephysics_body_bottom_boundary_add(EPhysics_World *world)
{
   Evas_Coord x, y, w, h;
   ephysics_world_render_geometry_get(world, &x, &y, &w, &h);
   return _ephysics_body_boundary_add(world, EPHYSICS_WORLD_BOUNDARY_BOTTOM,
                                      x, y + h, w, 10);
}

EAPI EPhysics_Body *
ephysics_body_left_boundary_add(EPhysics_World *world)
{
   Evas_Coord x, y, h;
   ephysics_world_render_geometry_get(world, &x, &y, NULL, &h);
   return _ephysics_body_boundary_add(world, EPHYSICS_WORLD_BOUNDARY_LEFT,
                                      x - 10, 0, 10, y + h);
}

EAPI EPhysics_Body *
ephysics_body_right_boundary_add(EPhysics_World *world)
{
   Evas_Coord x, y, w, h;
   ephysics_world_render_geometry_get(world, &x, &y, &w, &h);
   return _ephysics_body_boundary_add(world, EPHYSICS_WORLD_BOUNDARY_RIGHT,
                                      x + w, 0, 10, y + h);
}

void
ephysics_orphan_body_del(EPhysics_Body *body)
{
   EPhysics_Body_Callback *cb;

   EINA_INLIST_FOREACH(body->callbacks, cb)
     {
        if (cb->type == EPHYSICS_CALLBACK_BODY_DEL)
          cb->func(cb->data, body, NULL);
     }

   _ephysics_body_del(body);
}

EAPI void
ephysics_body_del(EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't delete body, it wasn't provided.");
        return;
     }

   ephysics_world_body_del(body->world, body, body->rigid_body);
   ephysics_orphan_body_del(body);

   INF("Body deleted.");
}

EAPI void
ephysics_body_evas_object_set(EPhysics_Body *body, Evas_Object *evas_obj, Eina_Bool use_obj_pos)
{
   int obj_x, obj_y, obj_w, obj_h;

   if (!body)
     {
        ERR("Can't set evas object to body, the last wasn't provided.");
        return;
     }

   if (!evas_obj)
     {
        ERR("Can't set evas object to body, the first wasn't provided.");
        return;
     }

   if (body->evas_obj)
     evas_object_event_callback_del(body->evas_obj, EVAS_CALLBACK_DEL,
                                    _ephysics_body_evas_obj_del_cb);

   body->evas_obj = evas_obj;
   evas_object_event_callback_add(evas_obj, EVAS_CALLBACK_DEL,
                                  _ephysics_body_evas_obj_del_cb, body);

   if (!use_obj_pos)
     return;

   evas_object_geometry_get(body->evas_obj, &obj_x, &obj_y, &obj_w, &obj_h);
   _ephysics_body_geometry_set(body, obj_x, obj_y, obj_w, obj_h);
}

EAPI Evas_Object *
ephysics_body_evas_object_unset(EPhysics_Body *body)
{
   Evas_Object *obj;
   if (!body)
     {
        ERR("Can't unset evas object from body, it wasn't provided.");
        return NULL;
     }

   obj = body->evas_obj;
   body->evas_obj = NULL;

   if (obj)
     evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL,
                                    _ephysics_body_evas_obj_del_cb);

   return obj;
}

EAPI Evas_Object *
ephysics_body_evas_object_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get evas object from body, it wasn't provided.");
        return NULL;
     }

   return body->evas_obj;
}

EAPI void
ephysics_body_geometry_set(EPhysics_Body *body, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   if (!body)
     {
        ERR("Can't set body position, body is null.");
        return;
     }

   if ((w <= 0) || (h <= 0))
     {
        ERR("Width and height must to be a non-null, positive value.");
        return;
     }

   _ephysics_body_geometry_set(body, x, y, w, h);
}

EAPI void
ephysics_body_geometry_get(const EPhysics_Body *body, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   btTransform trans;
   btVector3 vector;
   double rate;
   int wy, height;

   if (!body)
     {
        ERR("Can't get body position, body is null.");
        return;
     }

   body->rigid_body->getMotionState()->getWorldTransform(trans);
   vector = body->collision_shape->getLocalScaling();

   rate = ephysics_world_rate_get(body->world);
   ephysics_world_render_geometry_get(body->world, NULL, &wy, NULL, &height);
   height += wy;

   if (x) *x = round((trans.getOrigin().getX() - vector.x() / 2) * rate);
   if (y) *y = height - round((trans.getOrigin().getY() + vector.y() / 2)
                              * rate);
   if (w) *w = round(vector.x() * rate);
   if (h) *h = round(vector.y() * rate);
}

EAPI void
ephysics_body_mass_set(EPhysics_Body *body, double mass)
{
   if (!body)
     {
        ERR("Can't set body mass, body is null.");
        return;
     }

   btVector3 inertia(0, 0, 0);
   body->collision_shape->calculateLocalInertia(mass, inertia);
   body->rigid_body->setMassProps(mass, inertia);
   body->rigid_body->updateInertiaTensor();

   DBG("Body mass changed to %lf.", mass);
}

EAPI double
ephysics_body_mass_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get body mass, body is null.");
        return 0;
     }

   return 1 / body->rigid_body->getInvMass();
}

EAPI void
ephysics_body_evas_object_update(EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Couldn't update a null body.");
        return;
     }

   _ephysics_body_evas_object_default_update(body);
}

EAPI void
ephysics_body_event_callback_add(EPhysics_Body *body, EPhysics_Callback_Type type, EPhysics_Body_Event_Cb func, const void *data)
{
   EPhysics_Body_Callback *cb;

   if (!body)
     {
        ERR("Can't set body event callback, body is null.");
        return;
     }

   if (!func)
     {
        ERR("Can't set body event callback, function is null.");
        return;
     }

   if ((type <= EPHYSICS_CALLBACK_BODY_FIRST) ||
       (type >= EPHYSICS_CALLBACK_BODY_LAST))
     {
        ERR("Can't set body event callback, callback type is wrong.");
        return;
     }

   cb = (EPhysics_Body_Callback *) malloc(sizeof(EPhysics_Body_Callback));
   if (!cb)
     {
        ERR("Can't set body event callback, can't create cb instance.");
        return;
     }

   cb->func = func;
   cb->type = type;
   cb->data = (void *)data;

   body->callbacks = eina_inlist_append(body->callbacks, EINA_INLIST_GET(cb));
}

EAPI void *
ephysics_body_event_callback_del(EPhysics_Body *body, EPhysics_Callback_Type type, EPhysics_Body_Event_Cb func)
{
   EPhysics_Body_Callback *cb;
   void *cb_data;

   if (!body)
     {
        ERR("Can't delete body event callback, body is null.");
        return NULL;
     }

   EINA_INLIST_FOREACH(body->callbacks, cb)
     {
        if ((cb->type == type) && (cb->func == func)) {
             cb_data = cb->data;
             body->callbacks = eina_inlist_remove(body->callbacks,
                                                  EINA_INLIST_GET(cb));
             free(cb);
             return cb_data;
        }
     }

   return NULL;
}

EAPI void
ephysics_body_restitution_set(EPhysics_Body *body, double restitution)
{
   if (!body)
     {
        ERR("Can't set body restitution, body is null.");
        return;
     }

   body->rigid_body->setRestitution(btScalar(restitution));
}

EAPI double
ephysics_body_restitution_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get body restitution, body is null.");
        return 0;
     }

   return body->rigid_body->getRestitution();
}

EAPI void
ephysics_body_friction_set(EPhysics_Body *body, double friction)
{
   if (!body)
     {
        ERR("Can't set body friction, body is null.");
        return;
     }

   body->rigid_body->setFriction(btScalar(friction));
}

EAPI double
ephysics_body_friction_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get body friction, body is null.");
        return 0;
     }

   return body->rigid_body->getFriction();
}

EAPI EPhysics_World *
ephysics_body_world_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get the world a null body belongs to.");
        return NULL;
     }

   return body->world;
}

EAPI void
ephysics_body_central_impulse_apply(EPhysics_Body *body, double x, double y)
{
   if (!body)
     {
        ERR("Can't apply impulse to a null body.");
        return;
     }

   body->rigid_body->activate(1);
   body->rigid_body->applyCentralImpulse(btVector3(x, y, 0));
}

EAPI void
ephysics_body_torque_impulse_apply(EPhysics_Body *body, double roll)
{
   if (!body)
     {
        ERR("Can't apply torque impulse to a null body.");
        return;
     }

   body->rigid_body->activate(1);
   body->rigid_body->applyTorqueImpulse(btVector3(0, 0, roll));
}

EAPI void
ephysics_body_rotation_on_z_axis_enable_set(EPhysics_Body *body, Eina_Bool enable)
{
   if (!body)
     {
        ERR("Can't set rotation on a null body.");
        return;
     }

   if (!enable)
     body->rigid_body->setAngularFactor(btVector3(0, 0, 0));
   else
     body->rigid_body->setAngularFactor(btVector3(0, 0, 1));
}

EAPI Eina_Bool
ephysics_body_rotation_on_z_axis_enable_get(EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't check if rotation is enabled, body is null.");
        return EINA_FALSE;
     }

   return !!body->rigid_body->getAngularFactor().z();
}

EAPI double
ephysics_body_rotation_get(EPhysics_Body *body)
{
   btTransform trans;
   double rot;

   if (!body)
     {
        ERR("Can't get rotation, body is null.");
        return 0;
     }

   body->rigid_body->getMotionState()->getWorldTransform(trans);
   rot = - trans.getRotation().getAngle() * RAD_TO_DEG *
      trans.getRotation().getAxis().getZ();

   return rot;
}

#ifdef  __cplusplus
}
#endif
