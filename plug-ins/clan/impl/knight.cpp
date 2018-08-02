/* $Id: knight.cpp,v 1.1.6.10.4.18 2010-09-01 21:20:44 rufina Exp $
 *
 * ruffina, 2005
 */
/***************************************************************************
 * ��� ����� �� ���� ��� 'Dream Land' ����������� Igor {Leo} � Olga {Varda}*
 * ��������� ������ � ��������� ����� ����, � ����� ������ ������ ��������:*
 *    Igor S. Petrenko     {NoFate, Demogorgon}                            *
 *    Koval Nazar          {Nazar, Redrum}                                 *
 *    Doropey Vladimir     {Reorx}                                         *
 *    Kulgeyko Denis       {Burzum}                                        *
 *    Andreyanov Aleksandr {Manwe}                                         *
 *    � ��� ���������, ��� ��������� � ����� � ���� MUD                    *
 ***************************************************************************/

#include "knight.h"

#include "commonattributes.h"

#include "summoncreaturespell.h"
#include "affecthandlertemplate.h"
#include "spelltemplate.h"                                                 
#include "skillcommandtemplate.h"
#include "skill.h"
#include "skillmanager.h"

#include "pcharacter.h"
#include "pcharactermanager.h"
#include "npcharacter.h"
#include "object.h"
#include "room.h"
#include "affect.h"

#include "dreamland.h"
#include "act.h"
#include "interp.h"
#include "gsn_plugin.h"
#include "merc.h"
#include "vnum.h"
#include "mercdb.h"
#include "fight.h"
#include "act_move.h"
#include "handler.h"
#include "magic.h"
#include "def.h"

GSN(dispel_affects);
CLAN(knight);

#define OBJ_VNUM_DRAGONDAGGER       80
#define OBJ_VNUM_DRAGONMACE         81
#define OBJ_VNUM_PLATE              82
#define OBJ_VNUM_DRAGONSWORD        83
#define OBJ_VNUM_DRAGONLANCE        99

/*--------------------------------------------------------------------------
 * clan item 
 *-------------------------------------------------------------------------*/
void ClanItemKnight::actDisappear( )
{
    act( "$o1 �������� � ����� �����.", 
         obj->getRoom( )->people, obj, 0, TO_ALL );
}

/*--------------------------------------------------------------------------
 * clan altar 
 *-------------------------------------------------------------------------*/
void ClanAltarKnight::actAppear( )
{
    act( "{W���� ����� ����������� ������� � � ������ ��������������� $o1.{x", 
	 obj->in_room->people, obj, 0, TO_ALL );
}

void ClanAltarKnight::actDisappear( )
{
    act( "{W���� $o2 �������� � �� ������������ � �������!{x", 
         obj->getRoom( )->people, obj, NULL, TO_ALL );
}

void ClanAltarKnight::actNotify( Character *ch )
{
    act_p( "{W�������� ������ ������ ����� ��� ��������� ������������!{x", 
	    ch, 0, 0, TO_CHAR, POS_DEAD );
}

/*--------------------------------------------------------------------------
 * Protector 
 *-------------------------------------------------------------------------*/
void ClanGuardKnight::actGreet( PCharacter *wch )
{
    do_say(ch, "����� ����������, ����������� ������.");
}
void ClanGuardKnight::actPush( PCharacter *wch )
{
    act( "$C1 ������ ����, ������ �������, ���������� �����.\n\r...� ��� ��� �� ����������� �������� � �������.", wch, 0, ch, TO_CHAR );
    act( "$C1 ������ $c3, ������ ������������, ���������� �����.\n\r... � $c1 � ����� ��������� � ������ �������.", wch, 0, ch, TO_ROOM );
}

void ClanGuardKnight::actInvited( PCharacter *wch, Object *obj )
{
    do_say( ch, "{W�� ��� �! ���� ��� ���� - �� �� �������, ��� �� � ������!{x" );
//    act( "$C1 �������� $o4 � $c2.", wch, obj, ch, TO_ROOM );
//    act( "$C1 �������� � ���� $o4.", wch, obj, ch, TO_CHAR );
}

void ClanGuardKnight::actIntruder( PCharacter * )
{
    interpret_raw( ch, "cb", "����������... ����������� ���� ��������!" );
}

