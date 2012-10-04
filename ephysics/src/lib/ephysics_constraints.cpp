#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ephysics_private.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum _EPhysics_Constraint_Type {
   EPHYSICS_CONSTRAINT_P2P,
   EPHYSICS_CONSTRAINT_SLIDER,
} EPhysics_Constraint_Type;

struct _EPhysics_Constraint {
     btTypedConstraint *bt_constraint;
     EPhysics_World *world;
     EPhysics_Constraint_Type type;
     struct {
          EPhysics_Body *body1;
          EPhysics_Body *body2;
          Evas_Coord anchor_b1_x;
          Evas_Coord anchor_b1_y;
          Evas_Coord anchor_b2_x;
          Evas_Coord anchor_b2_y;
     } p2p;
};

/* FIXME: it shouldn't be this way.
 * All constraints should be generic and have all these options
 * set after add().
 */
static Eina_Bool
_ephysics_constraint_p2p_set(EPhysics_Constraint *constraint, double rate)
{
   if (!constraint->p2p.body2)
     constraint->bt_constraint = new btPoint2PointConstraint(
        *ephysics_body_rigid_body_get(constraint->p2p.body1),
        btVector3(constraint->p2p.anchor_b1_x / rate,
                  constraint->p2p.anchor_b1_y / rate, 0));
   else
     constraint->bt_constraint = new btPoint2PointConstraint(
        *ephysics_body_rigid_body_get(constraint->p2p.body1),
        *ephysics_body_rigid_body_get(constraint->p2p.body2),
        btVector3(constraint->p2p.anchor_b1_x / rate,
                  constraint->p2p.anchor_b1_y / rate, 0),
        btVector3(constraint->p2p.anchor_b2_x / rate,
                  constraint->p2p.anchor_b2_y / rate, 0));

   if (!constraint->bt_constraint)
     {
        ERR("Failed to create a btConstraint");
        free(constraint);
        return EINA_FALSE;
     }

   constraint->type = EPHYSICS_CONSTRAINT_P2P;
   ephysics_world_constraint_add(constraint->world, constraint,
                                 constraint->bt_constraint);

   return EINA_TRUE;
}

static void
_ephysics_constraint_p2p_recalc(EPhysics_Constraint *constraint, double rate)
{
   ephysics_world_constraint_del(constraint->world, constraint,
                                 constraint->bt_constraint);
   delete constraint->bt_constraint;

   _ephysics_constraint_p2p_set(constraint, rate);
}

static void
_ephysics_constraint_slider_linear_limit_set(EPhysics_Constraint *constraint, Evas_Coord left_x, Evas_Coord under_y, Evas_Coord right_x, Evas_Coord above_y, double rate)
{
   btGeneric6DofConstraint *slider_constraint;

   slider_constraint = (btGeneric6DofConstraint *)constraint->bt_constraint;
   rate = ephysics_world_rate_get(constraint->world);

   left_x = (left_x) / rate;
   right_x = (right_x) / rate;

   under_y = (under_y) / rate;
   above_y = (above_y) / rate;

   slider_constraint->setLinearLowerLimit(btVector3(-left_x, -under_y, 0));
   slider_constraint->setLinearUpperLimit(btVector3(right_x, above_y, 0));
}

void
ephysics_constraint_recalc(EPhysics_Constraint *constraint, double rate)
{
   Evas_Coord left_x, under_y, right_x, above_y;

   if (constraint->type == EPHYSICS_CONSTRAINT_P2P)
     {
        _ephysics_constraint_p2p_recalc(constraint, rate);
        return;
     }

   ephysics_constraint_slider_linear_limit_get(constraint, &left_x, &under_y,
                                               &right_x, &above_y);
   _ephysics_constraint_slider_linear_limit_set(constraint, left_x, under_y,
                                                right_x, above_y, rate);
}

