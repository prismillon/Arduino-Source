/*  Dropdown Option
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <QStandardItemModel>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include "Common/Qt/NoWheelComboBox.h"
#include "DropdownWidget.h"

namespace PokemonAutomation{



ConfigWidget* DropdownCell::make_ui(QWidget& parent){
    return new DropdownCellWidget(parent, *this);
}
ConfigWidget* DropdownOption::make_ui(QWidget& parent){
    return new DropdownOptionWidget(parent, *this);
}




DropdownCellWidget::~DropdownCellWidget(){
    m_value.remove_listener(*this);
}
DropdownCellWidget::DropdownCellWidget(QWidget& parent, DropdownCell& value)
    : NoWheelComboBox(&parent)
    , ConfigWidget(value, *this)
    , m_value(value)
{
    for (const auto& item : m_value.case_list()){
        this->addItem(QString::fromStdString(item.display_name));
        if (item.enabled){
            continue;
        }
        auto* model = qobject_cast<QStandardItemModel*>(this->model());
        if (model == nullptr){
            continue;
        }
        QStandardItem* line_handle = model->item(this->count() - 1);
        if (line_handle != nullptr){
            line_handle->setEnabled(false);
        }
    }
    this->setCurrentIndex((int)m_value);

    connect(
        this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, [=](int index){
            if (index < 0){
                m_value.restore_defaults();
                return;
            }
            m_value.set_index(index);
        }
    );

    m_value.add_listener(*this);
}


void DropdownCellWidget::update(){
    ConfigWidget::update();
    this->setCurrentIndex((int)m_value);
}
void DropdownCellWidget::value_changed(){
    QMetaObject::invokeMethod(this, [=]{
        update();
    }, Qt::QueuedConnection);
}





DropdownOptionWidget::DropdownOptionWidget(QWidget& parent, DropdownOption& value)
    : QWidget(&parent)
    , ConfigWidget(value, *this)
    , m_cell(new DropdownCellWidget(*this, value))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    QLabel* text = new QLabel(QString::fromStdString(value.label()), this);
    text->setWordWrap(true);
    layout->addWidget(text, 1);
    layout->addWidget(m_cell, 1);
}


void DropdownOptionWidget::update(){
    ConfigWidget::update();
    m_cell->update();
}





}
