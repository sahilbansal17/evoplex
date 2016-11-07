/**
 * Copyright (C) 2016 - Marcos Cardinot
 * @author Marcos Cardinot <mcardinot@gmail.com>
 */

#include <QFutureWatcher>
#include <QList>
#include <QtConcurrent/QtConcurrentRun>
#include <QtDebug>

#include "core/processesmgr.h"

ProcessesMgr::ProcessesMgr()
    : m_threads(QThread::idealThreadCount())
{
}

ProcessesMgr::~ProcessesMgr()
{
    killAll();
}

quint16 ProcessesMgr::add(Simulation* sim)
{
    quint16 key = m_processes.key(sim, 0);
    if (key == 0) {
        key = m_processes.lastKey() + 1;
        m_processes.insert(key, sim);
    }
    return key;
}

QList<quint16> ProcessesMgr::add(QList<Simulation*> sims)
{
    QList<quint16> ids;
    quint16 id = m_processes.lastKey();
    foreach (Simulation* sim, sims) {
        ids.append(id);
        m_processes.insert(id, sim);
        ++id;
    }
    return ids;
}

quint16 ProcessesMgr::addAndPlay(Simulation* sim)
{
    quint16 key = add(sim);
    play(key);
    return key;
}

QList<quint16> ProcessesMgr::addAndPlay(QList<Simulation*> sims)
{
   QList<quint16> keys = add(sims);
   play(keys);
   return keys;
}

void ProcessesMgr::play(quint16 id)
{
    if (m_runningProcesses.contains(id)
            && m_queuedProcesses.contains(id)) {
        return;
    } else if (!m_processes.contains(id)) {
        qWarning() << "[Processes] tried to play an nonexistent process:" << id;
        return;
    }

    if (m_runningProcesses.size() < m_threads) {
        m_runningProcesses.append(id);

        QFutureWatcher<quint16> watcher;
        connect(&watcher, SIGNAL(finished()), this, SLOT(threadFinished()));
        watcher.setFuture(QtConcurrent::run(this, &ProcessesMgr::runThread, id));

        m_queuedProcesses.removeAt(id);
    } else {
        m_queuedProcesses.append(id);
    }
}

void ProcessesMgr::play(QList<quint16> ids)
{
    foreach (quint16 id, ids) {
        play(id);
    }
}

void ProcessesMgr::pause(quint16 id)
{
    if (!m_runningProcesses.contains(id) || !m_processes.contains(id)) {
        return;
    }
    m_processes.value(id)->pause();
}

void ProcessesMgr::pauseAt(quint16 id, quint64 step)
{
    if (!m_runningProcesses.contains(id) || !m_processes.contains(id)) {
        return;
    }
    m_processes.value(id)->pauseAt(step);
}

void ProcessesMgr::stop(quint16 id)
{
    if (!m_runningProcesses.contains(id) || !m_processes.contains(id)) {
        return;
    }
    m_processes.value(id)->stop();
}

void ProcessesMgr::stopAt(quint16 id, quint64 step)
{
    if (!m_runningProcesses.contains(id) || !m_processes.contains(id)) {
        return;
    }
    m_processes.value(id)->stopAt(step);
}

quint16 ProcessesMgr::runThread(quint16 id)
{
    Simulation* sim = m_processes.value(id);
    sim->processSteps();
    return id;
}

// watcher
void ProcessesMgr::threadFinished()
{
    QFutureWatcher<quint16>* w = reinterpret_cast<QFutureWatcher<quint16>*>(sender());
    quint16 id = w->result();
    m_runningProcesses.removeAt(id);

    // marked to kill?
    if (m_processesToKill.contains(id)) {
        kill(id);
    }

    // call next process in the queue
    play(m_queuedProcesses.first());
}

void ProcessesMgr::setNumThreads(quint8 threads)
{
    if (m_threads == threads) {
        return;
    }

    const int p = qAbs(threads - m_threads);
    quint8 old = m_threads;
    m_threads = threads;

    if (threads > old) {
        for (int i = 0; i < p && !m_queuedProcesses.isEmpty(); ++i) {
            quint16 id = m_queuedProcesses.takeFirst();
            play(id);
        }
    } else if (threads < old) {
        for (int i = 0; i < p && !m_runningProcesses.isEmpty(); ++i) {
            quint16 id = m_runningProcesses.takeFirst();
            pause(id);
            m_queuedProcesses.push_front(id);
        }
    }
}

void ProcessesMgr::kill(quint16 id)
{
    if (m_runningProcesses.contains(id)) {
        m_processesToKill.append(id);
    } else {
        m_processesToKill.removeAt(id);
        delete m_processes.take(id);
        emit (killed(id));
    }
    m_queuedProcesses.removeAt(id); // just in case...
}

void ProcessesMgr::killAll()
{
    QList<quint16> ids = m_processes.keys();
    foreach (quint16 id, ids) {
        kill(id);
    }
}