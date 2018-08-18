#include "attrcolorselector.h"
#include "ui_attrcolorselector.h"

namespace evoplex {

AttrColorSelector::AttrColorSelector(QWidget* parent)
    : QWidget(parent),
      m_ui(new Ui_AttrColorSelector),
      m_cmapMgr(nullptr),
      m_cmap(nullptr)
{
    m_ui->setupUi(this);

    connect(m_ui->attr, SIGNAL(currentIndexChanged(int)), SLOT(updateCMap()));
    connect(m_ui->cmapName, SIGNAL(currentIndexChanged(QString)), SLOT(slotCMapName(QString)));
    connect(m_ui->cmapSize, SIGNAL(currentIndexChanged(int)), SLOT(updateCMap()));
}

AttrColorSelector::~AttrColorSelector()
{
    delete m_ui;
    delete m_cmap;
}

void AttrColorSelector::init(ColorMapMgr* cmapMgr, AttributesScope scope)
{
    m_cmapMgr = cmapMgr;
    m_attrScope = scope;

    blockAllSignals(true);
    m_ui->attr->clear();
    m_ui->cmapName->clear();
    m_ui->cmapSize->clear();
    blockAllSignals(false);

    if (m_attrScope.empty()) {
        return;
    }

    for (auto const& aRange : m_attrScope) {
        m_ui->attr->addItem(aRange->attrName());
    }
    m_ui->cmapName->insertItems(0, m_cmapMgr->names());
    m_ui->cmapName->setCurrentText(m_cmapMgr->defaultCMapKey().first);
    slotCMapName(m_cmapMgr->defaultCMapKey().first); // fill sizes
    m_ui->cmapSize->setCurrentText(QString::number(m_cmapMgr->defaultCMapKey().second));
    updateCMap();
}

void AttrColorSelector::updateCMap()
{
    delete m_cmap;
    m_cmap = nullptr;
    if (!m_attrScope.empty()) {
        auto attrRange = m_attrScope.value(m_ui->attr->currentText());
        m_cmap = ColorMap::create(attrRange, m_cmapMgr->colors(
            m_ui->cmapName->currentText(), m_ui->cmapSize->currentText().toInt()));
    }
    emit (cmapUpdated(m_cmap));
}

void AttrColorSelector::slotCMapName(const QString &name)
{
    m_ui->cmapSize->blockSignals(true);
    m_ui->cmapSize->clear();
    m_ui->cmapSize->insertItems(0, m_cmapMgr->sizes(name));
    m_ui->cmapSize->blockSignals(false);
    updateCMap();
}

void AttrColorSelector::blockAllSignals(bool b)
{
    m_ui->attr->blockSignals(b);
    m_ui->cmapName->blockSignals(b);
    m_ui->cmapSize->blockSignals(b);
}

} // evoplex
