// g_weapon.c

#include "g_local.h"
#include "m_player.h"


static qboolean is_quad;
static byte             is_silenced;

void setFFState(edict_t *ent);
void weapon_grenade_fire (edict_t *ent, qboolean held);

void P_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
        vec3_t  _distance;

        VectorCopy (distance, _distance);

        if (client->pers.firing_style == ACTION_FIRING_CLASSIC ||
                client->pers.firing_style == ACTION_FIRING_CLASSIC_HIGH)
        {
                if (client->pers.hand == LEFT_HANDED)
                        _distance[1] *= -1;
                        else if (client->pers.hand == CENTER_HANDED)
                        _distance[1] = 0;
        }
                else
        {
                _distance[1] = 0; // fire from center always
        }
        
        G_ProjectSource (point, _distance, forward, right, result);
}

// used for setting up the positions of the guns in shell ejection
void Old_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
        vec3_t  _distance;

        VectorCopy (distance, _distance);
        if (client->pers.hand == LEFT_HANDED)
                        _distance[1] = -1;      // changed from = to *=
                                                // Fireblade 2/28/99
                                                // zucc reversed, this is only used for setting up shell ejection and 
                                                // since those look good this shouldn't be changed
        else if (client->pers.hand == CENTER_HANDED)
                        _distance[1] = 0;
        G_ProjectSource (point, _distance, forward, right, result);
}

// this one is the real old project source
void Knife_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
        vec3_t  _distance;

        VectorCopy (distance, _distance);
        if (client->pers.hand == LEFT_HANDED)
                        _distance[1] *= -1;     // changed from = to *=                                         
        else if (client->pers.hand == CENTER_HANDED)
                        _distance[1] = 0;
        G_ProjectSource (point, _distance, forward, right, result);
}



/*
===============
PlayerNoise

  Each player can have two noise objects associated with it:
  a personal noise (jumping, pain, weapon firing), and a weapon
  target noise (bullet wall impacts)
  
        Monsters that don't directly see the player can move
        to a noise in hopes of seeing the player from there.
        ===============
*/
void PlayerNoise(edict_t *who, vec3_t where, int type)
{
        edict_t         *noise;
        
        if (type == PNOISE_WEAPON)
        {
                if (who->client->silencer_shots)
                {
                        who->client->silencer_shots--;
                        return;
                }
        }
        
        if (deathmatch->value)
                return;
        
        if (who->flags & FL_NOTARGET)
                return;
        
        
        if (!who->mynoise)
        {
                noise = G_Spawn();
                noise->classname = "player_noise";
                VectorSet (noise->mins, -8, -8, -8);
                VectorSet (noise->maxs, 8, 8, 8);
                noise->owner = who;
                noise->svflags = SVF_NOCLIENT;
                who->mynoise = noise;
                
                noise = G_Spawn();
                noise->classname = "player_noise";
                VectorSet (noise->mins, -8, -8, -8);
                VectorSet (noise->maxs, 8, 8, 8);
                noise->owner = who;
                noise->svflags = SVF_NOCLIENT;
                who->mynoise2 = noise;
        }
        
        if (type == PNOISE_SELF || type == PNOISE_WEAPON)
        {
                noise = who->mynoise;
                level.sound_entity = noise;
                level.sound_entity_framenum = level.framenum;
        }
        else // type == PNOISE_IMPACT
        {
                noise = who->mynoise2;
                level.sound2_entity = noise;
                level.sound2_entity_framenum = level.framenum;
        }
        
        VectorCopy (where, noise->s.origin);
        VectorSubtract (where, noise->maxs, noise->absmin);
        VectorAdd (where, noise->maxs, noise->absmax);
        noise->teleport_time = level.time;
        gi.linkentity (noise);
}


void PlaceHolder( edict_t *ent )
{
        ent->nextthink = level.time + 1000;
}

// keep the entity around so we can find it later if we need to respawn the weapon there
void SetSpecWeapHolder (edict_t *ent )
{
        ent->flags |= FL_RESPAWN;
        ent->svflags |= SVF_NOCLIENT;
        ent->solid = SOLID_NOT;
        ent->think = PlaceHolder;
        gi.linkentity (ent);
}

qboolean Pickup_Weapon (edict_t *ent, edict_t *other)
{
        int                     index, index2;
        gitem_t         *ammo;
        gitem_t         *item;
        
        int                     special = 0;
        int band = 0;
        
        index = ITEM_INDEX(ent->item);
        
        if ( ( ((int)(dmflags->value) & DF_WEAPONS_STAY) || coop->value) 
                && other->client->pers.inventory[index])
        {
                if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM) ) )
                        return false;   // leave the weapon for others to pickup
        }
        
        // find out if they have a bandolier
        item = FindItem(BAND_NAME);
        if ( other->client->pers.inventory[ITEM_INDEX(item)] )
                band = 1;
        else
                band = 0;
        
        
        
        // zucc special cases for picking up weapons
        // the mk23 should never be dropped, probably
        
        if ( stricmp(ent->item->pickup_name, MK23_NAME) == 0 )
        {
                if ( other->client->pers.inventory[index] ) // already has one
                {
                        if ( !(ent->spawnflags & DROPPED_ITEM) )
                        {
                                ammo = FindItem (ent->item->ammo);
                                return ( Add_Ammo(other, ammo, ammo->quantity) );
                        }
                }
                other->client->pers.inventory[index]++;
                if ( !(ent->spawnflags & DROPPED_ITEM) )
                        other->client->mk23_rds = other->client->mk23_max;
        }
        else if ( stricmp(ent->item->pickup_name, MP5_NAME) == 0 )
        {
                if ( other->client->unique_weapon_total < unique_weapons->value + band ) 
                {
                        other->client->pers.inventory[index]++;
                        other->client->unique_weapon_total++;
                        if ( !(ent->spawnflags & DROPPED_ITEM) )
                                other->client->mp5_rds = other->client->mp5_max;
                        special = 1;
                        gi.cprintf(other, PRINT_HIGH, "%s - Unique Weapon\n", ent->item->pickup_name);
                }
                else
                        return false; // we can't get it        
        }
        else if ( stricmp(ent->item->pickup_name, M4_NAME) == 0 )
        {
                if ( other->client->unique_weapon_total < unique_weapons->value + band ) 
                {
                        other->client->pers.inventory[index]++;
                        other->client->unique_weapon_total++;
                        if ( !(ent->spawnflags & DROPPED_ITEM) )
                                other->client->m4_rds = other->client->m4_max;  
                        special = 1;
                        gi.cprintf(other, PRINT_HIGH, "%s - Unique Weapon\n", ent->item->pickup_name);
                }
                else
                        return false; // we can't get it        
                
        }
        else if ( stricmp(ent->item->pickup_name, M3_NAME) == 0 )
        {
                if ( other->client->unique_weapon_total < unique_weapons->value + band ) 
                {
                        other->client->pers.inventory[index]++;                 
                        other->client->unique_weapon_total++;
                        if ( !(ent->spawnflags & DROPPED_ITEM) )
                        {
                                // any weapon that doesn't completely fill up each reload can 
                                //end up in a state where it has a full weapon and pending reload(s)
                                if ( other->client->weaponstate == WEAPON_RELOADING ) 
                                {
                                        if ( other->client->fast_reload )
                                        {
                                                other->client->shot_rds = other->client->shot_max - 2;
                                        }
                                        else
                                                other->client->shot_rds = other->client->shot_max - 1;
                                }
                                else
                                {
                                        other->client->shot_rds = other->client->shot_max;      
                                }
                        }
                        special = 1;
                        gi.cprintf(other, PRINT_HIGH, "%s - Unique Weapon\n", ent->item->pickup_name);
                }
                else
                        return false; // we can't get it        
                
        }
        else if ( stricmp(ent->item->pickup_name, HC_NAME) == 0 )
        {
                if ( other->client->unique_weapon_total < unique_weapons->value + band ) 
                {
                        other->client->pers.inventory[index]++;
                        other->client->unique_weapon_total++;
                        if ( !(ent->spawnflags & DROPPED_ITEM) )
                        {
                                other->client->cannon_rds = other->client->cannon_max;  
                                index2 = ITEM_INDEX(FindItem(ent->item->ammo));
                                if ( other->client->pers.inventory[index2] + 5 > other->client->pers.max_shells )
                                        other->client->pers.inventory[index2] = other->client->pers.max_shells;
                                else
                                        other->client->pers.inventory[index2] += 5;
                                
                        }
                        gi.cprintf(other, PRINT_HIGH, "%s - Unique Weapon\n", ent->item->pickup_name);
                        special = 1;
                }
                else
                        return false; // we can't get it        
                
        }
        else if ( stricmp(ent->item->pickup_name, SNIPER_NAME) == 0 )
        {
                if ( other->client->unique_weapon_total < unique_weapons->value + band ) 
                {
                        other->client->pers.inventory[index]++;
                        other->client->unique_weapon_total++;
                        if ( !(ent->spawnflags & DROPPED_ITEM) )
                        {
                                if ( other->client->weaponstate == WEAPON_RELOADING ) 
                                {
                                        if ( other->client->fast_reload )
                                        {
                                                other->client->sniper_rds = other->client->sniper_max - 2;
                                        }
                                        else
                                                other->client->sniper_rds = other->client->sniper_max - 1;
                                }
                                else
                                {
                                        other->client->sniper_rds = other->client->sniper_max;      
                                } 
                        }         
                        special = 1;
                        gi.cprintf(other, PRINT_HIGH, "%s - Unique Weapon\n", ent->item->pickup_name);
                }
                else
                        return false; // we can't get it        
                
        }
        else if ( stricmp(ent->item->pickup_name, DUAL_NAME) == 0 )
        {
                
                if ( other->client->pers.inventory[index] ) // already has one
                {
                        if ( !(ent->spawnflags & DROPPED_ITEM) )
                        {
                                ammo = FindItem (ent->item->ammo);
                                return (Add_Ammo(other, ammo, ammo->quantity) );
                        }
                }
                other->client->pers.inventory[index]++;
                if ( !(ent->spawnflags & DROPPED_ITEM) )
                {
                        other->client->dual_rds += other->client->mk23_max;
                        // assume the player uses the new (full) pistol
                        other->client->mk23_rds = other->client->mk23_max; 
                }
        }
        else if ( stricmp(ent->item->pickup_name, KNIFE_NAME) == 0 )
        {
                
                if ( other->client->pers.inventory[index] < other->client->knife_max )
                {
                        other->client->pers.inventory[index]++;
                        return true;
                }
                else
                {
                        return false;
                }
        }
        else if ( stricmp(ent->item->pickup_name, GRENADE_NAME) == 0 )
        {
                
                if ( other->client->pers.inventory[index] < other->client->grenade_max )
                {
                        other->client->pers.inventory[index]++;
                        if (! (ent->spawnflags & DROPPED_PLAYER_ITEM) && !(ent->spawnflags & DROPPED_ITEM) )
                        {
                                if (deathmatch->value)
                                {
                                        if ((int)(dmflags->value) & DF_WEAPONS_STAY)
                                                ent->flags |= FL_RESPAWN;
                                        else
                                                SetRespawn (ent, 30);
                                }
                                if (coop->value)
                                        ent->flags |= FL_RESPAWN;
                        }
                        
                        return true;
                }
                else
                {
                        return false;
                }
        }
        else 
        {
                
                other->client->pers.inventory[index]++;
                
                
                if (!(ent->spawnflags & DROPPED_ITEM) )
                {
                        // give them some ammo with it
                        ammo = FindItem (ent->item->ammo);              
                        
                        if ( (int)dmflags->value & DF_INFINITE_AMMO )
                                Add_Ammo (other, ammo, 1000);
                        else
                                Add_Ammo (other, ammo, ammo->quantity);
                        
                        if (! (ent->spawnflags & DROPPED_PLAYER_ITEM) )
                        {
                                if (deathmatch->value)
                                {
                                        if ((int)(dmflags->value) & DF_WEAPONS_STAY)
                                                ent->flags |= FL_RESPAWN;
                                        else
                                                SetRespawn (ent, 30);
                                }
                                if (coop->value)
                                        ent->flags |= FL_RESPAWN;
                        }
                }
        }
        /* zucc - don't want auto switching
        if (other->client->pers.weapon != ent->item && 
        (other->client->pers.inventory[index] == 1) &&
        ( !deathmatch->value || other->client->pers.weapon == FindItem("blaster") ) )
        other->client->newweapon = ent->item;
        */
        if ( !(ent->spawnflags & DROPPED_ITEM) 
                && !(ent->spawnflags & DROPPED_PLAYER_ITEM)
                && (SPEC_WEAPON_RESPAWN) )
        {
                SetSpecWeapHolder( ent );
        }
        
        
        return true;
}


// zucc vwep 3.17(?) vwep support
void ShowGun(edict_t *ent)
{
        
        int nIndex;
        char *pszIcon;
        
        // No weapon?
        if ( !ent->client->pers.weapon)
        {
                ent->s.modelindex2 = 0;
                return;
        }
        
        // Determine the weapon's precache index.
        
        nIndex = 0;
        pszIcon = ent->client->pers.weapon->icon;
        
        if ( strcmp( pszIcon, "w_mk23") == 0)
                nIndex = 1;
        else if ( strcmp( pszIcon, "w_mp5") == 0)
                nIndex = 2;
        else if ( strcmp( pszIcon, "w_m4") == 0)
                nIndex = 3;
        else if ( strcmp( pszIcon, "w_cannon") == 0)
                nIndex = 4;
        else if ( strcmp( pszIcon, "w_super90") == 0)
                nIndex = 5;
        else if ( strcmp( pszIcon, "w_sniper") == 0)
                nIndex = 6;
        else if ( strcmp( pszIcon, "w_akimbo") == 0)
                nIndex = 7;
        else if ( strcmp( pszIcon, "w_knife") == 0)
                nIndex = 8;
        else if ( strcmp( pszIcon, "a_m61frag") == 0)
                nIndex = 9;
        
        // Clear previous weapon model.
        ent->s.skinnum &= 255;
        
        // Set new weapon model.
        ent->s.skinnum |= (nIndex << 8);
        ent->s.modelindex2 = 255;
        /*
        char heldmodel[128];
        int len;

        if(!ent->client->pers.weapon)
        {
                ent->s.modelindex2 = 0;
                return;
        }

        strcpy(heldmodel, "players/");
        strcat(heldmodel, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
        for(len = 8; heldmodel[len]; len++)
        {
                if(heldmodel[len] == '/')
                        heldmodel[++len] = '\0';
        }
        strcat(heldmodel, ent->client->pers.weapon->icon);      
        strcat(heldmodel, ".md2");
        //gi.dprintf ("%s\n", heldmodel);
        ent->s.modelindex2 = gi.modelindex(heldmodel);  
*/
}









//#define GRENADE_IDLE_FIRST 40
//#define GRENADE_IDLE_LAST 69


