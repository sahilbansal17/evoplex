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

#ifndef BASEGRAPHGL_H
#define BASEGRAPHGL_H

#include <QOpenGLWidget>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTimer>
#include <vector>

#include "core/experiment.h"

#include "colormap.h"
#include "experimentwidget.h"
#include "graphwidget.h"
#include "maingui.h"

class Ui_BaseGraphGL;

namespace evoplex {

enum class CacheStatus {
    Ready,
    Updating,
    Scheduled
};

class GraphGLInterface
{
public:
    virtual void openSettings() = 0;

protected:
    virtual ~GraphGLInterface() = default;
    virtual void paintEvent(QPaintEvent*) = 0;
    virtual Node selectNode(const QPoint& pos) = 0;
    virtual Node selectedNode() const = 0;
    virtual void clearSelection() = 0;
    virtual CacheStatus refreshCache() = 0;
};

class BaseGraphGL : public QOpenGLWidget, public GraphGLInterface
{
    Q_OBJECT

public:
    ~BaseGraphGL() override;

protected:
    explicit BaseGraphGL(ExperimentPtr exp, GraphWidget* parent);

    Ui_BaseGraphGL* m_ui;
    GraphWidget* m_graphWidget;
    ExperimentPtr m_exp;
    const Trial* m_trial;

    int m_currStep;
    int m_nodeAttr;
    ColorMap* m_nodeCMap;

    QBrush m_background;
    int m_zoomLevel;
    qreal m_nodeSizeRate;
    qreal m_nodeRadius;
    QPointF m_origin;

    CacheStatus m_cacheStatus;

    CacheStatus refreshCache() override { return CacheStatus::Ready; }

    void clearSelection() override;

    void updateCache(bool force=false);

    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

signals:
    void updateWidgets(bool) const;

public slots:
    virtual void zoomIn();
    virtual void zoomOut();
    void updateView(bool forceUpdate);
    void setTrial(quint16 trialId);

private slots:
    void slotRestarted();
    void resetView();
    void setNodeCMap(ColorMap* cmap);

private:
    QTimer m_updateCacheTimer;
    QPointF m_posEntered;
    quint16 m_currTrialId;
    QMutex m_mutex;
    QRect m_inspGeo; // inspector geometry with margin
    std::vector<AttrWidget*> m_attrWidgets;

    void attrChanged(AttrWidget* aw) const;

    void setupInspector();

    void updateInspector(const Node& node);
};
}

#endif // BASEGRAPHGL_H
