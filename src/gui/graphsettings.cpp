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

#include "graphsettings.h"
#include "ui_graphsettings.h"
#include "maingui.h"

namespace evoplex {

GraphSettings::GraphSettings(ColorMapMgr* cMgr, ExperimentPtr exp, QWidget *parent)
    : QDialog(parent, MainGUI::kDefaultDlgFlags),
      m_ui(new Ui_GraphSettings),
      m_cMgr(cMgr),
      m_exp(exp)
{
    m_ui->setupUi(this);
    connect(m_exp.get(), SIGNAL(restarted()), SLOT(init()));
}

GraphSettings::~GraphSettings()
{
    delete m_ui;
}

void GraphSettings::init()
{
    Q_ASSERT_X(m_exp->modelPlugin(), "GraphSettings",
               "tried to init the graph settings for a null model!");

    m_ui->nodesColor->init(m_cMgr, m_exp->modelPlugin()->nodeAttrsScope());

    m_ui->edgesColor->init(m_cMgr, m_exp->modelPlugin()->edgeAttrsScope());
}

AttrColorSelector* GraphSettings::nodeColorSelector() const
{
    return m_ui->nodesColor;
}

AttrColorSelector* GraphSettings::edgeColorSelector() const
{
    return m_ui->edgesColor;
}

} // evoplex
