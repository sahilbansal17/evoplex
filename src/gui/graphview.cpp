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

#include <QPainter>

#include "core/trial.h"

#include "graphview.h"
#include "ui_basegraphgl.h"
#include "ui_graphsettings.h"
#include "utils.h"

namespace evoplex {

GraphView::GraphView(ColorMapMgr* cMgr, ExperimentPtr exp, GraphWidget* parent)
    : BaseGraphGL(exp, parent),
      m_settingsDlg(new GraphSettings(cMgr, exp, this)),
      m_edgeAttr(-1),
      m_edgeCMap(nullptr),
      m_edgeSizeRate(25.),
      m_nodePen(Qt::black)
{
    setWindowTitle("Graph");

    connect(m_settingsDlg->nodeColorSelector(),
            SIGNAL(cmapUpdated(ColorMap*)), SLOT(setNodeCMap(ColorMap*)));
    connect(m_settingsDlg->edgeColorSelector(),
            SIGNAL(cmapUpdated(ColorMap*)), SLOT(setEdgeCMap(ColorMap*)));
    m_settingsDlg->init();

    m_showNodes = m_ui->bShowNodes->isChecked();
    m_showEdges = m_ui->bShowEdges->isChecked();
    connect(m_ui->bShowNodes, &QPushButton::clicked,
        [this](bool b) { m_showNodes = b; updateCache(); });
    connect(m_ui->bShowEdges, &QPushButton::clicked,
        [this](bool b) { m_showEdges = b; updateCache(); });

    updateNodePen();

    setTrial(0); // init at trial 0
}

GraphView::Star GraphView::createStar(const Node& node,
        const qreal& edgeSizeRate, const QPointF& xy)
{
    Star star;
    star.xy = xy;

    if (m_showNodes) {
        star.node = node;
    }

    if (m_showEdges) {
        star.edges.reserve(node.outEdges().size());
        for (auto const& ep : node.outEdges()) {
            QPointF xy2 = nodePoint(ep.second.neighbour(), edgeSizeRate);
            QLineF line(xy, xy2);
            if (!m_showNodes || line.length() - m_nodeRadius * 2. > 4.0) {
                star.edges.emplace_back(line); // just add the visible edges
            }
        }
        star.edges.shrink_to_fit();
    }

    return star;
}

CacheStatus GraphView::refreshCache()
{
    if (paintingActive()) {
        return CacheStatus::Scheduled;
    }
    Utils::clearAndShrink(m_cache);
    if (!m_trial || !m_trial->graph() || (!m_showNodes && !m_showEdges)) {
        return CacheStatus::Ready;
    }

    const int m = 50;
    QRect frame = frameGeometry().marginsAdded(QMargins(m,m,m,m));

    qreal edgeSizeRate = currEdgeSize();
    m_cache.reserve(m_trial->graph()->nodes().size());
    for (auto const& np : m_trial->graph()->nodes()) {
        QPointF xy = nodePoint(np.second, edgeSizeRate);
        if (!frame.contains(xy.toPoint())) {
            continue;
        }
        m_cache.emplace_back(createStar(np.second, edgeSizeRate, xy));
    }
    m_cache.shrink_to_fit();

    return CacheStatus::Ready;
}

void GraphView::paintEvent(QPaintEvent*)
{
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), m_background);

    if (m_cacheStatus != CacheStatus::Ready) {
        painter.end();
        return;
    }

    if (m_selectedStar.node.isNull()) {
        painter.setOpacity(1.0);
        drawEdges(painter);
    } else {
        painter.setOpacity(0.2);
    }
    const double nodeRadius = m_nodeRadius;
    drawNodes(painter, nodeRadius);
    drawSelectedStar(painter, nodeRadius);

    painter.end();
}

Node GraphView::selectNode(const QPoint& pos)
{
    if (m_cacheStatus == CacheStatus::Ready) {
        for (const Star& star : m_cache) {
            if (pos.x() > star.xy.x()-m_nodeRadius
                    && pos.x() < star.xy.x()+m_nodeRadius
                    && pos.y() > star.xy.y()-m_nodeRadius
                    && pos.y() < star.xy.y()+m_nodeRadius) {
                m_selectedStar = star;
                return star.node;
            }
        }
    }
    return Node();
}

void GraphView::setEdgeCMap(ColorMap* cmap)
{
    m_edgeCMap = cmap;
    m_edgeAttr = cmap ? cmap->attrRange()->id() : -1;
    update();
}

void GraphView::updateNodePen()
{
    if (m_nodeRadius < 8) {
        m_nodePen = QColor(100,100,100,0);
    } else if (m_nodeRadius < 13) {
        // 255/(13-8)*(x-8)
        m_nodePen = QColor(100,100,100,51*(m_nodeRadius-8.));
    } else {
        m_nodePen = QColor(100,100,100,255);
    }
}

void GraphView::drawNode(QPainter& painter, const Star& s, double r) const
{
    const Value& value = s.node.attr(m_nodeAttr);
    painter.setBrush(m_nodeCMap->colorFromValue(value));
    painter.drawEllipse(s.xy, r, r);
}

void GraphView::drawNodes(QPainter& painter, double nodeRadius) const
{
    if (!m_showNodes || m_nodeAttr < 0 || !m_nodeCMap) {
        return;
    }
    painter.save();
    painter.setPen(m_nodePen);
    for (const Star& star : m_cache) {
        if (star.node.isNull()) {
            break;
        }
        drawNode(painter, star, nodeRadius);
    }
    painter.restore();
}

void GraphView::drawEdges(QPainter& painter) const
{
    if (!m_showEdges) {
        return;
    }
    painter.save();
    painter.setPen(Qt::gray);
    for (const Star& star : m_cache) {
        for (const QLineF& edge : star.edges) {
            painter.drawLine(edge);
        }
    }
    painter.restore();
}

void GraphView::drawSelectedStar(QPainter& painter, double nodeRadius) const
{
    if (m_selectedStar.node.isNull()) {
        return;
    }

    painter.setOpacity(1.0);

    // draw shadow of the seleted node
    painter.save();
    double shadowRadius = nodeRadius*1.5;
    QRadialGradient r(m_selectedStar.xy, shadowRadius, m_selectedStar.xy);
    r.setColorAt(0, Qt::black);
    r.setColorAt(1, m_background.color());
    painter.setBrush(r);
    painter.setPen(Qt::transparent);
    painter.drawEllipse(m_selectedStar.xy, shadowRadius, shadowRadius);
    painter.restore();

    painter.save();
    // highlight immediate edges
    for (const QLineF& edge : m_selectedStar.edges) {
        painter.setPen(QPen(Qt::black, 3));
        painter.drawLine(edge);
    }

    // draw selected node
    painter.setPen(m_nodePen);
    drawNode(painter, m_selectedStar, nodeRadius);

    // draw neighbours
    const Edges& oe = m_selectedStar.node.outEdges();
    const double esize = currEdgeSize();
    for (auto const& e : oe) {
        const Node& n = e.second.neighbour();
        Star s(n, nodePoint(n, esize), {});
        drawNode(painter, s, nodeRadius);
    }
    painter.restore();
}

} // evoplex
