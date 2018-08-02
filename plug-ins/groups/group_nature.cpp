/* $Id$
 *
 * ruffina, 2004
 */
#if 0
#include "class_druid.h"
#endif

#include "skill.h"
#include "spelltarget.h"
#include "spelltemplate.h"
#include "affecthandlertemplate.h"
#include "skillcommandtemplate.h"
#include "skillmanager.h"

#include "pcharactermanager.h"
#include "affect.h"
#include "pcharacter.h"
#include "room.h"
#include "npcharacter.h"
#include "object.h"

#include "dreamland.h"
#include "gsn_plugin.h"
#include "magic.h"
#include "fight.h"
#include "stats_apply.h"
#include "onehit_weapon.h"
#include "damage_impl.h"
#include "merc.h"
#include "mercdb.h"
#include "handler.h"
#include "save.h"
#include "act.h"
#include "vnum.h"
#include "interp.h"
#include "def.h"

PROF(ranger);
PROF(druid);

/*
 * 'tame' skill command
 */
SKILL_RUNP( tame )
{
    Character *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument,arg);

    if ( ch->is_npc() || !gsn_tame->usable( ch ) )
    {
	ch->send_to("��� � ���� �����!\n\r");
	return;
    }

    if (arg[0] == '\0')
    {
	ch->send_to("�� �� ���������� �����������.\n\r");
	act_p("$c1 �������� ��������� ��$g��|�|�� ����, �� ��� ������� � ������� �������������.",
		ch,0,0,TO_ROOM,POS_RESTING);
	return;
    }

    if ( (victim = get_char_room(ch,arg)) == 0)
    {
	ch->send_to("����� ��� �����.\n\r");
	return;
    }

    if (!victim->is_npc())
    {
	ch->pecho("%1$^C1 �� �����%1$n����|���� �����������.", victim);
	return;
    }
    
    /*
     * druidic tame: control animal attracted by magic bite
     */
    if (ch->getTrueProfession( ) == prof_druid) {
#if 0	
	DruidSummonedAnimal::Pointer animal;
	int chance;

	if (!ch->getNPC( )->behavior 
	    || !(animal = ch->getNPC( )->behavior.getDynamicPointer<DruidSummonedAnimal>( ))
	    || !animal->myHero( ch ))
	{
	    ch->println("��� �������� �� ��������� ������ ��������.");
	    return;
	}

	if (is_safe(ch, victim)) 
	    return;
	    
	if (overcharmed( ch ))
	    return;

	ch->setWait( gsn_tame->getBeats( )  );

	chance = gsn_tame->getEffective( ch );
	chance += 3 * (ch->getModifyLevel( ) - victim->getModifyLevel( ));
	chance += (ch->getCurrStat(STAT_CHA) - 20) * 2;
	chance = (chance * animal->biteQuality) / 100;
	
	if (number_percent( ) > chance) {
	    act("$c1 ����������� ����� � �������!", victim, 0, 0, TO_ROOM);
	    gsn_tame->improve( ch, false, victim );
	    interpret_raw(victim, "murder", ch->getNameP( ));
	    return;
	}
	
	ch->add_follower( victim );
	victim->leader = ch;

	af.where     = TO_AFFECTS;
	af.type      = gsn_tame;
	af.level     = ch->getModifyLevel( );
	af.duration  = -1;
	af.bitvector = AFF_CHARM;
	affect_to_char( victim, &af );
	
	act("$C1 ������ ��������� ����������� ����� ����.", ch, 0, victim, TO_CHAR);
	act("$C1 �������� ������� � ����� $c3.", ch, 0, victim, TO_NOTVICT);
	act("�� �������� �������� � ����� $c3.", ch, 0, victim, TO_VICT);
	gsn_tame->improve( ch, true, victim );
#endif	
	return;
    }
    
    /* 
     * ranger tame: remove aggression
     */
    if (ch->getTrueProfession( ) == prof_ranger) {
	if (!IS_SET(victim->act,ACT_AGGRESSIVE))
	{
	    ch->pecho("%1$^C1 ������ �� ��������%1$G��|��|��|��.", victim);
	    return;
	}

	ch->setWait( gsn_tame->getBeats( )  );

	if (number_percent() < gsn_tame->getEffective( ch ) + 15
		+ 4 * ( ch->getModifyLevel() - victim->getModifyLevel() ) )
	{
	    REMOVE_BIT(victim->act,ACT_AGGRESSIVE);
	    SET_BIT(victim->affected_by,AFF_CALM);
	    victim->println("�� ��������������.");
	    act("�� ������������ $C4.",ch,0,victim,TO_CHAR);
	    act("$c1 ����������� $C4.",ch,0,victim,TO_NOTVICT);
	    stop_fighting(victim,true);
	    gsn_tame->improve( ch, true, victim );
	}
	else
	{
	    ch->println("������� �� �������.");
	    act("$c1 �������� ��������� $C4, �� ������� ����������.",
		    ch,0,victim,TO_NOTVICT);
	    act("$c1 �������� ��������� ����, �� ������� ����������.",
		    ch,0,victim,TO_VICT);
	    gsn_tame->improve( ch, false, victim );
	}

	return;
    }
}

