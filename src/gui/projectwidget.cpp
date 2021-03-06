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

#include <QDebug>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QVariant>

#include "core/experimentsmgr.h"

#include "projectwidget.h"
#include "fontstyles.h"
#include "savedialog.h"
#include "titlebar.h"
#include "ui_projectwidget.h"

namespace evoplex {

ProjectWidget::ProjectWidget(ProjectPtr project, MainGUI* mainGUI, ProjectsPage* ppage)
    : PPageDockWidget(ppage),
      m_ui(new Ui_ProjectWidget),
      m_mainGUI(mainGUI),
      m_project(project)
{
    m_ui->setupUi(this);

    setObjectName("ProjectWidget");
    setWindowTitle(m_project->name());
    setFocusPolicy(Qt::StrongFocus);

    auto titleBar = new TitleBar(this);
    titleBar->setSubtitle("PROJECT");
    titleBar->setTitle(windowTitle());
    setTitleBarWidget(titleBar);

    m_ui->labelExps->setFont(FontStyles::subtitle2());

    connect(m_project.get(), SIGNAL(expAdded(int)), SLOT(slotInsertRow(int)));
    connect(m_project.get(), SIGNAL(expEdited(int)), SLOT(slotUpdateRow(int)));
    connect(m_project.get(), SIGNAL(expRemoved(int)), SLOT(slotRemoveRow(int)));
    connect(m_project.get(), SIGNAL(hasUnsavedChanges(bool)),
            SLOT(slotHasUnsavedChanges(bool)));

    m_ui->table->init(mainGUI->mainApp()->expMgr());

    connect(m_ui->table, SIGNAL(itemSelectionChanged()), SLOT(slotSelectionChanged()));
    connect(m_ui->table, SIGNAL(itemDoubleClicked(QTableWidgetItem*)),
            SLOT(onItemDoubleClicked(QTableWidgetItem*)));

    QSettings userPrefs;
    auto _visibleCols = QVariant::fromValue<QVariantList>({TableWidget::H_BUTTON,
            TableWidget::H_EXPID, TableWidget::H_MODEL, TableWidget::H_GRAPH });
    auto visibleCols = userPrefs.value("projectWidget/showColumns", _visibleCols).toList();
    if (visibleCols.isEmpty()) {
        visibleCols.append(_visibleCols);
    }

    QMenu* tableSettings = new QMenu(this);
    auto it = m_ui->table->headerLabels().begin();
    while (it != m_ui->table->headerLabels().end()) {
        auto a = new QAction(it.value(), this);
        a->setCheckable(true);
        connect(a, &QAction::toggled, [this, it](bool checked) {
            m_ui->table->setColumnHidden(it.key(), !checked);
        });
        a->setData(it.key());
        a->setChecked(visibleCols.contains(QVariant(it.key())));
        m_ui->table->setColumnHidden(it.key(), !a->isChecked());
        tableSettings->addAction(a);
        ++it;
    }

    auto bPauseAll = new QtMaterialIconButton(QIcon(":/icons/material/pauseall_white_24"), this);
    titleBar->addButton(bPauseAll, "pause all experiments");
    connect(bPauseAll, SIGNAL(pressed()), m_project.get(), SLOT(pauseAll()));
    auto bPlayAll = new QtMaterialIconButton(QIcon(":/icons/material/playall_white_24"), this);
    titleBar->addButton(bPlayAll, "play all experiments");
    connect(bPlayAll, SIGNAL(pressed()), m_project.get(), SLOT(playAll()));
    auto bSettings = new QtMaterialIconButton(QIcon(":/icons/material/settings_white_18"), this);
    titleBar->addButton(bSettings, "table settings");
    connect(bSettings, &QtMaterialIconButton::pressed, [tableSettings](){ tableSettings->exec(QCursor::pos()); });
}

ProjectWidget::~ProjectWidget()
{
    QVariantList visibleCols;
    int cols = m_ui->table->horizontalHeader()->count();
    for (int col = 0; col < cols; ++col) {
        if (!m_ui->table->isColumnHidden(col)) {
            visibleCols.append(col);
        }
    }
    QSettings userPrefs;
    userPrefs.setValue("projectWidget/showColumns", visibleCols);

    delete m_ui;
}

void ProjectWidget::closeEvent(QCloseEvent* event)
{
    if (m_project->isRunning()) {
        QMessageBox::StandardButton res = QMessageBox::question(this, "Evoplex",
                tr("There are running experiments in this project!\n"
                   "Would you like to close it anyway?"));
        if (res == QMessageBox::No) {
            event->ignore();
            return ;
        }
    }

    if (m_project->hasUnsavedChanges()) {
        QMessageBox::StandardButton res = QMessageBox::question(this, "Evoplex",
                tr("Do you want to save the changes you made to '%1'?\n"
                   "Your changes will be lost if you don’t save them.").arg(m_project->name()),
                QMessageBox::Discard | QMessageBox::Cancel | QMessageBox::Save, QMessageBox::Save);

        if (res == QMessageBox::Cancel || (res == QMessageBox::Save && !m_mainGUI->saveDialog()->save(m_project))) {
            event->ignore();
            return;
        }
    }

    emit (closed());
    event->accept();
    QDockWidget::closeEvent(event);
}

void ProjectWidget::fillRow(int row, const ExperimentPtr& exp)
{
    Q_ASSERT(exp && exp->inputs());

    m_ui->table->setSortingEnabled(false);

    // general stuff
    insertItem(row, TableWidget::H_EXPID, QString::number(exp->id()));
    insertItem(row, TableWidget::H_SEED, exp->inputs()->general(GENERAL_ATTR_SEED).toQString());
    insertItem(row, TableWidget::H_STOPAT, exp->inputs()->general(GENERAL_ATTR_STOPAT).toQString());
    insertItem(row, TableWidget::H_TRIALS, exp->inputs()->general(GENERAL_ATTR_TRIALS).toQString());

    if (exp->expStatus() == Status::Invalid) {
        m_ui->table->setSortingEnabled(true);
        return;
    }

    // lambda function to add the attributes of a plugin (ie, model or graph)
    auto pluginAtbs = [this, row](TableWidget::Header header, const QString& pluginId, const Attributes* attrs)
    {
        QString pluginAttrs = pluginId;
        for (const Value& v : attrs->values()) {
            pluginAttrs += QString(" | %1").arg(v.toQString());
        }
        QTableWidgetItem* item = new QTableWidgetItem(pluginAttrs);
        item->setTextAlignment(Qt::AlignCenter);
        item->setToolTip(pluginAttrs);
        //QFont font = item->font();
        //font.setItalic(true);
        //item->setFont(font);
        m_ui->table->setItem(row, header, item);
    };

    // model stuff
    pluginAtbs(TableWidget::H_MODEL, exp->modelId(), exp->inputs()->model());

    // graph stuff
    pluginAtbs(TableWidget::H_GRAPH, exp->graphId(), exp->inputs()->graph());

    m_ui->table->setSortingEnabled(true);
}

void ProjectWidget::slotInsertRow(int expId)
{
    ExperimentPtr exp = m_project->experiment(expId);
    fillRow(m_ui->table->insertRow(exp.get()), exp);
}

void ProjectWidget::slotUpdateRow(int expId)
{
    for (int row = 0; row < m_ui->table->rowCount(); ++row) {
        if (expId == m_ui->table->item(row, TableWidget::H_EXPID)->text().toInt()) {
            fillRow(row, m_project->experiment(expId));
            return;
        }
    }
    qFatal("failed to update row! It should never happen.");
}

void ProjectWidget::slotRemoveRow(int expId)
{
    for (int row = 0; row < m_ui->table->rowCount(); ++row) {
        if (expId == m_ui->table->item(row, TableWidget::H_EXPID)->text().toInt()) {
            m_ui->table->removeRow(row);
            return;
        }
    }
}

void ProjectWidget::insertItem(int row, TableWidget::Header header,
                               const QString& label, const QString& tooltip)
{
    QTableWidgetItem* item = new QTableWidgetItem(label);
    item->setTextAlignment(Qt::AlignCenter);
    item->setToolTip(tooltip);
    m_ui->table->setItem(row, header, item);
}

void ProjectWidget::slotSelectionChanged()
{
    if (m_ui->table->selectedItems().isEmpty()) {
        emit (expSelectionChanged(-1));
    } else {
        const int row = m_ui->table->selectedItems().first()->row();
        const int expId = m_ui->table->item(row, TableWidget::H_EXPID)->text().toInt();
        emit (expSelectionChanged(expId));
    }
}

void ProjectWidget::onItemDoubleClicked(QTableWidgetItem* item)
{
    int expId = m_ui->table->item(item->row(), TableWidget::H_EXPID)->text().toInt();
    emit (openExperiment(expId));
}

void ProjectWidget::slotHasUnsavedChanges(bool b)
{
    setWindowTitle(m_project->name() + (b ? "*" : ""));
    emit (hasUnsavedChanges(m_project->id()));
}

void ProjectWidget::clearSelection()
{
    m_ui->table->clearSelection();
}

} // evoplex
