/*  Menu Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Compiler.h"
#include "CommonFramework/ImageTools/ImageStats.h"
#include "CommonFramework/ImageTools/SolidColorTest.h"
#include "PokemonBDSP_MenuDetector.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{



MenuDetector::MenuDetector(Color color)
    : m_color(color)
    , m_line0(0.160 + 0.166 * 0, 0.110, 0.015, 0.488)
    , m_line1(0.160 + 0.166 * 1, 0.110, 0.015, 0.488)
    , m_line2(0.160 + 0.166 * 2, 0.110, 0.015, 0.488)
    , m_line3(0.160 + 0.166 * 3, 0.110, 0.015, 0.488)
    , m_line4(0.160 + 0.166 * 4, 0.110, 0.015, 0.488)
    , m_cross(0.20, 0.15, 0.60, 0.37)
{}

void MenuDetector::make_overlays(OverlaySet& items) const{
    items.add(m_color, m_line0);
    items.add(m_color, m_line1);
    items.add(m_color, m_line2);
    items.add(m_color, m_line3);
    items.add(m_color, m_line4);
    items.add(m_color, m_cross);
}
bool MenuDetector::detect(const QImage& screen) const{
    ImageStats stats0 = image_stats(extract_box(screen, m_line0));
    if (!is_white(stats0)){
        return false;
    }
    ImageStats stats1 = image_stats(extract_box(screen, m_line1));
    if (!is_white(stats1)){
        return false;
    }
    ImageStats stats2 = image_stats(extract_box(screen, m_line2));
    if (!is_white(stats2)){
        return false;
    }
    ImageStats stats3 = image_stats(extract_box(screen, m_line3));
    if (!is_white(stats3)){
        return false;
    }
    ImageStats stats4 = image_stats(extract_box(screen, m_line4));
    if (!is_white(stats4)){
        return false;
    }
    ImageStats cross = image_stats(extract_box(screen, m_cross));
//    cout << cross.average << cross.stddev << endl;
    if (cross.stddev.sum() < 100){
        return false;
    }
    return true;
}

void MenuWatcher::make_overlays(OverlaySet& items) const{
    MenuDetector::make_overlays(items);
}
bool MenuWatcher::process_frame(
    const QImage& frame,
    std::chrono::system_clock::time_point timestamp
){
    return detect(frame);
}



}
}
}
