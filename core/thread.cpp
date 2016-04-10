/*
 * Madrigal
 *
 * Copyright (c) 2011-2016 Craig Drummond <craig.p.drummond@gmail.com>
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

#include "thread.h"
#include "debug.h"
#include "globalstatic.h"
#include <QCoreApplication>
#include <QtGlobal>
#include <QTimer>
#include <signal.h>
#include <unistd.h>

static void segvHandler(int i) {
    _exit(i);
}

GLOBAL_STATIC(Core::ThreadCleaner, instance)

Core::ThreadCleaner::ThreadCleaner() {
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(stopAll()));
}

void Core::ThreadCleaner::stopAll() {
    DBUG(Thread) << "Remaining threads:" << threads.count();
    foreach (Thread *thread, threads) {
        DBUG(Thread) << "Cleanup" << thread->objectName();
        disconnect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));
    }

    foreach (Thread *thread, threads) {
        thread->stop();
    }

    QList<Thread *> stillRunning;
    foreach (Thread *thread, threads) {
        if (thread->wait(250)) {
            delete thread;
        } else {
            stillRunning.append(thread);
            DBUG(Thread) << "Failed to close" << thread->objectName();
        }
    }

    // Terminate any still running threads...
    signal(SIGSEGV, segvHandler); // Ignore SEGV in case a thread throws an error...
    foreach (Thread *thread, stillRunning) {
        thread->terminate();
    }
}

void Core::ThreadCleaner::threadFinished() {
    Thread *thread=qobject_cast<Thread *>(sender());
    if (thread) {
        thread->deleteLater();
        threads.removeAll(thread);
        DBUG(Thread) << "Thread finished" << thread->objectName() << "Total threads:" << threads.count();
    }
}

void Core::ThreadCleaner::add(Thread *thread) {
    threads.append(thread);
    connect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));
    DBUG(Thread) << "Thread created" << thread->objectName() << "Total threads:" << threads.count();
}

Core::Thread::Thread(const QString &name, QObject *p)
    : QThread(p)
{
    setObjectName(name);
    ThreadCleaner::self()->add(this);
}

Core::Thread::~Thread() {
    DBUG(Thread) << objectName() << "destroyed";
}

void Core::Thread::run() {
    emit started();
    QThread::run();
}

QTimer * Core::Thread::createTimer(QObject *parent) {
    QTimer *timer=new QTimer(parent ? parent : this);
    connect(this, SIGNAL(finished()), timer, SLOT(stop()));
    return timer;
}

void Core::Thread::deleteTimer(QTimer *timer) {
    if (timer) {
        disconnect(this, SIGNAL(finished()), timer, SLOT(stop()));
        timer->deleteLater();
    }
}