void ClanGuardKnight::actGhost( PCharacter * )
{
    do_say( ch, "{W������ ����� � ������ ������� �� ���� ����� ������!{x" );
}

void ClanGuardKnight::actGiveInvitation( PCharacter *wch, Object *obj )
{
    act( "$c1 ����������� ��������� �� �������.", ch, 0, 0, TO_ROOM );
    act( "$c1 ������ ����������� ������ �� $o6.", ch, obj, 0, TO_ROOM );
}

int ClanGuardKnight::getCast( Character *victim )
{
	int sn = -1;

	switch ( dice(1,16) )
	{
	case  0: 
	case  1:
		if (!victim->isAffected( gsn_spellbane ))
		    sn = gsn_dispel_affects;
		break;
	case  2:
	case  3:
		sn = gsn_acid_arrow;
		break;
	case  4: 
	case  5:
		sn = gsn_caustic_font;
		break; 
	case  6:
	case  7:
	case  8:
	case  9:
	case 10:
		sn = gsn_acid_blast;
		break;
	default:
		sn = -1;
		break;
	}

	return sn;
}

/*
 * 'guard' skill command
 */

SKILL_RUNP( guard )
{
    char arg[MAX_INPUT_LENGTH];
    Character *vict;
    PCharacter *victim, *pch, *gch;
    int cnt;

    if (!gsn_guard->available( ch ))
    {
	ch->send_to("���?\n\r");
	return;
    }

    if (!gsn_guard->usable( ch ))
	return;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
	ch->send_to("�������� ����?\n\r");
	return;
    }

    if (( vict = get_char_room( ch, arg ) ) == 0) {
	ch->send_to("����� ��� �����.\n\r");
	return;
    }

    if (vict->is_npc()) {
	act_p("$C1 �� ��������� � ����� ������!", ch, 0, vict, TO_CHAR,POS_RESTING);
	return;
    }

    victim = vict->getPC();
    pch = ch->getPC();

    if (!str_cmp(arg, "none") || !str_cmp(arg, "self") || victim == pch)
    {
	if (pch->guarding == 0)
	{
	    pch->send_to("�� �� ������ �������� ���� ��!\n\r");
	    return;
	}
	else
	{
	    guarding_stop( pch, pch->guarding );
	    return;
	}
    }

    if (pch->guarding != 0)
    {
	pch->send_to("�� �� ��������� ����-�� �������!\n\r");
	return;
    }

    if (victim->guarded_by != 0)
    {
	act_p("$C4 ��� ���-�� ��������.",pch,0,victim,TO_CHAR,POS_RESTING);
	return;
    }

    if (!is_same_group(victim, pch))
    {
	act_p("�� �� �� �������� � ��� �� ������, ��� � $C1.", pch,0,victim,TO_CHAR,POS_RESTING);
	return;
    }

    if (IS_AFFECTED(pch,AFF_CHARM))
    {
	act_p("�� ������ ������ ������� ��� ������, ��� �� ������ �������� $C4!", pch,0,victim,TO_VICT,POS_RESTING);
	return;
    }

    if (victim->fighting != 0)
    {
	pch->send_to("������ �� ���� �� ��������� �� ������ ��������� ��������?\n\r");
	return;
    }

    if (pch->fighting != 0)
    {
	pch->send_to("������ ������� ���� ��������, � ����� ���������� � ������ ����-���� ���.\n\r");
	return;
    }
    
    for (gch = victim->guarding, cnt = 2; gch; gch = gch->guarding, cnt++)
	if (gch == pch) {
	    pch->printf( "%d �����%s, ������������ ����-�-����, ������������ ����� ����������� �������!\r\n", 
	                 cnt, GET_COUNT(cnt, "�", "�", "��") );
	    return;
	}
    
    act_p("������ �� ��������� $C4.", pch, 0, victim, TO_CHAR,POS_RESTING);
    act_p("������ ���� �������� $c4.", pch, 0, victim, TO_VICT,POS_RESTING);
    act_p("$c1 ������ �������� $C4.", pch, 0, victim, TO_NOTVICT,POS_RESTING);

    pch->guarding = victim;
    victim->guarded_by = pch;
}

