/* $Id: configs.cpp,v 1.1.2.9.6.8 2009/11/08 17:46:27 rufina Exp $
 *
 * ruffina, 2005
 * command syntax and messages from DreamLand 2.0
 */

#include "configs.h"
#include "jsoncpp/json/json.h"

#include "commandmanager.h"
#include "commandtemplate.h"
#include "commonattributes.h"
#include "pcharacter.h"
#include "merc.h"
#include "mercdb.h"
#include "interp.h"
#include "comm.h"
#include "act.h"
#include "arg_utils.h"
#include "def.h"

#define MILD(ch)     (IS_SET((ch)->comm, COMM_MILDCOLOR))

#define CLR_HEADER(ch)  (MILD(ch) ? "w" : "W")
#define CLR_NAME(ch)    (MILD(ch) ? "c" : "C")
#define CLR_YES(ch)     (MILD(ch) ? "g" : "R")
#define CLR_NO(ch)      (MILD(ch) ? "G" : "G")

static void config_scroll(Character *ch, const DLString &constArguments);
static void config_scroll_print(Character *ch);
static void config_telegram(PCharacter *ch, const DLString &constArguments);
static void config_telegram_print(PCharacter *ch);
static void config_discord(PCharacter *ch, const DLString &constArguments);
static void config_discord_print(PCharacter *ch);

/*-------------------------------------------------------------------------
 * ConfigElement
 *------------------------------------------------------------------------*/
const DLString & ConfigElement::getRussianName( ) const
{
    return rname;
}

void ConfigElement::init( )
{
    if (autocmd) {
        commandManager->registrate( Pointer( this ) );
    }
}

void ConfigElement::destroy( )
{
    if (autocmd) {
        commandManager->unregistrate( Pointer( this ) );
    }
}

void ConfigElement::run( Character *ch, const DLString & )
{
    ch->printf("Эта команда устарела, используй {hc{y{lRрежим %s{lEconfig %s{x.\r\n", 
               rname.c_str(), name.c_str());
}

bool ConfigElement::handleArgument( PCharacter *ch, const DLString &arg ) const
{
    if (arg.empty( )) {
        bool yes = isSetBit(ch);
        printLine( ch );
        ch->printf("\nИспользуй команду {hc{y{lRрежим %s %s{lEconfig %s %s{x для изменения.\r\n",
                      rname.c_str(), yes ? "нет" : "да", name.c_str(), yes ? "no" : "yes");
        return true;
    }
    
    Flags &field = getField( ch );

    if (arg_is_yes(arg) || arg_is_switch_on(arg))
        field.setBit( bit.getValue( ) );
    else if (arg_is_no(arg) || arg_is_switch_off(arg))
        field.removeBit( bit.getValue( ) );
    else if (arg_oneof(arg, "toggle", "переключить"))
        field.toggleBit( bit.getValue( ) );
    else 
        return false;
    
    if (!printText( ch ))
        printLine( ch );

    return true;
}

bool ConfigElement::isSetBit( PCharacter *ch ) const
{
    return getField( ch ).isSet( bit );
}

bool ConfigElement::printText( PCharacter *ch ) const
{
    const DLString &msg = (isSetBit( ch ) ? msgOn.getValue( ) : msgOff.getValue( ) );

    if (!msg.empty( )) {
        ch->println( msg );
        return true;
    }

    return false;
}

void ConfigElement::printRow( PCharacter *ch ) const
{
    bool yes = isSetBit( ch );
    bool rus = ch->getConfig( )->rucommands;

    ch->printf( "| {%s%-14s {x|  {%s%-7s {x|\r\n", 
                      CLR_NAME(ch), 
                      rus ? rname.getValue( ).c_str( ) : name.getValue( ).c_str( ), 
                      yes ? CLR_YES(ch) : CLR_NO(ch),
                      yes ? "ВКЛ." : "ВЫКЛ." );
}

static const char * LINE_FORMAT = "  {%s%-12s {%s%5s {x%s\r\n";
static const char * LINE_FORMAT_RU = "  {%s%-14s {%s%5s {x%s\r\n";

