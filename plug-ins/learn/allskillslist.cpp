/* $Id: allskillslist.cpp,v 1.1.2.10.10.9 2008/07/31 04:56:05 rufina Exp $
 *
 * ruffina, 2004
 */
#include "allskillslist.h"

#include "skill.h"
#include "skillgroup.h"
#include "spell.h"
#include "skillmanager.h"
#include "pcharacter.h"

#include "act.h"
#include "loadsave.h"
#include "merc.h"
#include "mercdb.h"
#include "def.h"

PROF(universal);

const char * SkillInfo::colorLearned( )
{
    return color( learned );
}
const char * SkillInfo::colorReal( )
{
    return color( real );
}
const char * SkillInfo::color( int x )
{
    if (forgetting)
	return "{r";
    else if (x == 1)
	return "{R";
    else if (x >= maximum)
	return "{C";
    else if (x >= adept)
	return "{c";
    else 
	return "{x";
}

bool AllSkillsList::parse( DLString &argument, std::ostream &buf, Character *ch )
{
    DLString arg1, arg2;

    arg1 = argument.getOneArgument( );
    arg2 = argument.getOneArgument( );
    
    group = NULL;
    levLow = 1;
    levHigh = MAX_LEVEL;
    criteria = &SkillInfo::cmp_by_level;
    fUsableOnly = false;
    fShowHint = false;

    if (arg1.empty( )) {
        arg1 = "now";
        fShowHint = true;
    }

    if (arg_oneof( arg1, "help", "?", "������", "�������" )) {
        DLString what = fSpells ? "����������" : "������";
        if (ch->getProfession( ) == prof_universal) 
            buf 
            << "        {y{lR" << rcmd << "{lE" << cmd << "{lx{w: ��� ��������� ������ � ��������� ��������� "  << what << endl;
        else            
            buf 
            << "        {y{lR" << rcmd << "{lE" << cmd << "{lx{w: ��� ��������� �� ���� ������ "  << what << endl;

        buf << "        {y{lR" << rcmd << " ���{lE" << cmd << " all{lx{w: ��������� �� ���� ������� "  << what << endl;

        if (ch->getProfession( ) == prof_universal) 
            buf 
            << "        {y{lR" << rcmd << " �����{lE" << cmd << " curr{lx{w: ��� ��������� �� ���� ������ " << what << endl;

        buf << "        {y{lR" << rcmd << "{lE" << cmd << "{lx <�������>{w: " << what << ", ������� �������� �� ���� ������" << endl;
        buf << "        {y{lR" << rcmd << "{lE" << cmd << "{lx <�������1> <�������2>{w: " << what << ", ������� �������� �� ���� ��������� �������" << endl;
        buf << "        {y{lR" << rcmd << " ���� ���|�������|�������{lE" << cmd << " sort name|level|learned{lx{w: ����������� " << what << endl;
        buf << "        {y{lR" << rcmd << "{lE" << cmd << "{lx <�������� ������>{w: ��� " << what << " �� ���� ������" << endl
            << "" << endl
            << "��. ����� {W{lR? ������{lE? skills{lx{w, {W{lR? ������������{lE? glist{lx{w." << endl;

	return false;
    }
    
    if (arg_is_all( arg1 )) {
    }
    else if (arg_oneof_strict( arg1, "now", "������" )) {
	fUsableOnly = true;
    }
    else if (arg_oneof( arg1, "current", "�������" )) {
	levLow = 1;
	levHigh = ch->getRealLevel( );
    }
    else if (arg1.isNumber( )) {
	try {
	    levLow = arg1.toInt( );
	    
	    if (!arg2.empty( )) {
		if (!arg2.isNumber( )) {
		    buf << "������������ �������� �������." << endl;
		    return false;
		}

		levHigh = arg2.toInt( );
	    }
	    else
		levHigh = levLow;
	} 
	catch (const ExceptionBadType &e) {
	    buf << "������������ �������� �������." << endl;
	    return false;
	}
    }
    else if (arg_oneof( arg1, "sortby", "�����������" )) {
	if (arg2.empty( )) {
	    buf << "������� �������� ���������� ('{lEname{lR���{lx', '{lElevel{lR�������{lx' ��� '{lElearned{lR�������{lx')." << endl;
	    return false;
	}
	
	if (arg_oneof( arg2, "name", "���" ))
	    criteria = SkillInfo::cmp_by_name;
	else if (arg_oneof( arg2, "level", "�������" ))
	    criteria = SkillInfo::cmp_by_level;
	else if (arg_oneof( arg2, "learned", "�������" ))
	    criteria = SkillInfo::cmp_by_learned;
	else {
	    buf << "������������ �������� ����������." << endl;
	    return false;
	}
    }
    else {
	group = skillGroupManager->findUnstrict( arg1 );

	if (!group) {
	    buf << "����� ������ �� ����������." << endl;
	    return false;
	}
    }
    
    fRussian = ch->getConfig( )->ruskills;
    return true;
}