BOOL_SKILL( guard )::run( Character *wch, Character *mob ) 
{
    int chance;
    PCharacter *ch = wch->getPC( );
    
    if (wch->is_npc( ))
	return false;
    
    if (ch->guarded_by == 0 || ch->guarded_by->in_room != ch->in_room)
	return false;
    
    chance = (gsn_guard->getEffective( ch->guarded_by ) -
    ( int )( 1.5 * ( ch->getModifyLevel() - mob->getModifyLevel() ) ) );

    if (number_percent() < min(100,chance))
    {
	act_p("$c1 ������� ����� $C5!", ch->guarded_by,0,ch,TO_NOTVICT,POS_RESTING);
	act_p("$c1 ������� ����� �����!", ch->guarded_by,0,ch,TO_VICT,POS_RESTING);
	act_p("�� �������� ����� $C5!", ch->guarded_by,0,ch,TO_CHAR,POS_RESTING);
	gsn_guard->improve( ch->guarded_by, true, mob );
	return true;
    }
    else
    {
	gsn_guard->improve( ch->guarded_by, false, mob );
	return false;
    }
}



SPELL_DECL(Dragonplate);
VOID_SPELL(Dragonplate)::run( Character *ch, char *target_name, int sn, int level ) 
{ 
  int plate_vnum;
  Object *plate;
  Affect af;

  plate_vnum = OBJ_VNUM_PLATE;

  plate = create_object( get_obj_index(plate_vnum), level + 5);
  plate->timer = 2 * level;
  plate->cost  = 0;
  plate->level  = ch->getRealLevel( );

  af.where        = TO_OBJECT;
  af.type         = sn;
  af.level        = level;
  af.duration     = -1;
  af.modifier     = ch->applyCurse( level / 8 );
  af.bitvector    = 0;

  af.location     = APPLY_HITROLL;
  affect_to_obj( plate, &af);

  af.location     = APPLY_DAMROLL;
  affect_to_obj( plate, &af);

  obj_to_char(plate, ch);

  act_p("�� ����������� ������ � �������� $o4!",ch,plate,0,TO_CHAR,POS_RESTING);
  act_p("$c1 ���������� ������ � ������� $o4!",ch,plate,0,TO_ROOM,POS_RESTING);

}

/*
 * golden weapon behavior
 */
bool KnightWeapon::death( Character *ch )
{
    bool wielded;
    
    wielded = (obj->wear_loc == wear_wield || obj->wear_loc == wear_second_wield);

    act_p( "���� ������� ������ ��������.", ch, 0, 0, TO_CHAR, POS_DEAD );
    act( "������� ������ $c2 ��������.", ch, 0, 0, TO_ROOM );
    extract_obj( obj );
    
    if (!wielded || ch->is_npc( ) || chance( 80 ))
	return false;
    
    ch->hit = 1;
    
    while (ch->affected)
	affect_remove( ch, ch->affected );

    ch->unsetLastFightTime( );
    SET_DEATH_TIME(ch);
    return true;
}

void KnightWeapon::fight( Character *ch )
{
    int sn = -1;
    
    if (obj->wear_loc != wear_wield && obj->wear_loc != wear_second_wield)
	return;
    
    if (chance( 3 )) 
	sn = gsn_cure_critical;
    else if (chance( 8 ))
	sn = gsn_cure_serious;

    if (sn > 0) {
	act("$o1 ���������� ����� ������� ������!", ch, obj, 0, TO_CHAR);
	act("$o1 $c2 ���������� ����� ������� ������!", ch, obj, 0,TO_ROOM);

	spell( sn, ch->getModifyLevel( ), ch, ch, FSPELL_BANE );
    }
}


/*
 * 'dragonsword' spell 
 */
