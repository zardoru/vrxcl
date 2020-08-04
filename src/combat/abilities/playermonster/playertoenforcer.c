#include "g_local.h"

#define ENFORCER_FRAMES_IDLE_START		50
#define ENFORCER_FRAMES_IDLE_END		71
#define ENFORCER_FRAMES_RUN_START		92
#define ENFORCER_FRAMES_RUN_END		    99
#define ENFORCER_FRAMES_JUMP_START		120
#define ENFORCER_FRAMES_JUMP_END		122
#define ENFORCER_FRAMES_MG_START		194
#define ENFORCER_FRAMES_MG_END			194
#define ENFORCER_FRAMES_SMACK_START     203
#define ENFORCER_FRAMES_SMACK_END       207

void p_enforcer_mg_regen (edict_t *ent, int regen_frames, int regen_delay)
{
	int ammo;
	int max = ent->myskills.abilities[ENFORCER].max_ammo;
	int *current = &ent->myskills.abilities[ENFORCER].ammo;
	int *delay = &ent->myskills.abilities[ENFORCER].ammo_regenframe;

	if (*current > max)
		return;

	// don't regenerate ammo if player is firing
	if ((ent->client->buttons & BUTTON_ATTACK) && (ent->client->weapon_mode != 1))
		return;

	if (regen_delay > 0)
	{
		if (level.framenum < *delay)
			return;

		*delay = level.framenum + regen_delay;
	}
	else
		regen_delay = 1;

	ammo = floattoint((float)max / ((float)regen_frames / regen_delay));

	if (ammo < 1)
		ammo = 1;

	*current += ammo;
	if (*current > max)
		*current = max;
}

void p_enforcer_regen (edict_t *ent)
{
	int weapon_r_frames = ENFORCER_MG_REGEN_FRAMES;

	// morph mastery improves ammo/health regeneration
	if (ent->myskills.abilities[MORPH_MASTERY].current_level > 0)
	{
		weapon_r_frames *= 0.5;
	}

	p_enforcer_mg_regen(ent, qf2sf(weapon_r_frames), qf2sf(ENFORCER_MG_REGEN_DELAY));
}

void p_enforcer_jump (edict_t *ent)
{
	// run jump animation forward until last frame, then hold it
	if (ent->s.frame != ENFORCER_FRAMES_JUMP_END)
		G_RunFrames(ent, ENFORCER_FRAMES_JUMP_START, ENFORCER_FRAMES_JUMP_END, false);
}

