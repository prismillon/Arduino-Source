/*  Box Routines
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 *  Various functions to operate in box system, like release pokemon.
 */

#ifndef PokemonAutomation_PokemonSV_BoxRoutines_H
#define PokemonAutomation_PokemonSV_BoxRoutines_H

namespace PokemonAutomation{
    
    struct ProgramInfo;
    class ConsoleHandle;
    class BotBaseContext;

namespace NintendoSwitch{
namespace PokemonSV{

enum class BoxCursorLocation;

// Hold one column of at most five pokemon
void hold_one_column(BotBaseContext& context);

// Press button L to move to the box on the left
void move_to_left_box(BotBaseContext& context);

// Press button R to move to the box on the right
void move_to_right_box(BotBaseContext& context);

// Release current selected pokemon in the box system.
// If `ensure_empty` is true, it will make sure the pokemon is released and throw OperationFailedException if releasing failed.
// Note: if `ensure_empty` is true, this function will fail if releasing a party pokemon that is not the last of the party!
//       Because game will automatically fill the hole in the party. To release party pokemon, set `ensure_empty` to false.
bool release_one_pokemon(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context, bool ensure_empty = true);

// In box system, check how many slots in the party are empty
uint8_t check_empty_slots_in_party(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context);

// In box system, assuming the party is empty, load one column in the current box onto party.
// if has_clone_ride_pokemon is true, skip loading the first row of the box.
void load_one_column_to_party(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context,
    uint8_t column_index, bool has_clone_ride_pokemon = false);

// In box system, assuming the target column is empty, unload party (after the lead) to the target column.
// if has_clone_ride_pokemon is true, skip unloading the second row of the party.
void unload_one_column_from_party(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context,
    uint8_t column_index, bool has_clone_ride_pokemon = false);

// In box system, move the cursor to the desired slot.
void move_box_cursor(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context,
    const BoxCursorLocation& side, uint8_t row, uint8_t col);

// In box system, use button Y to swap two slots
void swap_two_box_slots(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context,
    const BoxCursorLocation& source_side, uint8_t source_row, uint8_t source_col,
    const BoxCursorLocation& target_side, uint8_t target_row, uint8_t target_col);

}
}
}
#endif