SPELL_DECL(Dragonsword);
VOID_SPELL(Dragonsword)::run( Character *ch, char *target_name, int sn, int level ) 
{ 
  int sword_vnum;
  Object *sword;
  char arg[MAX_INPUT_LENGTH];
  Affect af;

  target_name = one_argument(target_name, arg);
  sword_vnum = 0;

  if (!str_cmp(arg, "sword"))
    sword_vnum = OBJ_VNUM_DRAGONSWORD;
  else if (!str_cmp(arg, "mace"))
    sword_vnum = OBJ_VNUM_DRAGONMACE;
  else if (!str_cmp(arg, "dagger"))
    sword_vnum = OBJ_VNUM_DRAGONDAGGER;
  else if (!str_cmp(arg, "lance"))
    sword_vnum = OBJ_VNUM_DRAGONLANCE;
  else
    {
      ch->send_to( "��� ���������� ���������� � {Y������ �������� �������{x!\r\n" );
      return;
    }

  sword = create_object( get_obj_index(sword_vnum), level);
  sword->timer = level * 2;
  sword->cost  = 0;
    if( level <= 30)			  sword->value[2] = 4;
    else if( level > 30 && level <= 40)   sword->value[2] = 5;
    else if( level > 40 && level <= 50)   sword->value[2] = 6;
    else if( level > 50 && level <= 60)   sword->value[2] = 7;
    else if( level > 60 && level <= 70)   sword->value[2] = 9;
    else if( level > 70 && level <= 80)   sword->value[2] = 10;
    else sword->value[2] = 11;
  sword->level = ch->getRealLevel( );

  af.where        = TO_OBJECT;
  af.type         = sn;
  af.level        = level;
  af.duration     = -1;
  af.modifier     = ch->applyCurse( level / 5 );
  af.bitvector    = 0;

  af.location     = APPLY_HITROLL;
  affect_to_obj( sword, &af);

  af.location     = APPLY_DAMROLL;
  affect_to_obj( sword, &af);

  if (IS_GOOD(ch))
	 SET_BIT(sword->extra_flags,(ITEM_ANTI_NEUTRAL | ITEM_ANTI_EVIL));
  else if (IS_NEUTRAL(ch))
	 SET_BIT(sword->extra_flags,(ITEM_ANTI_GOOD | ITEM_ANTI_EVIL));
  else if (IS_EVIL(ch))
	 SET_BIT(sword->extra_flags,(ITEM_ANTI_NEUTRAL | ITEM_ANTI_GOOD));	
  obj_to_char(sword, ch);

  act_p( "�� ����������� ������ � �������� $o4!",
         ch,sword,0,TO_CHAR,POS_RESTING);
  act_p( "$c1 ���������� ������ � ������� $o4!",
         ch,sword,0,TO_ROOM,POS_RESTING);

}




SPELL_DECL(GoldenAura);
VOID_SPELL(GoldenAura)::run( Character *ch, Room *room, int sn, int level ) 
{ 
    Character *vch;
  Affect af;

  for (vch = room->people; vch != 0; vch = vch->next_in_room)
    {
      if (!is_same_group(vch, ch))
	continue;

	if (spellbane( ch, vch ))
	    continue;

      if( vch->isAffected(sn ) ) {
          if (vch == ch)
           act("�� ��� ������$g��|�|�� {Y������� �����{x.", ch,0,0,TO_CHAR);
          else
           act("$C1 ��� ������$G��|�|�� {Y������� �����{x.", ch,0,vch,TO_CHAR);
	  continue;
	}
	
      af.where		= TO_AFFECTS;
      af.type      = sn;
      af.level	 = level;
      af.duration  = 6 + level;
      af.modifier  = 0;
      af.location  = APPLY_NONE;
      af.bitvector = AFF_PROTECT_EVIL;
      if( !IS_AFFECTED(vch, AFF_PROTECT_EVIL ) )affect_to_char( vch, &af );

      af.modifier = ch->applyCurse( level / 8 );
      af.location = APPLY_HITROLL;
      af.bitvector = 0;
      affect_to_char(vch, &af);


      af.modifier = ch->applyCurse( 0 - level / 8 );
      af.location = APPLY_SAVING_SPELL;
      affect_to_char(vch, &af);

      af.where		= TO_DETECTS;
      af.modifier = ch->applyCurse( level / 8 );
      af.location = APPLY_NONE;
      af.bitvector = DETECT_FADE;
      affect_to_char(vch, &af);

      af.bitvector = DETECT_EVIL;
      affect_to_char(vch, &af);

      vch->send_to("{Y������� ����{x �������� ����.\n\r");
      if ( ch != vch )
	act_p("{Y������� ����{x �������� $C4.",ch,0,vch,TO_CHAR,POS_RESTING);

    }

}




