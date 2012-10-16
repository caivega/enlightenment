#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Evas.h>
#include <Ecore.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <LinearMath/btGeometryUtil.h>

#include <math.h>

#include "ephysics_private.h"
#include "ephysics_trimesh.h"
#include "ephysics_body_materials.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _EPhysics_Body_Callback EPhysics_Body_Callback;

struct _EPhysics_Body_Callback {
     EINA_INLIST;
     void (*func) (void *data, EPhysics_Body *body, void *event_info);
     void *data;
     EPhysics_Callback_Body_Type type;
     Eina_Bool deleted:1;
};

struct _EPhysics_Body_Collision {
     EPhysics_Body *contact_body;
     Evas_Coord x;
     Evas_Coord y;
     Evas_Coord z;
};

typedef struct _EPhysics_Body_Soft_Body_Slice
{
     int index;
     Evas_Object *evas_obj;
     double stacking;
} EPhysics_Body_Soft_Body_Slice;

static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
   {"children,changed", "i"},
   {NULL, NULL}
};

typedef struct _EPhysics_Body_Soft_Body_Smart_Data
{
   Evas_Object_Smart_Clipped_Data base;
   Evas_Object *base_obj;
   EPhysics_Body *body;
   Eina_List *slices;
} EPhysics_Body_Soft_Body_Smart_Data;

#define SMART_CLASS_NAME "EPhysics_Body_Soft_Body"

#define EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(o, ptr)                  \
  ptr = (EPhysics_Body_Soft_Body_Smart_Data *) evas_object_smart_data_get(o) \

EVAS_SMART_SUBCLASS_NEW(SMART_CLASS_NAME,
			_ephysics_body_soft_body_evas, Evas_Smart_Class,
			Evas_Smart_Class, evas_object_smart_clipped_class_get,
                        _smart_callbacks);

static int
_ephysics_body_soft_body_slice_stacking_sort_cb(const void *d1, const void *d2)
{
   const EPhysics_Body_Soft_Body_Slice *slice1, *slice2;

   slice1 = (const EPhysics_Body_Soft_Body_Slice *)d1;
   slice2 = (const EPhysics_Body_Soft_Body_Slice *)d2;

   if (!slice1) return 1;
   if (!slice2) return -1;

   if (slice1->stacking < slice2->stacking) return -1;
   if (slice2->stacking > slice2->stacking) return 1;

   return 0;
}

static void
_ephysics_body_soft_body_slices_apply(Evas_Object *obj)
{
   double rate;
   void *data;
   Eina_List *l;
   Evas_Coord x, y, h, wy, wh, y0, y1, y2, x0, x1, x2, z0, z1, z2;
   Evas_Map *map;
   btVector3 p0, p1, p2;
   btSoftBody::tFaceArray faces;
   EPhysics_Body_Soft_Body_Slice *slice;
   EPhysics_Body_Soft_Body_Smart_Data *smart_data;
   EPhysics_Body *body;
   Evas_Object *prev_obj = NULL;

   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, smart_data);
   body = smart_data->body;
   rate = ephysics_world_rate_get(body->world);
   ephysics_world_render_geometry_get(body->world, NULL, &wy, NULL,
                                      NULL, &wh, NULL);
   EINA_LIST_FOREACH(smart_data->slices, l, data)
     {
        slice = (EPhysics_Body_Soft_Body_Slice *)data;
        evas_object_geometry_get(slice->evas_obj, &x, &y, NULL, &h);

        faces = body->soft_body->m_faces;
        p0 = faces[slice->index].m_n[0]->m_x;
        p1 = faces[slice->index].m_n[1]->m_x;
        p2 = faces[slice->index].m_n[2]->m_x;

        y0 = wh + wy - (p0.y() * rate);
        y1 = wh + wy - (p1.y() * rate);
        y2 = wh + wy - (p2.y() * rate);

        x0 = p0.x() * rate;
        x1 = p1.x() * rate;
        x2 = p2.x() * rate;

        z0 = p0.z() * rate;
        z1 = p1.z() * rate;
        z2 = p2.z() * rate;

        slice->stacking = p0.z() + p1.z() + p2.z();

        evas_object_map_enable_set(slice->evas_obj, EINA_FALSE);
        map = (Evas_Map *)evas_object_map_get((const Evas_Object *)
                                              slice->evas_obj);

        evas_object_image_smooth_scale_set(slice->evas_obj, EINA_TRUE);

        evas_map_point_coord_set(map, 0, x0, y0, z0);
        evas_map_point_coord_set(map, 1, x1, y1, z1);
        evas_map_point_coord_set(map, 2, x2, y2, z2);
        evas_map_point_coord_set(map, 3, x2, y2, z2);

        evas_object_map_set(slice->evas_obj, map);
        evas_object_map_enable_set(slice->evas_obj, EINA_TRUE);
        evas_object_show(slice->evas_obj);
     }

   smart_data->slices = eina_list_sort(smart_data->slices,
                                eina_list_count(smart_data->slices),
                                _ephysics_body_soft_body_slice_stacking_sort_cb);

   EINA_LIST_FOREACH(smart_data->slices, l, data)
     {
        slice = (EPhysics_Body_Soft_Body_Slice *)data;

        if (!prev_obj)
          evas_object_lower(slice->evas_obj);
        else
          evas_object_stack_above(slice->evas_obj, prev_obj);

        prev_obj = slice->evas_obj;
     }
}

static EPhysics_Body_Soft_Body_Slice *
_ephysics_body_soft_body_slice_new(int index, Evas_Object *evas_obj)
{
   EPhysics_Body_Soft_Body_Slice *slice;

   slice = (EPhysics_Body_Soft_Body_Slice *)malloc(
                                          sizeof(EPhysics_Body_Soft_Body_Slice));
   if (!slice)
     {
        ERR("Couldn't allocate EPhysics_Soft_Body_Slice memory.");
        return NULL;
     }

   slice->index = index;
   slice->evas_obj = evas_obj;
   return slice;
}

static Eina_Bool
_ephysics_body_soft_body_slices_init(EPhysics_Body *body, Evas_Object *obj)
{
   double rate;
   Evas_Coord x, y, w, h, wy, wh, y0, y1, y2, x0, x1, x2, z0, z1, z2;
   Evas_Map *map;
   Evas *evas;
   EPhysics_Body_Soft_Body_Smart_Data *smart_data;
   btSoftBody::tFaceArray faces;
   EPhysics_Body_Soft_Body_Slice *slice;
   btVector3 p0, p1, p2;

   evas = evas_object_evas_get(body->evas_obj);
   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, smart_data);
   ephysics_world_render_geometry_get(body->world, NULL, &wy, NULL, NULL,
                                      &wh, NULL);
   rate = ephysics_world_rate_get(body->world);

   for (int i = 0; i < body->slices; i++)
     {
        slice = _ephysics_body_soft_body_slice_new(body->points_deform[i],
                                             evas_object_image_filled_add(evas));

        if (!slice)
          return EINA_FALSE;

        evas_object_image_source_set(slice->evas_obj, smart_data->base_obj);

        evas_object_geometry_get(smart_data->base_obj, &x, &y, &w, &h);
        evas_object_resize(slice->evas_obj, w, h);
        evas_object_move(slice->evas_obj, x, y);
        evas_object_show(slice->evas_obj);

        faces = body->soft_body->m_faces;
        p0 = faces[slice->index].m_n[0]->m_x;
        p1 = faces[slice->index].m_n[1]->m_x;
        p2 = faces[slice->index].m_n[2]->m_x;

        y0 = wh + wy - (p0.y() * rate);
        y1 = wh + wy - (p1.y() * rate);
        y2 = wh + wy - (p2.y() * rate);

        x0 = p0.x() * rate;
        x1 = p1.x() * rate;
        x2 = p2.x() * rate;

        z0 = p0.z() * rate;
        z1 = p1.z() * rate;
        z2 = p2.z() * rate;

        map = evas_map_new(4);
        evas_map_util_points_populate_from_object(map, slice->evas_obj);

        evas_map_point_image_uv_set(map, 0, x0 - x, y0 - y);
        evas_map_point_image_uv_set(map, 1, x1 - x, y1 - y);
        evas_map_point_image_uv_set(map, 2, x2 - x, y2 - y);
        evas_map_point_image_uv_set(map, 3, x2 - x, y2 - y);

        evas_map_point_coord_set(map, 0, x0, y0, z0);
        evas_map_point_coord_set(map, 1, x1, y1, z1);
        evas_map_point_coord_set(map, 2, x2, y2, z2);
        evas_map_point_coord_set(map, 3, x2, y2, z2);

        evas_object_map_set(slice->evas_obj, map);
        evas_object_map_enable_set(slice->evas_obj, EINA_TRUE);
        evas_map_free(map);

        smart_data->slices = eina_list_append(smart_data->slices, slice);
        evas_object_smart_member_add(slice->evas_obj, obj);
     }

   return EINA_TRUE;
}

static void
_ephysics_body_soft_body_evas_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   EPhysics_Body_Soft_Body_Smart_Data *smart_data;
   Eina_List *l;
   void *data;

   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, smart_data);
   EINA_LIST_FOREACH(smart_data->slices, l, data)
     {
        EPhysics_Body_Soft_Body_Slice *slice;

        slice = (EPhysics_Body_Soft_Body_Slice *)data;
        evas_object_move(slice->evas_obj, x, y);
     }
   _ephysics_body_soft_body_slices_apply(obj);
}

static void
_ephysics_body_soft_body_evas_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   EPhysics_Body_Soft_Body_Smart_Data *smart_data;
   Eina_List *l;
   void *data;

   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, smart_data);
   EINA_LIST_FOREACH(smart_data->slices, l, data)
     {
        EPhysics_Body_Soft_Body_Slice *slice;

        slice = (EPhysics_Body_Soft_Body_Slice *)data;
        evas_object_resize(slice->evas_obj, w, h);
     }
   _ephysics_body_soft_body_slices_apply(obj);
}

static void
_ephysics_body_soft_body_evas_smart_hide(Evas_Object *obj)
{
   EPhysics_Body_Soft_Body_Smart_Data *smart_data;

   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, smart_data);
   evas_object_hide(smart_data->base_obj);
}

static void
_ephysics_body_soft_body_evas_smart_show(Evas_Object *obj)
{
   EPhysics_Body_Soft_Body_Smart_Data *smart_data;

   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, smart_data);
   evas_object_show(smart_data->base_obj);
}

static void
_ephysics_body_soft_body_evas_smart_del(Evas_Object *obj)
{
   EPhysics_Body_Soft_Body_Smart_Data *smart_data;
   void *data;

   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, smart_data);
   EINA_LIST_FREE(smart_data->slices, data)
     {
        EPhysics_Body_Soft_Body_Slice *slice;

        slice = (EPhysics_Body_Soft_Body_Slice *)data;
        evas_object_del(slice->evas_obj);
        free(slice);
     }
   _ephysics_body_soft_body_evas_parent_sc->del(obj);
}