void ConfigElement::printLine( PCharacter *ch ) const
{
    bool yes = isSetBit( ch );

    if (ch->getConfig( )->rucommands)
        ch->printf( LINE_FORMAT_RU,
                        CLR_NAME(ch),
                        rname.getValue( ).c_str( ),
                        yes ? CLR_YES(ch) : CLR_NO(ch),
                        yes ? "ДА" : "НЕТ",
                        yes ? msgOn.c_str() : msgOff.c_str() );
    else
        ch->printf( LINE_FORMAT,
                        CLR_NAME(ch),
                        name.getValue( ).c_str( ),
                        yes ? CLR_YES(ch) : CLR_NO(ch),
                        yes ? "YES" : "NO",
                        yes ? msgOn.c_str() : msgOff.c_str() );
}

Flags & ConfigElement::getField( PCharacter *ch ) const
{
    static Flags zero;
    const FlagTable *table = bit.getTable( );

    if (table == &config_flags)
        return ch->config;
    
    if (table == &comm_flags)
        return ch->comm;
    
    if (table == &plr_flags)
        return ch->act;

    if (table == &add_comm_flags)
        return ch->add_comm;
    
    return zero;
}

/*-------------------------------------------------------------------------
 * ConfigGroup
 *------------------------------------------------------------------------*/
void ConfigGroup::printHeader( PCharacter *ch ) const
{
    ch->printf( "\r\n{%s%s{x\r\n", 
                    CLR_HEADER(ch),
                    name.getValue( ).c_str( ) );
}

/*-------------------------------------------------------------------------
 * ConfigCommand
 *------------------------------------------------------------------------*/
ConfigCommand * ConfigCommand::thisClass = NULL;

void ConfigCommand::initialization( )
{
    Groups::iterator g;
    
    thisClass = this;
    Class::regMoc<ConfigElement>( );
    Class::regMoc<ConfigGroup>( );
    Class::regMoc<ConfigCommand>( );
    CommandPlugin::initialization( );

    for (g = groups.begin( ); g != groups.end( ); g++) {
        ConfigGroup::iterator c;

        for (c = g->begin( ); c != g->end( ); c++) 
            (*c)->init( );
    }
}

void ConfigCommand::destruction( )
{
    Groups::iterator g;
    
    for (g = groups.begin( ); g != groups.end( ); g++) {
        ConfigGroup::iterator c;

        for (c = g->begin( ); c != g->end( ); c++) 
            (*c)->destroy( );
    }

    CommandPlugin::destruction( );
    Class::unregMoc<ConfigCommand>( );
    Class::unregMoc<ConfigGroup>( );
    Class::unregMoc<ConfigElement>( );
    thisClass = NULL;
}

void ConfigCommand::printAllRows( PCharacter *pch ) const
{
    Groups::const_iterator g;
    ConfigGroup::const_iterator c;

    for (g = groups.begin( ); g != groups.end( ); g++) 
        for (c = g->begin( ); c != g->end( ); c++) 
            if ((*c)->available( pch ))
                if ((*c)->autolist.getValue( ) && !(*c)->autotext.getValue( )) 
                    (*c)->printRow( pch );
}

void ConfigCommand::printAllTexts( PCharacter *pch ) const
{
    Groups::const_iterator g;
    ConfigGroup::const_iterator c;

    for (g = groups.begin( ); g != groups.end( ); g++) 
        for (c = g->begin( ); c != g->end( ); c++) 
            if ((*c)->available( pch ))
                if ((*c)->autolist.getValue( ) && (*c)->autotext.getValue( )) 
                    (*c)->printText( pch );
}

