/* $Id: scenarios.cpp,v 1.1.2.5.6.5 2010-09-01 21:20:44 rufina Exp $
 *
 * ruffina, 2005
 */
#include <map>

#include "scenarios.h"
#include "rainbow.h"
#include "rainbowinfo.h"
#include "gqchannel.h"

#include "skillmanager.h"
#include "object.h"
#include "room.h"
#include "npcharacter.h"
#include "pcharacter.h"
#include "affect.h"

#include "mercdb.h"
#include "act.h"
#include "handler.h"
#include "act_move.h"
#include "merc.h"
#include "descriptor.h"
#include "def.h"

/*--------------------------------------------------------------------------
 * Rainbow Scenario
 *-------------------------------------------------------------------------*/

bool RainbowScenario::checkArea( AREA_DATA *area ) const
{
    if (IS_SET(area->area_flag, AREA_WIZLOCK|AREA_NOQUEST|AREA_HOMETOWN|AREA_NOGATE))
	return false;
    if (area->low_range > 20)
	return false;

    return true;
}

bool RainbowScenario::checkMobile( NPCharacter *ch ) const
{
    if (!IS_SET(ch->form, FORM_BIPED))
	return false;
    if (!IS_SET(ch->form, FORM_MAMMAL))
	return false;
    if (IS_SET(ch->imm_flags, IMM_SUMMON|IMM_CHARM))
	return false;
    if ((ch->behavior && ch->behavior->hasDestiny( )) || ch->fighting)
	return false;
    if (IS_AFFECTED(ch, AFF_CHARM)) 
	return false;
    if (IS_SET(ch->act, ACT_AGGRESSIVE))
	return false;

    return true;
}

bool RainbowScenario::checkRoom( Room *room ) const    
{
    if (IS_SET(room->room_flags, ROOM_NO_QUEST))
	return false;

    if (!room->isCommon( ))
	return false;

    for (int d = 0; d < DIR_SOMEWHERE; d++)
	if (room->exit[d] 
	    && room->exit[d]->u1.to_room
	    && room->exit[d]->u1.to_room->exit[dirs[d].rev]
	    && room->exit[d]->u1.to_room->exit[dirs[d].rev]->u1.to_room == room)
	    return true;

    return false;
}

void RainbowScenario::onQuestInit( ) const
{
    Descriptor *d;
    Character *ch;
    
    for ( d = descriptor_list; d; d = d->next ) {
	if (d->connected != CON_PLAYING)
	    continue;
	if (!(ch = d->character))
	    continue;
	if (ch->is_npc( ))
	    continue;
	if (!canHearInitMsg( ch->getPC( ) ))
	    continue;

	ch->send_to( getInitMsg( ) + "\r\n" );
    }
}

/*--------------------------------------------------------------------------
 * Rainbow Default Scenario 
 *-------------------------------------------------------------------------*/

void RainbowDefaultScenario::canStart( ) const throw ( GQCannotStartException )
{
    if (weather_info.sky != SKY_RAINING)
	throw GQCannotStartException( "wrong weather" );
	
    if (weather_info.sunlight != SUN_LIGHT)
	throw GQCannotStartException( "wrong hour" );
}

bool RainbowDefaultScenario::checkRoom( Room *room ) const
{
    if (IS_SET(room->room_flags, ROOM_INDOORS|ROOM_DARK))
	return false;

    return RainbowScenario::checkRoom( room );
}

void RainbowDefaultScenario::printCount( int cnt, ostringstream& buf ) const
{
    buf << "� ���� ��� ���� " << GQChannel::BOLD << cnt << GQChannel::NORMAL
	<< " ����������" << GET_COUNT(cnt, "��", "��", "��")
	<< " �����" << GET_COUNT(cnt, "��", "��", "���") << ". ";
}

void RainbowDefaultScenario::printTime( ostringstream& buf ) const
{
    buf << "�������� ";
    RainbowGQuest::getThis( )->printRemainedTime( buf );
    buf << ", ����� ������� ��� ������." << endl;
}

void RainbowDefaultScenario::printWinnerMsgOther( const DLString &name, ostringstream& buf ) const 
{
    buf << GQChannel::BOLD << name << GQChannel::NORMAL
	<< " �������� {Y�{R�{M�{G�{B�{C�{x " << GQChannel::NORMAL 
	<< "��� �����!";
}

bool RainbowDefaultScenario::canHearInitMsg( PCharacter *ch ) const
{
    return IS_OUTSIDE(ch);
}