static void
_ephysics_body_soft_body_evas_smart_calculate(Evas_Object *obj)
{
   EPhysics_Body_Soft_Body_Smart_Data *smart_data;
   Evas_Coord x, y, w, h;
   Eina_List *l;
   void *data;

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, smart_data);
   EINA_LIST_FOREACH(smart_data->slices, l, data)
     {
        EPhysics_Body_Soft_Body_Slice *slice;

        slice = (EPhysics_Body_Soft_Body_Slice *)data;
        evas_object_move(slice->evas_obj, x, y);
        evas_object_resize(slice->evas_obj, w, h);
     }
}

static void
_ephysics_body_soft_body_evas_smart_add(Evas_Object *obj)
{
   EPhysics_Body_Soft_Body_Smart_Data *priv;

   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, priv);
   if (!priv) {
      priv = (EPhysics_Body_Soft_Body_Smart_Data *)calloc(1,
                                     sizeof(EPhysics_Body_Soft_Body_Smart_Data));
      if (!priv) return;
      evas_object_smart_data_set(obj, priv);
   }

   _ephysics_body_soft_body_evas_parent_sc->add(obj);
}

static void
_ephysics_body_soft_body_evas_smart_set_user(Evas_Smart_Class *sc)
{
   sc->add = _ephysics_body_soft_body_evas_smart_add;
   sc->move = _ephysics_body_soft_body_evas_smart_move;
   sc->del = _ephysics_body_soft_body_evas_smart_del;
   sc->resize = _ephysics_body_soft_body_evas_smart_resize;
   sc->hide = _ephysics_body_soft_body_evas_smart_hide;
   sc->show = _ephysics_body_soft_body_evas_smart_show;
   sc->calculate = _ephysics_body_soft_body_evas_smart_calculate;
}

static Evas_Object *
_ephysics_body_soft_body_evas_add(EPhysics_Body *body)
{
   EPhysics_Body_Soft_Body_Smart_Data *priv;
   Evas *evas;
   Evas_Object *obj;
   Evas_Coord x, y, w, h;

   evas = evas_object_evas_get(body->evas_obj);
   obj = evas_object_smart_add(evas,
                               _ephysics_body_soft_body_evas_smart_class_new());

   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, priv);
   priv->base_obj = body->evas_obj;
   priv->body = body;

   evas_object_geometry_get(body->evas_obj, &x, &y, &w, &h);
   evas_object_move(obj, x, y);
   evas_object_resize(obj, w, h);
   evas_object_show(obj);

   if(!_ephysics_body_soft_body_slices_init(body, obj))
     return NULL;

   evas_object_move(priv->base_obj, 0, -h);
   return obj;
}

static Evas_Object *
_ephysics_body_soft_body_evas_base_obj_get(Evas_Object *obj)
{
   EPhysics_Body_Soft_Body_Smart_Data *priv;

   EPHYSICS_BODY_SOFT_BODY_SMART_DATA_GET(obj, priv);
   return priv->base_obj;
}

static btTransform
_ephysics_body_transform_get(const EPhysics_Body *body)
{
   btTransform trans;
   btVector3 center;
   btScalar radius;

   if (body->type == EPHYSICS_BODY_TYPE_RIGID)
     {
        body->rigid_body->getMotionState()->getWorldTransform(trans);
        return trans;
     }

   body->soft_body->getCollisionShape()->getBoundingSphere(center, radius);
   trans.setIdentity();
   trans.setOrigin(center);
   return trans;
}

static void
_ephysics_body_transform_set(EPhysics_Body *body, btTransform trans)
{
   btTransform origin;
   btTransform dest;

   if (body->type == EPHYSICS_BODY_TYPE_CLOTH)
     {
        origin = _ephysics_body_transform_get(body);
        dest.setIdentity();
        dest.setOrigin(trans.getOrigin() / origin.getOrigin());
        body->soft_body->translate(trans.getOrigin() - origin.getOrigin());
        return;
     }
   body->rigid_body->getMotionState()->setWorldTransform(trans);
}

static btVector3
_ephysics_body_scale_get(const EPhysics_Body *body)
{
   if (body->type == EPHYSICS_BODY_TYPE_RIGID)
     return body->collision_shape->getLocalScaling();

   return body->soft_body->getCollisionShape()->getLocalScaling();
}

void
ephysics_body_activate(const EPhysics_Body *body, Eina_Bool activate)
{
   if (body->type == EPHYSICS_BODY_TYPE_RIGID)
     {
        body->rigid_body->activate(activate);
        return;
     }

   body->soft_body->activate(activate);
}

static void
_ephysics_body_forces_update(EPhysics_Body *body)
{
   body->force.x = body->rigid_body->getTotalForce().getX();
   body->force.y = body->rigid_body->getTotalForce().getY();
   body->force.z = body->rigid_body->getTotalForce().getZ();
   body->force.torque_x = body->rigid_body->getTotalTorque().getX();
   body->force.torque_y = body->rigid_body->getTotalTorque().getY();
   body->force.torque_z = body->rigid_body->getTotalTorque().getZ();
   body->rigid_body->clearForces();
}

static inline void
_ephysics_body_sleeping_threshold_set(EPhysics_Body *body, double linear_threshold, double angular_threshold, double rate)
{
   body->rigid_body->setSleepingThresholds(linear_threshold / rate,
                                           angular_threshold / RAD_TO_DEG);
}

static inline void
_ephysics_body_linear_velocity_set(EPhysics_Body *body, double x, double y, double z, double rate)
{
   ephysics_body_activate(body, EINA_TRUE);
   body->rigid_body->setLinearVelocity(btVector3(x / rate, -y / rate,
                                                 z / rate));
}

static void
_ephysics_body_event_callback_del(EPhysics_Body *body, EPhysics_Body_Callback *cb)
{
   if (cb->deleted) return;

   cb->deleted = EINA_TRUE;

   if (body->walking)
     {
        body->to_delete = eina_list_append(body->to_delete, cb);
        return;
     }

   body->callbacks = eina_inlist_remove(body->callbacks, EINA_INLIST_GET(cb));
   free(cb);
}

static Eina_Bool
_ephysics_body_event_callback_call(EPhysics_Body *body, EPhysics_Callback_Body_Type type, void *event_info)
{
   Eina_Bool called = EINA_FALSE;
   EPhysics_Body_Callback *cb;
   void *clb;

   body->walking++;
   EINA_INLIST_FOREACH(body->callbacks, cb)
     {
        if ((cb->type == type) && (!cb->deleted))
          {
             cb->func(cb->data, body, event_info);
             called = EINA_TRUE;
          }
     }
   body->walking--;

   if (body->walking > 0) return called;

   EINA_LIST_FREE(body->to_delete, clb)
     {
        cb = (EPhysics_Body_Callback *) clb;
        body->callbacks = eina_inlist_remove(body->callbacks,
                                             EINA_INLIST_GET(cb));
        free(cb);
     }

   return called;
}

void
ephysics_body_active_set(EPhysics_Body *body, Eina_Bool active)
{
   if (body->active == !!active) return;
   body->active = !!active;
   if (active) return;

   _ephysics_body_event_callback_call(body, EPHYSICS_CALLBACK_BODY_STOPPED,
                                      (void *) body->evas_obj);
};

Eina_Bool
ephysics_body_filter_collision(EPhysics_Body *body0, EPhysics_Body *body1)
{
   Eina_List *l;
   void *grp;

   if ((!body0->collision_groups) || (!body1->collision_groups))
     return EINA_TRUE;

   EINA_LIST_FOREACH(body0->collision_groups, l, grp)
     {
        if (eina_list_data_find(body1->collision_groups, grp))
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

EAPI Eina_Bool
ephysics_body_collision_group_add(EPhysics_Body *body, const char *group)
{
   Eina_Stringshare *group_str;

   if (!body)
     {
        ERR("Can't add body collision group, body is null.");
        return EINA_FALSE;
     }

   ephysics_world_lock_take(body->world);
   group_str = eina_stringshare_add(group);
   if (eina_list_data_find(body->collision_groups, group_str))
     {
        INF("Body already added to group: %s", group);
        eina_stringshare_del(group_str);
        ephysics_world_lock_release(body->world);
        return EINA_TRUE;
     }

   body->collision_groups = eina_list_append(body->collision_groups, group_str);
   ephysics_world_lock_release(body->world);
   return EINA_TRUE;
}

EAPI Eina_Bool
ephysics_body_collision_group_del(EPhysics_Body *body, const char *group)
{
   Eina_Stringshare *group_str;

   if (!body)
     {
        ERR("Can't remove body collision group, body is null.");
        return EINA_FALSE;
     }

   ephysics_world_lock_take(body->world);
   group_str = eina_stringshare_add(group);
   if (!eina_list_data_find(body->collision_groups, group_str))
     {
        INF("Body isn't part of group: %s", group);
        eina_stringshare_del(group_str);
        ephysics_world_lock_release(body->world);
        return EINA_TRUE;
     }

   body->collision_groups = eina_list_remove(body->collision_groups, group_str);
   eina_stringshare_del(group_str);
   eina_stringshare_del(group_str);
   ephysics_world_lock_release(body->world);
   return EINA_TRUE;
}

EAPI const Eina_List *
ephysics_body_collision_group_list_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get the body's collision group, body is null.");
        return NULL;
     }

   return body->collision_groups;
}

static EPhysics_Body *
_ephysics_body_new(EPhysics_World *world, btScalar mass, double cm_x, double cm_y, double cm_z)
{
   EPhysics_Body *body;

   body = (EPhysics_Body *) calloc(1, sizeof(EPhysics_Body));
   if (!body)
     {
        ERR("Couldn't create a new body instance.");
        return NULL;
     }

   body->mass = mass;
   body->world = world;
   body->cm.x = cm_x;
   body->cm.y = cm_y;
   body->cm.z = cm_z;

   return body;
}

static EPhysics_Body *
_ephysics_body_rigid_body_add(EPhysics_World *world, btCollisionShape *collision_shape, const char *type, double cm_x, double cm_y, double cm_z)
{
   btRigidBody::btRigidBodyConstructionInfo *rigid_body_ci;
   btDefaultMotionState *motion_state;
   btRigidBody *rigid_body;
   EPhysics_Body *body;
   btScalar mass = 1;
   btVector3 inertia;

   if (!collision_shape)
     {
        ERR("Couldn't create a %s shape.", type);
        return NULL;
     }

   body = _ephysics_body_new(world, mass, cm_x, cm_y, cm_z);
   if (!body)
     {
        ERR("Couldn't create a new body instance.");
        goto err_body;
     }

   motion_state = new btDefaultMotionState();
   if (!motion_state)
     {
        ERR("Couldn't create a motion state.");
        goto err_motion_state;
     }

   inertia = btVector3(0, 0, 0);
   collision_shape->calculateLocalInertia(mass, inertia);

   rigid_body_ci = new btRigidBody::btRigidBodyConstructionInfo(
      mass, motion_state, collision_shape, inertia);
   if (!rigid_body_ci)
     {
        ERR("Couldn't create a rigid body construction info.");
        goto err_rigid_body_ci;
     }

   rigid_body = new btRigidBody(*rigid_body_ci);
   if (!rigid_body)
     {
        ERR("Couldn't create a rigid body.");
        goto err_rigid_body;
     }

   body->type = EPHYSICS_BODY_TYPE_RIGID;
   body->collision_shape = collision_shape;
   body->rigid_body = rigid_body;
   body->rigid_body->setUserPointer(body);
   body->rigid_body->setLinearFactor(btVector3(1, 1, 0));
   body->rigid_body->setAngularFactor(btVector3(0, 0, 1));

   if (!ephysics_world_body_add(body->world, body))
     {
        ERR("Couldn't add body to world's bodies list");
        goto err_world_add;
     }

   delete rigid_body_ci;

   INF("Body %p of type %s added.", body, type);
   return body;

err_world_add:
   delete rigid_body;
err_rigid_body:
   delete rigid_body_ci;
err_rigid_body_ci:
   delete motion_state;
err_motion_state:
   free(body);
err_body:
   delete collision_shape;
   return NULL;
}

