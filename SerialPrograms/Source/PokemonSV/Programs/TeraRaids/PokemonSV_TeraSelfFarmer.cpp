/*  Tera Farmer
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <cmath>
#include <sstream>
#include "Common/Compiler.h"
#include "Common/Cpp/Exceptions.h"
#include "CommonFramework/GlobalSettingsPanel.h"
#include "CommonFramework/Tools/ErrorDumper.h"
#include "CommonFramework/Tools/StatsTracking.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
//#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "CommonFramework/Inference/BlackScreenDetector.h"
#include "CommonFramework/InferenceInfra/InferenceRoutines.h"
#include "NintendoSwitch/NintendoSwitch_Settings.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "NintendoSwitch/Programs/NintendoSwitch_GameEntry.h"
#include "Pokemon/Pokemon_Strings.h"
#include "Pokemon/Pokemon_Notification.h"
#include "PokemonSV/PokemonSV_Settings.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_DateSpam.h"
//#include "PokemonSV/Inference/PokemonSV_GradientArrowDetector.h"
#include "PokemonSV/Inference/PokemonSV_DialogDetector.h"
#include "PokemonSV/Inference/PokemonSV_PokemonSummaryReader.h"
#include "PokemonSV/Inference/PokemonSV_TeraCardDetector.h"
#include "PokemonSV/Inference/PokemonSV_BattleMenuDetector.h"
#include "PokemonSV/Inference/PokemonSV_MainMenuDetector.h"
//#include "PokemonSV/Inference/PokemonSV_PostCatchDetector.h"
//#include "PokemonSV/Programs/PokemonSV_GameEntry.h"
#include "PokemonSV/Programs/PokemonSV_Navigation.h"
#include "PokemonSV/Programs/PokemonSV_BasicCatcher.h"
#include "PokemonSV/Programs/TeraRaids/PokemonSV_TeraRoutines.h"
#include "PokemonSV/Programs/TeraRaids/PokemonSV_TeraBattler.h"
#include "PokemonSV_TeraSelfFarmer.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSV{

using namespace Pokemon;


TeraSelfFarmer_Descriptor::TeraSelfFarmer_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonSV:TeraSelfFarmer",
        STRING_POKEMON + " SV", "Tera Self Farmer",
        "ComputerControl/blob/master/Wiki/Programs/PokemonSV/TeraSelfFarmer.md",
        "Farm items from Tera raids.",
        FeedbackType::REQUIRED, false,
        PABotBaseLevel::PABOTBASE_12KB
    )
{}
struct TeraSelfFarmer_Descriptor::Stats : public StatsTracker{
    Stats()
        : m_skips(m_stats["Date Skips"])
        , m_raids(m_stats["Raids"])
        , m_wins(m_stats["Wins"])
        , m_losses(m_stats["Losses"])
        , m_skipped(m_stats["Skipped"])
        , m_errors(m_stats["Errors"])
        , m_caught(m_stats["Caught"])
        , m_shinies(m_stats["Shinies"])
    {
        m_display_order.emplace_back("Date Skips");
        m_display_order.emplace_back("Raids");
        m_display_order.emplace_back("Wins");
        m_display_order.emplace_back("Losses");
        m_display_order.emplace_back("Skipped");
        m_display_order.emplace_back("Errors", true);
        m_display_order.emplace_back("Caught", true);
        m_display_order.emplace_back("Shinies", true);
    }
    std::atomic<uint64_t>& m_skips;
    std::atomic<uint64_t>& m_raids;
    std::atomic<uint64_t>& m_wins;
    std::atomic<uint64_t>& m_losses;
    std::atomic<uint64_t>& m_skipped;
    std::atomic<uint64_t>& m_errors;
    std::atomic<uint64_t>& m_caught;
    std::atomic<uint64_t>& m_shinies;
};
std::unique_ptr<StatsTracker> TeraSelfFarmer_Descriptor::make_stats() const{
    return std::unique_ptr<StatsTracker>(new Stats());
}


const EnumDatabase<TeraSelfFarmer::Mode>& TeraSelfFarmer::database(){
    static EnumDatabase<TeraSelfFarmer::Mode> database0{
        {Mode::FARM_ITEMS_ONLY, "items-only",   "Items only. Don't catch anything."},
        {Mode::CATCH_ALL,       "catch-all",    "Catch everything using the specified ball."},
    };
    static EnumDatabase<TeraSelfFarmer::Mode> database1{
        {Mode::FARM_ITEMS_ONLY, "items-only",   "Items only. Don't catch anything."},
        {Mode::CATCH_ALL,       "catch-all",    "Catch everything using the specified ball."},
        {Mode::SHINY_HUNT,      "shiny-hunt",   "Save before each raid and catch. Stop program if shiny is found."},
    };
    return PreloadSettings::instance().DEVELOPER_MODE ? database1 : database0;
}


TeraSelfFarmer::TeraSelfFarmer()
    : MODE(
        "<b>Mode:</b>",
        database(),
        LockWhileRunning::LOCKED,
        Mode::FARM_ITEMS_ONLY
    )
    , LANGUAGE(
        "<b>Game Language:</b>",
        PokemonNameReader::instance().languages(),
        LockWhileRunning::UNLOCKED
    )
    , BALL_SELECT(
        "<b>Ball Select:</b>",
        LockWhileRunning::UNLOCKED,
        "poke-ball"
    )
    , MAX_CATCHES(
        "<b>Max Catches:</b><br>Stop program after catching this many " + STRING_POKEMON + ".",
        LockWhileRunning::UNLOCKED,
        50, 1, 999
    )
    , FIX_TIME_ON_CATCH(
        "<b>Fix Clock on Catch:</b><br>Fix the time when catching so the caught date will be correct.",
        LockWhileRunning::UNLOCKED, true
    )
    , TRY_TO_TERASTILIZE(
        "<b>Try to terastilize:</b><br>Blindly try to terastilize. Add 4s per move but allow to have a bigger win rate in 5* raids.",
        LockWhileRunning::UNLOCKED, false
    )
    , MAX_STARS(
        "<b>Max Stars:</b><br>Skip raids with more than this many stars to save time since you're likely to lose.",
        LockWhileRunning::UNLOCKED,
        4, 1, 7
    )
    , NOTIFICATION_STATUS_UPDATE("Status Update", true, false, std::chrono::seconds(3600))
    , NOTIFICATION_NONSHINY(
        "Non-Shiny Encounter",
        true, false,
        {"Notifs"},
        std::chrono::seconds(3600)
    )
    , NOTIFICATION_SHINY(
        "Shiny Encounter",
        true, true, ImageAttachmentMode::JPG,
        {"Notifs", "Showcase"}
    )
    , NOTIFICATIONS({
        &NOTIFICATION_STATUS_UPDATE,
        &NOTIFICATION_NONSHINY,
        &NOTIFICATION_SHINY,
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_RECOVERABLE,
        &NOTIFICATION_ERROR_FATAL,
    })
    , m_advanced_options(
        "<font size=4><b>Advanced Options:</b> You should not need to touch anything below here.</font>"
    )
    , RAID_SPAWN_DELAY(
        "<b>Raid Spawn Delay</b>",
        LockWhileRunning::UNLOCKED,
        TICKS_PER_SECOND,
        "3 * TICKS_PER_SECOND"
    )
{
    PA_ADD_OPTION(MODE);
    PA_ADD_OPTION(LANGUAGE);
    PA_ADD_OPTION(BALL_SELECT);
    PA_ADD_OPTION(MAX_CATCHES);
    PA_ADD_OPTION(FIX_TIME_ON_CATCH);
    PA_ADD_OPTION(TRY_TO_TERASTILIZE);
    PA_ADD_OPTION(MAX_STARS);
    PA_ADD_OPTION(NOTIFICATIONS);
}


void TeraSelfFarmer::process_catch_prompt(SingleSwitchProgramEnvironment& env, BotBaseContext& context){
    if (MODE == Mode::FARM_ITEMS_ONLY){
        pbf_press_dpad(context, DPAD_DOWN, 10, 10);
        pbf_mash_button(context, BUTTON_A, 125);
        return;
    }

    if (FIX_TIME_ON_CATCH){
        pbf_press_button(context, BUTTON_HOME, 10, GameSettings::instance().GAME_TO_HOME_DELAY);
        home_to_date_time(context, false, false);
        pbf_press_button(context, BUTTON_A, 20, 105);
        pbf_press_button(context, BUTTON_A, 20, 105);
        pbf_press_button(context, BUTTON_HOME, 20, ConsoleSettings::instance().SETTINGS_TO_HOME_DELAY);
        resume_game_from_home(env.console, context);
    }
    m_number_caught++;

    BattleBallReader reader(env.console, LANGUAGE);
//    pbf_mash_button(context, BUTTON_A, 250);
    pbf_press_button(context, BUTTON_A, 20, 150);
    context.wait_for_all_requests();

    int quantity = move_to_ball(reader, env.console, context, BALL_SELECT.slug());
    if (quantity == 0){
        throw FatalProgramException(env.logger(), "Unable to find appropriate ball. Did you run out?");
    }
    if (quantity < 0){
        env.log("Unable to read ball quantity.", COLOR_RED);
    }
    pbf_mash_button(context, BUTTON_A, 125);
}
void TeraSelfFarmer::read_summary(
    SingleSwitchProgramEnvironment& env,
    BotBaseContext& context,
    const VideoSnapshot& battle_snapshot
){
    PokemonSummaryDetector reader;
    VideoSnapshot screen = env.console.video().snapshot();
    if (!reader.detect(screen)){
        dump_image_and_throw_recoverable_exception(
            env, env.console, NOTIFICATION_ERROR_RECOVERABLE,
            "SummaryNotFound",
            "Unable to detect summary screen."
        );
    }

    if (reader.is_shiny(screen)){
        TeraSelfFarmer_Descriptor::Stats& stats = env.current_stats<TeraSelfFarmer_Descriptor::Stats>();
        stats.m_shinies++;
        send_encounter_notification(
            env,
            NOTIFICATION_NONSHINY,
            NOTIFICATION_SHINY,
            false, true,
            {{{}, ShinyType::UNKNOWN_SHINY}},
            std::nan(""),
            battle_snapshot
        );
        if (MODE == Mode::SHINY_HUNT){
            throw ProgramFinishedException();
        }
    }else{
        send_encounter_notification(
            env,
            NOTIFICATION_NONSHINY,
            NOTIFICATION_SHINY,
            false, false,
            {{{}, ShinyType::NOT_SHINY}},
            std::nan("")
        );
    }

    pbf_press_button(context, BUTTON_B, 20, 20);
}

bool TeraSelfFarmer::run_raid(SingleSwitchProgramEnvironment& env, BotBaseContext& context){
    TeraSelfFarmer_Descriptor::Stats& stats = env.current_stats<TeraSelfFarmer_Descriptor::Stats>();

    bool win = run_tera_battle(
        env, env.console, context,
        NOTIFICATION_ERROR_RECOVERABLE,
        true,
        TRY_TO_TERASTILIZE
    );

    if (win){
        stats.m_wins++;
    }else{
        stats.m_losses++;
        context.wait_for(std::chrono::seconds(3));
        return false;
    }

    if (MODE == Mode::FARM_ITEMS_ONLY){
        exit_tera_win_without_catching(env.console, context);
        return true;
    }

    VideoSnapshot battle_snapshot = env.console.video().snapshot();

    TeraResult result = exit_tera_win_by_catching(
        env.console, context,
        LANGUAGE,
        BALL_SELECT.slug(),
        MODE == Mode::SHINY_HUNT
    );

    stats.m_caught++;

    switch (result){
    case TeraResult::NO_DETECTION:
        send_encounter_notification(
            env,
            NOTIFICATION_NONSHINY,
            NOTIFICATION_SHINY,
            false, false,
            {{{}, ShinyType::UNKNOWN}},
            std::nan("")
        );
        break;
    case TeraResult::NOT_SHINY:
        send_encounter_notification(
            env,
            NOTIFICATION_NONSHINY,
            NOTIFICATION_SHINY,
            false, false,
            {{{}, ShinyType::NOT_SHINY}},
            std::nan("")
        );
        break;
    case TeraResult::SHINY:
        stats.m_shinies++;
        send_encounter_notification(
            env,
            NOTIFICATION_NONSHINY,
            NOTIFICATION_SHINY,
            false, true,
            {{{}, ShinyType::UNKNOWN_SHINY}},
            std::nan(""),
            battle_snapshot
        );
        if (MODE == Mode::SHINY_HUNT){
            throw ProgramFinishedException();
        }
    }
    return true;
}


void TeraSelfFarmer::program(SingleSwitchProgramEnvironment& env, BotBaseContext& context){
    TeraSelfFarmer_Descriptor::Stats& stats = env.current_stats<TeraSelfFarmer_Descriptor::Stats>();

    m_number_caught = 0;

    //  Connect the controller.
    pbf_press_button(context, BUTTON_LCLICK, 10, 10);

    bool first = true;

//    uint8_t year = MAX_YEAR;
    while (true){
        if (m_number_caught >= MAX_CATCHES){
            break;
        }

        env.update_stats();
        send_program_status_notification(env, NOTIFICATION_STATUS_UPDATE);


        //  Clear per-iteration flags.
//        m_battle_finished = false;
//        m_caught = false;
//        m_summary_read = false;


        if (!first){
            pbf_press_button(context, BUTTON_HOME, 10, GameSettings::instance().GAME_TO_HOME_DELAY);
//            home_roll_date_enter_game_autorollback(env.console, context, year);
            home_roll_date_enter_game(env.console, context, false);
            pbf_wait(context, RAID_SPAWN_DELAY);
            context.wait_for_all_requests();
            stats.m_skips++;
        }
        first = false;

        if (open_raid(env.console, context)){
//            env.log("Tera raid found!", COLOR_BLUE);
            stats.m_raids++;
        }else{
//            env.log("No Tera raid found.", COLOR_ORANGE);
            continue;
        }
        context.wait_for(std::chrono::milliseconds(500));

        VideoSnapshot screen = env.console.video().snapshot();
        TeraCardReader reader(COLOR_RED);
        size_t stars = reader.stars(screen);
        if (stars == 0){
            dump_image(env.logger(), env.program_info(), "ReadStarsFailed", *screen.frame);
        }else{
            env.log("Detected " + std::to_string(stars) + " star raid.", COLOR_PURPLE);
        }

        if (stars > MAX_STARS){
            env.log("Skipping raid...", COLOR_ORANGE);
            stats.m_skipped++;
            close_raid(env.console, context);
            continue;
        }

        if (MODE == Mode::SHINY_HUNT){
            close_raid(env.console, context);
            save_game_from_overworld(env.console, context);
            context.wait_for_all_requests();
            if (open_raid(env.console, context)){
                env.log("Tera raid found!", COLOR_BLUE);
            }else{
                env.log("No Tera raid found.", COLOR_ORANGE);
                continue;
            }
        }

        pbf_press_dpad(context, DPAD_DOWN, 10, 10);
        pbf_mash_button(context, BUTTON_A, 250);
        bool raid_won = run_raid(env, context);
        {
            std::stringstream ss;
            ss << "You ";
            if (raid_won){
                ss << "won";
            }
            else{
                ss << "lost";
            }
            ss << " a " << stars << " stars raid";
            env.log(ss.str());

        }
    }

    env.update_stats();
    send_program_finished_notification(env, NOTIFICATION_PROGRAM_FINISH);
}











}
}
}
