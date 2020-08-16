#include "g_local.h"

// from g_weapon.c
void check_dodge (edict_t *self, vec3_t start, vec3_t dir, int speed, int radius);

// RAFAEL
/*
=================
	fire_ionripper
=================
*/

void ionripper_sparks (edict_t *self)
{
    gi.WriteByte (svc_temp_entity);
    gi.WriteByte (TE_WELDING_SPARKS);
    gi.WriteByte (0);
    gi.WritePosition (self->s.origin);
    gi.WriteDir (vec3_origin);
    gi.WriteByte (0xe4 + (randomMT()&3));
    gi.multicast (self->s.origin, MULTICAST_PVS);

    G_FreeEdict (self);
}

// RAFAEL
void ionripper_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    if (other == self->owner)
        return;

    if (surf && (surf->flags & SURF_SKY))
    {
        G_FreeEdict (self);
        return;
    }

    if (self->owner->client)
        PlayerNoise (self->owner, self->s.origin, PNOISE_IMPACT);

    if (other->takedamage)
    {
        T_Damage (other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 1, DAMAGE_ENERGY, MOD_RIPPER);

    }
    else
    {
        return;
    }

    G_FreeEdict (self);
}


// RAFAEL
void fire_ionripper (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
    edict_t *ion;
    trace_t tr;

    VectorNormalize (dir);

    ion = G_Spawn ();
    VectorCopy (start, ion->s.origin);
    VectorCopy (start, ion->s.old_origin);
    vectoangles (dir, ion->s.angles);
    VectorScale (dir, speed, ion->velocity);

    ion->movetype = MOVETYPE_WALLBOUNCE;
    ion->clipmask = MASK_SHOT;
    ion->solid = SOLID_BBOX;
    ion->s.effects |= effect;

    ion->s.renderfx |= RF_FULLBRIGHT;

    VectorClear (ion->mins);
    VectorClear (ion->maxs);
    ion->s.modelindex = gi.modelindex ("models/objects/boomrang/tris.md2");
    ion->s.sound = gi.soundindex ("misc/lasfly.wav");
    ion->owner = self;
    ion->touch = ionripper_touch;
    ion->nextthink = level.time + 3;
    ion->think = ionripper_sparks;
    ion->dmg = damage;
    ion->dmg_radius = 100;
    gi.linkentity (ion);

    if (self->client)
        check_dodge (self, ion->s.origin, dir, speed, 100);

    tr = gi.trace (self->s.origin, NULL, NULL, ion->s.origin, ion, MASK_SHOT);
    if (tr.fraction < 1.0)
    {
        VectorMA (ion->s.origin, -10, dir, ion->s.origin);
        ion->touch (ion, tr.ent, NULL, NULL);
    }

}


// RAFAEL
/*
=================
fire_heat
=================
*/
/*
void heat_think (edict_t *self)
{
	edict_t		*target = NULL;
	edict_t		*aquire = NULL;
	vec3_t		vec;
	vec3_t		oldang;
	int			len;
	int			oldlen = 0;

	VectorClear (vec);

	// aquire new target
	while (( target = findradius (target, self->s.origin, 1024)) != NULL)
	{

		if (self->owner == target)
			continue;
		if (!target->svflags & SVF_MONSTER)
			continue;
		if (!target->client)
			continue;
		if (target->health <= 0)
			continue;
		if (!visible (self, target))
			continue;

		// if we need to reduce the tracking cone
		/*
		{
			vec3_t	vec;
			float	dot;
			vec3_t	forward;

			AngleVectors (self->s.angles, forward, NULL, NULL);
			VectorSubtract (target->s.origin, self->s.origin, vec);
			VectorNormalize (vec);
			dot = DotProduct (vec, forward);

			if (dot > 0.6)
				continue;
		}
		*/