static void
_ephysics_body_evas_obj_del_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EPhysics_Body *body = (EPhysics_Body *) data;
   body->evas_obj = NULL;
   DBG("Evas object deleted. Updating body: %p", body);
}

static void
_ephysics_body_soft_body_anchors_rebuild(int node, btRigidBody *rigid_body, btSoftBody *soft_body)
{
   btTransform world_trans = rigid_body->getWorldTransform();
   btVector3 local = world_trans.inverse() * soft_body->m_nodes[node].m_x;

   for (int i = 0; i < soft_body->m_anchors.size(); i++)
     {
        if (soft_body->m_anchors[i].m_node == &soft_body->m_nodes[node])
          soft_body->m_anchors[i].m_local = local;
     }
}

static void
_ephysics_body_cloth_constraints_rebuild(EPhysics_Body *body)
{
   btRigidBody *rigid_body;
   btSoftBody *soft_body;
   btSoftBody::Node *node;
   btSoftBody::Anchor anchor;
   int anchors_size = body->soft_body->m_anchors.size();

   soft_body = body->soft_body;

   if (anchors_size)
     {
        rigid_body = soft_body->m_anchors[0].m_body;
        for (int m = 0; m < anchors_size; m++)
          {
             anchor = soft_body->m_anchors[m];
             node = anchor.m_node;
             for (int n = 0; n < soft_body->m_nodes.size(); n++)
               {
                  if (node == &soft_body->m_nodes[n])
                    _ephysics_body_soft_body_anchors_rebuild(n, rigid_body,
                                                             soft_body);
               }
          }
     }
   soft_body->generateClusters(0);
   soft_body->generateBendingConstraints(2, soft_body->m_materials[
                                                          body->material_index]);
}

static void
_ephysics_body_soft_body_constraints_rebuild(EPhysics_Body *body)
{
   btSoftBody *soft_body = body->soft_body;
   btRigidBody *rigid_body = body->rigid_body;

   if (soft_body->m_anchors.size() > 0)
     {
        for (int i = 0; i < soft_body->m_nodes.size(); i++)
          _ephysics_body_soft_body_anchors_rebuild(i, rigid_body, soft_body);
     }
   else
     {
        for (int i = 0; i < soft_body->m_nodes.size(); i++)
          soft_body->appendAnchor(i, rigid_body);
     }

   soft_body->generateClusters(0);
   soft_body->generateBendingConstraints(10, soft_body->m_materials[
                                                          body->material_index]);
}

inline static double
_ephysics_body_volume_get(const EPhysics_Body *body)
{
   btVector3 vector = body->collision_shape->getLocalScaling();
   return vector.x() * vector.y() * vector.z();
}

static void
_ephysics_body_mass_set(EPhysics_Body *body, double mass)
{
   btVector3 inertia(0, 0, 0);

   if (body->density)
     mass = body->density * _ephysics_body_volume_get(body);

   if (body->soft_body)
     body->soft_body->setTotalMass(mass);
   else
     {
        body->collision_shape->calculateLocalInertia(mass, inertia);
        body->rigid_body->setMassProps(mass, inertia);
        body->rigid_body->updateInertiaTensor();
     }

   body->mass = mass;
   DBG("Body %p mass changed to %lf.", body, mass);
}

static void
_ephysics_body_resize(EPhysics_Body *body, Evas_Coord w, Evas_Coord h, Evas_Coord d)
{
   double rate, sx, sy, sz;

   rate = ephysics_world_rate_get(body->world);
   sx = w / rate;
   sy = h / rate;
   sz = d / rate;

   if (body->soft_body)
     {
        body->soft_body->scale(btVector3(sx, sy, sz));
        _ephysics_body_soft_body_constraints_rebuild(body);
     }
   else
     {
        body->collision_shape->setLocalScaling(btVector3(sx, sy, sz));

        if(!body->rigid_body->isStaticObject())
          _ephysics_body_mass_set(body, ephysics_body_mass_get(body));
     }

   body->w = w;
   body->h = h;
   body->d = d;

   ephysics_body_activate(body, EINA_TRUE);

   DBG("Body %p scale changed to (%lf, %lf, %lf).", body, sx, sy, sz);
}

static void
_ephysics_body_move(EPhysics_Body *body, Evas_Coord x, Evas_Coord y, Evas_Coord z)
{
   double rate, mx, my, mz;
   btTransform trans;
   int wy, height;
   btVector3 body_scale;

   rate = ephysics_world_rate_get(body->world);
   ephysics_world_render_geometry_get(body->world, NULL, &wy, NULL,
                                      NULL, &height, NULL);
   height += wy;

   mx = (x + body->w * body->cm.x) / rate;
   my = (height - (y + body->h * body->cm.y)) / rate;
   mz = (z + body->d * body->cm.z) / rate;

   trans = _ephysics_body_transform_get(body);
   trans.setOrigin(btVector3(mx, my, mz));
   body->rigid_body->proceedToTransform(trans);
   body->rigid_body->getMotionState()->setWorldTransform(trans);

   ephysics_body_activate(body, EINA_TRUE);

   DBG("Body %p position changed to (%lf, %lf, %lf).", body, mx, my, mz);
}

static void
_ephysics_body_geometry_set(EPhysics_Body *body, Evas_Coord x, Evas_Coord y, Evas_Coord z, Evas_Coord w, Evas_Coord h, Evas_Coord d, double rate)
{
   double mx, my, mz, sx, sy, sz;
   btTransform trans;
   int wy, height;
   btVector3 body_scale;

   ephysics_world_render_geometry_get(body->world, NULL, &wy, NULL,
                                      NULL, &height, NULL);
   height += wy;

   mx = (x + w * body->cm.x) / rate;
   my = (height - (y + h * body->cm.y)) / rate;
   mz = (z + d * body->cm.z) / rate;
   sx = w / rate;
   sy = h / rate;
   sz = d / rate;

   trans = _ephysics_body_transform_get(body);
   trans.setOrigin(btVector3(mx, my, mz));
   body_scale = btVector3(sx, sy, sz);

   if (body->type == EPHYSICS_BODY_TYPE_SOFT)
     {
        body->soft_body->scale(body_scale);
        body->rigid_body->proceedToTransform(trans);
        body->soft_body->transform(trans);
        _ephysics_body_soft_body_constraints_rebuild(body);
     }
   else if (body->type == EPHYSICS_BODY_TYPE_CLOTH)
     {
        body->soft_body->scale(body_scale);
        _ephysics_body_transform_set(body, trans);
        _ephysics_body_cloth_constraints_rebuild(body);
     }
   else
     {
        body->collision_shape->setLocalScaling(body_scale);
        body->rigid_body->proceedToTransform(trans);

        if (!body->rigid_body->isStaticObject())
          _ephysics_body_mass_set(body, ephysics_body_mass_get(body));
     }

   _ephysics_body_transform_set(body, trans);
   ephysics_body_activate(body, EINA_TRUE);

   body->w = w;
   body->h = h;
   body->d = d;

   DBG("Body %p position changed to (%lf, %lf, %lf).", body, mx, my, mz);
   DBG("Body %p scale changed to (%lf, %lf, %lf).", body, sx, sy, sz);
}

static void
_ephysics_body_evas_obj_resize_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   EPhysics_Body *body = (EPhysics_Body *) data;
   int w, h;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if ((w == body->w) && (h == body->h))
     return;

   DBG("Resizing body %p to w=%i, h=%i, d=%i", body, w, h, body->d);
   ephysics_world_lock_take(body->world);
   _ephysics_body_resize(body, w, h, body->d);
   ephysics_world_lock_release(body->world);
}

static void
_ephysics_body_del(EPhysics_Body *body)
{
   EPhysics_Body_Callback *cb;
   void *group;

   if (body->evas_obj)
     {
        evas_object_event_callback_del(body->evas_obj, EVAS_CALLBACK_DEL,
                                       _ephysics_body_evas_obj_del_cb);
        evas_object_event_callback_del(body->evas_obj, EVAS_CALLBACK_RESIZE,
                                       _ephysics_body_evas_obj_resize_cb);
     }

   while (body->callbacks)
     {
        cb = EINA_INLIST_CONTAINER_GET(body->callbacks,
                                       EPhysics_Body_Callback);
        body->callbacks = eina_inlist_remove(body->callbacks, body->callbacks);
        free(cb);
     }

   EINA_LIST_FREE(body->collision_groups, group)
      eina_stringshare_del((Eina_Stringshare *)group);

   free(body->points_deform);

   if (body->rigid_body)
     {
        delete body->rigid_body->getMotionState();
        delete body->collision_shape;
        delete body->rigid_body;
     }

   delete body->soft_body;

   free(body);
}

static void
_ephysics_body_evas_object_default_update(EPhysics_Body *body)
{
   int x, y, z, w, h, wx, wy, wh, cx, cy;
   EPhysics_Camera *camera;
   btTransform trans;
   btQuaternion quat;
   Evas_Map *map;
   double rate;

   if (!body->evas_obj)
     return;

   trans = _ephysics_body_transform_get(body);
   ephysics_world_render_geometry_get(body->world, &wx, &wy, NULL,
                                      NULL, &wh, NULL);
   camera = ephysics_world_camera_get(body->world);
   ephysics_camera_position_get(camera, &cx, &cy);
   cx -= wx;
   cy -= wy;

   evas_object_geometry_get(body->evas_obj, NULL, NULL, &w, &h);
   rate = ephysics_world_rate_get(body->world);
   x = (int) (trans.getOrigin().getX() * rate) - w * body->cm.x - cx;
   y = wh + wy - (int) (trans.getOrigin().getY() * rate) - h * body->cm.y - cy;
   z = (int) (trans.getOrigin().getZ() * rate);

   evas_object_move(body->evas_obj, x, y);

   if ((!w) || (!h))
     {
        DBG("Evas object with no geometry: %p, w=%i h=%i", body->evas_obj,
            w, h);
        return;
     }

   if (body->type != EPHYSICS_BODY_TYPE_RIGID)
     return;

   map = evas_map_new(4);
   evas_map_util_points_populate_from_object(map, body->evas_obj);

   quat = trans.getRotation();
   quat.normalize();
   evas_map_util_quat_rotate(map, quat.x(), quat.y(), quat.z(), -quat.w(),
                             x + (w * body->cm.x), y + (h * body->cm.y), z);

   if ((body->light_apply) ||
       (ephysics_world_light_all_bodies_get(body->world)))
     {
        int lr, lg, lb, ar, ag, ab;
        Evas_Coord lx, ly, lz;

        if (ephysics_world_light_get(body->world, &lx, &ly, &lz,
                                     &lr, &lg, &lb, &ar, &ag, &ab))
          evas_map_util_3d_lighting(map, lx, ly, lz, lr, lg, lb, ar, ag, ab);
     }

   evas_object_map_set(body->evas_obj, map);
   evas_object_map_enable_set(body->evas_obj, EINA_TRUE);
   evas_map_free(map);
}

