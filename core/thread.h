/*
 * Madrigal
 *
 * Copyright (c) 2016 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef THREAD_H
#define THREAD_H

#include <QThread>

class QTimer;
// ThreadCleaner *needs* to reside in the GUI thread. When a 'Thread' is created it will connect
// its finished signal to threadFinished(), this then calls deleteLater() to ensure that the
// thread is finished before it is deleted - and is deleted in the gui thread.
namespace Core {
class Thread;
class ThreadCleaner : public QObject {
    Q_OBJECT
public:
    static ThreadCleaner * self();
    ThreadCleaner();
    ~ThreadCleaner() { }

public Q_SLOTS:
    void threadFinished();

private Q_SLOTS:
    void stopAll();

private:
    void add(Thread *thread);

private:
    QList<Thread *> threads;
    friend class Thread;
};

class Thread : public QThread {
    Q_OBJECT
public:
    Thread(const QString &name, QObject *p=0);
    virtual ~Thread();

    // Make QThread::msleep accessible!
    using QThread::msleep;

    virtual void run();

    QTimer * createTimer(QObject *parent=0);
    void deleteTimer(QTimer *timer);

Q_SIGNALS:
    void started();

public Q_SLOTS:
    void stop() { quit(); }
};

}
#endif