COMMAND(ConfigCommand, "config")
{
    PCharacter *pch;
    DLString arguments, arg1, arg2;
    Groups::iterator g;
    ConfigGroup::iterator c;

    if (ch->is_npc( ))
        return;
    
    pch = ch->getPC( );

    arguments = constArguments;
    arg1 = arguments.getOneArgument( );
    arg2 = arguments.getOneArgument( );

    if (arg1.empty( )) {
        for (g = groups.begin( ); g != groups.end( ); g++) {
            g->printHeader( pch );
            
            for (c = g->begin( ); c != g->end( ); c++) 
                if ((*c)->available( pch ))
                    (*c)->printLine( pch );
        }

        config_scroll_print(pch);
        config_telegram_print(pch);
        config_discord_print(pch);
        ch->println("\r\nПодробнее смотри по команде {lR{yрежим {Dнастройка{w{lE{yconfig {Dнастройка{x.");
        return;
    }

    if (arg_oneof(arg1, "scroll", "экран", "буфер")) {
        config_scroll(pch, arg2);
        return; 
    }

    if (arg_oneof(arg1, "telegram", "телеграм")) {
        config_telegram(pch, arg2);
        return;
    }

    if (arg_oneof(arg1, "discord", "дискорд")) {
        config_discord(pch, arg2);
        return;
    }

    for (g = groups.begin( ); g != groups.end( ); g++) 
        for (c = g->begin( ); c != g->end( ); c++) 
            if ((*c)->available( pch ) 
                    && ((*c)->matches( arg1 ) || (*c)->matchesAlias( arg1 ))) 
            {
                if (!(*c)->handleArgument( pch, arg2 ))
                    pch->println("Неправильный переключатель. См. {W? {lRрежим{lEconfig{x.");

                return;
            }

    
    pch->println("Опция не найдена. Используй {hc{y{lRрежим{lEconfig{x для списка.");
}

/*-------------------------------------------------------------------------
 * 'autolist' command 
 *------------------------------------------------------------------------*/
CMDRUN( autolist )
{
    ch->println("Эта команда устарела, используй {hc{y{lRрежим{lEconfig{x для списка настроек.");
}


/*-------------------------------------------------------------------------
 * 'scroll' command 
 *------------------------------------------------------------------------*/
CMDRUN( scroll )
{
    ch->println("Эта команда устарела, используй {hc{y{lRрежим буфер{lEconfig scroll{x.");
}

static void config_scroll_print(Character *ch)
{
    DLString lines(ch->lines);
    DLString msg;
    if (ch->lines == 0)
        msg = "Ты получаешь длинные сообщения без буферизации.";
    else
        msg = dlprintf("Тебе непрерывно выводится %d лин%s текста.",
                    ch->lines.getValue( ), GET_COUNT(ch->lines.getValue( ), "ия","ии","ий") );

    if (ch->getConfig()->rucommands)
        ch->printf(LINE_FORMAT_RU,
                   CLR_NAME(ch), "буфер", CLR_NO(ch), lines.c_str(), msg.c_str());
    else
        ch->printf(LINE_FORMAT,
                   CLR_NAME(ch), "scroll", CLR_NO(ch), lines.c_str(), msg.c_str());
}

static void config_scroll(Character *ch, const DLString &constArguments)
{
    int lines;
    DLString arg;

    arg = constArguments;
    arg = arg.getOneArgument( );

    if (arg.empty( ))
    {
        config_scroll_print(ch);
        ch->println("Для изменения используй {y{lRрежим буфер{lEconfig scroll{x число.");
        return;
    }

    if (!arg.isNumber( )) {
        ch->send_to("Ты должен ввести количество линий.\n\r");
        return;
    }

    try {
        lines = arg.toInt( );
    }
    catch (const ExceptionBadType& ) {
        ch->send_to("Неправильное число.\r\n");
        return;
    }

    if (lines == 0)
    {
        ch->send_to("Буферизация вывода отключена.\n\r");
        ch->lines = 0;
        return;
    }

    if (lines < 10 || lines > 100)
    {
        ch->send_to("Введи значение между 10 и 100.\n\r");
        return;
    }

    ch->lines = lines;
    ch->printf( "Вывод установлен на %d лин%s.\n\r", lines,
                GET_COUNT(lines, "ию","ии","ий") );
}

/**
 * Print Telegram handle.
 */
static void config_telegram_print(PCharacter *ch)
{
    const DLString &user = get_string_attribute(ch, "telegram");
    bool yes = !user.empty();
    DLString msg = yes ? dlprintf("Твой персонаж связан с пользователем Telegram {C%s{x.", user.c_str()) :
                        "Твой персонаж не связан с пользователями Telegram, набери {y{hc{lEconfig telegram{lRрежим телеграм{x.";

    if (ch->getConfig()->rucommands)
        ch->printf(LINE_FORMAT_RU,
                   CLR_NAME(ch), "телеграм", yes ? CLR_YES(ch) : CLR_NO(ch), 
                   yes ? "ДА" : "НЕТ", msg.c_str());
    else
        ch->printf(LINE_FORMAT,
                   CLR_NAME(ch), "telegram", yes ? CLR_YES(ch) : CLR_NO(ch), 
                   yes ? "YES" : "NO", msg.c_str());
}