static void
_ephysics_body_outside_render_area_check(EPhysics_Body *body)
{
   int wx, wy, wz, ww, wh, wd, bx, by, bz, bw, bh, bd;

   ephysics_world_render_geometry_get(body->world, &wx, &wy, &wz,
                                      &ww, &wh, &wd);
   ephysics_body_geometry_get(body, &bx, &by, &bz, &bw, &bh, &bd);

   // FIXME: check what should be done regarding rotated bodies
   if (((ephysics_world_bodies_outside_top_autodel_get(body->world)) &&
        (by + bh < wy)) ||
       ((ephysics_world_bodies_outside_bottom_autodel_get(body->world)) &&
        (by > wy + wh)) ||
       ((ephysics_world_bodies_outside_left_autodel_get(body->world)) &&
        (bx + bh < wx)) ||
       ((ephysics_world_bodies_outside_right_autodel_get(body->world)) &&
        (bx > wx + ww)) ||
       ((ephysics_world_bodies_outside_front_autodel_get(body->world)) &&
        (bz + bd < wz)) ||
       ((ephysics_world_bodies_outside_back_autodel_get(body->world)) &&
        (bz > wz + wd)))
     {
        DBG("Body %p out of render area", body);
        ephysics_body_del(body);
     }
}

void
ephysics_body_forces_apply(EPhysics_Body *body)
{
   double rate;

   if (!((body->force.x) || (body->force.y) || (body->force.z) ||
         (body->force.torque_x) || (body->force.torque_y) ||
         (body->force.torque_z)))
       return;

   rate = ephysics_world_rate_get(body->world);
   ephysics_body_activate(body, EINA_TRUE);
   body->rigid_body->applyCentralForce(btVector3(body->force.x / rate,
                                                 body->force.y / rate,
                                                 body->force.z / rate));
   body->rigid_body->applyTorque(btVector3(body->force.torque_x,
                                           body->force.torque_y,
                                           body->force.torque_z));
}

void
ephysics_body_recalc(EPhysics_Body *body, double rate)
{
   Evas_Coord x, y, z, w, h, d;
   double vx, vy, vz, lt, at;

   ephysics_body_geometry_get(body, &x, &y, &z, &w, &h, &d);
   ephysics_body_linear_velocity_get(body, &vx, &vy, &vz);
   ephysics_body_sleeping_threshold_get(body, &lt, &at);

   _ephysics_body_geometry_set(body, x, y, z, w, h, d, rate);
   _ephysics_body_linear_velocity_set(body, vx, vy, vz, rate);
   _ephysics_body_sleeping_threshold_set(body, lt, at, rate);
}

void
ephysics_body_evas_object_update_select(EPhysics_Body *body)
{
   Eina_Bool callback_called = EINA_FALSE;

   if (!body)
     return;

   callback_called = _ephysics_body_event_callback_call(
      body, EPHYSICS_CALLBACK_BODY_UPDATE, (void *) body->evas_obj);

   if (!callback_called)
     _ephysics_body_evas_object_default_update(body);

   if (ephysics_world_bodies_outside_autodel_get(body->world))
     _ephysics_body_outside_render_area_check(body);
}

EAPI void
ephysics_body_collision_position_get(const EPhysics_Body_Collision *collision, Evas_Coord *x, Evas_Coord *y, Evas_Coord *z)
{
   if (!collision)
     {
        ERR("Can't get body's collision data, collision is null.");
        return;
     }

   if (x) *x = collision->x;
   if (y) *y = collision->y;
   if (z) *z = collision->z;
}

EAPI EPhysics_Body *
ephysics_body_collision_contact_body_get(const EPhysics_Body_Collision *collision)
{
   if (!collision)
     {
        ERR("Can't get body's collision contact body, collision is null.");
        return NULL;
     }

   return collision->contact_body;
}

void
ephysics_body_contact_processed(EPhysics_Body *body, EPhysics_Body *contact_body, btVector3 position)
{
   EPhysics_Body_Collision *collision;
   EPhysics_World *world;;
   double rate;
   int wy, wh;

   if ((!body) || (!contact_body))
     return;

   collision = (EPhysics_Body_Collision *)calloc(
      1, sizeof(EPhysics_Body_Collision));

   if (!collision)
     {
        ERR("Can't allocate collision data structure.");
        return;
     }

   world = contact_body->world;
   ephysics_world_render_geometry_get(world, NULL, &wy, NULL, NULL, &wh, NULL);
   rate = ephysics_world_rate_get(world);

   collision->contact_body = contact_body;
   collision->x = position.getX() * rate;
   collision->y = wh + wy - (position.getY() * rate);
   collision->z = position.getZ() * rate;

   _ephysics_body_event_callback_call(body, EPHYSICS_CALLBACK_BODY_COLLISION,
                                      (void *) collision);

   free(collision);
}

btRigidBody *
ephysics_body_rigid_body_get(const EPhysics_Body *body)
{
   return body->rigid_body;
}

btSoftBody *
ephysics_body_soft_body_get(const EPhysics_Body *body)
{
   return body->soft_body;
}

static void
_ephysics_body_soft_body_hardness_set(EPhysics_Body *body, double hardness)
{
   int m = body->material_index;
   btSoftBody *soft_body = body->soft_body;
   soft_body->m_cfg.kAHR = (hardness / body->anchor_prop) * 0.6;
   soft_body->m_materials[m]->m_kVST = (hardness / 1000);
   soft_body->m_materials[m]->m_kLST = (hardness / 1000);
   soft_body->m_materials[m]->m_kAST = (hardness / 1000);
   DBG("Soft body %p hardness set to %lf.", body, hardness);
}

EAPI void
ephysics_body_soft_body_hardness_set(EPhysics_Body *body, double hardness)
{
   if (!body)
     {
        ERR("Can't set soft body's hardness, body is null.");
        return;
     }

   if (!body->soft_body)
     {
        ERR("Can't set soft body's hardness, body seems not to be a soft body.");
        return;
     }

   if (hardness < 0 || hardness > 100)
     {
        ERR("Can't set soft body's hardness, it must be between 0 and 100.");
        return;
     }

   ephysics_world_lock_take(body->world);
   _ephysics_body_soft_body_hardness_set(body, hardness);
   ephysics_world_lock_release(body->world);
}

EAPI double
ephysics_body_soft_body_hardness_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get soft body's hardness, body is null.");
        return 0;
     }

   if (!body->soft_body)
     {
        ERR("Can't get soft body's hardness, body seems not to be a soft body.");
        return 0;
     }

   return (body->soft_body->m_materials[body->material_index]->m_kVST * 100);
}

static void
_ephysics_body_soft_body_default_config(EPhysics_Body *body, btSoftBody *soft_body)
{
   body->soft_body = soft_body;
   body->soft_body->getCollisionShape()->setMargin(btScalar(0.02));
   body->soft_body->setUserPointer(body);
   body->soft_body->setTotalMass(body->mass);

   body->soft_body->m_cfg.collisions += btSoftBody::fCollision::SDF_RS;
   body->soft_body->m_cfg.collisions += btSoftBody::fCollision::VF_SS;
   _ephysics_body_soft_body_hardness_set(body, 100);
}

static EPhysics_Body *
_ephysics_body_soft_body_add(EPhysics_World *world, btCollisionShape *collision_shape, btSoftBody *soft_body)
{
   EPhysics_Body *body;

   body = _ephysics_body_rigid_body_add(world, collision_shape, "soft box", 0.5,
                                        0.5, 0.5);
   if (!body)
     {
        if (body->deleted) return NULL;
        body->deleted = EINA_TRUE;
        ephysics_world_body_del(body->world, body);
        return NULL;
     }

   body->material_index = 0;
   body->anchor_prop = 1000;
   body->type = EPHYSICS_BODY_TYPE_SOFT;
   _ephysics_body_soft_body_default_config(body, soft_body);

   body->rigid_body->setCollisionFlags(
                                      btCollisionObject::CF_NO_CONTACT_RESPONSE);

   _ephysics_body_soft_body_constraints_rebuild(body);
   ephysics_world_soft_body_add(world, body);
   return body;
}

EAPI void
ephysics_body_cloth_anchor_full_add(EPhysics_Body *body1, EPhysics_Body *body2, EPhysics_Body_Cloth_Anchor_Side side)
{
   int rows;
   int columns;

   if (!body1 || !body2)
     {
        ERR("Could not add anchors, body1 or body2 is null");
        return;
     }

   if (body1->type != EPHYSICS_BODY_TYPE_CLOTH ||
       body2->type != EPHYSICS_BODY_TYPE_RIGID)
     {
        ERR("Cloth anchors are allowed only between cloth and rigid body.");
        return;
     }

   rows = body1->cloth_rows;
   columns = body1->cloth_columns;

   if (side == EPHYSICS_BODY_CLOTH_ANCHOR_SIDE_LEFT)
     {
        for (int i = 0; i < rows; i++)
          body1->soft_body->appendAnchor(i, body2->rigid_body);
        return;
     }

   if (side == EPHYSICS_BODY_CLOTH_ANCHOR_SIDE_RIGHT)
     {
        for (int i = 1; i <= rows; i++)
          body1->soft_body->appendAnchor((rows * columns) - i,
                                         body2->rigid_body);
        return;
     }

   if (side == EPHYSICS_BODY_CLOTH_ANCHOR_SIDE_TOP)
     {
        for (int i = 0; i <= rows; i++)
          body1->soft_body->appendAnchor(i * rows, body2->rigid_body);
        return;
     }

   if (side == EPHYSICS_BODY_CLOTH_ANCHOR_SIDE_BOTTOM)
     {
        for (int i = 0; i < columns; i++)
             body1->soft_body->appendAnchor((rows - 1) + rows * i,
                                           body2->rigid_body);
     }
}

EAPI void
ephysics_body_cloth_anchor_add(EPhysics_Body *body1, EPhysics_Body *body2, int node)
{
   if (!body1 || !body2)
     {
        ERR("Could not add anchors, body1 or body2 is null");
        return;
     }

   if (body1->type != EPHYSICS_BODY_TYPE_CLOTH ||
       body2->type != EPHYSICS_BODY_TYPE_RIGID)
     {
        ERR("Cloth anchors are allowed only between cloth and rigid body.");
        return;
     }

   body1->soft_body->appendAnchor(node, body2->rigid_body);
}