SPELL_DECL(HolyArmor);
VOID_SPELL(HolyArmor)::run( Character *ch, Character *, int sn, int level ) 
{ 
  Affect af;

  if ( ch->isAffected(sn ) )
    {
      ch->println("��������� ���� ��� �������� ���� �� �����������.");
      return;
    }

  af.where	= TO_AFFECTS;
  af.type      = sn;
  af.level     = level;
  af.duration  = level;
  af.location  = APPLY_AC;
  af.modifier  = ch->applyCurse( - max( 10, 10 * ( level / 5 ) ) );
  af.bitvector = 0;
  affect_to_char( ch, &af );
  act_p( "��������� ���� �������� $c4 �� �����������.",
          ch,0,0,TO_ROOM,POS_RESTING);
  ch->send_to("��������� ���� �������� ���� �� �����������.\n\r");


}

SPELL_DECL_T(Squire, SummonCreatureSpell);
TYPE_SPELL(NPCharacter *, Squire)::createMobile( Character *ch, int level ) const 
{
    NPCharacter *mob = createMobileAux( ch, ch->getModifyLevel( ), 
				     ch->max_hit, ch->max_mana,
				     number_range(level/20, level/15),
				     number_range(level/4, level/3),
				     number_range(level/10, level/8) );
    
    
    mob->setLongDescr( fmt( ch, mob->getLongDescr( ), ch ) );
    mob->setDescription( fmt( ch, mob->getDescription( ), ch ) );
    return mob;
}

/*-----------------------------------------------------------------
 * 'orden' command 
 *----------------------------------------------------------------*/
COMMAND(COrden, "orden")
{
    PCharacter *pch;
    const ClanOrgs *orgs;
    DLString arguments, cmd, arg;

    if (ch->is_npc( ))
	return;

    pch = ch->getPC( );
    
    if (pch->getClan( ) != clan_knight) {
	pch->println( "�� �� ������������ � ����� �������." );
	return;
    }
    
    if (!( orgs = clan_knight->getOrgs( ) )) {
	pch->println( "������ ������ ����������." );
	return;
    }
    
    arguments = constArguments;
    cmd = arguments.getOneArgument( );
    arg = arguments.getOneArgument( );
    
    if (cmd.empty( )) {
	doUsage( pch );
    }
    else if (arg_is_list( cmd )) {
	orgs->doList( pch );
    }
    else if (arg_oneof( cmd, "induct", "�������" )) {
	if (arg_is_self( arg ))
	    orgs->doSelfInduct( pch, arguments );
	else
	    orgs->doInduct( pch, arg );
    }
    else if (arg_oneof( cmd, "remove", "�������", "����" )) {
	if (arg_is_self( arg ))
	    orgs->doSelfRemove( pch );
	else
	    orgs->doRemove( pch, arg );
    }
    else if (arg_oneof( cmd, "members", "�����" )) {
	orgs->doMembers( pch );
    }
    else {
	doUsage( pch );
    }
}

void COrden::doUsage( PCharacter *pch )
{
    ostringstream buf;

    buf << "��� ����: " << endl
        << "{Worden list{x        - ���������� ������ �������" << endl
	<< "{Worden members{x     - ���������� ������ ������ ������" << endl
	<< "{Worden remove self{x - ����� �� ������" << endl
	<< endl
	<< "��� �����������: " << endl
	<< "{Worden induct <{xname{W>{x - ������� ����-���� � �����" << endl
	<< "{Worden remove <{xname{W>{x - ������� ����-���� �� ������" << endl
	<< endl
	<< "��� ������: " << endl
	<< "{Worden induct self <{xorden name{W>{x - ����� � ��������� �����" << endl;

    pch->send_to( buf );
}


/*----------------------------------------------------------------------------
 * KnightOrder 
 *---------------------------------------------------------------------------*/
KnightOrder::KnightOrder( )
              : classes( professionManager )
{
}

bool KnightOrder::canInduct( PCMemoryInterface *pci ) const
{
    return classes.isSet( pci->getProfession( ) );
}

const DLString & KnightOrder::getTitle( PCMemoryInterface *pci ) const
{
    return titles.build( pci );
}