EAPI EPhysics_Constraint *
ephysics_constraint_slider_add(EPhysics_Body *body)
{
   EPhysics_Constraint *constraint;
   btTransform trans;

   if (!body)
     {
        ERR("To create a constraint body must to be non null.");
        return NULL;
     }

   if (body->type == EPHYSICS_BODY_TYPE_CLOTH)
     {
        ERR("Constraints are allowed only between rigid -> rigid bodies or "
            "rigid -> soft bodies");
        return NULL;
     }

   constraint = (EPhysics_Constraint *) calloc(1, sizeof(EPhysics_Constraint));
   if (!constraint)
     {
        ERR("Couldn't create a new constraint.");
        return NULL;
     }

   ephysics_world_lock_take(ephysics_body_world_get(body));
   trans.setIdentity();
   constraint->bt_constraint = new
       btGeneric6DofConstraint(*ephysics_body_rigid_body_get(body), trans,
                               false);

   if (!constraint->bt_constraint)
     {
        ERR("Failed to create a btConstraint");
        free(constraint);
        return NULL;
     }

   constraint->type = EPHYSICS_CONSTRAINT_SLIDER;
   constraint->world = ephysics_body_world_get(body);
   ephysics_world_constraint_add(constraint->world, constraint,
                                 constraint->bt_constraint);

   INF("Constraint added.");
   ephysics_world_lock_release(ephysics_body_world_get(body));
   return constraint;
}

EAPI void
ephysics_constraint_slider_linear_limit_set(EPhysics_Constraint *constraint, Evas_Coord left_x, Evas_Coord under_y, Evas_Coord right_x, Evas_Coord above_y)
{
   if (!constraint)
     {
        ERR("Can't set constraint's linear limit, constraint is null.");
        return;
     }

   if (constraint->type != EPHYSICS_CONSTRAINT_SLIDER)
     {
        ERR("Can't set linear limit, this is not an slider constraint.");
        return;
     }

   ephysics_world_lock_take(constraint->world);
   _ephysics_constraint_slider_linear_limit_set(
      constraint, left_x, under_y, right_x, above_y,
      ephysics_world_rate_get(constraint->world));
   ephysics_world_lock_release(constraint->world);
}

EAPI void
ephysics_constraint_slider_linear_limit_get(const EPhysics_Constraint *constraint, Evas_Coord *left_x, Evas_Coord *under_y, Evas_Coord *right_x, Evas_Coord *above_y)
{
   btGeneric6DofConstraint *slider_constraint;
   btVector3 linear_limit;
   int rate;

   if (!constraint)
     {
        ERR("Can't get constraint's linear limit, constraint is null.");
        return;
     }

   if (constraint->type != EPHYSICS_CONSTRAINT_SLIDER)
     {
        ERR("Can't get linear limit, this is not an slider constraint.");
        return;
     }

   rate = ephysics_world_rate_get(constraint->world);
   slider_constraint = (btGeneric6DofConstraint *)constraint->bt_constraint;
   if (left_x || under_y)
     slider_constraint->getLinearLowerLimit(linear_limit);

   if (left_x) *left_x = linear_limit.getX() * rate;
   if (under_y) *under_y = linear_limit.getY() * rate;

   if (right_x || above_y)
     slider_constraint->getLinearUpperLimit(linear_limit);

   if (right_x) *right_x = linear_limit.getX() * rate;
   if (above_y) *above_y = linear_limit.getY() * rate;
}

EAPI void
ephysics_constraint_slider_angular_limit_set(EPhysics_Constraint *constraint, double counter_clock_z, double clock_wise_z)
{
   btGeneric6DofConstraint *slider_constraint;

   if (!constraint)
     {
        ERR("Can't set constraint's angular limit, constraint is null.");
        return;
     }

   if (constraint->type != EPHYSICS_CONSTRAINT_SLIDER)
     {
        ERR("Can't set angular limit, this is not an slider constraint.");
        return;
     }

   ephysics_world_lock_take(constraint->world);
   slider_constraint = (btGeneric6DofConstraint *)constraint->bt_constraint;
   slider_constraint->setAngularLowerLimit(btVector3(0, 0,
                                                 -counter_clock_z/RAD_TO_DEG));
   slider_constraint->setAngularUpperLimit(btVector3(0, 0,
                                                 clock_wise_z/RAD_TO_DEG));
   ephysics_world_lock_release(constraint->world);
}