/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon (edict_t *ent)
{
        gitem_t *item;
        if (ent->client->grenade_time)
        {
                ent->client->grenade_time = level.time;
                ent->client->weapon_sound = 0;
                weapon_grenade_fire (ent, false);
                ent->client->grenade_time = 0;
        }

                // zucc - prevent reloading queue for previous weapon from doing anything
                ent->client->reload_attempts = 0;

        ent->client->pers.lastweapon = ent->client->pers.weapon;
        ent->client->pers.weapon = ent->client->newweapon;
        ent->client->newweapon = NULL;
        ent->client->machinegun_shots = 0;

        if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
                ent->client->ammo_index = ITEM_INDEX(FindItem(ent->client->pers.weapon->ammo));
        else
                ent->client->ammo_index = 0;

        if (!ent->client->pers.weapon || ent->s.modelindex != 255) // zucc vwep
        {       // dead 
                ent->client->ps.gunindex = 0;
                return;
        }

        ent->client->weaponstate = WEAPON_ACTIVATING;
        ent->client->ps.gunframe = 0;
        
//FIREBLADE
        if (ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD)
                return;
//FIREBLADE

        ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
        
        // zucc hentai's animation for vwep
        ent->client->anim_priority = ANIM_PAIN;
        if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
        {
                        ent->s.frame = FRAME_crpain1;
                        ent->client->anim_end = FRAME_crpain4;
        }
        else
        {
                        ent->s.frame = FRAME_pain301;
                        ent->client->anim_end = FRAME_pain304;
                        
        }
        
        ShowGun(ent);   
        // zucc done
        
        
        
        
        if(stricmp(ent->client->pers.weapon->pickup_name, MK23_NAME) == 0)
        {
                ent->client->curr_weap = MK23_NUM;
        }
        else if(stricmp(ent->client->pers.weapon->pickup_name, MP5_NAME) == 0)
        {
                ent->client->curr_weap = MP5_NUM;
        }
        else if(stricmp(ent->client->pers.weapon->pickup_name, M4_NAME) == 0)
        {
                ent->client->curr_weap = M4_NUM;
        }
        else if(stricmp(ent->client->pers.weapon->pickup_name, M3_NAME) == 0)
        {
                ent->client->curr_weap = M3_NUM;
        }
        else if(stricmp(ent->client->pers.weapon->pickup_name, HC_NAME) == 0)
        {
                ent->client->curr_weap = HC_NUM;
        }

        else if(stricmp(ent->client->pers.weapon->pickup_name, SNIPER_NAME) == 0)
        {
                ent->client->curr_weap = SNIPER_NUM;
        }
        else if(stricmp(ent->client->pers.weapon->pickup_name, DUAL_NAME) == 0)
        {
                ent->client->curr_weap = DUAL_NUM;
        }
        else if(stricmp(ent->client->pers.weapon->pickup_name, KNIFE_NAME) == 0)
        {
                ent->client->curr_weap = KNIFE_NUM;
        }
        else if(stricmp(ent->client->pers.weapon->pickup_name, GRENADE_NAME) == 0)
        {
                ent->client->curr_weap = GRENADE_NUM;
        }

        item = FindItem(LASER_NAME);
        if ( ent->client->pers.inventory[ITEM_INDEX(item)] )
                        SP_LaserSight(ent, item);//item->use(ent, item);
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange (edict_t *ent)
{
        if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("slugs"))]
                &&  ent->client->pers.inventory[ITEM_INDEX(FindItem("railgun"))] )
        {
                ent->client->newweapon = FindItem ("railgun");
                return;
        }
        if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("cells"))]
                &&  ent->client->pers.inventory[ITEM_INDEX(FindItem("hyperblaster"))] )
        {
                ent->client->newweapon = FindItem ("hyperblaster");
                return;
        }
        if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]
                &&  ent->client->pers.inventory[ITEM_INDEX(FindItem("chaingun"))] )
        {
                ent->client->newweapon = FindItem ("chaingun");
                return;
        }
        if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]
                &&  ent->client->pers.inventory[ITEM_INDEX(FindItem("machinegun"))] )
        {
                ent->client->newweapon = FindItem ("machinegun");
                return;
        }
        if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("shells"))] > 1
                &&  ent->client->pers.inventory[ITEM_INDEX(FindItem("super shotgun"))] )
        {
                ent->client->newweapon = FindItem ("super shotgun");
                return;
        }
        if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("shells"))]
                &&  ent->client->pers.inventory[ITEM_INDEX(FindItem("shotgun"))] )
        {
                ent->client->newweapon = FindItem ("shotgun");
                return;
        }
        ent->client->newweapon = FindItem ("blaster");
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon (edict_t *ent)
{
        // if just died, put the weapon away
        if (ent->health < 1)
        {
                ent->client->newweapon = NULL;
                ChangeWeapon (ent);
        }

        // call active weapon think routine
        if (ent->client->pers.weapon && ent->client->pers.weapon->weaponthink)
        {
                is_quad = (ent->client->quad_framenum > level.framenum);
                if (ent->client->silencer_shots)
                        is_silenced = MZ_SILENCED;
                else
                        is_silenced = 0;
                ent->client->pers.weapon->weaponthink (ent);
        }
}


/*
================
Use_Weapon

ake the weapon ready if there is ammo
================
*/
void Use_Weapon (edict_t *ent, gitem_t *item)
{
        //int                   ammo_index;
        //gitem_t               *ammo_item;


//        if(ent->client->weaponstate == WEAPON_BANDAGING || ent->client->bandaging == 1 )
  //                      return;
        
        
        
        
                // see if we're already using it
        if (item == ent->client->pers.weapon)
                return;

        // zucc - let them change if they want
                /*if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO))
        {
                ammo_item = FindItem(item->ammo);
                ammo_index = ITEM_INDEX(ammo_item);

                if (!ent->client->pers.inventory[ammo_index])
                {
                        gi.cprintf (ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
                        return;
                }

                if (ent->client->pers.inventory[ammo_index] < item->quantity)
                {
                        gi.cprintf (ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
                        return;
                }
        }*/

        // change to this weapon when down
        ent->client->newweapon = item;
}




edict_t *FindSpecWeapSpawn( edict_t* ent )
{
        edict_t *spot = NULL;
        
                //gi.bprintf (PRINT_HIGH, "Calling the FindSpecWeapSpawn\n");
                spot = G_Find (spot, FOFS(classname), ent->classname);
                //gi.bprintf (PRINT_HIGH, "spot = %p and spot->think = %p and playerholder = %p, spot, (spot ? spot->think : 0), PlaceHolder\n");
        while ( spot && spot->think != PlaceHolder )//(spot->spawnflags & DROPPED_ITEM ) && spot->think != PlaceHolder )//spot->solid == SOLID_NOT )        
        {
                //              gi.bprintf (PRINT_HIGH, "Calling inside the loop FindSpecWeapSpawn\n");
                spot = G_Find (spot, FOFS(classname), ent->classname);
        }
/*      if (!spot)
        {
                gi.bprintf(PRINT_HIGH, "Failed to find a spawn spot for %s\n", ent->classname);
        }
        else
                gi.bprintf(PRINT_HIGH, "Found a spawn spot for %s\n", ent->classname);
*/
        return spot;
}

void ThinkSpecWeap( edict_t* ent );


static void SpawnSpecWeap(gitem_t *item, edict_t *spot)
{
/*        edict_t *ent;
        vec3_t  forward, right;
        vec3_t  angles;

        ent = G_Spawn();

        ent->classname = item->classname;
        ent->item = item;
        ent->spawnflags = DROPPED_PLAYER_ITEM;//DROPPED_ITEM;
        ent->s.effects = item->world_model_flags;
        ent->s.renderfx = RF_GLOW;
        VectorSet (ent->mins, -15, -15, -15);
        VectorSet (ent->maxs, 15, 15, 15);
        gi.setmodel (ent, ent->item->world_model);
        ent->solid = SOLID_TRIGGER;
        ent->movetype = MOVETYPE_TOSS;  
        ent->touch = Touch_Item;
        ent->owner = ent;

        angles[0] = 0;
        angles[1] = rand() % 360;
        angles[2] = 0;

        AngleVectors (angles, forward, right, NULL);
        VectorCopy (spot->s.origin, ent->s.origin);
        ent->s.origin[2] += 16;
        VectorScale (forward, 100, ent->velocity);
        ent->velocity[2] = 300;

        ent->think = NULL;

        gi.linkentity (ent);
                */
                SetRespawn(spot, 1);
                gi.linkentity(spot);
}

void temp_think_specweap( edict_t* ent )
{
        ent->touch = Touch_Item;
        if (deathmatch->value && !teamplay->value && !allweapon->value )
        {
                ent->nextthink = level.time + 74;
                ent->think = ThinkSpecWeap;
        }
                else if ( teamplay->value && !allweapon->value)
                {
                        ent->nextthink = level.time + 1000;
                        ent->think = PlaceHolder;
                }
                else // allweapon set
                {
                        ent->nextthink = level.time + 1;
                        ent->think = G_FreeEdict;
                }
}



// zucc make dropped weapons respawn elsewhere
void ThinkSpecWeap( edict_t* ent )
{
        edict_t *spot;

        if ((spot = FindSpecWeapSpawn(ent)) != NULL) {
                SpawnSpecWeap(ent->item, spot);         
                G_FreeEdict(ent);
        } else {
                ent->nextthink = level.time + 1;
                ent->think = G_FreeEdict;
        }
}





/*
================
Drop_Weapon
================
*/
void Drop_Weapon (edict_t *ent, gitem_t *item)
{
        int             index;
        gitem_t *replacement;
        edict_t *temp = NULL;
        
        if ((int)(dmflags->value) & DF_WEAPONS_STAY)
                return;

        
        if ( ent->client->weaponstate == WEAPON_BANDAGING ||
                ent->client->bandaging == 1 || ent->client->bandage_stopped == 1 )
        {
                gi.cprintf(ent, PRINT_HIGH, "You are too busy bandaging right now...\n");
                return;
        }
                // don't let them drop this, causes duplication
                if ( ent->client->newweapon == item )
                {
                        return;
                }


        index = ITEM_INDEX(item);
        // see if we're already using it
        //zucc special cases for dropping       
        if ( stricmp(item->pickup_name, MK23_NAME) == 0 )
        {
                gi.cprintf(ent, PRINT_HIGH, "Can't drop the %s.\n", MK23_NAME);
                return;
        }
        else if ( stricmp(item->pickup_name, MP5_NAME) == 0 )
        {
                
                if ( ent->client->pers.weapon == item && (ent->client->pers.inventory[index] == 1) )
                {
                        replacement = FindItem (MK23_NAME); // back to the pistol then
                        ent->client->newweapon = replacement;
                        ent->client->weaponstate = WEAPON_DROPPING;
                        ent->client->ps.gunframe = 51;
                        
                        //ChangeWeapon( ent );
                }
                ent->client->unique_weapon_total--; // dropping 1 unique weapon
                temp = Drop_Item (ent, item);
                temp->think = temp_think_specweap;
                ent->client->pers.inventory[index]--;
        }
        else if ( stricmp(item->pickup_name, M4_NAME) == 0 )
        {
                   
                        
                        if ( ent->client->pers.weapon == item  && (ent->client->pers.inventory[index] == 1) )
                {
                        replacement = FindItem (MK23_NAME); // back to the pistol then
                        ent->client->newweapon = replacement;
                        ent->client->weaponstate = WEAPON_DROPPING;
                        ent->client->ps.gunframe = 44;
                        
                        //ChangeWeapon( ent );
                }
                ent->client->unique_weapon_total--; // dropping 1 unique weapon
                temp = Drop_Item (ent, item);
                temp->think = temp_think_specweap;
                ent->client->pers.inventory[index]--;
        }
        else if ( stricmp(item->pickup_name, M3_NAME) == 0 )
        {
                
                        if ( ent->client->pers.weapon == item && (ent->client->pers.inventory[index] == 1) )
                {
                        replacement = FindItem (MK23_NAME); // back to the pistol then
                        ent->client->newweapon = replacement;
                        ent->client->weaponstate = WEAPON_DROPPING;
                        ent->client->ps.gunframe = 41;
                        //ChangeWeapon( ent );
                }
                ent->client->unique_weapon_total--; // dropping 1 unique weapon
                temp = Drop_Item (ent, item);
                temp->think = temp_think_specweap;
                ent->client->pers.inventory[index]--;
        }
        else if ( stricmp(item->pickup_name, HC_NAME) == 0 )
        {
                    if ( ent->client->pers.weapon == item && (ent->client->pers.inventory[index] == 1) )
                {
                        replacement = FindItem (MK23_NAME); // back to the pistol then
                        ent->client->newweapon = replacement;
                        ent->client->weaponstate = WEAPON_DROPPING;
                        ent->client->ps.gunframe = 61;
                        //ChangeWeapon( ent );
                }
                ent->client->unique_weapon_total--; // dropping 1 unique weapon
                temp = Drop_Item (ent, item);
                temp->think = temp_think_specweap;
                ent->client->pers.inventory[index]--;
        }
        else if ( stricmp(item->pickup_name, SNIPER_NAME) == 0 )
        {
                if ( ent->client->pers.weapon == item && (ent->client->pers.inventory[index] == 1) )
                {
                        // in case they are zoomed in
                        ent->client->ps.fov = 90;
                        ent->client->desired_fov = 90;
                        replacement = FindItem (MK23_NAME); // back to the pistol then
                        ent->client->newweapon = replacement;
                        ent->client->weaponstate = WEAPON_DROPPING;
                        ent->client->ps.gunframe = 50;
                        //ChangeWeapon( ent );
                }
                ent->client->unique_weapon_total--; // dropping 1 unique weapon
                temp = Drop_Item (ent, item);
                temp->think = temp_think_specweap;
                ent->client->pers.inventory[index]--;
        }
        else if ( stricmp(item->pickup_name, DUAL_NAME) == 0 )
        {
                    if ( ent->client->pers.weapon == item )
                {
                        replacement = FindItem (MK23_NAME); // back to the pistol then
                        ent->client->newweapon = replacement;
                        ent->client->weaponstate = WEAPON_DROPPING;
                        ent->client->ps.gunframe = 40;
                        //ChangeWeapon( ent );
                }
                ent->client->dual_rds = ent->client->mk23_rds;
        }
        else if ( stricmp(item->pickup_name, KNIFE_NAME) == 0 )
        {
                //gi.cprintf(ent, PRINT_HIGH, "Before checking knife inven frames = %d\n", ent->client->ps.gunframe);
                    
                        if ( ((ent->client->pers.weapon == item) ) && (ent->client->pers.inventory[index] == 1) )
                {
                        replacement = FindItem (MK23_NAME); // back to the pistol then
                        ent->client->newweapon = replacement;
                        if ( ent->client->resp.knife_mode ) // hack to avoid an error
                        {
                                ent->client->weaponstate = WEAPON_DROPPING;
                                ent->client->ps.gunframe = 111;
                        }
                        else
                                ChangeWeapon( ent );
                        //      gi.cprintf(ent, PRINT_HIGH, "After change weap from knife drop frames = %d\n", ent->client->ps.gunframe);
                }
        }
        else if ( stricmp(item->pickup_name, GRENADE_NAME) == 0 )
        {
                    if ( (ent->client->pers.weapon == item  ) && (ent->client->pers.inventory[index] == 1) )
                {
                                                if ( (ent->client->ps.gunframe >= GRENADE_IDLE_FIRST) && (ent->client->ps.gunframe <= GRENADE_IDLE_LAST) 
                                                                                                        || ( ent->client->ps.gunframe >= GRENADE_THROW_FIRST && ent->client->ps.gunframe <= GRENADE_THROW_LAST) ) 
                                                {
                                                        ent->client->ps.gunframe = 0;
                                                        fire_grenade2 (ent, ent->s.origin, tv(0,0,0), GRENADE_DAMRAD, 0, 2, GRENADE_DAMRAD*2, false);
                                                        item = FindItem(GRENADE_NAME);
                                                        ent->client->pers.inventory[ITEM_INDEX(item)]--;
                                                        ent->client->newweapon = FindItem(MK23_NAME);
                                                        ent->client->weaponstate = WEAPON_DROPPING;
                                                        ent->client->ps.gunframe = 0;
                                                        return;

                                                }
                                                
                                                
                                                replacement = FindItem (MK23_NAME); // back to the pistol then
                        ent->client->newweapon = replacement;
                        ent->client->weaponstate = WEAPON_DROPPING;
                        ent->client->ps.gunframe = 0;
                        //ChangeWeapon( ent );
                }
        }
        
        
        else if ( ((item == ent->client->pers.weapon) || (item == ent->client->newweapon))&& (ent->client->pers.inventory[index] == 1) )
        {
                gi.cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
                return;
        }
        
        if ( !temp )    
        {
                temp = Drop_Item (ent, item);
                ent->client->pers.inventory[index]--;
        }

}





//zucc drop special weapon (only 1 of them)
void DropSpecialWeapon ( edict_t* ent )
{

        // first check if their current weapon is a special weapon, if so, drop it.
        if ( (ent->client->pers.weapon == FindItem(MP5_NAME))
                || (ent->client->pers.weapon == FindItem(M4_NAME))
                || (ent->client->pers.weapon == FindItem(M3_NAME))
                || (ent->client->pers.weapon == FindItem(HC_NAME))
                || (ent->client->pers.weapon == FindItem(SNIPER_NAME)) )
                Drop_Weapon (ent, ent->client->pers.weapon);
        else if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(SNIPER_NAME))] > 0 )
                Drop_Weapon (ent, FindItem(SNIPER_NAME));
        else if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(HC_NAME))] > 0 )
                Drop_Weapon (ent, FindItem(HC_NAME));
        else if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(M3_NAME))] > 0 )
                Drop_Weapon (ent, FindItem(M3_NAME));
        else if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(MP5_NAME))] > 0 )
                Drop_Weapon (ent, FindItem(MP5_NAME));
        else if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(M4_NAME))] > 0 )
                Drop_Weapon (ent, FindItem(M4_NAME));
        // special case, aq does this, guess I can support it
        else if (ent->client->pers.weapon == FindItem(DUAL_NAME))
                ent->client->newweapon = FindItem(MK23_NAME);
        
}

// used for when we want to force a player to drop an extra special weapon
// for when they drop the bandolier and they are over the weapon limit
void DropExtraSpecial( edict_t* ent )
{
        gitem_t *item;
            
                
                if ( (ent->client->pers.weapon == FindItem(MP5_NAME))
                || (ent->client->pers.weapon == FindItem(M4_NAME))
                || (ent->client->pers.weapon == FindItem(M3_NAME))
                || (ent->client->pers.weapon == FindItem(HC_NAME))
                || (ent->client->pers.weapon == FindItem(SNIPER_NAME)) )
        {
                item = ent->client->pers.weapon;
                // if they have more than 1 then they are willing to drop one           
                if ( ent->client->pers.inventory[ITEM_INDEX(item)] > 1 )
                {
                        Drop_Weapon(ent, ent->client->pers.weapon );
                        return;
                }
        }
        // otherwise drop some weapon they aren't using
        if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(SNIPER_NAME))] > 0 
                && FindItem(SNIPER_NAME) != ent->client->pers.weapon )
                Drop_Weapon (ent, FindItem(SNIPER_NAME));
        else if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(HC_NAME))] > 0 
                && FindItem(HC_NAME) != ent->client->pers.weapon )
                Drop_Weapon (ent, FindItem(HC_NAME));
        else if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(M3_NAME))] > 0 
                && FindItem(M3_NAME) != ent->client->pers.weapon )
                Drop_Weapon (ent, FindItem(M3_NAME));
        else if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(MP5_NAME))] > 0 
                && FindItem(MP5_NAME) != ent->client->pers.weapon )
                Drop_Weapon (ent, FindItem(MP5_NAME));
        else if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(M4_NAME))] > 0 
                && FindItem(M4_NAME) != ent->client->pers.weapon )
                Drop_Weapon (ent, FindItem(M4_NAME));
        else
                gi.dprintf("Couldn't find the appropriate weapon to drop.\n");
        

}

