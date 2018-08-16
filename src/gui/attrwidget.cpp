/**
 *  This file is part of Evoplex.
 *
 *  Evoplex is a multi-agent system for networks.
 *  Copyright (C) 2018 - Marcos Cardinot <marcos@cardinot.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDebug>

#include "attrwidget.h"
#include "linebutton.h"

namespace evoplex {
AttrWidget::AttrWidget(AttributeRangePtr attrRange, QWidget* parent, QWidget* customWidget)
    : QWidget(parent),
      m_useCustomWidget(customWidget),
      m_attrRange(attrRange)
{
    m_widget = customWidget ? customWidget : newWidget(attrRange);
    m_widget->setAutoFillBackground(true); // important! see qt docs

    auto l = new QHBoxLayout(this);
    l->addWidget(m_widget);
    l->setMargin(0);
    l->setSpacing(0);
    setLayout(l);
}

Value AttrWidget::validate() const
{
    return m_attrRange->validate(value().toQString());
}

Value AttrWidget::value() const
{
    auto sp = qobject_cast<QSpinBox*>(m_widget);
    if (sp) return sp->value();

    auto dsp = qobject_cast<QDoubleSpinBox*>(m_widget);
    if (dsp) return dsp->value();

    auto chb = qobject_cast<QCheckBox*>(m_widget);
    if (chb) return chb->isChecked();

    auto le = qobject_cast<QLineEdit*>(m_widget);
    if (le) return le->text();

    auto lb = qobject_cast<LineButton*>(m_widget);
    if (lb) return lb->text();

    auto cb = qobject_cast<QComboBox*>(m_widget);
    if (cb) return cb->currentText();

    qFatal("unable to know the widget type.");
}

void AttrWidget::setValue(const Value& value)
{
    auto sp = qobject_cast<QSpinBox*>(m_widget);
    if (sp) { sp->setValue(value.toInt()); return; }

    auto dsp = qobject_cast<QDoubleSpinBox*>(m_widget);
    if (dsp) { dsp->setValue(value.toDouble()); return; }

    auto chb = qobject_cast<QCheckBox*>(m_widget);
    if (chb) { chb->setChecked(value.toBool()); return; }

    auto le = qobject_cast<QLineEdit*>(m_widget);
    if (le) { le->setText(value.toQString()); return; }

    auto lb = qobject_cast<LineButton*>(m_widget);
    if (lb) { lb->setText(value.toQString()); return; }

    auto cb = qobject_cast<QComboBox*>(m_widget);
    if (cb) {
        int idx = cb->findText(value.toQString());
        if (idx >= 0) { cb->setCurrentIndex(idx); return; }
    }

    qFatal("unable to know the widget type.");
}

QWidget* AttrWidget::newWidget(AttributeRangePtr attrRange)
{
    switch (attrRange->type()) {
    case AttributeRange::Double_Range: {
        auto sp = new QDoubleSpinBox(this);
        sp->setMaximum(attrRange->max().toDouble());
        sp->setMinimum(attrRange->min().toDouble());
        sp->setDecimals(8);
        sp->setButtonSymbols(QDoubleSpinBox::NoButtons);
        connect(sp, SIGNAL(editingFinished()), SIGNAL(editingFinished()));
        return sp;
    }
    case AttributeRange::Int_Range: {
        auto sp = new QSpinBox(this);
        sp->setMaximum(attrRange->max().toInt());
        sp->setMinimum(attrRange->min().toInt());
        sp->setButtonSymbols(QSpinBox::NoButtons);
        connect(sp, SIGNAL(editingFinished()), SIGNAL(editingFinished()));
        return sp;
    }
    case AttributeRange::Double_Set:
    case AttributeRange::Int_Set:
    case AttributeRange::String_Set: {
        auto sov = dynamic_cast<SetOfValues*>(attrRange.get());
        auto cb = new QComboBox(this);
        for (const Value& v : sov->values()) {
            cb->addItem(v.toQString());
        }
        connect(cb, SIGNAL(currentIndexChanged(int)), SIGNAL(editingFinished()));
        return cb;
    }
    case AttributeRange::Bool: {
        auto cb = new QCheckBox(this);
        connect(cb, SIGNAL(stateChanged(int)), SIGNAL(editingFinished()));
        return cb;
    }
    case AttributeRange::FilePath: {
        auto lb = new LineButton(this, LineButton::SelectTextFile);
        connect(lb->line(), SIGNAL(editingFinished()), SIGNAL(editingFinished()));
        return lb;
    }
    case AttributeRange::DirPath: {
        auto lb = new LineButton(this, LineButton::SelectDir);
        connect(lb->line(), SIGNAL(editingFinished()), SIGNAL(editingFinished()));
        return lb;
    }
    default:
        auto le = new QLineEdit(this);
        le->setText(attrRange->min().toQString());
        connect(le, SIGNAL(editingFinished()), SIGNAL(editingFinished()));
        return le;
    }
}

} // evoplex