void RainbowDefaultScenario::onQuestFinish( PCharacter *ch ) const
{
    Affect af;

    af.where = TO_RESIST;
    af.type = SkillManager::getThis( )->lookup( "rainbow shield" );
    af.duration = 180;
    af.level = 106;
    af.bitvector = RES_SUMMON|RES_CHARM|RES_SPELL|RES_WEAPON|RES_BASH
		   |RES_PIERCE|RES_SLASH|RES_FIRE|RES_COLD|RES_LIGHTNING
		   |RES_ACID|RES_NEGATIVE|RES_HOLY|RES_ENERGY|RES_MENTAL
		   |RES_LIGHT|RES_WOOD|RES_SILVER|RES_IRON;
    af.location = 0;
    af.modifier = 0;
    affect_join(ch, &af);
    
    act("�� ���������� � ���� ������, ������ ������������ ��������.", ch, 0, 0, TO_CHAR);
    act("$c1 ��������� ������ � ����.", ch, 0, 0, TO_ROOM);
    act("\r\n{Y�{R�{M�{G�{B�{C�{R�{M�{W ����� ����� ������� � {C����!{x", ch, 0, 0, TO_ALL);
    act("\r\n���� �������� {Y�{R�{M�{G�{B�{C�{R�{M�{G�{C�{Y�{R�{x �����!", ch, 0, 0, TO_CHAR);
    act("\r\n{Y�{R�{M�{G�{B�{C�{R�{M�{G�{C�{Y�{R�{x ����� �������� $c4!", ch, 0, 0, TO_ROOM);
}  

void RainbowDefaultScenario::onGivePiece( PCharacter *hero, NPCharacter *mob ) const
{
    act("$c1 ������� ������-�� ������� �������� � ���������� �� ���� �������.", mob, 0, hero, TO_ROOM);
}

void RainbowDefaultScenario::dressItem( Object *obj, int number ) const
{
    DLString name = pieces[number].getValue( );
    obj->fmtShortDescr( "%s %s", name.c_str( ), obj->getShortDescr( ) );
}

/*--------------------------------------------------------------------------
 * Rainbow Sins Scenario 
 *-------------------------------------------------------------------------*/

void RainbowSinsScenario::canStart( ) const throw ( GQCannotStartException )
{
    GlobalQuestInfo::PlayerList players;
    RainbowGQuestInfo::getThis( )->findParticipants( players );
    int evils = 0;

    for (GlobalQuestInfo::PlayerList::const_iterator p = players.begin( ); p != players.end( ); p++)
        if (IS_EVIL(*p))
            evils++;

    if (evils < 1)
        throw GQCannotStartException( "not enough evil in the world" );

    if (chance(90))
        throw GQCannotStartException( "won't start" );
}

void RainbowSinsScenario::printCount( int cnt, ostringstream& buf ) const
{
    buf << "���� ������� ������� " << GQChannel::BOLD << cnt << GQChannel::NORMAL
	<< " ������" << GET_COUNT(cnt, "��", "��", "��")
	<< " ����" << GET_COUNT(cnt, "", "�", "��") << ". ";
}

void RainbowSinsScenario::printTime( ostringstream& buf ) const
{
    buf << "�������� ��������� ����� ";
    RainbowGQuest::getThis( )->printRemainedTime( buf );
    buf << "." << endl;
}

void RainbowSinsScenario::printWinnerMsgOther( const DLString &name, ostringstream& buf ) const 
{
    buf << GQChannel::BOLD << name << GQChannel::NORMAL
	<< "  ������� �� ������ ���������!";
}

bool RainbowSinsScenario::canHearInitMsg( PCharacter *ch ) const
{
    return IS_OUTSIDE(ch);
}

void RainbowSinsScenario::onQuestFinish( PCharacter *ch ) const
{
    Affect af;

    af.where = TO_RESIST;
    af.type = SkillManager::getThis( )->lookup( "demonic mantle" );
    af.duration = 180;
    af.level = 106;
    af.bitvector = RES_SUMMON|RES_CHARM|RES_SPELL|RES_WEAPON|RES_BASH
		   |RES_PIERCE|RES_SLASH|RES_FIRE|RES_COLD|RES_LIGHTNING
		   |RES_ACID|RES_NEGATIVE|RES_HOLY|RES_ENERGY|RES_MENTAL
		   |RES_LIGHT|RES_WOOD|RES_SILVER|RES_IRON;
    af.location = 0;
    af.modifier = 0;
    affect_join(ch, &af);
   
    ch->pecho("\r\n�� ����� ���������� c��������� ��� � ���������� '{r�� �����%G��|�|��!{x'.", ch);
    act("\r\n�� ����� ���������� ���������� ��� � ���-�� ������� $c3.", ch, 0, 0, TO_ROOM );

    act("\r\n���� ��������� ������������ ������!", ch, 0, 0, TO_CHAR);
    act("\r\n������������ ������ ��������� $c4!", ch, 0, 0, TO_ROOM);
}  

void RainbowSinsScenario::onGivePiece( PCharacter *hero, NPCharacter *mob ) const
{
    act("$c1 ��������� ����������, ����� � $C6 ���������� ���������.", mob, 0, hero, TO_NOTVICT);
    act("$c1 ��������� ����������, ����� � ���� ���������� ���������.", mob, 0, hero, TO_VICT);
}

bool RainbowSinsScenario::checkMobile( NPCharacter *ch ) const
{
    if (!RainbowScenario::checkMobile( ch ))
        return false;

    if (IS_GOOD(ch))
        return false;

    return true;
}

void RainbowSinsScenario::dressItem( Object *obj, int number ) const
{
    DLString name = pieces[number].getValue( );
    obj->setShortDescr( name.c_str( ) );
}