/*		if (!infront (self, target))
			continue;

		VectorSubtract (self->s.origin, target->s.origin, vec);
		len = VectorLength (vec);

		if (aquire == NULL || len < oldlen)
		{
			aquire = target;
			self->target_ent = aquire;
			oldlen = len;
		}
	}

	if (aquire != NULL)
	{
		VectorCopy (self->s.angles, oldang);
		VectorSubtract (aquire->s.origin, self->s.origin, vec);

		vectoangles (vec, self->s.angles);

		VectorNormalize (vec);
		VectorCopy (vec, self->movedir);
		VectorScale (vec, 500, self->velocity);
	}

	self->nextthink = level.time + 0.1;
}
*/
// RAFAEL
/*
void fire_heat (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage)
{
	edict_t *heat;

	heat = G_Spawn();
	VectorCopy (start, heat->s.origin);
	VectorCopy (dir, heat->movedir);
	vectoangles (dir, heat->s.angles);
	VectorScale (dir, speed, heat->velocity);
	heat->movetype = MOVETYPE_FLYMISSILE;
	heat->clipmask = MASK_SHOT;
	heat->solid = SOLID_BBOX;
	heat->s.effects |= EF_ROCKET;
	VectorClear (heat->mins);
	VectorClear (heat->maxs);
	heat->s.modelindex = gi.modelindex ("models/objects/rocket/tris.md2");
	heat->owner = self;
	heat->touch = rocket_touch;

	heat->nextthink = level.time + 0.1;
	heat->think = heat_think;

	heat->dmg = damage;
	heat->radius_dmg = radius_damage;
	heat->dmg_radius = damage_radius;
	heat->s.sound = gi.soundindex ("weapons/rockfly.wav");

	if (self->client)
		check_dodge (self, heat->s.origin, dir, speed);

	gi.linkentity (heat);
}
*/


// RAFAEL
/*
=================
	fire_plasma
=================
*/

void plasma_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    vec3_t		origin;

    if (other == ent->owner)
        return;

    if (surf && (surf->flags & SURF_SKY))
    {
        G_FreeEdict (ent);
        return;
    }

    if (ent->owner->client)
        PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

    // calculate position for the explosion entity
    VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

    if (other->takedamage)
    {
        T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, 0, MOD_PHALANX);
    }

    T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, MOD_PHALANX);

    gi.WriteByte (svc_temp_entity);
    gi.WriteByte (TE_PLASMA_EXPLOSION);
    gi.WritePosition (origin);
    gi.multicast (ent->s.origin, MULTICAST_PVS);

    G_FreeEdict (ent);
}


// RAFAEL
void fire_plasma (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage)
{
    edict_t *plasma;

    // calling entity made a sound, used to alert monsters
    self->lastsound = level.framenum;

    plasma = G_Spawn();
    VectorCopy (start, plasma->s.origin);
    VectorCopy (dir, plasma->movedir);
    vectoangles (dir, plasma->s.angles);
    VectorScale (dir, speed, plasma->velocity);
    plasma->movetype = MOVETYPE_FLYMISSILE;
    plasma->clipmask = MASK_SHOT;
    plasma->solid = SOLID_BBOX;

    VectorClear (plasma->mins);
    VectorClear (plasma->maxs);

    plasma->owner = self;
    plasma->touch = plasma_touch;
    plasma->nextthink = level.time + 8000/speed;
    plasma->think = G_FreeEdict;
    plasma->dmg = damage;
    plasma->radius_dmg = radius_damage;
    plasma->dmg_radius = damage_radius;
    plasma->s.sound = gi.soundindex ("weapons/rockfly.wav");

    plasma->s.modelindex = gi.modelindex ("sprites/s_photon.sp2");
    plasma->s.effects |= EF_PLASMA | EF_ANIM_ALLFAST;

    if (self->client)
        check_dodge (self, plasma->s.origin, dir, speed, damage_radius);

    gi.linkentity (plasma);


}


/*
=================
trap
=================
*/