EAPI void
ephysics_body_cloth_anchor_del(EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Could not delete anchor, body is null.");
        return;
     }

   if (body->type != EPHYSICS_BODY_TYPE_CLOTH)
     {
        ERR("Could not delete anchors, body is not a cloth.");
        return;
     }

   body->soft_body->m_anchors.resize(0);
}

EAPI EPhysics_Body *
ephysics_body_cloth_add(EPhysics_World *world, unsigned short granularity)
{
   EPhysics_Body *body;
   btSoftBodyWorldInfo *world_info;
   btSoftBody *soft_body;
   const int rows = (!granularity) ? 15 : granularity;
   const int columns = (!granularity) ? 15 : granularity;

   if (!world)
     {
        ERR("Can't add circle, world is null.");
        return NULL;
     }

   world_info = ephysics_world_info_get(world);
   soft_body = btSoftBodyHelpers::CreatePatch(*world_info,
                                              btVector3(1, 2, 0),
                                              btVector3(1, 1, 0),
                                              btVector3(2, 2, 0),
                                              btVector3(2, 1, 0),
                                              rows, columns, 0, false);
   if (!soft_body)
     {
        ERR("Couldn't create a new soft body.");
        return NULL;
     }

   body = _ephysics_body_new(world, 1, 0.5, 0.5, 0.5);
   if (!body)
     goto no_body;

   soft_body->setPose(false, true);

   soft_body->appendMaterial();
   body->material_index = 1;
   soft_body->m_cfg.piterations = 5;

   _ephysics_body_soft_body_default_config(body, soft_body);
   _ephysics_body_cloth_constraints_rebuild(body);

   body->slices = soft_body->m_faces.size();
   body->points_deform = (int *)malloc(body->slices * sizeof(int));
   if (!body->points_deform)
     {
        ERR("Couldn't create points of deformation.");
        goto no_deform;
     }

   for (int i = 0; i < body->slices; i++)
     body->points_deform[i] = i;

   body->cloth_columns = columns;
   body->cloth_rows = rows;
   body->anchor_prop = 1;
   body->type = EPHYSICS_BODY_TYPE_CLOTH;

   ephysics_world_soft_body_add(world, body);
   return body;

no_deform:
   free(body);
no_body:
   delete soft_body;
   return NULL;
}

static void
_ephysics_body_soft_body_single_face_transform(btSoftBody *soft_body, int face_idx, int node_idx, btTransform trans, btScalar vel_reduction)
{
   btSoftBody::Node *node;
   ATTRIBUTE_ALIGNED16(btDbvtVolume) vol;
   const btScalar margin = soft_body->getCollisionShape()->getMargin();

   node = soft_body->m_faces[face_idx].m_n[node_idx];
   node->m_x = trans * node->m_x;
   node->m_q = trans * node->m_q;
   node->m_n = trans.getBasis() * node->m_n;
   node->m_v *= vel_reduction;
   vol = btDbvtVolume::FromCR(node->m_x, margin);
   soft_body->m_ndbvt.update(node->m_leaf, vol);
}

EAPI void
ephysics_body_soft_body_triangle_move(EPhysics_Body *body, int idx, Evas_Coord x, Evas_Coord y, Evas_Coord z)
{
   btScalar xx, yy, zz;
   Evas_Coord wh;
   double rate;
   btVector3 new_pos;
   btTransform diff;
   btSoftBody::Face face;
   btSoftBody::Node *node;

   if (body->type == EPHYSICS_BODY_TYPE_RIGID)
     {
        ERR("Could not move soft body triangle, body must be soft or cloth.");
        return;
     }

   if (idx < 0 || idx >= body->soft_body->m_faces.size())
     {
        ERR("Could not move soft body triangle, provided body triangle index "
            "ranges from 0 to %d", body->soft_body->m_faces.size());
        return;
     }

   rate = ephysics_world_rate_get(body->world);
   ephysics_world_render_geometry_get(body->world, NULL, NULL, NULL, &wh);

   xx = x / rate;
   yy = (wh -  y) / rate;
   zz = z / rate;

   new_pos = btVector3(xx, yy, zz);
   ephysics_world_lock_take(body->world);

   face = body->soft_body->m_faces[idx];
   for (int i = 0; i < 3; i++)
     {
        node = face.m_n[i];
        diff.setIdentity();
        diff.setOrigin(new_pos - node->m_x);
        _ephysics_body_soft_body_single_face_transform(body->soft_body, idx, i,
                                                       diff, 0.25);

        if (!i) continue;

        diff.setOrigin(diff.getOrigin() * 0.1);
        for (int m = 0; m < body->soft_body->m_faces.size(); m++)
          {
             if (m == idx) continue;
             _ephysics_body_soft_body_single_face_transform(body->soft_body, m,
                                                            i, diff, 0.05);
          }
     }

   body->soft_body->updateClusters();
   body->soft_body->updateBounds();
   body->soft_body->updateNormals();
   body->soft_body->updatePose();
   ephysics_world_lock_release(body->world);
}

EAPI int
ephysics_body_soft_body_triangle_index_get(EPhysics_Body *body, Evas_Coord x, Evas_Coord y)
{
   btVector3 ray_from;
   btVector3 ray_to;
   btSoftBody::sRayCast result;
   Evas_Coord wh, bw;
   btScalar xx, yy;
   double rate;
   int index = -1;

   if (body->type == EPHYSICS_BODY_TYPE_RIGID)
     {
        ERR("Can't get node index, operation not permited to rigid bodies");
        return -1;
     }

   ephysics_world_lock_take(body->world);
   rate = ephysics_world_rate_get(body->world);
   ephysics_world_render_geometry_get(body->world, NULL, NULL, NULL, &wh);
   ephysics_body_geometry_get(body, NULL, NULL, NULL, &bw, NULL, NULL);

   xx =  x / rate;
   yy = (wh -  y) / rate;

   ray_from = btVector3(xx, yy, -100);
   ray_to = btVector3(xx, yy, 100);

   if (body->soft_body->rayTest(ray_from, ray_to, result))
     index = result.index;
   ephysics_world_lock_release(body->world);

   return index;
}

EAPI EPhysics_Body *
ephysics_body_soft_circle_add(EPhysics_World *world)
{
   EPhysics_Body *body;
   btCollisionShape *shape;
   btSoftBodyWorldInfo *world_info;
   btSoftBody *soft_body;
   int points[19] = {16, 58, 44, 79, 97, 35, 6, 27, 45, 1, 38, 18, 21, 10, 26,
                     7, 86, 37, 55};

   if (!world)
     {
        ERR("Can't add circle, world is null.");
        return NULL;
     }

   ephysics_world_lock_take(world);
   shape = new btCylinderShapeZ(btVector3(0.5, 0.5, 0.5));

   if (!shape)
     {
        ERR("Couldn't create a new cylinder shape.");
        goto no_collision_shape;
     }

   world_info = ephysics_world_info_get(world);
   soft_body = btSoftBodyHelpers::CreateFromTriMesh(*world_info,
                        cylinder_vertices, &cylinder_indices[0][0],
                        CYLINDER_NUM_TRIANGLES);
   if (!soft_body)
     {
        ERR("Couldn't create a new soft body.");
        goto no_soft_body;
     }

   body = _ephysics_body_soft_body_add(world, shape, soft_body);
   if (!body)
     goto no_body;

   body->slices = 19;
   body->points_deform = (int *)malloc(body->slices * sizeof(int));
   if (!body->points_deform)
     {
        ERR("Couldn't create points of deformation.");
        goto no_deform;
     }

   for (int i = 0; i < body->slices; i++)
     body->points_deform[i] = points[i];

   ephysics_world_lock_release(world);
   return body;

no_body:
   delete soft_body;
no_soft_body:
   delete shape;
no_deform:
   ephysics_world_body_del(world, body);
no_collision_shape:
   ephysics_world_lock_release(world);
   return NULL;
}

EAPI EPhysics_Body *
ephysics_body_circle_add(EPhysics_World *world)
{
   btCollisionShape *collision_shape;
   EPhysics_Body *body;

   if (!world)
     {
        ERR("Can't add circle, world is null.");
        return NULL;
     }

   collision_shape = new btCylinderShapeZ(btVector3(0.5, 0.5, 0.5));
   if (!collision_shape)
     {
        ERR("Couldn't create a new cylinder shape.");
        return NULL;
     }

   ephysics_world_lock_take(world);
   body = _ephysics_body_rigid_body_add(world, collision_shape, "circle", 0.5,
                                        0.5, 0.5);
   ephysics_world_lock_release(world);
   return body;
}

EAPI EPhysics_Body *
ephysics_body_soft_box_add(EPhysics_World *world)
{
   EPhysics_Body *body;
   btCollisionShape *shape;
   btSoftBodyWorldInfo *world_info;
   btSoftBody *soft_body;
   int points[16] = {14, 85, 88, 28, 41, 55, 10, 24, 93, 79, 56, 86, 91, 8,
                     27, 1};


   if (!world)
     {
        ERR("Can't add circle, world is null.");
        return NULL;
     }

   ephysics_world_lock_take(world);
   shape = new btBoxShape(btVector3(0.25, 0.25, 0.25));
   if (!shape)
     {
        ERR("Couldn't create a new box shape.");
        goto no_collision_shape;
     }

   world_info = ephysics_world_info_get(world);
   soft_body = btSoftBodyHelpers::CreateFromTriMesh(*world_info,
                                            cube_vertices, &cube_indices[0][0],
                                            CUBE_NUM_TRIANGLES);
   if (!soft_body)
     {
        ERR("Couldn't create a new soft body.");
        goto no_soft_body;
     }

   body = _ephysics_body_soft_body_add(world, shape, soft_body);
   if (!body)
     goto no_body;

   body->slices = 16;
   body->points_deform = (int *)malloc(body->slices * sizeof(int));
   if (!body->points_deform)
     {
        ERR("Couldn't create points of deformation.");
        goto no_deform;
     }

   for (int i = 0; i < body->slices; i++)
     body->points_deform[i] = points[i];

   ephysics_world_lock_release(world);
   return body;

no_body:
   delete soft_body;
no_soft_body:
   delete shape;
no_deform:
   ephysics_world_body_del(world, body);
no_collision_shape:
   ephysics_world_lock_release(world);
   return NULL;
}

EAPI EPhysics_Body *
ephysics_body_box_add(EPhysics_World *world)
{
   btCollisionShape *collision_shape;
   EPhysics_Body *body;

   if (!world)
     {
        ERR("Can't add box, world is null.");
        return NULL;
     }

   collision_shape = new btBoxShape(btVector3(0.5, 0.5, 0.5));

   ephysics_world_lock_take(world);
   body = _ephysics_body_rigid_body_add(world, collision_shape, "box", 0.5,
                                        0.5, 0.5);
   ephysics_world_lock_release(world);
   return body;
}