//zucc ready special weapon
void ReadySpecialWeapon ( edict_t* ent )
{
        int weapons[5] = {MP5_NUM, M4_NUM, M3_NUM, HC_NUM, SNIPER_NUM};
        char *strings[5] = {MP5_NAME, M4_NAME, M3_NAME, HC_NAME, SNIPER_NAME};
        int curr, i;
        int last;
        
        
        if(ent->client->weaponstate == WEAPON_BANDAGING || ent->client->bandaging == 1 )
                return;
        
        
        for ( curr = 0; curr < 5; curr++ )
        {
                if ( ent->client->curr_weap == weapons[curr] )
                        break;
        }
        if ( curr >= 5 )
        {
                curr = -1;
                last = 5;
        }
        else
        {
                last = curr + 5;
        }
        
        for ( i = (curr + 1); i != last; i = (i + 1))
        {
                if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(strings[i % 5]))] )        
                {
                        ent->client->newweapon = FindItem(strings[i % 5]);
                        return;
                }
        }
        
        
}

/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/

//zucc - copied in BD's code, modified for use with other weapons

#define FRAME_FIRE_FIRST                (FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST                (FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST  (FRAME_IDLE_LAST + 1)

#define FRAME_RELOAD_FIRST              (FRAME_DEACTIVATE_LAST +1)
#define FRAME_LASTRD_FIRST   (FRAME_RELOAD_LAST +1)

#define MK23MAG 12
#define MP5MAG  30
#define M4MAG   24
#define DUALMAG 24

void Weapon_Generic (edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, 
                                         int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, 
                                         int FRAME_RELOAD_LAST, int FRAME_LASTRD_LAST,
                                         int *pause_frames, int *fire_frames, void (*fire)(edict_t *ent))
{
        int             n;
        int                     bFire = 0;
        int                     bOut = 0;
/*        int                     bBursting = 0;*/


        // zucc vwep
        if(ent->s.modelindex != 255)  
                return; // not on client, so VWep animations could do wacky things

//FIREBLADE
        if (ent->client->weaponstate == WEAPON_FIRING &&
                ((ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD) || 
                 lights_camera_action))
        {
                ent->client->weaponstate = WEAPON_READY;
        }
//FIREBLADE
        
        //+BD - Added Reloading weapon, done manually via a cmd
        if( ent->client->weaponstate == WEAPON_RELOADING)
        {
                if(ent->client->ps.gunframe < FRAME_RELOAD_FIRST || ent->client->ps.gunframe > FRAME_RELOAD_LAST)
                        ent->client->ps.gunframe = FRAME_RELOAD_FIRST;
                else if(ent->client->ps.gunframe < FRAME_RELOAD_LAST)
                {
                        ent->client->ps.gunframe++;             
                        switch ( ent->client->curr_weap )
                        {
                                //+BD - Check weapon to find out when to play reload sounds
                        case MK23_NUM:
                                {
                                        if(ent->client->ps.gunframe == 46)
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23out.wav"), 1, ATTN_NORM, 0);
                                        else if(ent->client->ps.gunframe == 53)
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23in.wav"), 1, ATTN_NORM, 0);                               
                                        else if(ent->client->ps.gunframe == 59) // 3
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23slap.wav"), 1, ATTN_NORM, 0);
                                        break;
                                }
                        case MP5_NUM:
                                {
                                        if(ent->client->ps.gunframe == 55)
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5out.wav"), 1, ATTN_NORM, 0);
                                        else if(ent->client->ps.gunframe == 59)
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5in.wav"), 1, ATTN_NORM, 0);                               
                                        else if(ent->client->ps.gunframe == 63) //61
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5slap.wav"), 1, ATTN_NORM, 0);                               
                                        break;
                                }
                        case M4_NUM:
                                {
                                        if(ent->client->ps.gunframe == 52)
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/m4a1out.wav"), 1, ATTN_NORM, 0);
                                        else if(ent->client->ps.gunframe == 58)
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/m4a1in.wav"), 1, ATTN_NORM, 0);                               
                                        break;
                                }
                        case M3_NUM:
                                {
                                                                        if ( ent->client->shot_rds >= ent->client->shot_max)
                                                                        {
                                                                                ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                                                                                ent->client->weaponstate = WEAPON_READY;
                                                                                return;
                                                                        }
                                                                        
                                                                        if(ent->client->ps.gunframe == 48)
                                                                        {
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/m3in.wav"), 1, ATTN_NORM, 0); 
                                        }
                                        if(ent->client->ps.gunframe == 49 )
                                        {
                                                if ( ent->client->fast_reload == 1 )
                                                {
                                                        ent->client->fast_reload = 0;
                                                        ent->client->shot_rds++;
                                                        ent->client->ps.gunframe = 44;
                                                }
                                        }
                                        break;
                                }
                        case HC_NUM:
                                {
                                        if(ent->client->ps.gunframe == 64)
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/copen.wav"), 1, ATTN_NORM, 0);
                                        else if(ent->client->ps.gunframe == 67)
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/cout.wav"), 1, ATTN_NORM, 0);                               
                                        else if(ent->client->ps.gunframe == 76) //61
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/cin.wav"), 1, ATTN_NORM, 0);                               
                                        else if (ent->client->ps.gunframe == 80) // 3
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/cclose.wav"), 1, ATTN_NORM, 0);
                                        break;
                                }
                        case SNIPER_NUM:
                                {
                                    
                                                                        if ( ent->client->sniper_rds >= ent->client->sniper_max)
                                                                        {
                                                                                ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                                                                                ent->client->weaponstate = WEAPON_READY;
                                                                                return;
                                                                        }
                                                                        
                                                                        if(ent->client->ps.gunframe == 59)
                                        {
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/ssgbolt.wav"), 1, ATTN_NORM, 0);      
                                        }
                                        else if(ent->client->ps.gunframe == 71)
                                        {
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/ssgin.wav"), 1, ATTN_NORM, 0);        
                                        }
                                        else if(ent->client->ps.gunframe == 73 )
                                        {
                                                if ( ent->client->fast_reload == 1 )
                                                {
                                                        ent->client->fast_reload = 0;
                                                        ent->client->sniper_rds++;
                                                        ent->client->ps.gunframe = 67;
                                                }
                                        }
                                        else if(ent->client->ps.gunframe == 76)
                                        {
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/ssgbolt.wav"), 1, ATTN_NORM, 0);      
                                        }
                                        break;
                                }
                        case DUAL_NUM:
                                {
                                        if(ent->client->ps.gunframe == 45)
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23out.wav"), 1, ATTN_NORM, 0);
                                        else if(ent->client->ps.gunframe == 53)
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23slap.wav"), 1, ATTN_NORM, 0);                               
                                        else if(ent->client->ps.gunframe == 60) // 3
                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23slap.wav"), 1, ATTN_NORM, 0);
                                        break;
                                }
                        default:
                                gi.dprintf("No weapon choice for reloading (sounds).\n");
                                break;
                                
                        }               
                }
                else
                {
                        ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                        ent->client->weaponstate = WEAPON_READY;
                        switch ( ent->client->curr_weap )
                        {
                        case MK23_NUM:
                                {
                                        ent->client->dual_rds -= ent->client->mk23_rds;
                                        ent->client->mk23_rds = ent->client->mk23_max;
                                        ent->client->dual_rds += ent->client->mk23_max;
                                        (ent->client->pers.inventory[ent->client->ammo_index])--;
                                        if ( ent->client->pers.inventory[ent->client->ammo_index] < 0) 
                                        {
                                                ent->client->pers.inventory[ent->client->ammo_index] = 0;
                                        }
                                        ent->client->fired = 0; // reset any firing delays
                                        break;
                                        //else
                                        //      ent->client->mk23_rds = ent->client->pers.inventory[ent->client->ammo_index];
                                }
                        case MP5_NUM:
                                {
                                        
                                        ent->client->mp5_rds = ent->client->mp5_max;
                                        (ent->client->pers.inventory[ent->client->ammo_index])--;
                                        if ( ent->client->pers.inventory[ent->client->ammo_index] < 0) 
                                        {
                                                ent->client->pers.inventory[ent->client->ammo_index] = 0;
                                        }
                                        ent->client->fired = 0; // reset any firing delays
                                        ent->client->burst = 0; // reset any bursting
                                        break;
                                }
                        case M4_NUM:
                                {
                                        
                                        ent->client->m4_rds = ent->client->m4_max;
                                        (ent->client->pers.inventory[ent->client->ammo_index])--;
                                        if ( ent->client->pers.inventory[ent->client->ammo_index] < 0) 
                                        {
                                                ent->client->pers.inventory[ent->client->ammo_index] = 0;
                                        }
                                        ent->client->fired = 0; // reset any firing delays
                                        ent->client->burst = 0; // reset any bursting                           
                                        ent->client->machinegun_shots = 0;
                                        break;
                                }
                        case M3_NUM:
                                {
                                        ent->client->shot_rds++;
                                        (ent->client->pers.inventory[ent->client->ammo_index])--;
                                        if ( ent->client->pers.inventory[ent->client->ammo_index] < 0) 
                                        {
                                                ent->client->pers.inventory[ent->client->ammo_index] = 0;
                                        }       
                                        break;
                                }
                        case HC_NUM:
                                {
                                        ent->client->cannon_rds = ent->client->cannon_max;
                                        (ent->client->pers.inventory[ent->client->ammo_index]) -= 2;
                                        if ( ent->client->pers.inventory[ent->client->ammo_index] < 0) 
                                        {
                                                ent->client->pers.inventory[ent->client->ammo_index] = 0;
                                        }
                                        break;
                                }
                        case SNIPER_NUM:
                                {
                                        ent->client->sniper_rds++;
                                        (ent->client->pers.inventory[ent->client->ammo_index])--;
                                        if ( ent->client->pers.inventory[ent->client->ammo_index] < 0) 
                                        {
                                                ent->client->pers.inventory[ent->client->ammo_index] = 0;
                                        }       
                                        return;
                                }
                        case DUAL_NUM:
                                {
                                        ent->client->dual_rds = ent->client->dual_max;
                                        ent->client->mk23_rds = ent->client->mk23_max;
                                        (ent->client->pers.inventory[ent->client->ammo_index]) -= 2;
                                        if ( ent->client->pers.inventory[ent->client->ammo_index] < 0) 
                                        {
                                                ent->client->pers.inventory[ent->client->ammo_index] = 0;
                                        }                       
                                        break;
                                }
                        default:
                                gi.dprintf("No weapon choice for reloading.\n");
                                break;
                        }

                        
                }
        }
                
        if( ent->client->weaponstate == WEAPON_END_MAG)
        {
                        if(ent->client->ps.gunframe < FRAME_LASTRD_LAST)
                                ent->client->ps.gunframe++;
                        else
                                ent->client->ps.gunframe = FRAME_LASTRD_LAST;
                        // see if our weapon has ammo (from something other than reloading)
                        if ( (( ent->client->curr_weap == MK23_NUM ) && ( ent->client->mk23_rds > 0 ))
                                || (( ent->client->curr_weap == MP5_NUM ) && ( ent->client->mp5_rds > 0 ))
                                || (( ent->client->curr_weap == M4_NUM ) && ( ent->client->m4_rds > 0 ))
                                || (( ent->client->curr_weap == M3_NUM ) && ( ent->client->shot_rds > 0 ))
                                || (( ent->client->curr_weap == HC_NUM ) && ( ent->client->cannon_rds > 0 ))
                                || (( ent->client->curr_weap == SNIPER_NUM ) && ( ent->client->sniper_rds > 0 ))
                                || (( ent->client->curr_weap == DUAL_NUM ) && ( ent->client->dual_rds > 0 )) )
                        {
                                ent->client->weaponstate = WEAPON_READY;
                                ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                        }
                        if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) 
                                //FIREBLADE
                                && (ent->solid != SOLID_NOT || ent->deadflag == DEAD_DEAD) && !lights_camera_action)
                                //FIREBLADE
                        {
                                ent->client->latched_buttons &= ~BUTTON_ATTACK;
                                {
                                        if (level.time >= ent->pain_debounce_time)
                                        {
                                                gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                                                ent->pain_debounce_time = level.time + 1;
                                        }
                                        
                                        
                                }
                                
                        }                
        }
        
        if (ent->client->weaponstate == WEAPON_DROPPING)
        {
                if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
                {
                        
                        ChangeWeapon (ent);
                        return;
                }
                // zucc for vwep
                else if((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4)
                {
                        ent->client->anim_priority = ANIM_REVERSE;
                        if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                        {
                                ent->s.frame = FRAME_crpain4+1;
                                ent->client->anim_end = FRAME_crpain1;
                        }
                        else
                        {
                                ent->s.frame = FRAME_pain304+1;
                                ent->client->anim_end = FRAME_pain301;
                                
                        }
                }


                ent->client->ps.gunframe++;
                return;
        }
        
        if ( ent->client->weaponstate == WEAPON_BANDAGING )
        {
                if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST )
                {
                        ent->client->weaponstate = WEAPON_BUSY;
                        ent->client->idle_weapon = BANDAGE_TIME;
                        return;
                }
                ent->client->ps.gunframe++;
                return;
        }


        if (ent->client->weaponstate == WEAPON_ACTIVATING)
        {
                if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST)
                {
                        ent->client->weaponstate = WEAPON_READY;
                        ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                        return;
                }
                
                // sounds for activation?
                switch( ent->client->curr_weap )
                {
                case MK23_NUM:
                                        {
                                                if (ent->client->dual_rds >= ent->client->mk23_max )
                                                        ent->client->mk23_rds = ent->client->mk23_max;
                                                else
                                                        ent->client->mk23_rds = ent->client->dual_rds;
                                                if(ent->client->ps.gunframe == 3) // 3
                                                {
                                                        if ( ent->client->mk23_rds > 0 )
                                                        {
                                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23slide.wav"), 1, ATTN_NORM, 0);
                                                        }
                                                        else
                                                        {
                                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                                                                //mk23slap
                                                                ent->client->ps.gunframe = 62;
                                                                ent->client->weaponstate = WEAPON_END_MAG;
                                                        }
                                                }
                                                ent->client->fired = 0;                                 //reset any firing delays
                                                break;
                        }
                case MP5_NUM:
                        {
                                if(ent->client->ps.gunframe == 3) // 3
                                        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5slide.wav"), 1, ATTN_NORM, 0);
                                ent->client->fired = 0;                                 //reset any firing delays
                                ent->client->burst = 0;
                                break;
                        }
                case M4_NUM:
                        {
                                if(ent->client->ps.gunframe == 3) // 3
                                        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/m4a1slide.wav"), 1, ATTN_NORM, 0);
                                ent->client->fired = 0;                                 //reset any firing delays
                                ent->client->burst = 0;
                                ent->client->machinegun_shots = 0;
                                break;
                        }
                case M3_NUM:
                        {
                                ent->client->fired = 0;                                 //reset any firing delays
                                ent->client->burst = 0;
                                ent->client->fast_reload = 0;
                                break;
                        }
                case HC_NUM:
                        {
                                ent->client->fired = 0;                                 //reset any firing delays
                                ent->client->burst = 0;
                                break;
                        }
                case SNIPER_NUM:
                        {
                                ent->client->fired = 0;                                 //reset any firing delays
                                ent->client->burst = 0;
                                ent->client->fast_reload = 0;
                                break;
                        }
                case DUAL_NUM:
                        {
                            if ( ent->client->dual_rds <= 0 && ent->client->ps.gunframe == 3)    
                                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                                                        if ( ent->client->dual_rds <= 0 && ent->client->ps.gunframe == 4)
                                                        {
                                                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                                                                ent->client->ps.gunframe = 68;
                                                                ent->client->weaponstate = WEAPON_END_MAG;
                                                                ent->client->resp.sniper_mode = 0;
                                
                                                ent->client->desired_fov = 90;
                                                                ent->client->ps.fov = 90;
                                                                ent->client->fired = 0;        //reset any firing delays
                                                                ent->client->burst = 0;
                            
                                                                return;                 
                                                        }                                               
                                                        ent->client->fired = 0;        //reset any firing delays
                            ent->client->burst = 0;
                            break;
                        }
                        
                default:
                        gi.dprintf("Activated unknown weapon.\n");
                        break;
                }

                ent->client->resp.sniper_mode = 0;
                // has to be here for dropping the sniper rifle, in the drop command didn't work...
                ent->client->desired_fov = 90;
                ent->client->ps.fov = 90;
                ent->client->ps.gunframe++;
                return;
        }
                
        if (ent->client->weaponstate == WEAPON_BUSY)
        {
                        if ( ent->client->bandaging == 1 )
                        {
                                if ( !(ent->client->idle_weapon) )
                                {
                                        Bandage( ent );
                                        return;
                                }
                                else
                                {
                                        (ent->client->idle_weapon)--;
                                        return;
                                }
                        }
                        
                        // for after bandaging delay
                        if ( !(ent->client->idle_weapon) && ent->client->bandage_stopped )
                        {
                                ent->client->weaponstate = WEAPON_ACTIVATING;
                                ent->client->ps.gunframe = 0;
                                ent->client->bandage_stopped = 0;
                                return;
                        }
                        else if ( ent->client->bandage_stopped )
                        {
                                (ent->client->idle_weapon)--;
                                return;
                        }
                        
                        if( ent->client->curr_weap == SNIPER_NUM )
                        {
                                if ( ent->client->desired_fov == 90 )
                                                                {    
                                                                                ent->client->ps.fov = 90;
                                                                                ent->client->weaponstate = WEAPON_READY;
                                                                                ent->client->idle_weapon = 0;
                                                                }
                                if ( !(ent->client->idle_weapon) && ent->client->desired_fov != 90 )
                                {
                                        ent->client->ps.fov = ent->client->desired_fov;
                                        ent->client->weaponstate = WEAPON_READY;
                                        ent->client->ps.gunindex = 0;
                                                                                return;
                                }
                                else                                                    
                                        (ent->client->idle_weapon)--;
                                                                                                
                        }
        }


        if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING)
                        && (ent->client->weaponstate != WEAPON_BURSTING )
                        && (ent->client->bandage_stopped == 0) )
        {
                        ent->client->weaponstate = WEAPON_DROPPING;
                        ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
                        ent->client->desired_fov = 90;
                        ent->client->ps.fov = 90;
                        ent->client->resp.sniper_mode = 0;
                        if ( ent->client->pers.weapon )
                                ent->client->ps.gunindex = gi.modelindex( ent->client->pers.weapon->view_model );
                        
                        // zucc more vwep stuff
                        if((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
                        {
                                ent->client->anim_priority = ANIM_REVERSE;
                                if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                                {
                                        ent->s.frame = FRAME_crpain4+1;
                                        ent->client->anim_end = FRAME_crpain1;
                                }
                                else
                                {
                                        ent->s.frame = FRAME_pain304+1;
                                        ent->client->anim_end = FRAME_pain301;
                                        
                                }
                        }
                        return;
        }

        // bandaging case
        if ( (ent->client->bandaging)
                && (ent->client->weaponstate != WEAPON_FIRING) 
                && (ent->client->weaponstate != WEAPON_BURSTING)
                && (ent->client->weaponstate != WEAPON_BUSY)
                && (ent->client->weaponstate != WEAPON_BANDAGING) )
        {
                ent->client->weaponstate = WEAPON_BANDAGING;
                ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
                return;
        }
        
        if (ent->client->weaponstate == WEAPON_READY)
        {
                if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) 
//FIREBLADE
                        && (ent->solid != SOLID_NOT || ent->deadflag == DEAD_DEAD) && !lights_camera_action)
//FIREBLADE
                {
                        ent->client->latched_buttons &= ~BUTTON_ATTACK;
                        switch ( ent->client->curr_weap )
                        {
                        case MK23_NUM:
                                {
                                        //      gi.cprintf (ent, PRINT_HIGH, "Calling ammo check %d\n", ent->client->mk23_rds);
                                        if ( ent->client->mk23_rds > 0 )
                                        {
                                                //              gi.cprintf(ent, PRINT_HIGH, "Entered fire selection\n");
                                                if ( ent->client->resp.mk23_mode != 0 && ent->client->fired == 0 )
                                                {
                                                        ent->client->fired = 1;
                                                        bFire = 1;
                                                }
                                                else if ( ent->client->resp.mk23_mode == 0 )
                                                {
                                                        bFire = 1;
                                                }
                                        }
                                        else
                                                bOut = 1;
                                        break;
                                        
                                }
                        case MP5_NUM:
                                {
                                        if ( ent->client->mp5_rds > 0 )
                                        {
                                                if ( ent->client->resp.mp5_mode != 0 && ent->client->fired == 0 && ent->client->burst == 0 )
                                                {
                                                        ent->client->fired = 1;
                                                        ent->client->ps.gunframe = 70;
                                                        ent->client->burst = 1;
                                                        ent->client->weaponstate = WEAPON_BURSTING;
                                                        
                                                        // start the animation
                                                        ent->client->anim_priority = ANIM_ATTACK;
                                                        if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                                                        {
                                                                ent->s.frame = FRAME_crattak1-1;
                                                                ent->client->anim_end = FRAME_crattak9;
                                                        }
                                                        else
                                                        {
                                                                ent->s.frame = FRAME_attack1-1;
                                                                ent->client->anim_end = FRAME_attack8;
                                                        }
                                                }
                                                else if ( ent->client->resp.mp5_mode == 0 && ent->client->fired == 0 )
                                                {
                                                        bFire = 1;
                                                }
                                        }
                                        else
                                                bOut = 1;
                                        break;
                                }
                        case M4_NUM:
                                {
                                        if ( ent->client->m4_rds > 0 )
                                        {
                                                if ( ent->client->resp.m4_mode != 0 && ent->client->fired == 0 && ent->client->burst == 0 )
                                                {
                                                        ent->client->fired = 1;
                                                        ent->client->ps.gunframe = 64;
                                                        ent->client->burst = 1;
                                                        ent->client->weaponstate = WEAPON_BURSTING;
                                                        
                                                        // start the animation
                                                        ent->client->anim_priority = ANIM_ATTACK;
                                                        if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                                                        {
                                                                ent->s.frame = FRAME_crattak1-1;
                                                                ent->client->anim_end = FRAME_crattak9;
                                                        }
                                                        else
                                                        {
                                                                ent->s.frame = FRAME_attack1-1;
                                                                ent->client->anim_end = FRAME_attack8;
                                                        }
                                                }
                                                else if ( ent->client->resp.m4_mode == 0 && ent->client->fired == 0 )
                                                {
                                                        bFire = 1;
                                                }
                                        }
                                        else
                                                bOut = 1;
                                        break;
                                }
                        case M3_NUM:
                                {
                                        if ( ent->client->shot_rds > 0 )
                                        {
                                                bFire = 1;
                                        }
                                        else
                                                bOut = 1;
                                        break;
                                }
                        case HC_NUM:
                                {
                                        if ( ent->client->cannon_rds == 2 )
                                        {
                                                bFire = 1;
                                        }
                                        else
                                                bOut = 1;
                                        break;
                                }
                        case SNIPER_NUM:
                                {
                                        if ( ent->client->ps.fov != ent->client->desired_fov)
                                                ent->client->ps.fov = ent->client->desired_fov;
                                        // if they aren't at 90 then they must be zoomed, so remove their weapon from view
                                        if ( ent->client->ps.fov != 90 )
                                        {
                                                ent->client->ps.gunindex = 0;
                                                ent->client->no_sniper_display = 0;
                                        }
                                        
                                        if ( ent->client->sniper_rds > 0 )
                                        {
                                                bFire = 1;
                                        }
                                        else
                                                bOut = 1;
                                        break;
                                }
                        case DUAL_NUM:
                                {
                                        if ( ent->client->dual_rds > 0 )
                                        {
                                                bFire = 1;
                                        }
                                        else
                                                bOut = 1;
                                        break;
                                }
                                
                                
                        default:
                                {
                                        gi.cprintf (ent, PRINT_HIGH, "Calling non specific ammo code\n");
                                        if ((!ent->client->ammo_index) || ( ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity))        
                                        {               
                                                bFire = 1;
                                        }
                                        else
                                        {
                                                bFire = 0;
                                                bOut = 1;
                                        }
                                }
                                
                        }
                        if ( bFire )
                        {
                                ent->client->ps.gunframe = FRAME_FIRE_FIRST;
                                ent->client->weaponstate = WEAPON_FIRING;
                                
                                // start the animation
                                ent->client->anim_priority = ANIM_ATTACK;
                                if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                                {
                                        ent->s.frame = FRAME_crattak1-1;
                                        ent->client->anim_end = FRAME_crattak9;
                                }
                                else
                                {
                                        ent->s.frame = FRAME_attack1-1;
                                        ent->client->anim_end = FRAME_attack8;
                                }
                        }
                        else if ( bOut ) // out of ammo
                        {
                                if (level.time >= ent->pain_debounce_time)
                                {
                                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                                        ent->pain_debounce_time = level.time + 1;
                                }
                                //+BD - Disabled for manual weapon change
                                //NoAmmoWeaponChange (ent);
                        }
                }
                else
                {
                        
                        if ( ent->client->ps.fov != ent->client->desired_fov)
                                ent->client->ps.fov = ent->client->desired_fov;
                        // if they aren't at 90 then they must be zoomed, so remove their weapon from view
                        if ( ent->client->ps.fov != 90 )
                        {
                                ent->client->ps.gunindex = 0;
                                ent->client->no_sniper_display = 0;
                        }
                        
                        if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
                        {
                                ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                                return;
                        }
                        
                 /*       if (pause_frames)
                        {
                                for (n = 0; pause_frames[n]; n++)
                                {
                                        if (ent->client->ps.gunframe == pause_frames[n])
                                        {
                                                if (rand()&15)
                                                        return;
                                        }
                                }
                        }
                   */     
                        ent->client->ps.gunframe++;
                        ent->client->fired = 0; // weapon ready and button not down, now they can fire again
                        ent->client->burst = 0;
                        ent->client->machinegun_shots = 0;
                        return;
                }
        }
        
        if (ent->client->weaponstate == WEAPON_FIRING  )
        {
                for (n = 0; fire_frames[n]; n++)
                {
                        if (ent->client->ps.gunframe == fire_frames[n])
                        {
                                if (ent->client->quad_framenum > level.framenum)                                                                gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
                                
                                fire (ent);
                                break;
                        }
                }
                
                if (!fire_frames[n])
                        ent->client->ps.gunframe++;
                
                if (ent->client->ps.gunframe == FRAME_IDLE_FIRST+1)
                        ent->client->weaponstate = WEAPON_READY;
        }
        // player switched into 
        if ( ent->client->weaponstate == WEAPON_BURSTING )
        {
                for (n = 0; fire_frames[n]; n++)
                {
                        if (ent->client->ps.gunframe == fire_frames[n])
                        {
                                if (ent->client->quad_framenum > level.framenum)                                                                gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
                                {
                //                      gi.cprintf (ent, PRINT_HIGH, "Calling fire code, frame = %d.\n", ent->client->ps.gunframe);
                                        fire (ent);
                                }
                                break;
                        }
                }
                
                if (!fire_frames[n])
                        ent->client->ps.gunframe++;
                
                //gi.cprintf (ent, PRINT_HIGH, "Calling stricmp, frame = %d.\n", ent->client->ps.gunframe);
                
                
                if (ent->client->ps.gunframe == FRAME_IDLE_FIRST+1)
                        ent->client->weaponstate = WEAPON_READY;

                if ( ent->client->curr_weap == MP5_NUM )
                {
                        if (ent->client->ps.gunframe >= 76)
                        {
                                ent->client->weaponstate = WEAPON_READY;
                                ent->client->ps.gunframe = FRAME_IDLE_FIRST+1;
                        }
                //      gi.cprintf (ent, PRINT_HIGH, "Succes stricmp now: frame = %d.\n", ent->client->ps.gunframe);

                }
                if ( ent->client->curr_weap == M4_NUM ) 
                {
                        if (ent->client->ps.gunframe >= 69)
                        {
                                ent->client->weaponstate = WEAPON_READY;
                                ent->client->ps.gunframe = FRAME_IDLE_FIRST+1;
                        }
                //      gi.cprintf (ent, PRINT_HIGH, "Succes stricmp now: frame = %d.\n", ent->client->ps.gunframe);

                }

        }
}




