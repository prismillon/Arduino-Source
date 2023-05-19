/*  TotK Bow Item Duper
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Cpp/PrettyPrint.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/InferenceInfra/InferenceRoutines.h"
#include "CommonFramework/Tools/StatsTracking.h"
#include "CommonFramework/Tools/VideoResolutionCheck.h"
#include "NintendoSwitch/NintendoSwitch_Settings.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_ScalarButtons.h"
#include "NintendoSwitch/Programs/NintendoSwitch_GameEntry.h"


#include "ZeldaTotK/ZeldaTotK_Settings.h"

#include "ZeldaTotK_BowItemDuper.h"

namespace PokemonAutomation {
namespace NintendoSwitch {
namespace ZeldaTotK {

using namespace Pokemon;

BowItemDuper_Descriptor::BowItemDuper_Descriptor()
    : SingleSwitchProgramDescriptor(
        "ZeldaTotK:BowItemDuper",
        "Zelda: TotK", "Bow Item Duper",
        "ComputerControl/blob/master/Wiki/Programs/ZeldaTotK/BowItemDuper.md",
        "Use the Bow Swap Glitch to farm any fusable items.",
        FeedbackType::REQUIRED,
        AllowCommandsWhenRunning::DISABLE_COMMANDS,
        PABotBaseLevel::PABOTBASE_12KB
    )
{}

struct BowItemDuper_Descriptor::Stats : public StatsTracker {
    Stats()
        : dupe_attempts(m_stats["Dupe Attempts"])
        , errors(m_stats["Errors"])
    {
        m_display_order.emplace_back("Dupe Attempts");
        m_display_order.emplace_back("Errors", true);
    }
    std::atomic<uint64_t>& dupe_attempts;
    std::atomic<uint64_t>& errors;
};
std::unique_ptr<StatsTracker> BowItemDuper_Descriptor::make_stats() const {
    return std::unique_ptr<StatsTracker>(new Stats());
}

BowItemDuper::BowItemDuper()
    : ATTEMPTS(
        "<b>Duplication Attempts:</b><br>The number of times you wish to run this routine.",
        LockWhileRunning::LOCKED,
        100
    )
    , TICK_DELAY(
        "<b>Delay Between Menu Mashing:</b><br>Adjustable delay for exiting and reentering the menu. This should not typically be lower than 5 nor exceed 15 for successful results.",
        LockWhileRunning::LOCKED,
        15
    )
    , GO_HOME_WHEN_DONE(false)
    , NOTIFICATION_STATUS_UPDATE("Status Update", true, false, std::chrono::seconds(3600))
    , NOTIFICATIONS({
        &NOTIFICATION_STATUS_UPDATE,
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
        })
{
    PA_ADD_OPTION(ATTEMPTS);
    PA_ADD_OPTION(TICK_DELAY);
    PA_ADD_OPTION(GO_HOME_WHEN_DONE);
    PA_ADD_OPTION(NOTIFICATIONS);
}

void BowItemDuper::program(SingleSwitchProgramEnvironment& env, BotBaseContext& context) {
    assert_16_9_720p_min(env.logger(), env.console);

    // just do a forever loop where we have to do stuff
    uint32_t c = 0;

    while (c < ATTEMPTS) {

        env.log("Current Attempts: " + tostr_u_commas(c));

        // zr to pull out bow
        pbf_press_button(context, BUTTON_ZR, 12, 12);
        // dpad up to try to fuse the item, release for a while to make sure fused
        pbf_press_dpad(context, DPAD_UP, 20, 200);
        
        // then go ahead and enter the menu and do the various menuing in order to actually perform glitch
        pbf_press_button(context, BUTTON_PLUS, 20, 150);

        // drop current bow
        pbf_press_button(context, BUTTON_A, 20, 20);
        pbf_press_dpad(context, DPAD_DOWN, 10, 10);
        pbf_press_button(context, BUTTON_A, 20, 20);
        // now go to other bow and equip
        pbf_press_dpad(context, DPAD_RIGHT, 10, 5);
        pbf_press_button(context, BUTTON_A, 15, 15);
        pbf_press_button(context, BUTTON_A, 20, 20);

        // NOW WE HAVE TO PRESS PLUS AS FAST AS POSSIBLE
        pbf_press_button(context, BUTTON_PLUS, 5, TICK_DELAY);
        pbf_press_button(context, BUTTON_PLUS, 5, 5);

        // navigate back to the "current" bow, then drop it
        pbf_press_dpad(context, DPAD_LEFT, 10, 10);
        pbf_press_button(context, BUTTON_A, 20, 20);
        pbf_press_dpad(context, DPAD_DOWN, 10, 10);
        pbf_press_button(context, BUTTON_A, 20, 20);

        // back to overworld
        pbf_press_button(context, BUTTON_PLUS, 20, 20);

        // turn around
        pbf_move_left_joystick(context, 128, 0, 10, 10);
        // wait half a second
        pbf_wait(context, (0.5 * TICKS_PER_SECOND));

        // pick up both bows
        pbf_press_button(context, BUTTON_A, 20, 20);
        pbf_press_button(context, BUTTON_A, 20, 20);

        // turn back around and wait before reopening menu
        pbf_move_left_joystick(context, 128, 255, 10, 10);
        // wait half a second
        pbf_wait(context, (0.5 * TICKS_PER_SECOND));

        // now go back into the menu
        pbf_press_button(context, BUTTON_PLUS, 20, 50);

        // reset the cursor location, then go back to overworld to start again
        pbf_press_dpad(context, DPAD_LEFT, 15, 15);

        pbf_press_button(context, BUTTON_PLUS, 20, 20);

        // wait for everything before we try anything else
        context.wait_for_all_requests();

        // increment counter, increment stats
        c++;
        stats.dupe_attempts++;
        env.update_stats();
        send_program_status_notification(env, NOTIFICATION_STATUS_UPDATE);

    }

    // then we can finish up
    GO_HOME_WHEN_DONE.run_end_of_program(context);
    send_program_finished_notification(env, NOTIFICATION_PROGRAM_FINISH);
}

}
}
}