EAPI EPhysics_Body *
ephysics_body_shape_add(EPhysics_World *world, EPhysics_Shape *shape)
{
   double max_x, max_y, max_z, min_x, min_y, min_z, cm_x, cm_y, cm_z,
          range_x, range_y, range_z;
   btConvexHullShape *full_shape, *simplified_shape;
   btAlignedObjectArray<btVector3> vertexes, planes;
   const Eina_Inlist *points;
   EPhysics_Point *point;
   EPhysics_Body *body;
   int array_size, i;
   btShapeHull *hull;
   btVector3 point3d;
   btScalar margin;

   if (!world)
     {
        ERR("Can't add shape, world is null.");
        return NULL;
     }

   if (!shape)
     {
        ERR("Can't add shape, shape is null.");
        return NULL;
     }

   points = ephysics_shape_points_get(shape);
   if (eina_inlist_count(points) < 3)
     {
        ERR("At least 3 points are required to add a shape");
        return NULL;
     }

   full_shape = new btConvexHullShape();
   if (!full_shape)
     {
        ERR("Couldn't create a generic convex shape.");
        return NULL;
     }

   point = EINA_INLIST_CONTAINER_GET(points, EPhysics_Point);
   max_x = min_x = point->x;
   max_y = min_y = point->y;
   max_z = min_z = point->z;
   cm_x = cm_y = cm_z = 0;

   /* FIXME : only vertices should be used to calculate the center of mass */
   EINA_INLIST_FOREACH(points, point)
     {
        if (point->x > max_x) max_x = point->x;
        if (point->x < min_x) min_x = point->x;
        if (point->y > max_y) max_y = point->y;
        if (point->y < min_y) min_y = point->y;
        if (point->z > max_z) max_z = point->z;
        if (point->z < min_z) min_z = point->z;

        cm_x += point->x;
        cm_y += point->y;
        cm_z += point->z;
     }

   cm_x /= eina_inlist_count(points);
   cm_y /= eina_inlist_count(points);
   cm_z /= eina_inlist_count(points);
   range_x = max_x - min_x;
   range_y = max_y - min_y;
   range_z = max_z - min_z;

   EINA_INLIST_FOREACH(points, point)
     {
        double x, y, z;

        x = (point->x - cm_x) / range_x;
        y = - (point->y - cm_y) / range_y;
        z = (point->z - cm_z) / range_z;

        point3d = btVector3(x, y, z);
        vertexes.push_back(point3d);
     }

   /* Shrink convex shape to consider margin. Otherwise it would have a gap */
   btGeometryUtil::getPlaneEquationsFromVertices(vertexes, planes);
   array_size = planes.size();
   for (i = 0; i < array_size; ++i)
     planes[i][3] += full_shape->getMargin();

   vertexes.clear();
   btGeometryUtil::getVerticesFromPlaneEquations(planes, vertexes);

   array_size = vertexes.size();
   for (i = 0; i < array_size; ++i)
     full_shape->addPoint(vertexes[i]);

   hull = new btShapeHull(full_shape);
   if (!hull)
     {
        delete full_shape;
        ERR("Couldn't create a shape hull.");
        return NULL;
     }

   margin = full_shape->getMargin();
   hull->buildHull(margin);
   simplified_shape = new btConvexHullShape(&(hull->getVertexPointer()->getX()),
                                            hull->numVertices());
   delete hull;
   delete full_shape;
   if (!simplified_shape)
     {
        ERR("Couldn't create a simplified shape.");
        return NULL;
     }

   ephysics_world_lock_take(world);
   body = _ephysics_body_rigid_body_add(world,
                                        (btCollisionShape *)simplified_shape,
                                        "generic", (cm_x - min_x) / range_x,
                                        1 - (cm_y - min_y) / range_y,
                                        (cm_z - min_z) / range_z);
   ephysics_world_lock_release(world);
   return body;
}

void
ephysics_body_world_boundaries_resize(EPhysics_World *world)
{
   Evas_Coord x, y, z, w, h, d;
   EPhysics_Body *bound;

   ephysics_world_render_geometry_get(world, &x, &y, &z, &w, &h, &d);

   bound = ephysics_world_boundary_get(world, EPHYSICS_WORLD_BOUNDARY_BOTTOM);
   if (bound)
     ephysics_body_geometry_set(bound, x, y + h, z, w, 10, d);

   bound = ephysics_world_boundary_get(world, EPHYSICS_WORLD_BOUNDARY_RIGHT);
   if (bound)
     ephysics_body_geometry_set(bound, x + w, y, z, 10, h, d);

   bound = ephysics_world_boundary_get(world, EPHYSICS_WORLD_BOUNDARY_LEFT);
   if (bound)
     ephysics_body_geometry_set(bound,  x - 10, y, z, 10, h, d);

   bound = ephysics_world_boundary_get(world, EPHYSICS_WORLD_BOUNDARY_TOP);
   if (bound)
     ephysics_body_geometry_set(bound, x, y - 10, z, w, 10, d);

   bound = ephysics_world_boundary_get(world, EPHYSICS_WORLD_BOUNDARY_FRONT);
   if (bound)
     ephysics_body_geometry_set(bound, x, y, z - 10, w, h, 10);

   bound = ephysics_world_boundary_get(world, EPHYSICS_WORLD_BOUNDARY_BACK);
   if (bound)
     ephysics_body_geometry_set(bound, x, y, z + d, w, h, 10);
}

static EPhysics_Body *
_ephysics_body_boundary_add(EPhysics_World *world, EPhysics_World_Boundary boundary, Evas_Coord x, Evas_Coord y, Evas_Coord z, Evas_Coord w, Evas_Coord h, Evas_Coord d)
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
   ephysics_body_geometry_set(body, x, y, z, w, h, d);

   return body;
}

EAPI EPhysics_Body *
ephysics_body_top_boundary_add(EPhysics_World *world)
{
   Evas_Coord x, y, z, w, d;
   EPhysics_Body *body;

   ephysics_world_render_geometry_get(world, &x, &y, &z, &w, NULL, &d);
   body =  _ephysics_body_boundary_add(world, EPHYSICS_WORLD_BOUNDARY_TOP,
                                       x, y - 10, z, w, 10, d);
   return body;
}

EAPI EPhysics_Body *
ephysics_body_bottom_boundary_add(EPhysics_World *world)
{
   Evas_Coord x, y, z, w, h, d;
   EPhysics_Body *body;

   ephysics_world_render_geometry_get(world, &x, &y, &z, &w, &h, &d);
   body = _ephysics_body_boundary_add(world, EPHYSICS_WORLD_BOUNDARY_BOTTOM,
                                      x, y + h, z, w, 10, d);
   return body;
}

EAPI EPhysics_Body *
ephysics_body_left_boundary_add(EPhysics_World *world)
{
   Evas_Coord x, y, z, h, d;
   EPhysics_Body *body;

   ephysics_world_render_geometry_get(world, &x, &y, &z, NULL, &h, &d);
   body = _ephysics_body_boundary_add(world, EPHYSICS_WORLD_BOUNDARY_LEFT,
                                      x - 10, y, z, 10, h, d);
   return body;
}

EAPI EPhysics_Body *
ephysics_body_right_boundary_add(EPhysics_World *world)
{
   Evas_Coord x, y, z, w, h, d;
   EPhysics_Body *body;

   ephysics_world_render_geometry_get(world, &x, &y, &z, &w, &h, &d);
   body = _ephysics_body_boundary_add(world, EPHYSICS_WORLD_BOUNDARY_RIGHT,
                                      x + w, y, z, 10, h, d);
   return body;
}

EAPI EPhysics_Body *
ephysics_body_front_boundary_add(EPhysics_World *world)
{
   Evas_Coord x, y, z, w, h;
   EPhysics_Body *body;

   ephysics_world_render_geometry_get(world, &x, &y, &z, &w, &h, NULL);
   body = _ephysics_body_boundary_add(world, EPHYSICS_WORLD_BOUNDARY_FRONT,
                                      x, y, z - 10, w, h, 10);
   return body;
}

EAPI EPhysics_Body *
ephysics_body_back_boundary_add(EPhysics_World *world)
{
   Evas_Coord x, y, z, w, h, d;
   EPhysics_Body *body;

   ephysics_world_render_geometry_get(world, &x, &y, &z, &w, &h, &d);
   body = _ephysics_body_boundary_add(world, EPHYSICS_WORLD_BOUNDARY_BACK,
                                      x, y, z + d, w, h, 10);
   return body;
}

void
ephysics_orphan_body_del(EPhysics_Body *body)
{
   _ephysics_body_event_callback_call(body, EPHYSICS_CALLBACK_BODY_DEL,
                                      (void *) body->evas_obj);
   _ephysics_body_del(body);
   INF("Body %p deleted.", body);
}

EAPI void
ephysics_body_del(EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't delete body, it wasn't provided.");
        return;
     }

   if (body->deleted) return;
   ephysics_world_lock_take(body->world);
   body->deleted = EINA_TRUE;
   ephysics_world_body_del(body->world, body);
   ephysics_world_lock_release(body->world);
}

EAPI Evas_Object *
ephysics_body_evas_object_set(EPhysics_Body *body, Evas_Object *evas_obj, Eina_Bool use_obj_pos)
{
   int obj_x, obj_y, obj_w, obj_h;
   double rate;

   if (!body)
     {
        ERR("Can't set evas object to body, the last wasn't provided.");
        return NULL;
     }

   if (!evas_obj)
     {
        ERR("Can't set evas object to body, the first wasn't provided.");
        return NULL;
     }

   if (body->evas_obj)
     {
        evas_object_event_callback_del(body->evas_obj, EVAS_CALLBACK_DEL,
                                       _ephysics_body_evas_obj_del_cb);
        evas_object_event_callback_del(body->evas_obj, EVAS_CALLBACK_RESIZE,
                                       _ephysics_body_evas_obj_resize_cb);
     }

   body->evas_obj = evas_obj;


   evas_object_event_callback_add(evas_obj, EVAS_CALLBACK_DEL,
                                  _ephysics_body_evas_obj_del_cb, body);

   if (!use_obj_pos)
     return evas_obj;

   evas_object_geometry_get(body->evas_obj, &obj_x, &obj_y, &obj_w, &obj_h);
   rate = ephysics_world_rate_get(body->world);
   ephysics_world_lock_take(body->world);
   _ephysics_body_geometry_set(body, obj_x, obj_y, - rate / 2,
                               obj_w, obj_h, rate, rate);

   if (body->soft_body)
     {
        evas_object_event_callback_del(evas_obj, EVAS_CALLBACK_DEL,
                                       _ephysics_body_evas_obj_del_cb);

        evas_obj = _ephysics_body_soft_body_evas_add(body);
        body->evas_obj = evas_obj;

        evas_object_event_callback_add(body->evas_obj, EVAS_CALLBACK_DEL,
                                       _ephysics_body_evas_obj_del_cb, body);
     }

   ephysics_world_lock_release(body->world);
   evas_object_event_callback_add(body->evas_obj, EVAS_CALLBACK_RESIZE,
                                  _ephysics_body_evas_obj_resize_cb, body);

   return evas_obj;
}