/*
======================================================================

GRENADE

======================================================================
*/

#define GRENADE_TIMER           3.0
#define GRENADE_MINSPEED        400
#define GRENADE_MAXSPEED        800

void weapon_grenade_fire (edict_t *ent, qboolean held)
{
        vec3_t  offset;
        vec3_t  forward, right;
        vec3_t  start;
        int             damage = 125;
        float   timer;
        int             speed;
        float   radius;

        radius = damage+40;
        if (is_quad)
                damage *= 4;

        VectorSet(offset, 8, 8, ent->viewheight-8);
        AngleVectors (ent->client->v_angle, forward, right, NULL);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

        timer = ent->client->grenade_time - level.time;
        speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);
        fire_grenade2 (ent, start, forward, GRENADE_DAMRAD, speed, timer, GRENADE_DAMRAD*2, held);

        if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
                ent->client->pers.inventory[ent->client->ammo_index]--;

        ent->client->grenade_time = level.time + 1.0;
}

void Weapon_Grenade (edict_t *ent)
{
        if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
        {
                ChangeWeapon (ent);
                return;
        }

        if (ent->client->weaponstate == WEAPON_ACTIVATING)
        {
                ent->client->weaponstate = WEAPON_READY;
                ent->client->ps.gunframe = 16;
                return;
        }

        if (ent->client->weaponstate == WEAPON_READY)
        {
                if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
                {
                        ent->client->latched_buttons &= ~BUTTON_ATTACK;
                        if (ent->client->pers.inventory[ent->client->ammo_index])
                        {
                                ent->client->ps.gunframe = 1;
                                ent->client->weaponstate = WEAPON_FIRING;
                                ent->client->grenade_time = 0;
                        }
                        else
                        {
                                if (level.time >= ent->pain_debounce_time)
                                {
                                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                                        ent->pain_debounce_time = level.time + 1;
                                }
                                NoAmmoWeaponChange (ent);
                        }
                        return;
                }

                if ((ent->client->ps.gunframe == 29) || (ent->client->ps.gunframe == 34) || (ent->client->ps.gunframe == 39) || (ent->client->ps.gunframe == 48))
                {
                        if (rand()&15)
                                return;
                }

                if (++ent->client->ps.gunframe > 48)
                        ent->client->ps.gunframe = 16;
                return;
        }

        if (ent->client->weaponstate == WEAPON_FIRING)
        {
                if (ent->client->ps.gunframe == 5)
                        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);

                if (ent->client->ps.gunframe == 11)
                {
                        if (!ent->client->grenade_time)
                        {
                                ent->client->grenade_time = level.time + GRENADE_TIMER + 0.2;
                                ent->client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
                                                                //ent->client->weapon_sound = gi.soundindex("weapons/grenlb1b.wav");
                        }

                        // they waited too long, detonate it in their hand
                        if (!ent->client->grenade_blew_up && level.time >= ent->client->grenade_time)
                        {
                                ent->client->weapon_sound = 0;
                                weapon_grenade_fire (ent, true);
                                ent->client->grenade_blew_up = true;
                        }

                        if (ent->client->buttons & BUTTON_ATTACK)
                                return;

                        if (ent->client->grenade_blew_up)
                        {
                                if (level.time >= ent->client->grenade_time)
                                {
                                        ent->client->ps.gunframe = 15;
                                        ent->client->grenade_blew_up = false;
                                }
                                else
                                {
                                        return;
                                }
                        }
                }

                if (ent->client->ps.gunframe == 12)
                {
                        ent->client->weapon_sound = 0;
                        weapon_grenade_fire (ent, false);
                }

                if ((ent->client->ps.gunframe == 15) && (level.time < ent->client->grenade_time))
                        return;

                ent->client->ps.gunframe++;

                if (ent->client->ps.gunframe == 16)
                {
                        ent->client->grenade_time = 0;
                        ent->client->weaponstate = WEAPON_READY;
                }
        }
}



/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire (edict_t *ent)
{
        vec3_t  offset;
        vec3_t  forward, right;
        vec3_t  start;
        int             damage = 120;
        float   radius;

        radius = damage+40;
        if (is_quad)
                damage *= 4;

        VectorSet(offset, 8, 8, ent->viewheight-8);
        AngleVectors (ent->client->v_angle, forward, right, NULL);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

        VectorScale (forward, -2, ent->client->kick_origin);
        ent->client->kick_angles[0] = -1;

        fire_grenade (ent, start, forward, damage, 600, 2.5, radius);

        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        gi.WriteByte (MZ_GRENADE | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);

        ent->client->ps.gunframe++;

        PlayerNoise(ent, start, PNOISE_WEAPON);

        if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
                ent->client->pers.inventory[ent->client->ammo_index]--;
}