static bool has_water_around( Character *ch )
{
    if (IS_WATER(ch->in_room))
        return true;
    
    if (!IS_OUTSIDE(ch))
        return false;
    
    if (weather_info.sky >= SKY_RAINING)
        return true;

    return false;
}

/*
 * 'hydroblast' spell
 */
SPELL_DECL(Hydroblast);
VOID_SPELL(Hydroblast)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    if (!has_water_around( ch )) {
	 ch->send_to("����� ������������ ������ �������.\n\r");
	 ch->wait = 0;
	 return;
    }
    
    act("�������� ���� ������ $c2 ���������� ������, ������� �����.", ch, 0, 0, TO_ROOM);
    act("�������� ���� ������ ���� ���������� ������, ������� �����.", ch, 0, 0, TO_CHAR);
    dam = dice( level , 14 );
    damage(ch,victim,dam,sn,DAM_BASH,true, DAMF_SPELL);
}

/*
 * 'entangle' spell
 */
SPELL_DECL(Entangle);
VOID_SPELL(Entangle)::run( Character *ch, Object *grave, int sn, int level ) 
{
    int dam;
    PCharacter *victim;

    if (ch->in_room->sector_type == SECT_INSIDE ||
	ch->in_room->sector_type == SECT_CITY ||
	ch->in_room->sector_type == SECT_DESERT ||
	ch->in_room->sector_type == SECT_WATER_NOSWIM ||
	ch->in_room->sector_type == SECT_AIR)
    {
	ch->send_to("�� ���� �������� �� ������ ����� �����.\n\r");
	return;
    }

    if (grave->pIndexData->vnum != OBJ_VNUM_GRAVE) {
	ch->send_to("��� �������� ������ ��������� ��� �����������.\r\n");
	return;
    }

    victim = PCharacterManager::findPlayer( grave->getOwner( ) );

    if (!victim || !DIGGED(victim)) {
	ch->send_to("���.. � ������-�� ��������..\r\n");
	LogStream::sendError( ) << "Unexistent grave owner: " << grave->getOwner( ) << endl;
	return;
    }

    if (number_percent( ) > ch->getSkill( sn ) || is_safe_nomessage( ch, victim)) {
	act_p("������ ����������� ���������� � �������� ���������.", ch, 0, 0, TO_ALL, POS_RESTING);
	return;
    }

    act_p("����� ���������� ��������� � ������, ������� ���� �����.", victim, 0, 0, TO_CHAR, POS_RESTING);
    act_p("������� ��������� ��������� ������, �������� ������� ������� ��� �����!", ch, 0, 0, TO_ALL, POS_RESTING);
    act_p("��-��� ����� ��������� ����������� ��������.", ch, 0, 0, TO_ALL, POS_RESTING);

    undig( victim );

    dam = number_range(level, 4 * level);
    if ( saves_spell( level, victim, DAM_PIERCE, ch, DAMF_SPELL ) )
	dam /= 2;

    damage(ch,victim, ch->getModifyLevel(),gsn_entangle,DAM_PIERCE, true, DAMF_SPELL);
}

VOID_SPELL(Entangle)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;
    Affect todex;

    if (ch->in_room->sector_type == SECT_INSIDE ||
	ch->in_room->sector_type == SECT_CITY ||
	ch->in_room->sector_type == SECT_DESERT ||
	ch->in_room->sector_type == SECT_WATER_NOSWIM ||
	ch->in_room->sector_type == SECT_AIR)
    {
	ch->send_to("�� ���� �������� �� ������ ����� �����.\n\r");
	return;
    }

    act("������� ��������� ���������� ������ �����, ������� ���� $c2!",
        victim, 0, 0, TO_ROOM);
    act("������� ��������� ���������� ������ �����, ������� ���� ����!",
        victim, 0, 0, TO_CHAR);
	
    dam = number_range(level, 4 * level);
    if (saves_spell( level, victim, DAM_PIERCE, ch, DAMF_SPELL ))
	dam /= 2;

    victim->move -= victim->max_move / 3;
    victim->move = max( 0, (int)victim->move );
    
    todex.type = sn;
    todex.level = level;
    todex.duration = level / 10;
    todex.location = APPLY_DEX;
    todex.modifier = -1;
    todex.bitvector = 0;
    affect_join( victim, &todex);

    damage(ch, victim, ch->getModifyLevel(), gsn_entangle, DAM_PIERCE, true, DAMF_SPELL);
}