EAPI Evas_Object *
ephysics_body_evas_object_unset(EPhysics_Body *body)
{
   Evas_Object *obj, *smart;

   if (!body)
     {
        ERR("Can't unset evas object from body, it wasn't provided.");
        return NULL;
     }

   obj = body->evas_obj;
   body->evas_obj = NULL;

   if (obj)
     {
        evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL,
                                       _ephysics_body_evas_obj_del_cb);
        evas_object_event_callback_del(obj, EVAS_CALLBACK_RESIZE,
                                       _ephysics_body_evas_obj_resize_cb);
     }

   if (evas_object_smart_type_check(obj, SMART_CLASS_NAME))
     {
        smart = obj;
        obj = _ephysics_body_soft_body_evas_base_obj_get(obj);
        evas_object_del(smart);
     }

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
ephysics_body_geometry_set(EPhysics_Body *body, Evas_Coord x, Evas_Coord y, Evas_Coord z, Evas_Coord w, Evas_Coord h, Evas_Coord d)
{
   if (!body)
     {
        ERR("Can't set body geometry, body is null.");
        return;
     }

   if ((w <= 0) || (h <= 0) || (d <= 0))
     {
        ERR("Width, height and depth must to be a non-null, positive value.");
        return;
     }

   ephysics_world_lock_take(body->world);
   _ephysics_body_geometry_set(body, x, y, z, w, h, d,
                               ephysics_world_rate_get(body->world));
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_resize(EPhysics_Body *body, Evas_Coord w, Evas_Coord h, Evas_Coord d)
{
   if (!body)
     {
        ERR("Can't set body size, body is null.");
        return;
     }

   if ((w <= 0) || (h <= 0) || (d <= 0))
     {
        ERR("Width, height and depth must to be a non-null, positive value.");
        return;
     }

   ephysics_world_lock_take(body->world);
   _ephysics_body_resize(body, w, h, d);
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_move(EPhysics_Body *body, Evas_Coord x, Evas_Coord y, Evas_Coord z)
{
   if (!body)
     {
        ERR("Can't set body position, body is null.");
        return;
     }

   ephysics_world_lock_take(body->world);
   _ephysics_body_move(body, x, y, z);
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_geometry_get(const EPhysics_Body *body, Evas_Coord *x, Evas_Coord *y, Evas_Coord *z, Evas_Coord *w, Evas_Coord *h, Evas_Coord *d)
{
   btTransform trans;
   btVector3 scale;
   double rate;
   int wy, height;

   if (!body)
     {
        ERR("Can't get body position, body is null.");
        return;
     }

   trans = _ephysics_body_transform_get(body);
   scale = _ephysics_body_scale_get(body);

   rate = ephysics_world_rate_get(body->world);
   ephysics_world_render_geometry_get(body->world, NULL, &wy, NULL,
                                      NULL, &height, NULL);
   height += wy;

   if (x) *x = round((trans.getOrigin().getX() - scale.x() / 2) * rate);
   if (y) *y = height - round((trans.getOrigin().getY() + scale.y() / 2)
                              * rate);
   if (z) *z = round((trans.getOrigin().getZ() - scale.z() / 2) * rate);
   if (w) *w = body->w;
   if (h) *h = body->h;
   if (d) *d = body->d;
}

EAPI void
ephysics_body_mass_set(EPhysics_Body *body, double mass)
{
   if (!body)
     {
        ERR("Can't set body mass, body is null.");
        return;
     }

   if (mass < 0)
     {
        ERR("Can't set body's mass, it must to be non-negative.");
        return;
     }

   ephysics_world_lock_take(body->world);
   body->material = EPHYSICS_BODY_MATERIAL_CUSTOM;
   body->density = 0;
   _ephysics_body_mass_set(body, mass);
   ephysics_world_lock_release(body->world);
}

EAPI double
ephysics_body_mass_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get body mass, body is null.");
        return 0;
     }

   return body->mass;
}

EAPI void
ephysics_body_linear_velocity_set(EPhysics_Body *body, double x, double y, double z)
{
   if (!body)
     {
        ERR("Can't set body linear velocity, body is null.");
        return;
     }

   _ephysics_body_linear_velocity_set(body, x, y, z,
                                      ephysics_world_rate_get(body->world));
   DBG("Linear velocity of body %p set to (%lf, %lf, %lf).", body, x, y, z);
}

EAPI void
ephysics_body_linear_velocity_get(const EPhysics_Body *body, double *x, double *y, double *z)
{
   double rate;

   if (!body)
     {
        ERR("Can't get linear velocity, body is null.");
        return;
     }

   rate = ephysics_world_rate_get(body->world);
   if (x) *x = body->rigid_body->getLinearVelocity().getX() * rate;
   if (y) *y = -body->rigid_body->getLinearVelocity().getY() * rate;
   if (z) *z = body->rigid_body->getLinearVelocity().getZ() * rate;
}

EAPI void
ephysics_body_angular_velocity_set(EPhysics_Body *body, double x, double y, double z)
{
   if (!body)
     {
        ERR("Can't set angular velocity, body is null.");
        return;
     }

   ephysics_world_lock_take(body->world);
   body->rigid_body->setAngularVelocity(btVector3(-x / RAD_TO_DEG,
                                                  -y / RAD_TO_DEG,
                                                  -z/RAD_TO_DEG));
   DBG("Angular velocity of body %p set to (%lf, %lf, %lf).", body, x, y, z);
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_angular_velocity_get(const EPhysics_Body *body, double *x, double *y, double *z)
{
   if (!body)
     {
        ERR("Can't get angular velocity, body is null.");
        return;
     }

   if (x) *x = -body->rigid_body->getAngularVelocity().getX() * RAD_TO_DEG;
   if (y) *y = -body->rigid_body->getAngularVelocity().getY() * RAD_TO_DEG;
   if (z) *z = -body->rigid_body->getAngularVelocity().getZ() * RAD_TO_DEG;
}

EAPI void
ephysics_body_sleeping_threshold_set(EPhysics_Body *body, double linear_threshold, double angular_threshold)
{
   if (!body)
     {
        ERR("Can't set sleeping thresholds, body is null.");
        return;
     }

   ephysics_world_lock_take(body->world);
   _ephysics_body_sleeping_threshold_set(body, linear_threshold,
                                         angular_threshold,
                                         ephysics_world_rate_get(body->world));
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_sleeping_threshold_get(const EPhysics_Body *body, double *linear_threshold, double *angular_threshold)
{
   double rate;

   if (!body)
     {
        ERR("Can't get linear sleeping threshold, body is null.");
        return;
     }

   rate = ephysics_world_rate_get(body->world);
   if (linear_threshold)
     *linear_threshold = body->rigid_body->getLinearSleepingThreshold() * rate;
   if (angular_threshold)
     *angular_threshold = body->rigid_body->getAngularSleepingThreshold() *
        RAD_TO_DEG;
}

EAPI void
ephysics_body_stop(EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't stop a null body.");
        return;
     }

   ephysics_world_lock_take(body->world);
   body->rigid_body->setLinearVelocity(btVector3(0, 0, 0));
   body->rigid_body->setAngularVelocity(btVector3(0, 0, 0));
   ephysics_world_lock_release(body->world);

   DBG("Body %p stopped", body);
}

EAPI void
ephysics_body_damping_set(EPhysics_Body *body, double linear_damping, double angular_damping)
{
    if (!body)
     {
        ERR("Can't set body damping, body is null.");
        return;
     }

   ephysics_world_lock_take(body->world);
    body->rigid_body->setDamping(btScalar(linear_damping),
                                 btScalar(angular_damping));
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_damping_get(const EPhysics_Body *body, double *linear_damping, double *angular_damping)
{
    if (!body)
     {
        ERR("Can't get damping, body is null.");
        return;
     }

    if (linear_damping) *linear_damping = body->rigid_body->getLinearDamping();
    if (angular_damping) *angular_damping =
      body->rigid_body->getAngularDamping();
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
ephysics_body_event_callback_add(EPhysics_Body *body, EPhysics_Callback_Body_Type type, EPhysics_Body_Event_Cb func, const void *data)
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

   if ((type < 0) || (type >= EPHYSICS_CALLBACK_BODY_LAST))
     {
        ERR("Can't set body event callback, callback type is wrong.");
        return;
     }

   cb = (EPhysics_Body_Callback *)calloc(1, sizeof(EPhysics_Body_Callback));
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
ephysics_body_event_callback_del(EPhysics_Body *body, EPhysics_Callback_Body_Type type, EPhysics_Body_Event_Cb func)
{
   EPhysics_Body_Callback *cb;
   void *cb_data = NULL;

   if (!body)
     {
        ERR("Can't delete body event callback, body is null.");
        return NULL;
     }

   EINA_INLIST_FOREACH(body->callbacks, cb)
     {
        if ((cb->type != type) || (cb->func != func))
          continue;

        cb_data = cb->data;
        _ephysics_body_event_callback_del(body, cb);
        break;

     }

   return cb_data;
}

EAPI void *
ephysics_body_event_callback_del_full(EPhysics_Body *body, EPhysics_Callback_Body_Type type, EPhysics_Body_Event_Cb func, void *data)
{
   EPhysics_Body_Callback *cb;
   void *cb_data = NULL;

   if (!body)
     {
        ERR("Can't delete body event callback, body is null.");
        return NULL;
     }

   EINA_INLIST_FOREACH(body->callbacks, cb)
     {
        if ((cb->type != type) || (cb->func != func) || (cb->data != data))
          continue;

        cb_data = cb->data;
        _ephysics_body_event_callback_del(body, cb);
        break;
     }

   return cb_data;
}

static void
_ephysics_body_restitution_set(EPhysics_Body *body, double restitution)
{
   if (body->type == EPHYSICS_BODY_TYPE_RIGID)
     {
        body->rigid_body->setRestitution(btScalar(restitution));
        return;
     }

   body->soft_body->setRestitution(btScalar(restitution));
   DBG("Body %p restitution set to %lf", body, restitution);
}

EAPI void
ephysics_body_restitution_set(EPhysics_Body *body, double restitution)
{
   if (!body)
     {
        ERR("Can't set body restitution, body is null.");
        return;
     }

   ephysics_world_lock_take(body->world);
   body->material = EPHYSICS_BODY_MATERIAL_CUSTOM;
   _ephysics_body_restitution_set(body, restitution);
   ephysics_world_lock_release(body->world);
}

EAPI double
ephysics_body_restitution_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get body restitution, body is null.");
        return 0;
     }

   if (body->type == EPHYSICS_BODY_TYPE_RIGID)
     return body->rigid_body->getRestitution();

   return body->soft_body->getRestitution();
}

static void
_ephysics_body_friction_set(EPhysics_Body *body, double friction)
{
   if (body->type == EPHYSICS_BODY_TYPE_RIGID)
     {
        body->rigid_body->setFriction(btScalar(friction));
        return;
     }

   body->soft_body->setFriction(btScalar(friction));
   DBG("Body %p friction set to %lf", body, friction);
}

EAPI void
ephysics_body_friction_set(EPhysics_Body *body, double friction)
{
   if (!body)
     {
        ERR("Can't set body friction, body is null.");
        return;
     }

   ephysics_world_lock_take(body->world);
   body->material = EPHYSICS_BODY_MATERIAL_CUSTOM;
   _ephysics_body_friction_set(body, friction);
   ephysics_world_lock_release(body->world);
}

EAPI double
ephysics_body_friction_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get body friction, body is null.");
        return 0;
     }

   if (body->type == EPHYSICS_BODY_TYPE_RIGID)
     return body->rigid_body->getFriction();

   return body->soft_body->getFriction();
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
ephysics_body_central_impulse_apply(EPhysics_Body *body, double x, double y, double z)
{
   double rate;

   if (!body)
     {
        ERR("Can't apply impulse to a null body.");
        return;
     }

   rate = ephysics_world_rate_get(body->world);

   ephysics_world_lock_take(body->world);
   ephysics_body_activate(body, EINA_TRUE);
   body->rigid_body->applyCentralImpulse(btVector3(x / rate, - y / rate,
                                                   z / rate));
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_impulse_apply(EPhysics_Body *body, double x, double y, double z, Evas_Coord pos_x, Evas_Coord pos_y, Evas_Coord pos_z)
{
   double rate;

   if (!body)
     {
        ERR("Can't apply impulse to a null body.");
        return;
     }

   rate = ephysics_world_rate_get(body->world);

   ephysics_world_lock_take(body->world);
   ephysics_body_activate(body, EINA_TRUE);
   body->rigid_body->applyImpulse(btVector3(x / rate, - y / rate, z / rate),
                                  btVector3((double) pos_x / rate,
                                            (double) pos_y / rate,
                                            (double) pos_z / rate));
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_linear_movement_enable_set(EPhysics_Body *body, Eina_Bool enable_x, Eina_Bool enable_y, Eina_Bool enable_z)
{
   if (!body)
     {
        ERR("Can't set linear factor on a null body.");
        return;
     }

   ephysics_world_lock_take(body->world);
   body->rigid_body->setLinearFactor(btVector3(!!enable_x, !!enable_y,
                                               !!enable_z));
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_linear_movement_enable_get(const EPhysics_Body *body, Eina_Bool *enable_x, Eina_Bool *enable_y, Eina_Bool *enable_z)
{
   if (!body)
     {
        ERR("Can't check if linear factor is enabled, body is null.");
        return;
     }

   if (enable_x) *enable_x = !!body->rigid_body->getLinearFactor().x();
   if (enable_y) *enable_y = !!body->rigid_body->getLinearFactor().y();
   if (enable_z) *enable_z = !!body->rigid_body->getLinearFactor().z();
}

EAPI void
ephysics_body_torque_impulse_apply(EPhysics_Body *body, double pitch, double yaw, double roll)
{
   if (!body)
     {
        ERR("Can't apply torque impulse to a null body.");
        return;
     }

   ephysics_world_lock_take(body->world);
   ephysics_body_activate(body, EINA_TRUE);
   body->rigid_body->applyTorqueImpulse(btVector3(-pitch, -yaw, -roll));
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_angular_movement_enable_set(EPhysics_Body *body, Eina_Bool enable_x, Eina_Bool enable_y, Eina_Bool enable_z)
{
   if (!body)
     {
        ERR("Can't set rotation on a null body.");
        return;
     }

   ephysics_world_lock_take(body->world);
   body->rigid_body->setAngularFactor(btVector3(!!enable_x, !!enable_y,
                                                !!enable_z));
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_angular_movement_enable_get(const EPhysics_Body *body, Eina_Bool *enable_x, Eina_Bool *enable_y, Eina_Bool *enable_z)
{
   if (!body)
     {
        ERR("Can't check if rotation is enabled, body is null.");
        return;
     }

   if (enable_x) *enable_x = !!body->rigid_body->getAngularFactor().x();
   if (enable_y) *enable_y = !!body->rigid_body->getAngularFactor().y();
   if (enable_z) *enable_z = !!body->rigid_body->getAngularFactor().z();
}

EAPI void
ephysics_body_rotation_get(const EPhysics_Body *body, double *rot_x, double *rot_y, double *rot_z)
{
   btTransform trans;

   if (!body)
     {
        ERR("Can't get rotation, body is null.");
        return;
     }

   trans = _ephysics_body_transform_get(body);

   if (rot_x) *rot_x = - trans.getRotation().getAngle() * RAD_TO_DEG *
     trans.getRotation().getAxis().getX();
   if (rot_y) *rot_y = - trans.getRotation().getAngle() * RAD_TO_DEG *
     trans.getRotation().getAxis().getY();
   if (rot_z) *rot_z = - trans.getRotation().getAngle() * RAD_TO_DEG *
     trans.getRotation().getAxis().getZ();
}

EAPI void
ephysics_body_rotation_set(EPhysics_Body *body, double rot_x, double rot_y, double rot_z)
{
   btTransform trans;
   btQuaternion quat;

   if (!body)
     {
        ERR("Can't set rotation, body is null.");
        return;
     }

   ephysics_world_lock_take(body->world);
   ephysics_body_activate(body, EINA_TRUE);
   trans = _ephysics_body_transform_get(body);

   quat.setEuler(-rot_x / RAD_TO_DEG, -rot_y / RAD_TO_DEG, -rot_z / RAD_TO_DEG);
   trans.setRotation(quat);

   if (body->soft_body)
     body->soft_body->transform(trans);

   body->rigid_body->proceedToTransform(trans);
   body->rigid_body->getMotionState()->setWorldTransform(trans);

   DBG("Body %p rotation set to (%lf, %lf, %lf)", body, rot_x, rot_y, rot_z);
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_rotation_quaternion_get(const EPhysics_Body *body, double *x, double *y, double *z, double *w)
{
   btTransform trans;
   btQuaternion quat;

   if (!body)
     {
        ERR("Can't get rotation, body is null.");
        return;
     }

   trans = _ephysics_body_transform_get(body);
   quat = trans.getRotation();
   quat.normalize();

   if (x) *x = quat.x();
   if (y) *y = quat.y();
   if (z) *z = quat.z();
   if (w) *w = -quat.w();
}

EAPI void
ephysics_body_data_set(EPhysics_Body *body, void *data)
{
   if (!body)
     {
        ERR("Can't set data, body is null.");
        return;
     }

   body->data = data;
}

EAPI void *
ephysics_body_data_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get data, body is null.");
        return NULL;
     }

   return body->data;
}

EAPI void
ephysics_body_central_force_apply(EPhysics_Body *body, double x, double y, double z)
{
   double rate;

   if (!body)
     {
        ERR("Can't apply force to a null body.");
        return;
     }

   ephysics_world_lock_take(body->world);
   rate = ephysics_world_rate_get(body->world);
   ephysics_body_forces_apply(body);
   body->rigid_body->applyCentralForce(btVector3(x / rate, - y / rate,
                                                 z / rate));
   _ephysics_body_forces_update(body);
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_force_apply(EPhysics_Body *body, double x, double y, double z, Evas_Coord pos_x, Evas_Coord pos_y, Evas_Coord pos_z)
{
   double rate;

   if (!body)
     {
        ERR("Can't apply force to a null body.");
        return;
     }

   rate = ephysics_world_rate_get(body->world);
   ephysics_world_lock_take(body->world);
   ephysics_body_forces_apply(body);
   body->rigid_body->applyForce(btVector3(x / rate, - y / rate, z / rate),
                                btVector3((double) pos_x / rate,
                                          (double) pos_y / rate,
                                          (double) pos_z / rate));
   _ephysics_body_forces_update(body);
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_torque_apply(EPhysics_Body *body, double torque_x, double torque_y, double torque_z)
{
   if (!body)
     {
        ERR("Can't apply force to a null body.");
        return;
     }

   ephysics_world_lock_take(body->world);
   ephysics_body_forces_apply(body);
   body->rigid_body->applyTorque(btVector3(-torque_x, -torque_y, -torque_z));
   _ephysics_body_forces_update(body);
   ephysics_world_lock_release(body->world);
}

EAPI void
ephysics_body_forces_get(const EPhysics_Body *body, double *x, double *y, double *z)
{
   double rate, gx, gy, gz;

   if (!body)
     {
        ERR("Can't get forces from a null body.");
        return;
     }

   rate = ephysics_world_rate_get(body->world);
   ephysics_world_gravity_get(body->world, &gx, &gy, &gz);

   if (x) *x = body->force.x * rate + gx;
   if (y) *y = -body->force.y * rate + gy;
   if (z) *z = body->force.z * rate + gz;
}

EAPI void
ephysics_body_torques_get(const EPhysics_Body *body, double *x, double *y, double *z)
{
   if (!body)
     {
        ERR("Can't get torques from a null body.");
        return;
     }

   if (x) *x = -body->force.torque_x;
   if (y) *y = -body->force.torque_y;
   if (z) *z = -body->force.torque_z;
}

EAPI void
ephysics_body_forces_clear(EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't clear forces of a null body.");
        return;
     }

   body->force.x = 0;
   body->force.y = 0;
   body->force.z = 0;
   body->force.torque_x = 0;
   body->force.torque_y = 0;
   body->force.torque_z = 0;
}

EAPI void
ephysics_body_center_mass_get(const EPhysics_Body *body, double *x, double *y, double *z)
{
   if (!body)
     {
        ERR("Can't get center of mass from a null body.");
        return;
     }

   if (x) *x = body->cm.x;
   if (y) *y = body->cm.y;
   if (z) *z = body->cm.z;
}

EAPI void
ephysics_body_density_set(EPhysics_Body *body, double density)
{
   if (!body)
     {
        ERR("Can't set the density of a null body's material.");
        return;
     }

   body->material = EPHYSICS_BODY_MATERIAL_CUSTOM;
   body->density = density;
   ephysics_world_lock_take(body->world);
   _ephysics_body_mass_set(body, 0);
   ephysics_world_lock_release(body->world);
}

EAPI double
ephysics_body_density_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get the density of a null body's material.");
        return 0;
     }

   return body->density;
}

EAPI void
ephysics_body_material_set(EPhysics_Body *body, EPhysics_Body_Material material)
{
   if (!body)
     {
        ERR("Can't set body's material.");
        return;
     }

   if (material >= EPHYSICS_BODY_MATERIAL_LAST)
     {
        ERR("Invalid material.");
        return;
     }

   if (material != ephysics_material_props[material].material)
     {
        ERR("Material properties weren't found.");
        return;
     }

   ephysics_world_lock_take(body->world);
   body->density = ephysics_material_props[material].density;
   _ephysics_body_mass_set(body, 0);
   _ephysics_body_friction_set(body,
                               ephysics_material_props[material].friction);
   _ephysics_body_restitution_set(
      body, ephysics_material_props[material].restitution);
   body->material = material;
   ephysics_world_lock_release(body->world);
}

EAPI EPhysics_Body_Material
ephysics_body_material_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("Can't get body's material.");
        return EPHYSICS_BODY_MATERIAL_LAST;
     }

   return body->material;
}

EAPI void
ephysics_body_light_set(EPhysics_Body *body, Eina_Bool enable)
{
   if (!body)
     {
        ERR("No body, no light.");
        return;
     }

   body->light_apply = !!enable;
}

EAPI Eina_Bool
ephysics_body_light_get(const EPhysics_Body *body)
{
   if (!body)
     {
        ERR("No body, no light.");
        return EINA_FALSE;
     }

   return body->light_apply;
}


#ifdef  __cplusplus
}
#endif