void Weapon_GrenadeLauncher (edict_t *ent)
{
        static int      pause_frames[]  = {34, 51, 59, 0};
        static int      fire_frames[]   = {6, 0};

        Weapon_Generic (ent, 5, 16, 59, 64, 0, 0, pause_frames, fire_frames, weapon_grenadelauncher_fire);
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (edict_t *ent)
{
        vec3_t  offset, start;
        vec3_t  forward, right;
        int             damage;
        float   damage_radius;
        int             radius_damage;

        damage = 100 + (int)(random() * 20.0);
        radius_damage = 120;
        damage_radius = 120;
        if (is_quad)
        {
                damage *= 4;
                radius_damage *= 4;
        }

        AngleVectors (ent->client->v_angle, forward, right, NULL);

        VectorScale (forward, -2, ent->client->kick_origin);
        ent->client->kick_angles[0] = -1;

        VectorSet(offset, 8, 8, ent->viewheight-8);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
        fire_rocket (ent, start, forward, damage, 650, damage_radius, radius_damage);

        // send muzzle flash
        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        gi.WriteByte (MZ_ROCKET | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);

        ent->client->ps.gunframe++;

        PlayerNoise(ent, start, PNOISE_WEAPON);

        if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
                ent->client->pers.inventory[ent->client->ammo_index]--;
}

void Weapon_RocketLauncher (edict_t *ent)
{
        static int      pause_frames[]  = {25, 33, 42, 50, 0};
        static int      fire_frames[]   = {5, 0};

        Weapon_Generic (ent, 4, 12, 50, 54, 0,0, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
}


/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

void Blaster_Fire (edict_t *ent, vec3_t g_offset, int damage, qboolean hyper, int effect)
{
        vec3_t  forward, right;
        vec3_t  start;
        vec3_t  offset;

        if (is_quad)
                damage *= 4;
        AngleVectors (ent->client->v_angle, forward, right, NULL);
        VectorSet(offset, 24, 8, ent->viewheight-8);
        VectorAdd (offset, g_offset, offset);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

        VectorScale (forward, -2, ent->client->kick_origin);
        ent->client->kick_angles[0] = -1;

        fire_blaster (ent, start, forward, damage, 1000, effect, hyper);

        // send muzzle flash
        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        if (hyper)
                gi.WriteByte (MZ_HYPERBLASTER | is_silenced);
        else
                gi.WriteByte (MZ_BLASTER | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);

        PlayerNoise(ent, start, PNOISE_WEAPON);
}


void Weapon_Blaster_Fire (edict_t *ent)
{
        int             damage;

        if (deathmatch->value)
                damage = 15;
        else
                damage = 10;
        Blaster_Fire (ent, vec3_origin, damage, false, EF_BLASTER);
        ent->client->ps.gunframe++;
}

void Weapon_Blaster (edict_t *ent)
{
        static int      pause_frames[]  = {19, 32, 0};
        static int      fire_frames[]   = {5, 0};

        Weapon_Generic (ent, 4, 8, 52, 55, 0, 0, pause_frames, fire_frames, Weapon_Blaster_Fire);
}


void Weapon_HyperBlaster_Fire (edict_t *ent)
{
        float   rotation;
        vec3_t  offset;
        int             effect;
        int             damage;

        ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

        if (!(ent->client->buttons & BUTTON_ATTACK))
        {
                ent->client->ps.gunframe++;
        }
        else
        {
                if (! ent->client->pers.inventory[ent->client->ammo_index] )
                {
                        if (level.time >= ent->pain_debounce_time)
                        {
                                gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                                ent->pain_debounce_time = level.time + 1;
                        }
                        NoAmmoWeaponChange (ent);
                }
                else
                {
                        rotation = (ent->client->ps.gunframe - 5) * 2*M_PI/6;
                        offset[0] = -4 * sin(rotation);
                        offset[1] = 0;
                        offset[2] = 4 * cos(rotation);

                        if ((ent->client->ps.gunframe == 6) || (ent->client->ps.gunframe == 9))
                                effect = EF_HYPERBLASTER;
                        else
                                effect = 0;
                        if (deathmatch->value)
                                damage = 15;
                        else
                                damage = 20;
                        Blaster_Fire (ent, offset, damage, true, effect);
                        if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
                                ent->client->pers.inventory[ent->client->ammo_index]--;
                }

                ent->client->ps.gunframe++;
                if (ent->client->ps.gunframe == 12 && ent->client->pers.inventory[ent->client->ammo_index])
                        ent->client->ps.gunframe = 6;
        }

        if (ent->client->ps.gunframe == 12)
        {
                gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
                ent->client->weapon_sound = 0;
        }

}

void Weapon_HyperBlaster (edict_t *ent)
{
        static int      pause_frames[]  = {0};
        static int      fire_frames[]   = {6, 7, 8, 9, 10, 11, 0};

        Weapon_Generic (ent, 5, 20, 49, 53, 0, 0, pause_frames, fire_frames, Weapon_HyperBlaster_Fire);
}

/*
======================================================================

ACHINEGUN / CHAINGUN

======================================================================
*/

void Machinegun_Fire (edict_t *ent)
{
        int     i;
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          angles;
        int                     damage = 8;
        int                     kick = 2;
        vec3_t          offset;

        if (!(ent->client->buttons & BUTTON_ATTACK))
        {
                ent->client->machinegun_shots = 0;
                ent->client->ps.gunframe++;
                return;
        }

        if (ent->client->ps.gunframe == 5)
                ent->client->ps.gunframe = 4;
        else
                ent->client->ps.gunframe = 5;

        if (ent->client->pers.inventory[ent->client->ammo_index] < 1)
        {
                ent->client->ps.gunframe = 6;
                if (level.time >= ent->pain_debounce_time)
                {
                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                        ent->pain_debounce_time = level.time + 1;
                }
                NoAmmoWeaponChange (ent);
                return;
        }

        if (is_quad)
        {
                damage *= 4;
                kick *= 4;
        }

        for (i=1 ; i<3 ; i++)
        {
                ent->client->kick_origin[i] = crandom() * 0.35;
                ent->client->kick_angles[i] = crandom() * 0.7;
        }
        ent->client->kick_origin[0] = crandom() * 0.35;
        ent->client->kick_angles[0] = ent->client->machinegun_shots * -1.5;

        // raise the gun as it is firing
        if (!deathmatch->value)
        {
                ent->client->machinegun_shots++;
                if (ent->client->machinegun_shots > 9)
                        ent->client->machinegun_shots = 9;
        }

        // get start / end positions
        VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
        AngleVectors (angles, forward, right, NULL);
        VectorSet(offset, 0, 8, ent->viewheight-8);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
        fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);

        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        gi.WriteByte (MZ_MACHINEGUN | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);

        PlayerNoise(ent, start, PNOISE_WEAPON);

        if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
                ent->client->pers.inventory[ent->client->ammo_index]--;
}

void Weapon_Machinegun (edict_t *ent)
{
        static int      pause_frames[]  = {23, 45, 0};
        static int      fire_frames[]   = {4, 5, 0};

        Weapon_Generic (ent, 3, 5, 45, 49, 0, 0, pause_frames, fire_frames, Machinegun_Fire);
}

void Chaingun_Fire (edict_t *ent)
{
        int                     i;
        int                     shots;
        vec3_t          start;
        vec3_t          forward, right, up;
        float           r, u;
        vec3_t          offset;
        int                     damage;
        int                     kick = 2;

        if (deathmatch->value)
                damage = 6;
        else
                damage = 8;

        if (ent->client->ps.gunframe == 5)
                gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);

        if ((ent->client->ps.gunframe == 14) && !(ent->client->buttons & BUTTON_ATTACK))
        {
                ent->client->ps.gunframe = 32;
                ent->client->weapon_sound = 0;
                return;
        }
        else if ((ent->client->ps.gunframe == 21) && (ent->client->buttons & BUTTON_ATTACK)
                && ent->client->pers.inventory[ent->client->ammo_index])
        {
                ent->client->ps.gunframe = 15;
        }
        else
        {
                ent->client->ps.gunframe++;
        }

        if (ent->client->ps.gunframe == 22)
        {
                ent->client->weapon_sound = 0;
                gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
        }
        else
        {
                ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");
        }

        if (ent->client->ps.gunframe <= 9)
                shots = 1;
        else if (ent->client->ps.gunframe <= 14)
        {
                if (ent->client->buttons & BUTTON_ATTACK)
                        shots = 2;
                else
                        shots = 1;
        }
        else
                shots = 3;

        if (ent->client->pers.inventory[ent->client->ammo_index] < shots)
                shots = ent->client->pers.inventory[ent->client->ammo_index];

        if (!shots)
        {
                if (level.time >= ent->pain_debounce_time)
                {
                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                        ent->pain_debounce_time = level.time + 1;
                }
                NoAmmoWeaponChange (ent);
                return;
        }

        if (is_quad)
        {
                damage *= 4;
                kick *= 4;
        }

        for (i=0 ; i<3 ; i++)
        {
                ent->client->kick_origin[i] = crandom() * 0.35;
                ent->client->kick_angles[i] = crandom() * 0.7;
        }

        for (i=0 ; i<shots ; i++)
        {
                // get start / end positions
                AngleVectors (ent->client->v_angle, forward, right, up);
                r = 7 + crandom()*4;
                u = crandom()*4;
                VectorSet(offset, 0, r, u + ent->viewheight-8);
                P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

                fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
        }

        // send muzzle flash
        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        gi.WriteByte ((MZ_CHAINGUN1 + shots - 1) | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);

        PlayerNoise(ent, start, PNOISE_WEAPON);

        if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
                ent->client->pers.inventory[ent->client->ammo_index] -= shots;
}


void Weapon_Chaingun (edict_t *ent)
{
        static int      pause_frames[]  = {38, 43, 51, 61, 0};
        static int      fire_frames[]   = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0};

        Weapon_Generic (ent, 4, 31, 61, 64, 0, 0, pause_frames, fire_frames, Chaingun_Fire);
}


/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

void weapon_shotgun_fire (edict_t *ent)
{
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          offset;
        int                     damage = 4;
        int                     kick = 8;

        if (ent->client->ps.gunframe == 9)
        {
                ent->client->ps.gunframe++;
                return;
        }

        AngleVectors (ent->client->v_angle, forward, right, NULL);

        VectorScale (forward, -2, ent->client->kick_origin);
        ent->client->kick_angles[0] = -2;

        VectorSet(offset, 0, 8,  ent->viewheight-8);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

        if (is_quad)
        {
                damage *= 4;
                kick *= 4;
        }

        if (deathmatch->value)
                fire_shotgun (ent, start, forward, damage, kick, 500, 500, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, MOD_SHOTGUN);
        else
                fire_shotgun (ent, start, forward, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MOD_SHOTGUN);

        // send muzzle flash
        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        gi.WriteByte (MZ_SHOTGUN | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);

        ent->client->ps.gunframe++;
        PlayerNoise(ent, start, PNOISE_WEAPON);

        if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
                ent->client->pers.inventory[ent->client->ammo_index]--;
}

void Weapon_Shotgun (edict_t *ent)
{
        static int      pause_frames[]  = {22, 28, 34, 0};
        static int      fire_frames[]   = {8, 9, 0};

        Weapon_Generic (ent, 7, 18, 36, 39, 0, 0, pause_frames, fire_frames, weapon_shotgun_fire);
}


void weapon_supershotgun_fire (edict_t *ent)
{
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          offset;
        vec3_t          v;
        int                     damage = 6;
        int                     kick = 12;

        AngleVectors (ent->client->v_angle, forward, right, NULL);

        VectorScale (forward, -2, ent->client->kick_origin);
        ent->client->kick_angles[0] = -2;

        VectorSet(offset, 0, 8,  ent->viewheight-8);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

        if (is_quad)
        {
                damage *= 4;
                kick *= 4;
        }

        v[PITCH] = ent->client->v_angle[PITCH];
        v[YAW]   = ent->client->v_angle[YAW] - 5;
        v[ROLL]  = ent->client->v_angle[ROLL];
        AngleVectors (v, forward, NULL, NULL);
        fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT/2, MOD_SSHOTGUN);
        v[YAW]   = ent->client->v_angle[YAW] + 5;
        AngleVectors (v, forward, NULL, NULL);
        fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT/2, MOD_SSHOTGUN);

        // send muzzle flash
        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        gi.WriteByte (MZ_SSHOTGUN | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);

        ent->client->ps.gunframe++;
        PlayerNoise(ent, start, PNOISE_WEAPON);

        if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
                ent->client->pers.inventory[ent->client->ammo_index] -= 2;
}

void Weapon_SuperShotgun (edict_t *ent)
{
        static int      pause_frames[]  = {29, 42, 57, 0};
        static int      fire_frames[]   = {7, 0};

        Weapon_Generic (ent, 6, 17, 57, 61, 0, 0, pause_frames, fire_frames, weapon_supershotgun_fire);
}



/*
======================================================================

RAILGUN

======================================================================
*/

void weapon_railgun_fire (edict_t *ent)
{
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          offset;
        int                     damage;
        int                     kick;

        if (deathmatch->value)
        {       // normal damage is too extreme in dm
                damage = 100;
                kick = 200;
        }
        else
        {
                damage = 150;
                kick = 250;
        }

        if (is_quad)
        {
                damage *= 4;
                kick *= 4;
        }

        AngleVectors (ent->client->v_angle, forward, right, NULL);

        VectorScale (forward, -3, ent->client->kick_origin);
        ent->client->kick_angles[0] = -3;

        VectorSet(offset, 0, 7,  ent->viewheight-8);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
        fire_rail (ent, start, forward, damage, kick);

        // send muzzle flash
        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        gi.WriteByte (MZ_RAILGUN | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);

        ent->client->ps.gunframe++;
        PlayerNoise(ent, start, PNOISE_WEAPON);

        if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
                ent->client->pers.inventory[ent->client->ammo_index]--;
}


void Weapon_Railgun (edict_t *ent)
{
        static int      pause_frames[]  = {56, 0};
        static int      fire_frames[]   = {4, 0};

        Weapon_Generic (ent, 3, 18, 56, 61, 0, 0, pause_frames, fire_frames, weapon_railgun_fire);
}


/*
======================================================================

BFG10K

======================================================================
*/

void weapon_bfg_fire (edict_t *ent)
{
        vec3_t  offset, start;
        vec3_t  forward, right;
        int             damage;
        float   damage_radius = 1000;

        if (deathmatch->value)
                damage = 200;
        else
                damage = 500;

        if (ent->client->ps.gunframe == 9)
        {
                // send muzzle flash
                gi.WriteByte (svc_muzzleflash);
                gi.WriteShort (ent-g_edicts);
                gi.WriteByte (MZ_BFG | is_silenced);
                gi.multicast (ent->s.origin, MULTICAST_PVS);

                ent->client->ps.gunframe++;

                PlayerNoise(ent, start, PNOISE_WEAPON);
                return;
        }

        // cells can go down during windup (from power armor hits), so
        // check again and abort firing if we don't have enough now
        if (ent->client->pers.inventory[ent->client->ammo_index] < 50)
        {
                ent->client->ps.gunframe++;
                return;
        }

        if (is_quad)
                damage *= 4;

        AngleVectors (ent->client->v_angle, forward, right, NULL);

        VectorScale (forward, -2, ent->client->kick_origin);

        // make a big pitch kick with an inverse fall
        ent->client->v_dmg_pitch = -40;
        ent->client->v_dmg_roll = crandom()*8;
        ent->client->v_dmg_time = level.time + DAMAGE_TIME;

        VectorSet(offset, 8, 8, ent->viewheight-8);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
        fire_bfg (ent, start, forward, damage, 400, damage_radius);

        ent->client->ps.gunframe++;

        PlayerNoise(ent, start, PNOISE_WEAPON);

        if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
                ent->client->pers.inventory[ent->client->ammo_index] -= 50;
}

void Weapon_BFG (edict_t *ent)
{
        static int      pause_frames[]  = {39, 45, 50, 55, 0};
        static int      fire_frames[]   = {9, 17, 0};

        Weapon_Generic (ent, 8, 32, 55, 58, 0, 0, pause_frames, fire_frames, weapon_bfg_fire);
}




#define MK23_SPREAD     140
#define MP5_SPREAD 250
#define M4_SPREAD 300
#define SNIPER_SPREAD 425
#define DUAL_SPREAD     300

int AdjustSpread( edict_t * ent, int spread )
{
        
        int running = 225; // minimum speed for running
        int walking = 10; // minimum speed for walking
        int laser = 0;
        float factor[] = {.7, 1, 2, 6};
        int stage = 0;

        // 225 is running
        // < 10 will be standing
        float xyspeed = (ent->velocity[0]*ent->velocity[0] + ent->velocity[1]*ent->velocity[1]);
        
        if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) // crouching
                return( spread * .65);

        if ( (ent->client->pers.inventory[ITEM_INDEX(FindItem(LASER_NAME))]) 
                        && (ent->client->curr_weap == MK23_NUM
                        || ent->client->curr_weap == MP5_NUM
                        || ent->client->curr_weap == M4_NUM ) )
                laser = 1;


        // running
        if ( xyspeed > running*running )
                stage = 3;
        // walking
        else if ( xyspeed >= walking*walking )
                stage = 2;
        // standing
        else
                stage = 1;
        
        // laser advantage
        if (laser)
        {
                        if (stage == 1)
                                stage = 0;
                        else
                                stage = 1;
        }
                
        return (int)(spread * factor[stage]);
}



//======================================================================


//======================================================================
// mk23 derived from tutorial by GreyBear