EAPI void
ephysics_constraint_slider_angular_limit_get(const EPhysics_Constraint *constraint, double *counter_clock_z, double *clock_wise_z)
{
   btGeneric6DofConstraint *slider_constraint;
   btVector3 angular_limit;

   if (!constraint)
     {
        ERR("Can't get constraint's angular limit, constraint is null.");
        return;
     }

   if (constraint->type != EPHYSICS_CONSTRAINT_SLIDER)
     {
        ERR("Can't get angular limit, this is not an slider constraint.");
        return;
     }

   slider_constraint = (btGeneric6DofConstraint *)constraint->bt_constraint;
   if (counter_clock_z)
     {
        slider_constraint->getAngularLowerLimit(angular_limit);
        *counter_clock_z = angular_limit.getZ();
     }

   if (clock_wise_z)
     {
        slider_constraint->getAngularUpperLimit(angular_limit);
        *clock_wise_z = angular_limit.getZ();
     }
}

EAPI EPhysics_Constraint *
ephysics_constraint_p2p_add(EPhysics_Body *body1, EPhysics_Body *body2, Evas_Coord anchor_b1_x, Evas_Coord anchor_b1_y, Evas_Coord anchor_b2_x, Evas_Coord anchor_b2_y)
{
   EPhysics_Constraint *constraint;

   if (!body1)
     {
        ERR("To create a constraint body1 must to be non null.");
        return NULL;
     }

   if ((body2) &&
       (ephysics_body_world_get(body1) != ephysics_body_world_get(body2)))
     {
        ERR("To create a constraint both bodies must belong to the same"
            "world.");
        return NULL;
     }

   if (body1->type == EPHYSICS_BODY_TYPE_CLOTH ||
       body2->type == EPHYSICS_BODY_TYPE_CLOTH)
     {
        ERR("Constraints are allowed only between rigid -> rigid bodies or"
            "rigid -> soft bodies");
        return NULL;
     }

   constraint = (EPhysics_Constraint *) calloc(1, sizeof(EPhysics_Constraint));
   if (!constraint)
     {
        ERR("Couldn't create a new constraint.");
        return NULL;
     }

   constraint->world = ephysics_body_world_get(body1);
   constraint->p2p.body1 = body1;
   constraint->p2p.body2 = body2;
   constraint->p2p.anchor_b1_x = anchor_b1_x;
   constraint->p2p.anchor_b1_y = anchor_b1_y;
   constraint->p2p.anchor_b2_x = anchor_b2_x;
   constraint->p2p.anchor_b2_y = anchor_b2_y;

   ephysics_world_lock_take(constraint->world);
   if (!_ephysics_constraint_p2p_set(
         constraint, ephysics_world_rate_get(ephysics_body_world_get(
               constraint->p2p.body1))))
     {
        ephysics_world_lock_release(constraint->world);
        free(constraint);
        return NULL;
     }

   ephysics_world_lock_release(constraint->world);
   INF("Constraint added.");
   return constraint;
}

EAPI void
ephysics_constraint_del(EPhysics_Constraint *constraint)
{
   if (!constraint)
     {
        ERR("Can't delete constraint, it wasn't provided.");
        return;
     }

   ephysics_world_lock_take(constraint->world);
   ephysics_world_constraint_del(constraint->world, constraint,
                                 constraint->bt_constraint);
   delete constraint->bt_constraint;
   free(constraint);

   INF("Constraint deleted.");
   ephysics_world_lock_release(constraint->world);
}


#ifdef  __cplusplus
}
#endif