void p_enforcer_firemg (edict_t *ent)
{
	int		damage = ENFORCER_MG_INITIAL_DMG+ENFORCER_MG_ADDON_DMG*ent->myskills.abilities[ENFORCER].current_level;
	vec3_t	forward, right, start;

	if (ent->myskills.abilities[ENFORCER].ammo < 2)
		return;
	ent->myskills.abilities[ENFORCER].ammo -= 2;

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	G_ProjectSource(ent->s.origin, monster_flash_offset[MZ2_INFANTRY_MACHINEGUN_1], forward, right, start);
    fire_bullet(ent, start, forward, damage, 5, DEFAULT_BULLET_VSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
    fire_bullet(ent, start, forward, damage, 5, DEFAULT_BULLET_VSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ2_INFANTRY_MACHINEGUN_1);
	gi.multicast (start, MULTICAST_PVS);
}

void p_enforcer_smack( edict_t *ent ) {
    int damage;
	trace_t tr;
	edict_t *other=NULL;
	vec3_t	v;
    vec3_t	forward, right, start, offset, end;

	// must be on the ground to smack
	if (!ent->groundentity)
		return;

    if (level.time < ent->monsterinfo.attack_finished)
        return;

    ent->monsterinfo.attack_finished = level.time + 0.5;

	AngleVectors (ent->client->v_angle, forward, right, NULL); 
	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	ent->lastsound = level.framenum;
	damage = ENFORCER_SMACK_INITIAL_DMG + ENFORCER_SMACK_ADDON_DMG * ent->monsterinfo.level;
	gi.sound (ent, CHAN_AUTO, gi.soundindex ("infantry/infatck2.wav"), 1, ATTN_NORM, 0);
	
    // figure out if we hit anything
    VectorMA(start, ENFORCER_SMACK_RANGE, forward, end);
    tr = gi.trace (start, NULL, NULL, end, ent, MASK_SHOT);
    if (!((tr.surface) && (tr.surface->flags & SURF_SKY))) {
        if (tr.fraction < 1.0) {            
            if (tr.ent->takedamage) {
                // we hit something, so hurt it
				T_Damage(tr.ent, ent, ent, forward, tr.endpos, tr.plane.normal, damage, damage, 0, MOD_TANK_PUNCH);
            }        
        }
    }
}

void p_enforcer_attack (edict_t *ent) {
	ent->client->idle_frames = 0;

    if (ent->client->weapon_mode == 1) {
		// smack mode
		if (ent->s.frame != ENFORCER_FRAMES_SMACK_END)
			G_RunFrames(ent, ENFORCER_FRAMES_SMACK_START, ENFORCER_FRAMES_SMACK_END, false);
		else
			ent->s.frame = ENFORCER_FRAMES_SMACK_START; // loop from this frame forward
		p_enforcer_smack(ent);
	} else {
		// chaingun mode
		if (ent->s.frame != ENFORCER_FRAMES_MG_END)
			G_RunFrames(ent, ENFORCER_FRAMES_MG_START, ENFORCER_FRAMES_MG_END, false);
		else
			ent->s.frame = ENFORCER_FRAMES_MG_START; // loop from this frame forward
		p_enforcer_firemg(ent);
	}
}

void RunEnforcerFrames (edict_t *ent, usercmd_t *ucmd) {
	if ((ent->mtype != MORPH_ENFORCER) || (ent->deadflag == DEAD_DEAD))
		return;

	ent->s.modelindex2 = 0; // no weapon model

	if (!ent->myskills.administrator)
		ent->s.skinnum = 0;
	else
		ent->s.skinnum = 2;

	if (level.framenum >= ent->count)
	{
		p_enforcer_regen(ent);
		
        if ( ent->monsterinfo.attack_finished )

		// play jump animation if we are off ground and not submerged in water
        if (!ent->groundentity && (ent->waterlevel < 2))
			p_enforcer_jump(ent);
		// play running animation if we are moving forward or strafing
		else if ((ucmd->forwardmove > 0) || ucmd->sidemove)
			G_RunFrames(ent, ENFORCER_FRAMES_RUN_START, ENFORCER_FRAMES_RUN_END, false);
		// play animation in reverse if we are going backwards
		else if (ucmd->forwardmove < 0)
			G_RunFrames(ent, ENFORCER_FRAMES_RUN_START, ENFORCER_FRAMES_RUN_END, true);
		// attack
		else if ((ent->client->buttons & BUTTON_ATTACK))
			p_enforcer_attack(ent);

		else
			G_RunFrames(ent, ENFORCER_FRAMES_IDLE_START, ENFORCER_FRAMES_IDLE_END, false); // run idle frames

		ent->count = level.framenum + qf2sf(1);
	}
}

void Cmd_PlayerToEnforcer_f (edict_t *ent)
{
	vec3_t	boxmin, boxmax;
	trace_t	tr;
	int cost = ENFORCER_INIT_COST;
	//Talent: More Ammo
    int talentLevel = vrx_get_talent_level(ent, TALENT_MORE_AMMO);

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_PlayerToEnforcer_f()\n", ent->client->pers.netname);

	// try to switch back
	if (ent->mtype || PM_PlayerHasMonster(ent)) {
		// don't let a player-tank unmorph if they are cocooned
		if (ent->owner && ent->owner->inuse && ent->owner->movetype == MOVETYPE_NONE)
			return;

		if (que_typeexists(ent->curses, 0)) {
			safe_cprintf(ent, PRINT_HIGH, "You can't morph while cursed!\n");
			return;
		}

		V_RestoreMorphed(ent, 0);
        return;
    }

    //Talent: Morphing
    if (vrx_get_talent_slot(ent, TALENT_MORPHING) != -1)
        cost *= 1.0 - 0.25 * vrx_get_talent_level(ent, TALENT_MORPHING);

    if (!V_CanUseAbilities(ent, ENFORCER, cost, true))
        return;

    if (vrx_has_flag(ent) && !hw->value) {
        safe_cprintf(ent, PRINT_HIGH, "Can't morph while carrying flag!\n");
        return;
    }

    // make sure don't get stuck in a wall
    VectorSet (boxmin, -16, -16, -24);
    VectorSet (boxmax, 16, 16, -8);
    tr = gi.trace(ent->s.origin, boxmin, boxmax, ent->s.origin, ent, MASK_SHOT);
    if (tr.fraction < 1) {
		safe_cprintf(ent, PRINT_HIGH, "Not enough room to morph!\n");
		return;
	}
	
	V_ModifyMorphedHealth(ent, MORPH_ENFORCER, true);

	VectorCopy(boxmin, ent->mins);
	VectorCopy(boxmax, ent->maxs);

	ent->monsterinfo.attack_finished = level.time + 0.5;// can't attack immediately

	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->client->ability_delay = level.time + ENFORCER_DELAY;

	ent->mtype = MORPH_ENFORCER;
	ent->s.modelindex = gi.modelindex ("models/monsters/infantry/tris.md2");
	ent->s.modelindex2 = 0;
	ent->s.skinnum = 0;

	// set maximum machinegun ammo
	ent->myskills.abilities[ENFORCER].max_ammo = ENFORCER_MG_INITIAL_AMMO+ENFORCER_MG_ADDON_AMMO
		*ent->myskills.abilities[ENFORCER].current_level;

	// Talent: More Ammo
	// increases ammo 10% per talent level
	if(talentLevel > 0) ent->myskills.abilities[ENFORCER].max_ammo *= 1.0 + 0.1*talentLevel;

	// give them some starting ammo
	ent->myskills.abilities[ENFORCER].ammo = ENFORCER_MG_START_AMMO;

	ent->client->refire_frames = 0; // reset charged weapon
	ent->client->weapon_mode = 0; // reset weapon mode
	ent->client->pers.weapon = NULL;
	ent->client->ps.gunindex = 0;

	lasersight_off(ent);

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/morph.wav"), 1, ATTN_NORM, 0);
}