void Pistol_Fire(edict_t *ent)
{
        int     i;
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          angles;
        int             damage = 90;
        int             kick = 150;
        vec3_t          offset; 
        int             spread = MK23_SPREAD;
                int                             height;




                if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
                {
                        height = 8;
                }
                else
                        height = 0;
        
        //If the user isn't pressing the attack button, advance the frame and go away....
        if (!(ent->client->buttons & BUTTON_ATTACK))
        {
                ent->client->ps.gunframe++;
                return;
        }
        ent->client->ps.gunframe++;     
        
        //Oops! Out of ammo!
        if (ent->client->mk23_rds < 1)
        {
                ent->client->ps.gunframe = 13;
                if (level.time >= ent->pain_debounce_time)
                {
                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"),1, ATTN_NORM, 0);
                        ent->pain_debounce_time = level.time + 1;
                }
                //NoAmmoWeaponChange (ent);
                return;
        }
        
        
        //Calculate the kick angles
        for (i=1 ; i<3 ; i++)
        {
                ent->client->kick_origin[i] = crandom() * 0.35;
                ent->client->kick_angles[i] = crandom() * 0.7;
        }
        ent->client->kick_origin[0] = crandom() * 0.35;
        ent->client->kick_angles[0] = ent->client->machinegun_shots * -1.5;
        
        // get start / end positions
        VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
        AngleVectors (angles, forward, right, NULL);
        VectorSet(offset, 0, 8, ent->viewheight-height);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);    
                if (!sv_shelloff->value)
        {
                        vec3_t result;
                        Old_ProjectSource (ent->client, ent->s.origin, offset, forward, right, result);    
                        EjectShell(ent, result, 0);
                }

        spread = AdjustSpread( ent, spread );
        
        if ( ent->client->resp.mk23_mode )
                spread *= .7;

//      gi.cprintf(ent, PRINT_HIGH, "Spread is %d\n", spread);

        if ((ent->client->mk23_rds == 1))
        {
                //Hard coded for reload only.
                ent->client->ps.gunframe=62;
                ent->client->weaponstate = WEAPON_END_MAG;
                fire_bullet (ent, start, forward, damage, kick, spread, spread,MOD_MK23);
                ent->client->mk23_rds--;
                ent->client->dual_rds--;
        }
        else
                
        {
                //If no reload, fire normally.
                fire_bullet (ent, start, forward, damage, kick, spread, spread,MOD_MK23);
                ent->client->mk23_rds--;
                ent->client->dual_rds--;
        }
        
        // silencer suppresses both sound and muzzle flash 
        if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(SIL_NAME))] )
        {
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("misc/silencer.wav"), 1, ATTN_NORM, 0);        
        }
        else
        {
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23fire.wav"), 1, ATTN_NORM, 0);
                
                
                //Display the yellow muzzleflash light effect
                gi.WriteByte (svc_muzzleflash);
                gi.WriteShort (ent-g_edicts);
                //If not silenced, play a shot sound for everyone else
                gi.WriteByte (MZ_MACHINEGUN | is_silenced);
                gi.multicast (ent->s.origin, MULTICAST_PVS);
                PlayerNoise(ent, start, PNOISE_WEAPON);
        }       
}

void Weapon_MK23 (edict_t *ent)
{
        //Idle animation entry points - These make the fidgeting look more random
        static int      pause_frames[]  = {13, 22, 40};
        //The frames at which the weapon will fire
        static int      fire_frames[]   = {10, 0};
        
        //The call is made...
        Weapon_Generic (ent, 9, 12, 37, 40, 61, 65, pause_frames, fire_frames, Pistol_Fire);
}


void MP5_Fire(edict_t *ent)
{
        int     i;
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          angles;
        int             damage = 55;
        int             kick = 90;
        vec3_t          offset; 
        int                             spread = MP5_SPREAD;    
                int                             height;

                if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
                        height = 8;
                else
                        height = 0;
        
                
        //If the user isn't pressing the attack button, advance the frame and go away....
        if (!(ent->client->buttons & BUTTON_ATTACK) && !(ent->client->burst) )
        {
                ent->client->ps.gunframe++;
                return;
        }

        if ( ent->client->burst == 0 && !(ent->client->resp.mp5_mode) )
        {
                if (ent->client->ps.gunframe == 12)
                        ent->client->ps.gunframe = 11;
                else
                        ent->client->ps.gunframe = 12;
        }
        //burst mode
        else if ( ent->client->burst == 0 && ent->client->resp.mp5_mode )
        {
                ent->client->ps.gunframe = 72;
                ent->client->weaponstate = WEAPON_BURSTING;
                ent->client->burst = 1;
                ent->client->fired = 1;
        }

        else if ( ent->client->ps.gunframe >= 70 && ent->client->ps.gunframe <= 75 )
        {
                ent->client->ps.gunframe++;     
                
        }


        //Oops! Out of ammo!
        if (ent->client->mp5_rds < 1)
        {
                ent->client->ps.gunframe = 13;
                if (level.time >= ent->pain_debounce_time)
                {
                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"),1, ATTN_NORM, 0);
                        ent->pain_debounce_time = level.time + 1;
                }
                //NoAmmoWeaponChange (ent);
                return;
        }
        

        spread = AdjustSpread( ent, spread );
        if ( ent->client->burst )
                spread *= .7;


        //Calculate the kick angles
        for (i=1 ; i<3 ; i++)
        {
                ent->client->kick_origin[i] = crandom() * 0.25;
                ent->client->kick_angles[i] = crandom() * 0.5;
        }
        ent->client->kick_origin[0] = crandom() * 0.35;
        ent->client->kick_angles[0] = ent->client->machinegun_shots * -1.5;
        
        // get start / end positions
        VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
        AngleVectors (angles, forward, right, NULL);
        VectorSet(offset, 0, 8, ent->viewheight-height);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);    
        
                fire_bullet (ent, start, forward, damage, kick, spread, spread,MOD_MP5);
        ent->client->mp5_rds--;
        
                if (!sv_shelloff->value)
        {
                        vec3_t result;
                        Old_ProjectSource (ent->client, ent->s.origin, offset, forward, right, result);    
                        EjectShell(ent, result, 0);
                }



        // zucc vwep

        ent->client->anim_priority = ANIM_ATTACK;
        if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
        {
                ent->s.frame = FRAME_crattak1 - (int) (random()+0.25);
                ent->client->anim_end = FRAME_crattak9;
        }
        else
        {
                ent->s.frame = FRAME_attack1 - (int) (random()+0.25);
                ent->client->anim_end = FRAME_attack8;
        }

        // zucc vwep done

        // silencer suppresses both sound and muzzle flash 
        if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(SIL_NAME))] )
        {
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("misc/silencer.wav"), 1, ATTN_NORM, 0);        
        }
        else
        {       
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5fire1.wav"), 1, ATTN_NORM, 0);
                
                
                //Display the yellow muzzleflash light effect
                gi.WriteByte (svc_muzzleflash);
                gi.WriteShort (ent-g_edicts);
                //If not silenced, play a shot sound for everyone else
                gi.WriteByte (MZ_MACHINEGUN | is_silenced);
                gi.multicast (ent->s.origin, MULTICAST_PVS);
                PlayerNoise(ent, start, PNOISE_WEAPON);
        }               
}

void Weapon_MP5 (edict_t *ent)
{
        //Idle animation entry points - These make the fidgeting look more random
        static int      pause_frames[]  = {13, 30, 47};
        //The frames at which the weapon will fire
        static int      fire_frames[]   = {11, 12, 71, 72, 73, 0};
        
        //The call is made...
        Weapon_Generic (ent, 10, 12, 47, 51, 69, 77, pause_frames, fire_frames, MP5_Fire);
}

void M4_Fire(edict_t *ent)
{
        int                         i;
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          angles;
        int             damage = 90;
        int             kick = 90;
        vec3_t          offset; 
        int             spread = M4_SPREAD;     
        int                             height;

                if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
                        height = 8;
                else
                        height = 0;
        
        //If the user isn't pressing the attack button, advance the frame and go away....
        if (!(ent->client->buttons & BUTTON_ATTACK) && !(ent->client->burst) )
        {
                ent->client->ps.gunframe++;
                ent->client->machinegun_shots = 0;
                return;
        }
        
        if ( ent->client->burst == 0 && !(ent->client->resp.m4_mode) )
        {
                if (ent->client->ps.gunframe == 12)
                        ent->client->ps.gunframe = 11;
                else
                        ent->client->ps.gunframe = 12;
        }
        //burst mode
        else if ( ent->client->burst == 0 && ent->client->resp.m4_mode )
        {
                ent->client->ps.gunframe = 66;
                ent->client->weaponstate = WEAPON_BURSTING;
                ent->client->burst = 1;
                ent->client->fired = 1;
        }
        
        else if ( ent->client->ps.gunframe >= 64 && ent->client->ps.gunframe <= 69 )
        {
                ent->client->ps.gunframe++;     
                
        }
        
        
        //Oops! Out of ammo!
        if (ent->client->m4_rds < 1)
        {
                ent->client->ps.gunframe = 13;
                if (level.time >= ent->pain_debounce_time)
                {
                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"),1, ATTN_NORM, 0);
                        ent->pain_debounce_time = level.time + 1;
                }
                //NoAmmoWeaponChange (ent);
                return;
        }
        
        // causes the ride up
        if ( ent->client->weaponstate != WEAPON_BURSTING )
        {
                ent->client->machinegun_shots++;
                if (ent->client->machinegun_shots > 23)
                        ent->client->machinegun_shots = 23;
        }
        else // no kick when in burst mode
        {
                ent->client->machinegun_shots = 0;
        }
        
        
        spread = AdjustSpread( ent, spread );
        if ( ent->client->burst )
                spread *= .7;
        
        
        //      gi.cprintf(ent, PRINT_HIGH, "Spread is %d\n", spread);
        
        
        //Calculate the kick angles
        for (i=1 ; i<3 ; i++)
        {
                ent->client->kick_origin[i] = crandom() * 0.25;
                ent->client->kick_angles[i] = crandom() * 0.5;
        }
        ent->client->kick_origin[0] = crandom() * 0.35;
        ent->client->kick_angles[0] = ent->client->machinegun_shots * -.7;
        
        // get start / end positions
        VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
        AngleVectors (angles, forward, right, NULL);
        VectorSet(offset, 0, 8, ent->viewheight-height);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);    
        
                fire_bullet_sparks (ent, start, forward, damage, kick, spread, spread,MOD_M4);
                ent->client->m4_rds--;
        
        if (!sv_shelloff->value)
        {
                        vec3_t result;
                        Old_ProjectSource (ent->client, ent->s.origin, offset, forward, right, result);    
                        EjectShell(ent, result, 0);
                }

        
        // zucc vwep
        
        ent->client->anim_priority = ANIM_ATTACK;
        if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
        {
                ent->s.frame = FRAME_crattak1 - (int) (random()+0.25);
                ent->client->anim_end = FRAME_crattak9;
        }
        else
        {
                ent->s.frame = FRAME_attack1 - (int) (random()+0.25);
                ent->client->anim_end = FRAME_attack8;
        }
        
        // zucc vwep done
        
        // silencer suppresses both sound and muzzle flash 
        /*      if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(SIL_NAME))] )
        {
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("misc/silencer.wav"), 1, ATTN_NORM, 0);        
        }
        else
        */
        {
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/m4a1fire.wav"), 1, ATTN_NORM, 0);
                
                
                //Display the yellow muzzleflash light effect
                gi.WriteByte (svc_muzzleflash);
                gi.WriteShort (ent-g_edicts);
                //If not silenced, play a shot sound for everyone else
                gi.WriteByte (MZ_MACHINEGUN | is_silenced);
                gi.multicast (ent->s.origin, MULTICAST_PVS);
                PlayerNoise(ent, start, PNOISE_WEAPON);
        }       
}

void Weapon_M4 (edict_t *ent)
{
        //Idle animation entry points - These make the fidgeting look more random
        static int      pause_frames[]  = {13, 24, 39};
        //The frames at which the weapon will fire
        static int      fire_frames[]   = {11, 12, 65, 66, 67, 0};
        
        //The call is made...
        Weapon_Generic (ent, 10, 12, 39, 44, 63, 71, pause_frames, fire_frames, M4_Fire);
}


void InitShotgunDamageReport();
void ProduceShotgunDamageReport(edict_t *);

void M3_Fire (edict_t *ent)
{
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          offset;
        int                     damage = 17; //actionquake is 15 standard
        int                     kick = 20;
                int                             height;

                if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
                        height = 8;
                else
                        height = 0;
        
        if (ent->client->ps.gunframe == 9)
        {
                ent->client->ps.gunframe++;
                return;
        }

                
                AngleVectors (ent->client->v_angle, forward, right, NULL);

        VectorScale (forward, -2, ent->client->kick_origin);
        ent->client->kick_angles[0] = -2;

        VectorSet(offset, 0, 8,  ent->viewheight-height);
        
                
                
                if ( ent->client->ps.gunframe == 14 )
                {
                        if (!sv_shelloff->value)
                        {
                                vec3_t result;
                                Old_ProjectSource (ent->client, ent->s.origin, offset, forward, right, result);    
                                EjectShell(ent, result, 0);
            }
                        ent->client->ps.gunframe++;
                        return;
                }



        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

/*      if (is_quad)
        {
                damage *= 4;
                kick *= 4;
        }*/


	setFFState(ent);
        InitShotgunDamageReport();  //FB 6/3/99
        if (deathmatch->value)
                fire_shotgun (ent, start, forward, damage, kick, 800, 800, 12/*DEFAULT_DEATHMATCH_SHOTGUN_COUNT*/, MOD_M3);
        else
                fire_shotgun (ent, start, forward, damage, kick, 800, 800, 12/*DEFAULT_SHOTGUN_COUNT*/, MOD_M3);
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/shotgf1b.wav"), 1, ATTN_NORM, 0);
        // send muzzle flash
        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        gi.WriteByte (MZ_SHOTGUN | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);
                
                ProduceShotgunDamageReport(ent);  //FB 6/3/99

        ent->client->ps.gunframe++;
        PlayerNoise(ent, start, PNOISE_WEAPON);

        //if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
        //      ent->client->pers.inventory[ent->client->ammo_index]--;
        ent->client->shot_rds--;

}

void Weapon_M3 (edict_t *ent)
{
        static int      pause_frames[]  = {22, 28, 0};
        static int      fire_frames[]   = {8, 9, 14, 0};

        Weapon_Generic (ent, 7, 15, 35, 41, 52, 60, pause_frames, fire_frames, M3_Fire);
}


void HC_Fire (edict_t *ent)
{
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          offset;
        vec3_t          v;
        int                     damage = 20;
        int                     kick = 40;
                int                             height;

                if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
                        height = 8;
                else
                        height = 0;
        
        AngleVectors (ent->client->v_angle, forward, right, NULL);

        VectorScale (forward, -2, ent->client->kick_origin);
        ent->client->kick_angles[0] = -2;

        VectorSet(offset, 0, 8,  ent->viewheight-height);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

        /*if (is_quad)
        {
                damage *= 4;
                kick *= 4;
        }*/

        v[PITCH] = ent->client->v_angle[PITCH];
        v[YAW]   = ent->client->v_angle[YAW] - 5;
        v[ROLL]  = ent->client->v_angle[ROLL];
        AngleVectors (v, forward, NULL, NULL);
        // default hspread is 1k and default vspread is 500
	setFFState(ent);
        InitShotgunDamageReport();  //FB 6/3/99
        fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD*4, DEFAULT_SHOTGUN_VSPREAD*4, 34/2, MOD_HC);
        v[YAW]   = ent->client->v_angle[YAW] + 5;
        AngleVectors (v, forward, NULL, NULL);
        fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD*4, DEFAULT_SHOTGUN_VSPREAD*5, 34/2, MOD_HC);
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/cannon_fire.wav"), 1, ATTN_NORM, 0);
        // send muzzle flash
        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        gi.WriteByte (MZ_SSHOTGUN | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);
        ProduceShotgunDamageReport(ent);  //FB 6/3/99

        ent->client->ps.gunframe++;
        PlayerNoise(ent, start, PNOISE_WEAPON);

//      if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
//              ent->client->pers.inventory[ent->client->ammo_index] -= 2;

        ent->client->cannon_rds -= 2;
}

void Weapon_HC (edict_t *ent)
{
        static int      pause_frames[]  = {29, 42, 57, 0};
        static int      fire_frames[]   = {7, 0};

        Weapon_Generic (ent, 6, 17, 57, 61, 82, 83, pause_frames, fire_frames, HC_Fire);
}