// RAFAEL
extern void SP_item_foodcube (edict_t *best);
// RAFAEL
static void Trap_Think (edict_t *ent)
{
    edict_t	*target = NULL;
    edict_t	*best = NULL;
    vec3_t	vec;
    int		len, i;
    int		oldlen = 8000;
    vec3_t	forward, right, up;

    if (ent->timestamp < level.time)
    {
        BecomeExplosion1(ent);
        // note to self
        // cause explosion damage???
        return;
    }

    ent->nextthink = level.time + 0.1;

    if (!ent->groundentity)
        return;

    // ok lets do the blood effect
    if (ent->s.frame > 4)
    {
        if (ent->s.frame == 5)
        {
            if (ent->wait == 64)
                gi.sound(ent, CHAN_VOICE, gi.soundindex ("weapons/trapdown.wav"), 1, ATTN_IDLE, 0);

            ent->wait -= 2;
            ent->delay += level.time;

            for (i=0; i<3; i++)
            {

                best = G_Spawn();

                if (strcmp (ent->enemy->classname, "monster_gekk") == 0)
                {
                    best->s.modelindex = gi.modelindex ("models/objects/gekkgib/torso/tris.md2");
                    best->s.effects |= TE_GREENBLOOD;
                }
                else if (ent->mass > 200)
                {
                    best->s.modelindex = gi.modelindex ("models/objects/gibs/chest/tris.md2");
                    best->s.effects |= TE_BLOOD;
                }
                else
                {
                    best->s.modelindex = gi.modelindex ("models/objects/gibs/sm_meat/tris.md2");
                    best->s.effects |= TE_BLOOD;
                }

                AngleVectors (ent->s.angles, forward, right, up);

                RotatePointAroundVector( vec, up, right, ((360.0/3)* i)+ent->delay);
                VectorMA (vec, ent->wait/2, vec, vec);
                VectorAdd(vec, ent->s.origin, vec);
                VectorAdd(vec, forward, best->s.origin);

                best->s.origin[2] = ent->s.origin[2] + ent->wait;

                VectorCopy (ent->s.angles, best->s.angles);

                best->solid = SOLID_NOT;
                best->s.effects |= EF_GIB;
                best->takedamage = DAMAGE_YES;

                best->movetype = MOVETYPE_TOSS;
                best->svflags |= SVF_MONSTER;
                best->deadflag = DEAD_DEAD;

                VectorClear (best->mins);
                VectorClear (best->maxs);

                best->watertype = gi.pointcontents(best->s.origin);
                if (best->watertype & MASK_WATER)
                    best->waterlevel = 1;

                best->nextthink = level.time + 0.1;
                best->think = G_FreeEdict;
                gi.linkentity (best);
            }

            if (ent->wait < 19)
                ent->s.frame ++;

            return;
        }
        ent->s.frame ++;
        if (ent->s.frame == 8)
        {
            ent->nextthink = level.time + 1.0;
            ent->think = G_FreeEdict;

            best = G_Spawn ();
            SP_item_foodcube (best);
            VectorCopy (ent->s.origin, best->s.origin);
            best->s.origin[2]+= 16;
            best->velocity[2] = 400;
            best->count = ent->mass;
            gi.linkentity (best);
            return;
        }
        return;
    }

    ent->s.effects &= ~EF_TRAP;
    if (ent->s.frame >= 4)
    {
        ent->s.effects |= EF_TRAP;
        VectorClear (ent->mins);
        VectorClear (ent->maxs);

    }

    if (ent->s.frame < 4)
        ent->s.frame++;

    while ((target = findradius(target, ent->s.origin, 256)) != NULL)
    {
        if (target == ent)
            continue;
        if (!(target->svflags & SVF_MONSTER) && !target->client)
            continue;
        // if (target == ent->owner)
        //	continue;
        if (target->health <= 0)
            continue;
        if (!visible (ent, target))
            continue;
        if (!best)
        {
            best = target;
            continue;
        }
        VectorSubtract (ent->s.origin, target->s.origin, vec);
        len = VectorLength (vec);
        if (len < oldlen)
        {
            oldlen = len;
            best = target;
        }
    }

    // pull the enemy in
    if (best)
    {
        vec3_t	forward;

        if (best->groundentity)
        {
            best->s.origin[2] += 1;
            best->groundentity = NULL;
        }
        VectorSubtract (ent->s.origin, best->s.origin, vec);
        len = VectorLength (vec);
        if (best->client)
        {
            VectorNormalize (vec);
            VectorMA (best->velocity, 250, vec, best->velocity);
        }
        else
        {
            best->ideal_yaw = vectoyaw(vec);
//			M_ChangeYaw (best);
            AngleVectors (best->s.angles, forward, NULL, NULL);
            VectorScale (forward, 256, best->velocity);
        }

        gi.sound(ent, CHAN_VOICE, gi.soundindex ("weapons/trapsuck.wav"), 1, ATTN_IDLE, 0);

        if (len < 32)
        {
            if (best->mass < 400)
            {
                T_Damage (best, ent, ent->owner, vec3_origin, best->s.origin, vec3_origin, 100000, 1, 0, MOD_TRAP);
                ent->enemy = best;
                ent->wait = 64;
                VectorCopy (ent->s.origin, ent->s.old_origin);
                ent->timestamp = level.time + 30;
                if (deathmatch->value)
                    ent->mass = best->mass/4;
                else
                    ent->mass = best->mass/10;
                // ok spawn the food cube
                ent->s.frame = 5;
            }
            else
            {
                BecomeExplosion1(ent);
                // note to self
                // cause explosion damage???
                return;
            }

        }
    }
}