void AllSkillsList::make( Character *ch )
{
    SkillInfo info;

    for (int sn = 0; sn < SkillManager::getThis( )->size( ); sn++) {
	Skill *skill = SkillManager::getThis( )->find( sn );
	Spell::Pointer spell = skill->getSpell( );

	if (!skill->visible( ch ))
	    continue;

	if (fUsableOnly && !skill->usable( ch, false ))
	    continue;
    
	if (fSpells != (spell && spell->isCasted( )))
	    continue;
	
	if (group && skill->getGroup( ) != group)
	    continue;

	info.level = skill->getLevel( ch );

	if (info.level > levHigh || info.level < levLow)
	    continue;
	
	info.name = skill->getNameFor( ch );
	info.real = skill->getEffective( ch );
	info.spell = fSpells;
	info.available = skill->available( ch );
	info.maximum = skill->getMaximum( ch );
	
	if (ch->is_npc( )) {
	    info.learned = skill->getLearned( ch );
	    info.forgetting = false;
	    info.adept = info.learned;
	}
	else {
	    PCSkillData &data = ch->getPC( )->getSkillData( sn );
	    info.learned = data.learned;
	    info.forgetting = data.forgetting;
	    info.adept = skill->getAdept( ch->getPC( ) );
	}
	
	if (spell && spell->isCasted( ))
	    info.mana = spell->getManaCost( ch );
	else
	    info.mana = skill->getMana( );
	
	push_back( info );
    }

    sort( criteria );
}


void AllSkillsList::display( std::ostream & buf )
{
    int prevLevel = 0, firstColumn = true;

    if (empty( )) {
	buf << "�� ������� �� ������ " << (fSpells ? "����������" : "������") << "." << endl;
	return;
    }
    
    buf << "{W=========================================================="
	<< (fRussian ? "{x" : "====================={x")
	<< endl
        << dlprintf( (fRussian ? 
                       "%7s| %-30s| %-7s |%4s{W|{x " :
                       "%7s| %-18s| %-7s |%4s{W|{x "),
	              "�������", (fSpells ? "����������" : "������"), "�������", "����" )
	<< dlprintf( (fRussian ? 
	               "" : "%-18s| %-7s |%4s"),
	             (fSpells ? "����������" : "������"), "�������", "����")
	<< endl
	<< (fRussian ?
	    "{W-------+----------------------------------+---------+----+{x" : 
            "{W-------+--------------------+---------+----+--------------------+---------+----{x")
	<< endl;

    for (iterator i = begin( ); i != end( ); i++) {
	SkillInfo info = *i;
	
	if (info.level != prevLevel) {
	    if (!firstColumn) {
		firstColumn = true;
		buf << "                    |         |" << endl;
	    }
	    
	    buf << dlprintf( "  %3d  |", info.level );
	} 
	else if (firstColumn)
	    buf << "       |";
	
	if (fRussian)
	    buf << dlprintf( " {c%-30s{x|", info.name.c_str( ) );
	else
	    buf << dlprintf( " {c%-18s{x|", info.name.c_str( ) );

	if (info.available)
	    buf << dlprintf( " %s%3d{x(%s%3d{x)|", 
	                      info.colorLearned( ), info.learned, 
			      info.colorReal( ), info.real );
	else
	    buf << "   n/a   |";
	
	if (info.mana > 0 && info.available)
	    buf << dlprintf( fRussian ? " %3d" : " %-3d", info.mana );
	else
	    buf << "    ";
	
	if (firstColumn)
	    buf << "{W|{x";
	else 
	    buf << endl;
	
	prevLevel = info.level;

	if (!fRussian)
	    firstColumn = !firstColumn;
	else
	    buf << endl;
    }

    if (!firstColumn) 
	buf << "                    |         |" << endl;

    if (fShowHint)
        buf << endl << "��. ����� {W{lR" << rcmd << "{lE" << cmd << "{lx ?{w." << endl;
}