void Sniper_Fire(edict_t *ent)
{
        int     i;
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          angles;
        int             damage = 250;
        int             kick = 200;
        vec3_t          offset; 
        int             spread = SNIPER_SPREAD;


                
        
        if ( ent->client->ps.gunframe == 13 )
        {
                        gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/ssgbolt.wav"), 1, ATTN_NORM, 0);
                        ent->client->ps.gunframe++;
                        
                        
                        AngleVectors (ent->client->v_angle, forward, right, NULL);
                        VectorSet(offset, 0, 0, ent->viewheight-0);
                        
                        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);    
                        
                        EjectShell(ent, start, 0);
                        return;
        }
        
        if ( ent->client->ps.gunframe == 21 )
        {
                if ( ent->client->ps.fov != ent->client->desired_fov)
                        ent->client->ps.fov = ent->client->desired_fov;
                // if they aren't at 90 then they must be zoomed, so remove their weapon from view
                if ( ent->client->ps.fov != 90 )
                {
                        ent->client->ps.gunindex = 0;
                        ent->client->no_sniper_display = 0;
                }
                ent->client->ps.gunframe++;

                return;
        }
        
        
        //If the user isn't pressing the attack button, advance the frame and go away....
        if (!(ent->client->buttons & BUTTON_ATTACK))
        {
                ent->client->ps.gunframe++;
                return;
        }
        ent->client->ps.gunframe++;     
        
        //Oops! Out of ammo!
        if (ent->client->sniper_rds < 1)
        {
                ent->client->ps.gunframe = 22;
                if (level.time >= ent->pain_debounce_time)
                {
                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"),1, ATTN_NORM, 0);
                        ent->pain_debounce_time = level.time + 1;
                }
                //NoAmmoWeaponChange (ent);
                return;
        }

        spread = AdjustSpread( ent, spread );
        if ( ent->client->resp.sniper_mode == SNIPER_2X )
                spread = 0;
        else if ( ent->client->resp.sniper_mode == SNIPER_4X )
                spread = 0;
        else if ( ent->client->resp.sniper_mode == SNIPER_6X )
                spread = 0; 


        
        //Calculate the kick angles
        for (i=1 ; i<3 ; i++)
        {
                ent->client->kick_origin[i] = crandom() * 0;
                ent->client->kick_angles[i] = crandom() * 0;
        }
        ent->client->kick_origin[0] = crandom() * 0;
        ent->client->kick_angles[0] = ent->client->machinegun_shots * 0;
        
        // get start / end positions
        VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
        AngleVectors (ent->client->v_angle, forward, right, NULL);
        VectorSet(offset, 0, 0, ent->viewheight-0);
        
                P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);    
        
        
        {
                //If no reload, fire normally.
                fire_bullet_sniper (ent, start, forward, damage, kick, spread, spread,MOD_SNIPER);
                ent->client->sniper_rds--;
                ent->client->ps.fov = 90; // so we can watch the next round get chambered
                ent->client->ps.gunindex = gi.modelindex( ent->client->pers.weapon->view_model );
                ent->client->no_sniper_display = 1;
        }
                
        
        
        // silencer suppresses both sound and muzzle flash 
        if ( ent->client->pers.inventory[ITEM_INDEX(FindItem(SIL_NAME))] )
        {
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("misc/silencer.wav"), 1, ATTN_NORM, 0);        
        }
        else
        {
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/ssgfire.wav"), 1, ATTN_NORM, 0);
                //Display the yellow muzzleflash light effect
                gi.WriteByte (svc_muzzleflash);
                gi.WriteShort (ent-g_edicts);
                //If not silenced, play a shot sound for everyone else
                gi.WriteByte (MZ_MACHINEGUN | is_silenced);
                gi.multicast (ent->s.origin, MULTICAST_PVS);
                PlayerNoise(ent, start, PNOISE_WEAPON);
        }       
}

void Weapon_Sniper (edict_t *ent)
{
        //Idle animation entry points - These make the fidgeting look more random
        static int      pause_frames[]  = {21, 40};
        //The frames at which the weapon will fire
        static int      fire_frames[]   = {9, 13, 21, 0};
        
        //The call is made...
        Weapon_Generic (ent, 8, 21, 41, 50, 81, 95, pause_frames, fire_frames, Sniper_Fire);
}

void Dual_Fire(edict_t *ent)
{
        int     i;
        vec3_t          start;
        vec3_t          forward, right;
        vec3_t          angles;
        int             damage = 90;
        int             kick = 90;
        vec3_t          offset; 
        int                             spread = DUAL_SPREAD;
        int                             height;

        if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
                height = 8;
        else
                height = 0;
        
        
        spread = AdjustSpread( ent, spread );
        
        if (ent->client->dual_rds < 1)
        {
                ent->client->ps.gunframe = 68;
                if (level.time >= ent->pain_debounce_time)
                {
                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"),1, ATTN_NORM, 0);
                        ent->pain_debounce_time = level.time + 1;
                }
                //NoAmmoWeaponChange (ent);
                return;
        }
        
                                
                                
        //If the user isn't pressing the attack button, advance the frame and go away....
        if ( ent->client->ps.gunframe == 8 )
        {
                //gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23fire.wav"), 1, ATTN_NORM, 0);
                ent->client->ps.gunframe++;
                
                VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
                AngleVectors (angles, forward, right, NULL);

                VectorSet(offset, 0, 8, ent->viewheight-height);
                P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);    
                if ( ent->client->dual_rds > 1 )
                {
                
                        fire_bullet (ent, start, forward, damage, kick, spread, spread,MOD_DUAL);
                        
                        if (!sv_shelloff->value)
                        {
                                vec3_t result;
                                Old_ProjectSource (ent->client, ent->s.origin, offset, forward, right, result);    
                                EjectShell(ent, result, 2);
                        }
                        
                        if ( ent->client->dual_rds > ent->client->mk23_max + 1 )
                        {
                                ent->client->dual_rds -= 2;
                        }
                        else if ( ent->client->dual_rds > ent->client->mk23_max ) // 13 rounds left
                        {
                                ent->client->dual_rds -= 2;
                                ent->client->mk23_rds--;
                        }
                        else
                        {
                                ent->client->dual_rds -= 2;
                                ent->client->mk23_rds -= 2;
                        }
                        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23fire.wav"), 1, ATTN_NORM, 0);
                        
                        if (ent->client->dual_rds == 0)
                        {
                ent->client->ps.gunframe=68;
                ent->client->weaponstate = WEAPON_END_MAG;
                                
                        }
                        
                        
                }
                else
                {
                        ent->client->dual_rds = 0;
                        ent->client->mk23_rds = 0;
                        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/noammo.wav"),1, ATTN_NORM, 0);
                        //ent->pain_debounce_time = level.time + 1;
                        ent->client->ps.gunframe=68;
                        ent->client->weaponstate = WEAPON_END_MAG;
                        
                }
                return;
        }
        
        if ( ent->client->ps.gunframe == 9 )
        {
                ent->client->ps.gunframe += 2;
        
                return;
        }
        
        
        /*if (!(ent->client->buttons & BUTTON_ATTACK))
        {
        ent->client->ps.gunframe++;
        return;
}*/
        ent->client->ps.gunframe++;     
        
        //Oops! Out of ammo!
        if (ent->client->dual_rds < 1)
        {
                ent->client->ps.gunframe = 12;
                if (level.time >= ent->pain_debounce_time)
                {
                        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"),1, ATTN_NORM, 0);
                        ent->pain_debounce_time = level.time + 1;
                }
                //NoAmmoWeaponChange (ent);
                return;
        }
        
        
        //Calculate the kick angles
        for (i=1 ; i<3 ; i++)
        {
                ent->client->kick_origin[i] = crandom() * 0.25;
                ent->client->kick_angles[i] = crandom() * 0.5;
        }
        ent->client->kick_origin[0] = crandom() * 0.35;
        
        // get start / end positions
        VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
        AngleVectors (angles, forward, right, NULL);
        // first set up for left firing
        VectorSet(offset, 0, -20, ent->viewheight-height);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);    
        
        if (!sv_shelloff->value)
        {
                vec3_t result;
                Old_ProjectSource (ent->client, ent->s.origin, offset, forward, right, result);    
                EjectShell(ent, result, 1);
        }
        
        
        
        //If no reload, fire normally.
        fire_bullet (ent, start, forward, damage, kick, spread, spread,MOD_DUAL);
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23fire.wav"), 1, ATTN_NORM, 0);             
        
        
        
        //gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23fire.wav"), 1, ATTN_NORM, 0);
        //Display the yellow muzzleflash light effect
        gi.WriteByte (svc_muzzleflash);
        gi.WriteShort (ent-g_edicts);
        //If not silenced, play a shot sound for everyone else
        gi.WriteByte (MZ_MACHINEGUN | is_silenced);
        gi.multicast (ent->s.origin, MULTICAST_PVS);
        PlayerNoise(ent, start, PNOISE_WEAPON);
        
        
}

void Weapon_Dual (edict_t *ent)
{
        //Idle animation entry points - These make the fidgeting look more random
        static int      pause_frames[]  = {13, 22, 32};
        //The frames at which the weapon will fire
        static int      fire_frames[]   = {7,8,9, 0};
        
        //The call is made...
        Weapon_Generic (ent, 6, 10, 32, 40, 65, 68, pause_frames, fire_frames, Dual_Fire);
}



//zucc 

#define FRAME_PREPARETHROW_FIRST        (FRAME_DEACTIVATE_LAST +1)
#define FRAME_IDLE2_FIRST                       (FRAME_PREPARETHROW_LAST +1)
#define FRAME_THROW_FIRST                       (FRAME_IDLE2_LAST +1)
#define FRAME_STOPTHROW_FIRST           (FRAME_THROW_LAST +1)
#define FRAME_NEWKNIFE_FIRST            (FRAME_STOPTHROW_LAST +1)

void Weapon_Generic_Knife(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, 
                                         int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, 
                                         int FRAME_PREPARETHROW_LAST, int FRAME_IDLE2_LAST,
                                         int FRAME_THROW_LAST, int FRAME_STOPTHROW_LAST,
                                         int FRAME_NEWKNIFE_LAST,
                                         int *pause_frames, int *fire_frames, int (*fire)(edict_t *ent))
{
                
        if(ent->s.modelindex != 255) // zucc vwep
                return; // not on client, so VWep animations could do wacky things

//FIREBLADE
        if (ent->client->weaponstate == WEAPON_FIRING &&
                ((ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD) || lights_camera_action))
        {
                ent->client->weaponstate = WEAPON_READY;
        }
//FIREBLADE
        
        if( ent->client->weaponstate == WEAPON_RELOADING)
        {
                if(ent->client->ps.gunframe < FRAME_NEWKNIFE_LAST)
                {
                        ent->client->ps.gunframe++;             
                }
                else
                {
                        ent->client->ps.gunframe = FRAME_IDLE2_FIRST;
                        ent->client->weaponstate = WEAPON_READY;
                }
        }
        
        if (ent->client->weaponstate == WEAPON_DROPPING)
        {
                if (ent->client->resp.knife_mode == 1)
                {
                        if (ent->client->ps.gunframe == FRAME_NEWKNIFE_FIRST)
                        {
                                ChangeWeapon (ent);
                                return;
                        }
                        else
                        {
                                // zucc going to have to do this a bit different because
                                // of the way I roll gunframes backwards for the thrownknife position
                                if((ent->client->ps.gunframe - FRAME_NEWKNIFE_FIRST) == 4)
                                {
                                        ent->client->anim_priority = ANIM_REVERSE;
                                        if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                                        {
                                                ent->s.frame = FRAME_crpain4+1;
                                                ent->client->anim_end = FRAME_crpain1;
                                        }
                                        else
                                        {
                                                ent->s.frame = FRAME_pain304+1;
                                                ent->client->anim_end = FRAME_pain301;
                                                
                                        }
                                }
                                ent->client->ps.gunframe--;
                        }                               
                                
                }
                else
                {
                        if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
                        {
                                ChangeWeapon (ent);
                                return;
                        }
                        else if((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4)
                        {
                                ent->client->anim_priority = ANIM_REVERSE;
                                if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                                {
                                        ent->s.frame = FRAME_crpain4+1;
                                        ent->client->anim_end = FRAME_crpain1;
                                }
                                else
                                {
                                        ent->s.frame = FRAME_pain304+1;
                                        ent->client->anim_end = FRAME_pain301;
                                        
                                }
                        }

                        

                        ent->client->ps.gunframe++;
                }
                return;
        }
        if (ent->client->weaponstate == WEAPON_ACTIVATING)
        {
                
                if ( ent->client->resp.knife_mode == 1 && ent->client->ps.gunframe == 0 )
                {
//                      gi.cprintf(ent, PRINT_HIGH, "NewKnifeFirst\n");
                        ent->client->ps.gunframe = FRAME_PREPARETHROW_FIRST;
                        return;
                }
                if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST 
                        || ent->client->ps.gunframe == FRAME_STOPTHROW_LAST )   
                {
                        ent->client->weaponstate = WEAPON_READY;
                        ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                        return;
                }
                if (ent->client->ps.gunframe == FRAME_PREPARETHROW_LAST)
                {
                        ent->client->weaponstate = WEAPON_READY;
                        ent->client->ps.gunframe = FRAME_IDLE2_FIRST;
                        return;
                }
                
                ent->client->resp.sniper_mode = 0;
                // has to be here for dropping the sniper rifle, in the drop command didn't work...
                ent->client->desired_fov = 90;
                ent->client->ps.fov = 90;
                
                ent->client->ps.gunframe++;
//              gi.cprintf(ent, PRINT_HIGH, "After increment frames = %d\n", ent->client->ps.gunframe);
                return;
        }

        

        // bandaging case
        if ( (ent->client->bandaging)
                && (ent->client->weaponstate != WEAPON_FIRING) 
                && (ent->client->weaponstate != WEAPON_BURSTING)
                && (ent->client->weaponstate != WEAPON_BUSY)
                && (ent->client->weaponstate != WEAPON_BANDAGING) )
        {
                ent->client->weaponstate = WEAPON_BANDAGING;
                ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
                return;
        }





        if ( ent->client->weaponstate == WEAPON_BUSY )
        {
                
                
                if ( ent->client->bandaging == 1 )
                {
                        if ( !(ent->client->idle_weapon) )
                        {
                                Bandage( ent );
                                //ent->client->weaponstate = WEAPON_ACTIVATING;
                //                ent->client->ps.gunframe = 0;
                        }
                        else
                                (ent->client->idle_weapon)--;
                        return;
                }

                // for after bandaging delay
                                if ( !(ent->client->idle_weapon) && ent->client->bandage_stopped )
                                {
                                        ent->client->weaponstate = WEAPON_ACTIVATING;
                    ent->client->ps.gunframe = 0;
                                        ent->client->bandage_stopped = 0;
                                        return;
                                }
                                else if ( ent->client->bandage_stopped == 1 )
                                {
                                        (ent->client->idle_weapon)--;
                                        return;
                                }

                
                if (ent->client->ps.gunframe == 98 )
                {
                        ent->client->weaponstate = WEAPON_READY;
                        return;
                }
                else
                        ent->client->ps.gunframe++;
        }

        if ( ent->client->weaponstate == WEAPON_BANDAGING )
        {
                if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST )
                {
                        ent->client->weaponstate = WEAPON_BUSY;
                        ent->client->idle_weapon = BANDAGE_TIME;
                        return;
                }
                ent->client->ps.gunframe++;
                return;
        }

        
        if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING)
                && (ent->client->weaponstate != WEAPON_BUSY ) )
        {
                
                if ( ent->client->resp.knife_mode == 1 ) // throwing mode
                {
                        ent->client->ps.gunframe = FRAME_NEWKNIFE_LAST;
                        // zucc more vwep stuff
                        if((FRAME_NEWKNIFE_LAST - FRAME_NEWKNIFE_FIRST) < 4)
                        {
                                ent->client->anim_priority = ANIM_REVERSE;
                                if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                                {
                                        ent->s.frame = FRAME_crpain4+1;
                                        ent->client->anim_end = FRAME_crpain1;
                                }
                                else
                                {
                                        ent->s.frame = FRAME_pain304+1;
                                        ent->client->anim_end = FRAME_pain301;
                                        
                                }
                        }               
                }
                else // not in throwing mode
                {
                        ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
                        // zucc more vwep stuff
                        if((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
                        {
                                ent->client->anim_priority = ANIM_REVERSE;
                                if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                                {
                                        ent->s.frame = FRAME_crpain4+1;
                                        ent->client->anim_end = FRAME_crpain1;
                                }
                                else
                                {
                                        ent->s.frame = FRAME_pain304+1;
                                        ent->client->anim_end = FRAME_pain301;
                                        
                                }
                        }               
                }
                ent->client->weaponstate = WEAPON_DROPPING;
                return;
        }
        
        if (ent->client->weaponstate == WEAPON_READY)
        {
            if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) 
//FIREBLADE
                        && (ent->solid != SOLID_NOT || ent->deadflag == DEAD_DEAD) && !lights_camera_action)
//FIREBLADE
                {
                        ent->client->latched_buttons &= ~BUTTON_ATTACK;
                        
                        
                        if ( ent->client->resp.knife_mode == 1 )
                        {
                                ent->client->ps.gunframe = FRAME_THROW_FIRST;
                        }
                        else
                        {
                                ent->client->ps.gunframe = FRAME_FIRE_FIRST;
                        }       
                        ent->client->weaponstate = WEAPON_FIRING;
                        
                        // start the animation
                        ent->client->anim_priority = ANIM_ATTACK;
                        if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                        {
                                ent->s.frame = FRAME_crattak1-1;
                                ent->client->anim_end = FRAME_crattak9;
                        }
                        else
                        {
                                ent->s.frame = FRAME_attack1-1;
                                ent->client->anim_end = FRAME_attack8;
                        }
                        return;
                }
                if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
                {
                        ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                        return;
                }
                
                if ( ent->client->ps.gunframe == FRAME_IDLE2_LAST)
                {
                        ent->client->ps.gunframe = FRAME_IDLE2_FIRST;
                        return;
                }
/* // zucc this causes you to not be able to throw a knife while it is flipping
                if ( ent->client->ps.gunframe == 93 )
                {
                        ent->client->weaponstate = WEAPON_BUSY;
                        return;
                }
                                */
                //gi.cprintf(ent, PRINT_HIGH, "Before a gunframe additon frames = %d\n", ent->client->ps.gunframe);
                ent->client->ps.gunframe++;
                return;
        }
        if (ent->client->weaponstate == WEAPON_FIRING  )
        {
                int n;
                for (n = 0; fire_frames[n]; n++)
                {
                        if (ent->client->ps.gunframe == fire_frames[n])
                        {
                                if (ent->client->quad_framenum > level.framenum)                                                                gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
                                
                                if ( fire(ent) )
                                        break;
                                else // we ran out of knives and switched weapons
                                        return;
                        }
                }
                
                
                
                if (!fire_frames[n])
                        ent->client->ps.gunframe++;

                /*if ( ent->client->ps.gunframe == FRAME_STOPTHROW_FIRST + 1 )
                {
                        ent->client->ps.gunframe = FRAME_NEWKNIFE_FIRST;
                        ent->client->weaponstate = WEAPON_RELOADING;
                }*/
                
                if (ent->client->ps.gunframe == FRAME_IDLE_FIRST+1 ||
                        ent->client->ps.gunframe == FRAME_IDLE2_FIRST+1 )
                        ent->client->weaponstate = WEAPON_READY;
        }
        
}


