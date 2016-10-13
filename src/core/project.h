/**
 * Copyright (C) 2016 - Marcos Cardinot
 * @author Marcos Cardinot <mcardinot@gmail.com>
 */

#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>

#include "core/environment.h"
#include "core/mainapp.h"
#include "core/simulation.h"

class Project: public QObject
{
    Q_OBJECT

public:
    Project(const MainApp& mainapp, MainApp::Model* model,
            const QString& name, const QString& descr="", const QString& dir="");

    virtual ~Project();

    Simulation* newSimulation(Graph* graph, QList<QMetaProperty> params);

    inline const QString& getName() { return m_name; }
    inline const QString& getDir() { return m_dir; }
    inline MainApp::Model* getModel() { return m_model; }

private:
    const MainApp& m_mainApp;
    MainApp::Model* m_model;
    QString m_name;
    QString m_description;
    QString m_dir;
};

#endif // PROJECT_H