/**
 * Link your character to a Telegram handle.
 */
static void config_telegram(PCharacter *ch, const DLString &constArguments)
{
    DLString arg = constArguments;
    XMLStringAttribute::Pointer user = ch->getAttributes().getAttr<XMLStringAttribute>("telegram");

    if (arg.empty()) {
        if (!user->empty())
            ch->printf("Твой персонаж связан с пользователем Telegram {C%s{x.\r\n"
                       "Используй {hc{y{lRрежим телеграм очистить{lEconfig telegram clear{x для очистки.\r\n", 
                       user->getValue().c_str());
        else
            ch->println("Твой персонаж не связан с пользователями Telegram.\r\n"
                        "Используй {y{lRрежим телеграм @имя{lEconfig telegram @username{x для установки.");        
        return;
    }

    if (arg_is_clear(arg)) {
        if (user->empty()) 
            ch->println("Нечего очищать.");
        else {
            ch->printf("Связь с пользователем {C%s{x очищена.\r\n", user->getValue().c_str());
            user->clear();
        }
        return;
    }

    if (arg.at(0) != '@') {
        ch->println("Задай имя пользователя начиная с @.");
        return;
    }

    user->setValue(arg);
    ch->printf("Теперь {C%s{x привязан к твоему персонажу.\r\n", arg.c_str());
}

static void config_discord_print(PCharacter *ch)
{
    ostringstream msg;

    Json::Value discord;
    get_json_attribute(ch, "discord", discord);
    bool yes = !discord["id"].empty();
    
    if (yes)
        msg << "Пользователь " << discord["username"] 
            << " (" << discord["id"] << "), статус " << discord["status"] 
            << ", секретное слово " << discord["token"];
    else
        msg << "Твой персонаж не связан с пользователем Discord, набери {y{hc{lRрежим дискорд{lEconfig discord{x";

    if (ch->getConfig()->rucommands)
        ch->printf(LINE_FORMAT_RU,
                   CLR_NAME(ch), "дискорд", yes ? CLR_YES(ch) : CLR_NO(ch), 
                   yes ? "ДА" : "НЕТ", msg.str().c_str());
    else
        ch->printf(LINE_FORMAT,
                   CLR_NAME(ch), "discord", yes ? CLR_YES(ch) : CLR_NO(ch), 
                   yes ? "YES" : "NO", msg.str().c_str());
}

static void config_discord(PCharacter *ch, const DLString &constArguments)
{
    DLString arg = constArguments;
    ostringstream buf;
    Json::Value discord;
    get_json_attribute(ch, "discord", discord);

    if (arg_is_clear(arg)) {
        bool linked = !discord["id"].empty();
        discord.clear();
        discord["token"] = create_nonce(6);
        set_json_attribute(ch, "discord", discord);

        if (linked)
            ch->send_to("Связь с пользователем Discord очищена. ");

        ch->printf("Новое секретное слово: {W%s{x.\r\n",
                   discord["token"].asString().c_str());
        return;
    }

    if (discord["token"].empty()) {
        discord["token"] = create_nonce(6);
        set_json_attribute(ch, "discord", discord);
    }

    if (discord["id"].empty()) {
        buf << "Для связи этого персонажа с пользователем Discord: " << endl
            << "  {W*{x зайди на сервер {hlhttps://discord.gg/RPaz6ut{x" << endl
            << "  {W*{x зайди на канал {W#dreamland{x или открой приват с ботом {WВалькирия{x" << endl
            << "  {W*{x набери {W/link " << discord["token"].asString() << "{x" << endl
            << "Для смены секретного слова набери {hc{y{lRрежим дискорд очистить{lEconfig discord clear{x." << endl;
    } else {
        buf << "Твой персонаж связан с пользователем " << discord["username"] << "(" 
            << discord["id"] << "), статус " << discord["status"] << endl;
        buf << "Для повторной линковки набери {W/link " << discord["token"] <<"{x в привате с ботом {WВалькирия{x" << endl
            << "Для очистки и смены секретного слова набери {hc{y{lRрежим дискорд очистить{lEconfig discord clear{x.";
    }

    ch->send_to(buf);
}