int Knife_Fire (edict_t *ent)
{
        vec3_t          start, v;
        vec3_t          forward, right;

        vec3_t          offset;
        int             damage = 200;
        int             throwndamage = 250;
        int             kick = 50; // doesn't throw them back much..
        int             knife_return = 3;
        gitem_t*        item;

        AngleVectors (ent->client->v_angle, forward, right, NULL);

        VectorScale (forward, -2, ent->client->kick_origin);
        ent->client->kick_angles[0] = -2;

        VectorSet(offset, 0, 8,  ent->viewheight-8);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

        
        
        v[PITCH] = ent->client->v_angle[PITCH];
        v[ROLL]  = ent->client->v_angle[ROLL];
        
        // zucc updated these to not have offsets anymore for 1.51,
                // this should fix the complaints about the knife not
                // doing enough damage 

        if ( ent->client->ps.gunframe == 6 ) 
        {
                v[YAW]   = ent->client->v_angle[YAW];// + 5;
                AngleVectors (v, forward, NULL, NULL);
        }
        else if ( ent->client->ps.gunframe == 7 )
        {
                v[YAW]  = ent->client->v_angle[YAW];// + 2;
                AngleVectors (v, forward, NULL, NULL);
        }
        else if ( ent->client->ps.gunframe == 8 )
        {
                v[YAW]   = ent->client->v_angle[YAW] ;
                AngleVectors (v, forward, NULL, NULL);
        }
        else if ( ent->client->ps.gunframe == 9 )
        {
                v[YAW]  = ent->client->v_angle[YAW];// - 2;
                AngleVectors (v, forward, NULL, NULL);
        }
        else if ( ent->client->ps.gunframe == 10 )
        {
                v[YAW]   = ent->client->v_angle[YAW]; //-5;
                AngleVectors (v, forward, NULL, NULL);
        }
                
        
        if ( ent->client->resp.knife_mode == 0 )
        {
                knife_return = knife_attack (ent, start, forward, damage, kick  );
                
                if (knife_return < ent->client->knife_sound)
                        ent->client->knife_sound = knife_return;
                
                if ( ent->client->ps.gunframe == 8) // last slicing frame, time for some sound
                {
                        if ( ent->client->knife_sound == -2 ) 
                        {
                                // we hit a player
                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/stab.wav"), 1, ATTN_NORM, 0);
                        }
                        else if ( ent->client->knife_sound == -1 )
                        {
                                // we hit a wall
                                
                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/clank.wav"), 1, ATTN_NORM, 0);
                        }
                        else // we missed
                        {
                                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/swish.wav"), 1, ATTN_NORM, 0);
                        }
                        ent->client->knife_sound = 0;
                }
        }
        
        else
        {
                // do throwing stuff here
                
                damage = throwndamage;
                
                // throwing sound
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/throw.wav"), 1, ATTN_NORM, 0);

                                
                                // below is exact code from action for how it sets the knife up
                                AngleVectors (ent->client->v_angle, forward, right, NULL);
                                VectorSet(offset, 24, 8, ent->viewheight-8);
                                VectorAdd (offset, vec3_origin, offset);
                                forward[2] += .17;

                                // zucc using old style because the knife coming straight out looks
                                // pretty stupid
                                
                                
                                Knife_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

                item = FindItem(KNIFE_NAME);
                ent->client->pers.inventory[ITEM_INDEX(item)]--;
                if ( ent->client->pers.inventory[ITEM_INDEX(item)] <= 0 )
                {
                        ent->client->newweapon = FindItem( MK23_NAME );
                        ChangeWeapon( ent );
                                                // zucc was at 1250, dropping speed to 1200
                        knife_throw (ent, start, forward, damage, 1200 ); 
                        return 0;
                }
                else
                {
                        ent->client->weaponstate = WEAPON_RELOADING;
                        ent->client->ps.gunframe = 111;
                }
                
                

                /*AngleVectors (ent->client->v_angle, forward, right, NULL);
                
                  VectorScale (forward, -2, ent->client->kick_origin);
                  ent->client->kick_angles[0] = -1;
                  
                        VectorSet(offset, 8, 8, ent->viewheight-8);
                        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
                */
                //      fire_rocket (ent, start, forward, damage, 650, 200, 200);
                
                knife_throw (ent, start, forward, damage, 1200 ); 
                
                
                // 
        }
        
        ent->client->ps.gunframe++;
        if ( ent->client->ps.gunframe == 106 ) // over the throwing frame limit
                ent->client->ps.gunframe = 64;  // the idle animation frame for throwing
                                                                                // not sure why the frames weren't ordered
                                                                                // with throwing first, unwise.
        PlayerNoise(ent, start, PNOISE_WEAPON);
        return 1;

}

void Weapon_Knife (edict_t *ent)
{
        static int      pause_frames[]  = {22, 28, 0};
        static int      fire_frames[]   = {6, 7, 8, 9, 10, 105, 0};
        // I think we need a special version of the generic function for this...
        Weapon_Generic_Knife (ent, 5, 12, 52, 59, 63, 102, 105, 110, 117, pause_frames, fire_frames, Knife_Fire);
}


// zucc - I thought about doing a gas grenade and even experimented with it some.  It
// didn't work out that great though, so I just changed this code to be the standard
// action grenade.  So the function name is just misleading...

void gas_fire (edict_t *ent )
{
        vec3_t  offset;
        vec3_t  forward, right;
        vec3_t  start;
        int             damage = GRENADE_DAMRAD;
        float   timer;
        int             speed;
        float   radius;
/*        int held = false;*/
        gitem_t* item;
        
        radius = damage + 40;

        VectorSet(offset, 8, 8, ent->viewheight-8);
        AngleVectors (ent->client->v_angle, forward, right, NULL);
        P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

        timer = 2.0;
        
        if ( ent->client->resp.grenade_mode == 0 )
                speed = 400;
        else if ( ent->client->resp.grenade_mode == 1 )
                speed = 720;
        else
                speed = 920;
        
        fire_grenade2 (ent, start, forward, GRENADE_DAMRAD, speed, timer, GRENADE_DAMRAD*2, false);


        
        item = FindItem(GRENADE_NAME);
        ent->client->pers.inventory[ITEM_INDEX(item)]--;
        if ( ent->client->pers.inventory[ITEM_INDEX(item)] <= 0 )
        {
                ent->client->newweapon = FindItem( MK23_NAME );
                ChangeWeapon( ent );
                return;
        }
        else
        {
                ent->client->weaponstate = WEAPON_RELOADING;
                ent->client->ps.gunframe = 0;
        }


//      ent->client->grenade_time = level.time + 1.0;
        ent->client->ps.gunframe++;
}


#define GRENADE_ACTIVATE_LAST 3
#define GRENADE_PULL_FIRST 72
#define GRENADE_PULL_LAST 79
//#define GRENADE_THROW_FIRST 4
//#define GRENADE_THROW_LAST 9 // throw it on frame 8?
#define GRENADE_PINIDLE_FIRST 10
#define GRENADE_PINIDLE_LAST 39
// moved these up in the file (actually now in g_local.h)
//#define GRENADE_IDLE_FIRST 40
//#define GRENADE_IDLE_LAST 69
                                         
// the 2 deactivation frames are a joke, they look like shit, probably best to roll
// the activation frames backwards.  Aren't really enough of them either.

void Weapon_Gas (edict_t *ent)
{
        gitem_t* item;
        if(ent->s.modelindex != 255) // zucc vwep
                return; // not on client, so VWep animations could do wacky things
        
        //FIREBLADE
        if (ent->client->weaponstate == WEAPON_FIRING &&
                ((ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD) || 
                lights_camera_action))
        {
                ent->client->weaponstate = WEAPON_READY;
        }
        //FIREBLADE
        
        if( ent->client->weaponstate == WEAPON_RELOADING)
        {
                if(ent->client->ps.gunframe < GRENADE_ACTIVATE_LAST)
                {
                        ent->client->ps.gunframe++;             
                }
                else
                {
                        ent->client->ps.gunframe = GRENADE_PINIDLE_FIRST;
                        ent->client->weaponstate = WEAPON_READY;
                }
        }
        
        if (ent->client->weaponstate == WEAPON_DROPPING)
        {
                if (ent->client->ps.gunframe == 0)
                {
                        ChangeWeapon (ent);
                        return;
                }
                else
                {
                        // zucc going to have to do this a bit different because
                        // of the way I roll gunframes backwards for the thrownknife position
                        if((ent->client->ps.gunframe) == 3)
                        {
                                ent->client->anim_priority = ANIM_REVERSE;
                                if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                                {
                                        ent->s.frame = FRAME_crpain4+1;
                                        ent->client->anim_end = FRAME_crpain1;
                                }
                                else
                                {
                                        ent->s.frame = FRAME_pain304+1;
                                        ent->client->anim_end = FRAME_pain301;
                                        
                                }
                        }
                        ent->client->ps.gunframe--;
                        return;
                }                               
                
        }
        if (ent->client->weaponstate == WEAPON_ACTIVATING)
        {
                
                if (ent->client->ps.gunframe == GRENADE_ACTIVATE_LAST )
                {
                        ent->client->ps.gunframe = GRENADE_PINIDLE_FIRST;
                        ent->client->weaponstate = WEAPON_READY;
                        return;
                }
                
                if ( ent->client->ps.gunframe == GRENADE_PULL_LAST )
                {
                        ent->client->ps.gunframe = GRENADE_IDLE_FIRST;
                        ent->client->weaponstate = WEAPON_READY;
                        gi.cprintf(ent, PRINT_HIGH, "Pin pulled, ready for %s range throw\n", ent->client->resp.grenade_mode == 0 ? "short" : (ent->client->resp.grenade_mode == 1 ? "medium" : "long" ) );
                        return;
                }
                
                if ( ent->client->ps.gunframe == 75 )
                {
                        gi.sound(ent, CHAN_WEAPON, gi.soundindex("misc/grenade.wav"), 1, ATTN_NORM, 0);
                }
                
                ent->client->resp.sniper_mode = 0;
                // has to be here for dropping the sniper rifle, in the drop command didn't work...
                ent->client->desired_fov = 90;
                ent->client->ps.fov = 90;
                
                ent->client->ps.gunframe++;
                //              gi.cprintf(ent, PRINT_HIGH, "After increment frames = %d\n", ent->client->ps.gunframe);
                return;
        }
        
        
        // bandaging case
        if ( (ent->client->bandaging)
                && (ent->client->weaponstate != WEAPON_FIRING) 
                && (ent->client->weaponstate != WEAPON_BURSTING)
                && (ent->client->weaponstate != WEAPON_BUSY)
                && (ent->client->weaponstate != WEAPON_BANDAGING) )
        {
                ent->client->weaponstate = WEAPON_BANDAGING;
                ent->client->ps.gunframe = GRENADE_ACTIVATE_LAST;
                return;
        }
        
        
        if ( ent->client->weaponstate == WEAPON_BUSY )
        {
                
                
                if ( ent->client->bandaging == 1 )
                {
                        if ( !(ent->client->idle_weapon) )
                        {
                                Bandage( ent );
                        }
                        else
                                (ent->client->idle_weapon)--;
                        return;
                }
                // for after bandaging delay
                if ( !(ent->client->idle_weapon) && ent->client->bandage_stopped )
                {
                        gitem_t *item;
                        item = FindItem(GRENADE_NAME);
                        if ( ent->client->pers.inventory[ITEM_INDEX(item)] <= 0 )
                        {
                                ent->client->newweapon = FindItem( MK23_NAME );
                                ent->client->bandage_stopped = 0;
                                ChangeWeapon( ent );
                                return;
                        }
                        
                        ent->client->weaponstate = WEAPON_ACTIVATING;
                        ent->client->ps.gunframe = 0;
                        ent->client->bandage_stopped = 0;
                }
                else if ( ent->client->bandage_stopped )
                        (ent->client->idle_weapon)--;
                
                
        }
        
        if ( ent->client->weaponstate == WEAPON_BANDAGING )
        {
                if (ent->client->ps.gunframe == 0 )
                {
                        ent->client->weaponstate = WEAPON_BUSY;
                        ent->client->idle_weapon = BANDAGE_TIME;
                        return;
                }
                ent->client->ps.gunframe--;
                return;
        }
        
        
        if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING)
                && (ent->client->weaponstate != WEAPON_BUSY ) )
        {
                
                // zucc - check if they have a primed grenade
                
                if ( ent->client->curr_weap == GRENADE_NUM
                        && ( ( ent->client->ps.gunframe >= GRENADE_IDLE_FIRST
                        && ent->client->ps.gunframe <= GRENADE_IDLE_LAST )
                        || ( ent->client->ps.gunframe >= GRENADE_THROW_FIRST
                        && ent->client->ps.gunframe <= GRENADE_THROW_LAST ) ) )
                {
                        fire_grenade2 (ent, ent->s.origin, tv(0,0,0), GRENADE_DAMRAD, 0, 2, GRENADE_DAMRAD*2, false);
                        item = FindItem(GRENADE_NAME);
                        ent->client->pers.inventory[ITEM_INDEX(item)]--;
                        if ( ent->client->pers.inventory[ITEM_INDEX(item)] <= 0 )
                        {
                                ent->client->newweapon = FindItem( MK23_NAME );
                                ChangeWeapon( ent );
                                return;
                        }
                        
                }
                
                
                ent->client->ps.gunframe = GRENADE_ACTIVATE_LAST;
                // zucc more vwep stuff
                if((GRENADE_ACTIVATE_LAST) < 4)
                {
                        ent->client->anim_priority = ANIM_REVERSE;
                        if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                        {
                                ent->s.frame = FRAME_crpain4+1;
                                ent->client->anim_end = FRAME_crpain1;
                        }
                        else
                        {
                                ent->s.frame = FRAME_pain304+1;
                                ent->client->anim_end = FRAME_pain301;
                                
                        }
                }               
                
                ent->client->weaponstate = WEAPON_DROPPING;
                return;
        }
        
        if (ent->client->weaponstate == WEAPON_READY)
        {
                if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) 
                        //FIREBLADE
                        && (ent->solid != SOLID_NOT || ent->deadflag == DEAD_DEAD) && 
                        !lights_camera_action)
                        //FIREBLADE
                        
                {
                        
                        
                        if ( ent->client->ps.gunframe <= GRENADE_PINIDLE_LAST &&
                                ent->client->ps.gunframe >= GRENADE_PINIDLE_FIRST )
                        {
                                ent->client->ps.gunframe = GRENADE_PULL_FIRST;
                                ent->client->weaponstate = WEAPON_ACTIVATING;                                     
                                ent->client->latched_buttons &= ~BUTTON_ATTACK;
                        }
                        else
                        {
                                if (ent->client->ps.gunframe == GRENADE_IDLE_LAST)
                                {
                                        ent->client->ps.gunframe = GRENADE_IDLE_FIRST;
                                        return;
                                }
                                ent->client->ps.gunframe++;
                                return;
                        }       
                }
                
                if ( ent->client->ps.gunframe >= GRENADE_IDLE_FIRST &&
                        ent->client->ps.gunframe <= GRENADE_IDLE_LAST )
                {
                        ent->client->ps.gunframe = GRENADE_THROW_FIRST;
                        ent->client->anim_priority = ANIM_ATTACK;
                        if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
                        {
                                ent->s.frame = FRAME_crattak1-1;
                                ent->client->anim_end = FRAME_crattak9;
                        }
                        else
                        {
                                ent->s.frame = FRAME_attack1-1;
                                ent->client->anim_end = FRAME_attack8;
                        }
                        ent->client->weaponstate = WEAPON_FIRING;
                        return;
                        
                        
                        
                }
                
                if ( ent->client->ps.gunframe == GRENADE_PINIDLE_LAST)
                {
                        ent->client->ps.gunframe = GRENADE_PINIDLE_FIRST;
                        return;
                }
                
                ent->client->ps.gunframe++;
                return;
        }
        if (ent->client->weaponstate == WEAPON_FIRING  )
        {
                
                
                if ( ent->client->ps.gunframe == 8 )
                {
                        gas_fire(ent);
                        return;
                }
                
                
                
                ent->client->ps.gunframe++;
                
                if (ent->client->ps.gunframe == GRENADE_IDLE_FIRST+1 ||
                        ent->client->ps.gunframe == GRENADE_PINIDLE_FIRST+1 )
                        ent->client->weaponstate = WEAPON_READY;
        }
}
